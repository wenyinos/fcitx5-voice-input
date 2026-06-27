#include <string>

#include <fcitx-config/iniparser.h>
#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/event.h>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/log.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addoninstance.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <fcitx/userinterface.h>

#include "engine.h"

#include "asr/openai_asr.h"
#include "llm/llm_client.h"

namespace fcitx {

VoiceInputEngine::VoiceInputEngine(Instance* instance)
    : instance_(instance), pipeline_(std::make_unique<Pipeline>()) {
    fcitx::registerDomain(FCITX_GETTEXT_DOMAIN, VOICE_INPUT_LOCALE_DIR);
    eventDispatcher_.attach(&instance_->eventLoop());
    reloadConfig();
}

VoiceInputEngine::~VoiceInputEngine() {
    pipeline_->Abort();
}

void VoiceInputEngine::reloadConfig() {
    readAsIni(config_, "conf/voiceinput.conf");
}

void VoiceInputEngine::setConfig(const RawConfig& rawConfig) {
    config_.load(rawConfig, true);

    bool saved = safeSaveAsIni(config_, "conf/voiceinput.conf");
    FCITX_INFO() << "[voice-input] setConfig saved=" << saved;

    if (initialized_) {
        pipeline_->SetConfig(config_);
    }
}

void VoiceInputEngine::activate(const InputMethodEntry& entry,
                                InputContextEvent& event) {
    FCITX_UNUSED(entry);
    FCITX_UNUSED(event);
    InitializeIfNeeded();
    activeIc_ = event.inputContext();
    uint64_t generation = activeGeneration_.fetch_add(1) + 1;
    pendingStopGeneration_ = generation;
    sessionGeneration_.store(generation);

    pipeline_->SetGeneration(generation);

    bool wasRunning = pipeline_->IsRunning();
    pipeline_->Start();

    FCITX_INFO() << "[voice-input] Activate: gen=" << generation
                 << " wasRunning=" << wasRunning
                 << " ic=" << (activeIc_ != nullptr);

    statusText_.clear();
    SetStatus("\xF0\x9F\x8E\x99 \xe5\xbd\x95\xe9\x9f\xb3\xe4\xb8\xad...");  // "🎙 录音中..."
}

void VoiceInputEngine::deactivate(const InputMethodEntry& entry,
                                  InputContextEvent& event) {
    FCITX_UNUSED(entry);
    FCITX_UNUSED(event);
    uint64_t generation = activeGeneration_.fetch_add(1) + 1;
    pendingStopGeneration_ = generation;

    FCITX_INFO() << "[voice-input] Deactivate: gen=" << generation;

    ClearUI();

    delayedStopEvent_ = instance_->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC,
        now(CLOCK_MONOTONIC) + 200000,
        0,
        [this, generation](EventSourceTime*, uint64_t) {
            if (pendingStopGeneration_ != generation) {
                FCITX_INFO() << "[voice-input] DelayedStop: cancelled gen="
                             << generation;
                return true;
            }
            FCITX_INFO() << "[voice-input] DelayedStop: executing gen="
                         << generation;
            sessionGeneration_.store(0);
            pipeline_->Stop();
            activeIc_ = nullptr;
            return true;
        });
    delayedStopEvent_->setOneShot();
}

std::vector<InputMethodEntry> VoiceInputEngine::listInputMethods() {
    std::vector<InputMethodEntry> entries;
    entries.emplace_back("voiceinput", _("Voice Input"), "zh_CN",
                         "voiceinput");
    entries.back().setLabel("\xF0\x9F\x8E\x99").setConfigurable(true);
    return entries;
}

void VoiceInputEngine::keyEvent(const InputMethodEntry& entry,
                                KeyEvent& keyEvent) {
    FCITX_UNUSED(entry);
    FCITX_UNUSED(keyEvent);
}

void VoiceInputEngine::OnAsrResult(const std::string& text) {
    uint64_t generation = sessionGeneration_.load();
    FCITX_INFO() << "[voice-input] OnAsrResult: text='"
                 << text.substr(0, 30) << "'"
                 << " sessionGen=" << generation
                 << " activeGen=" << activeGeneration_.load();
    eventDispatcher_.schedule([this, generation]() {
        if (generation == 0 || activeGeneration_.load() != generation) {
            FCITX_INFO() << "[voice-input] PollResults skipped: gen="
                         << generation << " active=" << activeGeneration_.load();
            return;
        }
        PollResults();
    });
}

