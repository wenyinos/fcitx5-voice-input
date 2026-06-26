#pragma once

#include <fcitx-config/configuration.h>
#include <fcitx-config/option.h>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/key.h>

namespace fcitx {

FCITX_CONFIGURATION(VoiceInputConfig,
    // Trigger key to activate voice input
    Option<KeyList> triggerKeys{this, "TriggerKeys",
                                _("Trigger Keys"),
                                KeyList{Key("Control+Alt+v")}};

    // ASR backend selection: "openai" or "sherpa"
    Option<std::string> asrBackend{this, "ASRBackend",
                                   _("ASR Backend"),
                                   "openai"};

    // OpenAI-compatible API settings
    Option<std::string> openaiEndpoint{this, "OpenAIEndpoint",
                                       _("OpenAI API Endpoint"),
                                       "https://api.openai.com/v1"};
    Option<std::string> openaiApiKey{this, "OpenAIApiKey",
                                     _("OpenAI API Key"), ""};
    Option<std::string> openaiModel{this, "OpenAIModel",
                                    _("OpenAI Model"),
                                    "whisper-1"};
    Option<std::string> openaiLanguage{this, "OpenAILanguage",
                                       _("Output Language"), ""};

    // Sherpa-onnx local model settings
    Option<std::string> modelPath{this, "ModelPath",
                                  _("Model Path"), ""};
    Option<std::string> modelName{this, "ModelName",
                                  _("Model Name"), ""};

    // Performance
    Option<int, IntConstrain> numThreads{this, "NumThreads",
                                         _("Number of Threads"), 4,
                                         IntConstrain(1, 32)};

    // VAD (Voice Activity Detection)
    // Stored as 0-100 percentage internally; divide by 100 for 0.0-1.0
    Option<int, IntConstrain> vadThreshold{this, "VADThreshold",
                                           _("VAD Threshold (%)"), 30,
                                           IntConstrain(0, 100)};
    Option<int, IntConstrain> silenceThresholdMs{
        this, "SilenceThresholdMs", _("Silence Threshold (ms)"), 800,
        IntConstrain(100, 10000)};

    // LLM settings
    Option<std::string> llmEndpoint{this, "LLMEndpoint",
                                    _("LLM Endpoint"), ""};
    Option<std::string> llmApiKey{this, "LLMApiKey",
                                  _("LLM API Key"), ""};

    // Scene settings
    Option<std::string> activeScene{this, "ActiveScene",
                                    _("Active Scene"), ""};
);

} // namespace fcitx
