#include "sherpa_asr.h"

#include <cstring>
#include <algorithm>
#include <iostream>

// Include sherpa-onnx C API
#include "sherpa-onnx/c-api/c-api.h"

namespace fcitx {

SherpaAsrEngine::SherpaAsrEngine() = default;

SherpaAsrEngine::~SherpaAsrEngine() {
    Stop();
    if (recognizer_) {
        DestroyOnlineRecognizer(recognizer_);
        recognizer_ = nullptr;
    }
}

bool SherpaAsrEngine::Init(const Config& config) {
    config_ = config;

    // ── Build sherpa-onnx config ──────────────────────────────────────
    SherpaOnnxOnlineRecognizerConfig recognizerConfig;
    std::memset(&recognizerConfig, 0, sizeof(recognizerConfig));

    // Model config
    recognizerConfig.fbank_config = {};
    recognizerConfig.decoder_config = {};
    recognizerConfig.decoder_config.num_active_paths = 4;

    // If model path is provided, set up the model config
    SherpaOnnxOnlineModelConfig modelConfig;
    std::memset(&modelConfig, 0, sizeof(modelConfig));

    if (!config_.modelPath.empty()) {
        modelConfig.model_path = config_.modelPath.c_str();
    }

    // Try to detect model type from model name or path
    // For Zipformer models (most common for Chinese):
    if (config_.modelName.find("zipformer") != std::string::npos ||
        config_.modelPath.find("zipformer") != std::string::npos) {
        // Zipformer config
        modelConfig.num_threads = config_.numThreads;
        modelConfig.provider = "cpu";
    }

    recognizerConfig.model_config = modelConfig;

    // Create recognizer
    recognizer_ = CreateOnlineRecognizer(&recognizerConfig);
    if (!recognizer_) {
        std::cerr << "[voice-input] Failed to create sherpa-onnx recognizer"
                  << std::endl;
        return false;
    }

    return true;
}

void SherpaAsrEngine::Start() {
    if (!recognizer_) return;

    // Clear any remaining audio
    AudioChunk dummy;
    while (audioQueue_.TryPop(dummy)) {}

    // Create new online stream
    stream_ = CreateOnlineStream(recognizer_);
    sessionActive_ = true;

    if (!inferenceThread_ && running_) {
        inferenceThread_ = std::make_unique<std::thread>(
            &SherpaAsrEngine::InferenceLoop, this);
    }
}

void SherpaAsrEngine::FeedAudio(const float* pcm, size_t frames) {
    if (!sessionActive_) return;

    AudioChunk chunk;
    chunk.samples.resize(frames);
    std::memcpy(chunk.samples.data(), pcm, frames * sizeof(float));
    audioQueue_.Push(std::move(chunk));
}

void SherpaAsrEngine::Stop() {
    sessionActive_ = false;
    audioQueue_.Push(AudioChunk{});  // wake up the inference thread

    if (inferenceThread_ && inferenceThread_->joinable()) {
        inferenceThread_->join();
        inferenceThread_.reset();
    }

    // Get final result from the stream
    if (stream_ && recognizer_) {
        while (IsOnlineStreamReady(recognizer_, stream_)) {
            DecodeOnlineStream(recognizer_, stream_);
        }
        const char* result = GetOnlineStreamResultAsString(recognizer_, stream_);
        if (result && std::strlen(result) > 0) {
            if (resultCb_) {
                resultCb_(std::string(result), true);
            }
        }
    }

    if (stream_) {
        DestroyOnlineStream(stream_);
        stream_ = nullptr;
    }
}

void SherpaAsrEngine::InferenceLoop() {
    while (running_ && sessionActive_) {
        auto chunk = audioQueue_.Pop();

        // Empty chunk signals stop
        if (chunk.samples.empty()) break;

        if (!stream_ || !recognizer_) continue;

        // Feed audio to sherpa-onnx stream
        AcceptWaveformOnlineStream(stream_, 16000,
                                   chunk.samples.data(),
                                   chunk.samples.size());

        // Decode
        while (IsOnlineStreamReady(recognizer_, stream_)) {
            DecodeOnlineStream(recognizer_, stream_);
        }

        // Get partial result
        const char* result = GetOnlineStreamResultAsString(recognizer_, stream_);
        if (result && std::strlen(result) > 0) {
            if (resultCb_) {
                resultCb_(std::string(result), false);  // partial
            }
        }
    }
}

} // namespace fcitx
