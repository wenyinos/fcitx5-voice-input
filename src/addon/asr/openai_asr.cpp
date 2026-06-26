#include "openai_asr.h"

#include <cstring>
#include <iostream>
#include <sstream>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace fcitx {
namespace {

// ── WAV writing helpers ─────────────────────────────────────────────
#pragma pack(push, 1)
struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize = 0;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;       // PCM
    uint16_t numChannels = 1;       // mono
    uint32_t sampleRate = 16000;
    uint32_t byteRate = 32000;      // 16000 * 1 * 2
    uint16_t blockAlign = 2;        // 1 * 2
    uint16_t bitsPerSample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize = 0;
};
#pragma pack(pop)

std::vector<uint8_t> FloatPcmToWav(const float* pcm, size_t frames) {
    WavHeader header;
    size_t dataBytes = frames * sizeof(int16_t);
    header.fileSize = static_cast<uint32_t>(sizeof(WavHeader) - 8 + dataBytes);
    header.dataSize = static_cast<uint32_t>(dataBytes);

    std::vector<uint8_t> wav(sizeof(WavHeader) + dataBytes);
    std::memcpy(wav.data(), &header, sizeof(WavHeader));

    auto* samplePtr = reinterpret_cast<int16_t*>(wav.data() + sizeof(WavHeader));
    for (size_t i = 0; i < frames; ++i) {
        float s = pcm[i];
        // Clamp to [-1, 1]
        if (s < -1.0f) s = -1.0f;
        if (s > 1.0f) s = 1.0f;
        samplePtr[i] = static_cast<int16_t>(s * 32767.0f);
    }

    return wav;
}

// ── Curl write callback ─────────────────────────────────────────────
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), total);
    return total;
}

} // anonymous namespace

// ── OpenaiCompatAsrEngine ──────────────────────────────────────────

OpenaiCompatAsrEngine::OpenaiCompatAsrEngine() = default;

OpenaiCompatAsrEngine::~OpenaiCompatAsrEngine() {
    cancelled_ = true;
    if (workerThread_ && workerThread_->joinable()) {
        workerThread_->join();
    }
}

bool OpenaiCompatAsrEngine::Init(const Config& config) {
    apiEndpoint_ = config.apiEndpoint;
    apiKey_ = config.apiKey;
    modelName_ = config.modelName;
    language_ = config.language;

    if (apiEndpoint_.empty()) {
        apiEndpoint_ = "https://api.openai.com/v1";
    }
    if (modelName_.empty()) {
        modelName_ = "whisper-1";
    }
    if (language_.empty()) {
        language_ = "zh";
    }

    // Validate: we need at least an API key
    if (apiKey_.empty()) {
        std::cerr << "[voice-input:openai] API key not configured" << std::endl;
        return false;
    }

    return true;
}

void OpenaiCompatAsrEngine::Start() {
    pcmBuffer_.clear();
}

void OpenaiCompatAsrEngine::FeedAudio(const float* pcm, size_t frames) {
    pcmBuffer_.insert(pcmBuffer_.end(), pcm, pcm + frames);
}

void OpenaiCompatAsrEngine::Stop() {
    if (pcmBuffer_.empty()) {
        // No audio to transcribe
        if (resultCb_) {
            resultCb_("", true);
        }
        return;
    }

    // Wait for previous worker to finish, then start a new one
    if (workerThread_ && workerThread_->joinable()) {
        workerThread_->join();
    }
    cancelled_ = false;
    workerThread_ = std::make_unique<std::thread>(
        &OpenaiCompatAsrEngine::TranscribeWorker, this);
}

