#include "llm_client.h"

#include <cstring>
#include <string>

#include <curl/curl.h>
#include <json/json.h>

#include <fcitx-utils/log.h>

namespace fcitx {

namespace {

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* response = static_cast<std::string*>(userp);
    size_t total = size * nmemb;
    response->append(static_cast<char*>(contents), total);
    return total;
}

} // namespace

LLMClient::LLMClient(Config config)
    : config_(std::move(config)) {}

LLMClient::~LLMClient() = default;

std::string LLMClient::Process(const std::string& text) {
    if (config_.model.empty()) {
        return text;
    }

    // Build URL
    std::string url = config_.endpoint;
    if (!url.empty() && url.back() != '/') {
        url += '/';
    }
    url += "chat/completions";

    // Build JSON body
    Json::Value body;
    body["model"] = config_.model;
    body["temperature"] = 0.1;

    Json::Value messages(Json::arrayValue);

    if (!config_.systemPrompt.empty()) {
        Json::Value sysMsg;
        sysMsg["role"] = "system";
        sysMsg["content"] = config_.systemPrompt;
        messages.append(sysMsg);
    }

    Json::Value userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = text;
    messages.append(userMsg);

    body["messages"] = messages;

    Json::StreamWriterBuilder writer;
    std::string bodyStr = Json::writeString(writer, body);

    // HTTP request
    CURL* curl = curl_easy_init();
    if (!curl) {
        FCITX_ERROR() << "[voice-input:llm] Failed to init curl";
        return {};
    }

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    std::string authHeader = "Authorization: Bearer " + config_.apiKey;
    headers = curl_slist_append(headers, authHeader.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, bodyStr.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "fcitx5-voice-input/0.1.0");

    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || httpCode != 200) {
        FCITX_WARN() << "[voice-input:llm] Request failed: "
                     << curl_easy_strerror(res)
                     << " http=" << httpCode;
        return {};
    }

    // Parse response
    Json::Value json;
    Json::Reader reader;
    if (!reader.parse(response, json)) {
        FCITX_WARN() << "[voice-input:llm] JSON parse failed";
        return {};
    }

    std::string result = json["choices"][0]["message"]["content"].asString();
    if (result.empty()) {
        FCITX_WARN() << "[voice-input:llm] Empty response content";
        return {};
    }

    FCITX_INFO() << "[voice-input:llm] Processed: \""
                 << text.substr(0, 30) << "\" → \""
                 << result.substr(0, 30) << "\"";
    return result;
}

} // namespace fcitx
