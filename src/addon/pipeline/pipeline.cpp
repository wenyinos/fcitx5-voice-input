#include "pipeline.h"

#include <fcitx-utils/log.h>

#include "config/voiceinput-config.h"
#include "vad/vad.h"

namespace fcitx {

Pipeline::Pipeline()
    : state_(State::IDLE)
    , vad_(std::make_unique<VAD>())
    , audioBuffer_(std::make_unique<AudioRingBuffer>(kSampleRate * kMaxSeconds))
    , stateLock_()
    , stateCallback_(nullptr)
    , resultCallback_(nullptr)
    , asrEngine_(nullptr) {}

Pipeline::~Pipeline() {
    Abort();
}

void Pipeline::Init(const VoiceInputConfig &config) {
    config_ = config;
    vad_->SetVadThreshold(config_.vadThreshold.value() / 100.0f);
    vad_->SetSilenceMs(config_.silenceThresholdMs.value());

    FCITX_INFO() << "[voice-input] Pipeline initialized";
}

void Pipeline::SetConfig(const VoiceInputConfig &config) {
    std::lock_guard<std::mutex> lock(stateLock_);
    config_ = config;
    vad_->SetVadThreshold(config_.vadThreshold.value() / 100.0f);
    vad_->SetSilenceMs(config_.silenceThresholdMs.value());
}

} // namespace fcitx
