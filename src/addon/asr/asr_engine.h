#pragma once

#include <string>
#include <functional>
#include <vector>

namespace fcitx {

/**
 * ASR engine interface.
 *
 * Supports both local (sherpa-onnx) and cloud providers.
 * All implementations must be thread-safe: Start()/Stop() called from
 * main thread, Process() called from ASR thread.
 */
class AsrEngine {
public:
    struct Config {
        std::string modelPath;
        std::string modelName;
        int numThreads = 4;
    };

    // Result callback — called from ASR thread, caller must dispatch to
    // main thread via eventLoop().addDeferredEvent().
    using ResultCallback = std::function<void(const std::string& text, bool isFinal)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    virtual ~AsrEngine() = default;

    // Initialize the engine with model/config.
    // Returns false if initialization fails (e.g. model not found).
    virtual bool Init(const Config& config) = 0;

    // Start a new recognition session (clean state).
    virtual void Start() = 0;

    // Feed PCM audio data (16kHz mono F32) for recognition.
    // Non-blocking: audio is queued internally.
    virtual void FeedAudio(const float* pcm, size_t frames) = 0;

    // Signal end of input, get final result.
    // After calling this, Start() must be called again for new session.
    virtual void Stop() = 0;

    // Unique name for this engine.
    virtual const char* Name() const = 0;

    // Set callbacks.
    void SetResultCallback(ResultCallback cb) { resultCb_ = std::move(cb); }
    void SetErrorCallback(ErrorCallback cb) { errorCb_ = std::move(cb); }

protected:
    ResultCallback resultCb_;
    ErrorCallback errorCb_;
};

} // namespace fcitx
