#pragma once

#ifdef ENABLE_SHERPA_ONNX

#include <memory>
#include <thread>
#include <atomic>

#include "asr_engine.h"
#include "utils/thread_safe_queue.h"

// Forward declarations for sherpa-onnx C API
struct SherpaOnnxOnlineRecognizer;
struct SherpaOnnxOnlineStream;
struct SherpaOnnxOnlineRecognizerConfig;

namespace fcitx {

/**
 * Local ASR engine using sherpa-onnx.
 *
 * Runs inference on a dedicated thread. Audio is fed via a thread-safe queue.
 * Results are delivered via callbacks (which run on the ASR thread — caller
 * must dispatch to main thread via Fcitx event loop).
 *
 * Only compiled when ENABLE_SHERPA_ONNX is defined.
 */
class SherpaAsrEngine : public AsrEngine {
public:
    SherpaAsrEngine();
    ~SherpaAsrEngine() override;

    SherpaAsrEngine(const SherpaAsrEngine&) = delete;
    SherpaAsrEngine& operator=(const SherpaAsrEngine&) = delete;

    bool Init(const Config& config) override;
    void Start() override;
    void FeedAudio(const float* pcm, size_t frames) override;
    void Stop() override;
    const char* Name() const override { return "sherpa-onnx"; }

private:
    void InferenceLoop();

    // Sherpa-onnx objects
    SherpaOnnxOnlineRecognizer* recognizer_ = nullptr;
    SherpaOnnxOnlineStream* stream_ = nullptr;

    // Thread management
    std::unique_ptr<std::thread> inferenceThread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> sessionActive_{false};

    // Audio input queue
    struct AudioChunk {
        std::vector<float> samples;
    };
    ThreadSafeQueue<AudioChunk> audioQueue_;

    Config config_;
};

} // namespace fcitx

#endif // ENABLE_SHERPA_ONNX
