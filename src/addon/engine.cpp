#include "engine.h"

#include <fcitx-utils/log.h>
#include <fcitx-utils/eventdispatcher.h>
#include <fcitx/addonloader.h>
#include <fcitx/addonmanager.h>
#include <fcitx/instance.h>

#include "asr/openai_asr.h"

#ifdef ENABLE_SHERPA_ONNX
#include "asr/sherpa_asr.h"
#endif

namespace fcitx {

class VoiceInputAddonFactory : public AddonFactory {
public:
    AddonInstance* create(AddonManager* manager) override {
        return new VoiceInputEngine(manager->instance());
    }
};
FCITX_ADDON_FACTORY(VoiceInputAddonFactory);

VoiceInputEngine::VoiceInputEngine(Instance* instance)
    : instance_(instance)
    , pipeline_(std::make_unique<Pipeline>())
{
    eventDispatcher_.attach(&instance_->eventLoop());
}

VoiceInputEngine::~VoiceInputEngine() {
    pipeline_->Abort();
}

void VoiceInputEngine::activate(const InputMethodEntry& entry,
                                 InputContextEvent& event) {
    InitializeIfNeeded();
    activeIc_ = event.inputContext();
}

void VoiceInputEngine::deactivate(const InputMethodEntry& entry,
                                   InputContextEvent& event) {
    if (pipeline_->GetState() == Pipeline::State::RECORDING) {
        pipeline_->StopRecording();
    }
    activeIc_ = nullptr;
}

void VoiceInputEngine::keyEvent(const InputMethodEntry& entry,
                                 KeyEvent& keyEvent) {
    auto* ic = keyEvent.inputContext();
    if (!ic) return;

    Key triggerKey = config_.triggerKey;

    // Trigger key pressed → start recording
    if (keyEvent.key() == triggerKey && !keyEvent.isRelease()) {
        if (pipeline_->GetState() == Pipeline::State::IDLE) {
            pipeline_->StartRecording();
            activeIc_ = ic;
            keyEvent.filter();
        }
        return;
    }

    // Trigger key released → stop recording
    if (keyEvent.key() == triggerKey && keyEvent.isRelease()) {
        if (pipeline_->GetState() == Pipeline::State::RECORDING) {
            pipeline_->StopRecording();
            keyEvent.filter();
        }
        return;
    }
}

void VoiceInputEngine::OnPipelineStateChange(
    Pipeline::State oldState, Pipeline::State newState) {
    FCITX_DEBUG() << "[voice-input] State: "
                  << pipeline_->StateName();
}

void VoiceInputEngine::OnAsrResult(const std::string& text) {
    auto* ic = activeIc_;
    if (!ic) return;

    std::string result = text;

    eventDispatcher_.schedule(
        [this, ic, result]() {
            if (activeIc_ == ic) {
                ic->commitString(result);
            }
        });
}

void VoiceInputEngine::InitializeIfNeeded() {
    if (initialized_) return;
    initialized_ = true;

    LoadConfig();

    // Setup pipeline callbacks
    pipeline_->SetStateCallback(
        [this](Pipeline::State oldState, Pipeline::State newState) {
            OnPipelineStateChange(oldState, newState);
        });

    pipeline_->SetResultCallback(
        [this](const std::string& text) {
            OnAsrResult(text);
        });

    pipeline_->Init(config_);

    // Create ASR engine based on backend selection
    auto asrConfig = AsrEngine::Config{};

    if (config_.asrBackend == "sherpa-onnx") {
#ifdef ENABLE_SHERPA_ONNX
        asrConfig.modelPath = config_.modelPath;
        asrConfig.modelName = config_.modelName;
        asrConfig.numThreads = config_.numThreads;

        auto asr = std::make_unique<SherpaAsrEngine>();
        if (asr->Init(asrConfig)) {
            pipeline_->SetAsrEngine(std::move(asr));
        } else {
            FCITX_WARN() << "[voice-input] Sherpa-onnx init failed, "
                            "falling back to OpenAI-compatible";
        }
#else
        FCITX_WARN() << "[voice-input] asrBackend=sherpa-onnx but "
                        "ENABLE_SHERPA_ONNX not set, using OpenAI";
#endif
    }

    // Default: OpenAI-compatible (also serves as fallback)
    if (!pipeline_->HasAsrEngine()) {
        asrConfig.apiEndpoint = config_.openaiEndpoint;
        asrConfig.apiKey = config_.openaiApiKey;
        asrConfig.modelName = config_.openaiModel;
        asrConfig.language = config_.openaiLanguage;

        auto asr = std::make_unique<OpenaiCompatAsrEngine>();
        if (asr->Init(asrConfig)) {
            pipeline_->SetAsrEngine(std::move(asr));
            FCITX_INFO() << "[voice-input] Using OpenAI-compatible ASR: "
                         << config_.openaiEndpoint
                         << " model=" << config_.openaiModel;
        } else {
            FCITX_WARN() << "[voice-input] OpenAI ASR init failed "
                            "(no API key?), running capture-only";
        }
    }
}

void VoiceInputEngine::LoadConfig() {
    // For now, use defaults — Fcitx config loading integration TBD
    config_ = VoiceInputConfig::Defaults();
}

} // namespace fcitx