namespace {

size_t Utf8CharCount(const std::string& s) {
    size_t count = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if ((s[i] & 0xC0) != 0x80) count++;
    }
    return count;
}

} // namespace

void VoiceInputEngine::PollResults() {
    auto& queue = pipeline_->ResultQueue();
    AsrResult result;
    while (queue.TryPop(result)) {
        FCITX_INFO() << "[voice-input] PollResult: text='"
                     << result.text.substr(0, 50) << "'"
                     << " gen=" << result.generation
                     << " activeGen=" << activeGeneration_.load()
                     << " ic=" << (activeIc_ != nullptr)
                     << " match=" << (activeGeneration_.load() == result.generation)
                     << " refined=" << result.isLLMRefined;
        if (!result.text.empty()
            && result.generation != 0
            && activeGeneration_.load() == result.generation
            && activeIc_) {
            if (result.isLLMRefined) {
                // Replace previous commit with LLM-refined text
                size_t oldLen = Utf8CharCount(lastCommittedText_);
                if (oldLen > 0) {
                    activeIc_->deleteSurroundingText(-static_cast<int>(oldLen), 0);
                }
                activeIc_->commitString(result.text);
            } else {
                activeIc_->commitString(result.text);
            }
            lastCommittedText_ = result.text;
        }
    }
}

void VoiceInputEngine::SetStatus(const std::string& text) {
    eventDispatcher_.schedule([this, text]() {
        statusText_ = text;
        if (activeIc_) {
            activeIc_->updateUserInterface(UserInterfaceComponent::StatusArea);
        }
    });
}

void VoiceInputEngine::ClearUI() {
    eventDispatcher_.schedule([this]() {
        statusText_.clear();
        if (activeIc_) {
            activeIc_->inputPanel().reset();
            activeIc_->updateUserInterface(UserInterfaceComponent::InputPanel);
            activeIc_->updateUserInterface(UserInterfaceComponent::StatusArea);
        }
    });
}

std::string VoiceInputEngine::subModeLabelImpl(const InputMethodEntry& entry,
                                                InputContext& ic) {
    FCITX_UNUSED(entry);
    if (&ic == activeIc_ && !statusText_.empty()) {
        return statusText_;
    }
    return {};
}

void VoiceInputEngine::InitializeIfNeeded() {
    if (initialized_) return;
    initialized_ = true;

    pipeline_->SetResultCallback(
        [this](const std::string& text) {
            OnAsrResult(text);
        });

    pipeline_->Init(config_);

    auto asrConfig = AsrEngine::Config{};
    asrConfig.apiEndpoint = config_.openaiEndpoint.value();
    asrConfig.apiKey = config_.openaiApiKey.value();
    asrConfig.modelName = config_.openaiModel.value();
    asrConfig.language = config_.openaiLanguage.value();

    auto asr = std::make_unique<OpenaiCompatAsrEngine>();
    if (asr->Init(asrConfig)) {
        pipeline_->SetAsrEngine(std::move(asr));
        FCITX_INFO() << "[voice-input] Using OpenAI-compatible ASR: "
                     << config_.openaiEndpoint.value()
                     << " model=" << config_.openaiModel.value();
    } else {
        FCITX_WARN() << "[voice-input] OpenAI ASR init failed";
    }

    // LLM post-processing
    std::string llmModel = config_.llmModel.value();
    if (!llmModel.empty()) {
        auto llmConfig = LLMClient::Config{};
        llmConfig.endpoint = config_.openaiEndpoint.value();
        llmConfig.apiKey = config_.openaiApiKey.value();
        llmConfig.model = llmModel;
        llmConfig.systemPrompt = config_.llmSystemPrompt.value();

        auto llm = std::make_unique<LLMClient>(std::move(llmConfig));
        pipeline_->SetLLMClient(std::move(llm));
        FCITX_INFO() << "[voice-input] LLM post-processing enabled: "
                     << " model=" << llmModel;
    }
}

} // namespace fcitx

class VoiceInputAddonFactory : public fcitx::AddonFactory {
public:
    fcitx::AddonInstance* create(fcitx::AddonManager* manager) override {
        return new fcitx::VoiceInputEngine(manager->instance());
    }
};
FCITX_ADDON_FACTORY(VoiceInputAddonFactory);