void OpenaiCompatAsrEngine::TranscribeWorker() {
    // Take ownership of the buffer
    std::vector<float> audio;
    std::swap(audio, pcmBuffer_);

    if (cancelled_) return;

    // Encode to WAV
    std::vector<uint8_t> wavData = FloatPcmToWav(audio.data(), audio.size());

    if (cancelled_) return;

    // Make HTTP request
    std::string response = DoHttpRequest(wavData);

    if (cancelled_) return;

    // Parse JSON response
    try {
        auto json = nlohmann::json::parse(response);

        // Check for error
        if (json.contains("error")) {
            std::string errMsg = json["error"].value("message", "unknown error");
            if (errorCb_) {
                errorCb_("API error: " + errMsg);
            }
            return;
        }

        std::string text = json.value("text", "");
        if (!text.empty() && resultCb_) {
            resultCb_(text, true);
        }
    } catch (const std::exception& e) {
        if (errorCb_) {
            errorCb_("JSON parse error: " + std::string(e.what()));
        }
    }
}

std::vector<uint8_t> OpenaiCompatAsrEngine::EncodeToWav(const float* pcm, size_t frames) {
    return FloatPcmToWav(pcm, frames);
}

std::string OpenaiCompatAsrEngine::DoHttpRequest(const std::vector<uint8_t>& wavData) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        if (errorCb_) errorCb_("Failed to initialize libcurl");
        return "";
    }

    std::string response;
    struct curl_slist* headers = nullptr;

    // Build URL: {endpoint}/audio/transcriptions
    std::string url = apiEndpoint_;
    if (!url.empty() && url.back() != '/') {
        url += '/';
    }
    url += "audio/transcriptions";

    // Set up multipart form
    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part;

    // Audio file
    part = curl_mime_addpart(mime);
    curl_mime_name(part, "file");
    curl_mime_data(part, reinterpret_cast<const char*>(wavData.data()), wavData.size());
    curl_mime_filename(part, "audio.wav");
    curl_mime_type(part, "audio/wav");

    // Model
    part = curl_mime_addpart(mime);
    curl_mime_name(part, "model");
    curl_mime_data(part, modelName_.c_str(), CURL_ZERO_TERMINATED);

    // Language
    part = curl_mime_addpart(mime);
    curl_mime_name(part, "language");
    curl_mime_data(part, language_.c_str(), CURL_ZERO_TERMINATED);

    // Response format
    part = curl_mime_addpart(mime);
    curl_mime_name(part, "response_format");
    curl_mime_data(part, "json", CURL_ZERO_TERMINATED);

    // Headers
    std::string authHeader = "Authorization: Bearer " + apiKey_;
    headers = curl_slist_append(headers, authHeader.c_str());

    // Curl options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "fcitx5-voice-input/0.1.0");

    // Also grab the Content-Type header to detect errors in non-JSON responses
    std::string contentType;
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,
        +[](void* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
            auto* ct = static_cast<std::string*>(userdata);
            std::string line(static_cast<char*>(ptr), size * nmemb);
            if (line.rfind("Content-Type:", 0) == 0) {
                *ct = line.substr(line.find(':') + 1);
                // Trim whitespace
                if (!ct->empty() && (*ct)[0] == ' ') ct->erase(0, 1);
                if (!ct->empty() && (*ct).back() == '\r') ct->pop_back();
                if (!ct->empty() && (*ct).back() == '\n') ct->pop_back();
            }
            return size * nmemb;
        },
        &contentType);

    // Perform request
    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    // Cleanup
    curl_mime_free(mime);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::string err = curl_easy_strerror(res);
        if (errorCb_) errorCb_("HTTP request failed: " + err);
        return "";
    }

    if (httpCode != 200) {
        std::string errMsg = "HTTP " + std::to_string(httpCode);
        if (!response.empty()) {
            // Try to extract error message from response
            try {
                auto json = nlohmann::json::parse(response);
                if (json.contains("error")) {
                    errMsg += ": " + json["error"].value("message", response);
                } else {
                    errMsg += ": " + response;
                }
            } catch (...) {
                errMsg += ": " + response;
            }
        }
        if (errorCb_) errorCb_("API error: " + errMsg);
        return "";
    }

    return response;
}

} // namespace fcitx
