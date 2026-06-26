#pragma once

#include <fcitx-config/configuration.h>
#include <fcitx-config/option.h>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/key.h>

namespace fcitx {

FCITX_CONFIGURATION(VoiceInputConfig,
    // Trigger key to activate voice input
    Option<KeyList> triggerKeys{this, "TriggerKeys", _("Trigger Keys"),
                                _("Key combination to start/stop voice input"),
                                {Key("Control+Alt+v")}};

    // ASR backend selection: "openai" or "sherpa"
    Option<std::string> asrBackend{this, "ASRBackend", _("ASR Backend"),
                                   _("Speech recognition backend"),
                                   "openai"};

    // OpenAI-compatible API settings
    Option<std::string> openaiEndpoint{this, "OpenAIEndpoint",
                                       _("OpenAI API Endpoint"),
                                       _("Base URL for OpenAI-compatible API"),
                                       "https://api.openai.com/v1"};
    Option<std::string> openaiApiKey{this, "OpenAIApiKey", _("OpenAI API Key"),
                                     _("API key for OpenAI-compatible service"),
                                     ""};
    Option<std::string> openaiModel{this, "OpenAIModel", _("OpenAI Model"),
                                    _("ASR model ID (e.g. whisper-1)"),
                                    "whisper-1"};
    Option<std::string> openaiLanguage{this, "OpenAILanguage",
                                       _("Output Language"),
                                       _("Language code for ASR output (e.g. zh, en)"),
                                       ""};

    // Sherpa-onnx local model settings
    Option<std::string> modelPath{this, "ModelPath", _("Model Path"),
                                  _("Path to sherpa-onnx model directory"),
                                  ""};
    Option<std::string> modelName{this, "ModelName", _("Model Name"),
                                  _("Name of the sherpa-onnx model"),
                                  ""};

    // Performance
    Option<int, IntConstrain> numThreads{
        this, "NumThreads", _("Number of Threads"),
        _("Thread count for ASR inference"), 4,
        IntConstrain(1, 32)};

    // VAD (Voice Activity Detection)
    Option<double> vadThreshold{this, "VADThreshold", _("VAD Threshold"),
                                _("Voice activity detection probability threshold (0.0-1.0)"),
                                0.3};
    Option<int, IntConstrain> silenceThresholdMs{
        this, "SilenceThresholdMs", _("Silence Threshold (ms)"),
        _("Period of silence to stop recording"), 800,
        IntConstrain(100, 10000)};

    // LLM settings
    Option<std::string> llmEndpoint{this, "LLMEndpoint", _("LLM Endpoint"),
                                    _("LLM API endpoint URL"), ""};
    Option<std::string> llmApiKey{this, "LLMApiKey", _("LLM API Key"),
                                  _("API key for LLM"), ""};

    // Scene settings
    Option<std::string> activeScene{this, "ActiveScene", _("Active Scene"),
                                    _("Active scene configuration name"), ""};
);

} // namespace fcitx
