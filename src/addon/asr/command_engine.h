#pragma once

#ifdef ENABLE_CLOUD_ASR

#include "asr_engine.h"

namespace fcitx {

/**
 * Cloud ASR engine — runs an external command for ASR.
 *
 * The command receives PCM audio via stdin and outputs JSON results
 * to stdout. Configurable for Doubao ASR, OpenAI Whisper API, etc.
 *
 * This is a stub for Phase 2. Not included in MVP.
 */
class CommandEngine : public AsrEngine {
public:
    CommandEngine() = default;
    ~CommandEngine() override = default;

    bool Init(const Config& config) override { return false; }
    void Start() override {}
    void FeedAudio(const float* pcm, size_t frames) override {}
    void Stop() override {}
    const char* Name() const override { return "command"; }
};

} // namespace fcitx

#endif // ENABLE_CLOUD_ASR
