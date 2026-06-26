#include "config.h"

namespace fcitx {

nlohmann::json VoiceInputConfig::ToJson() const {
    return {
        {"triggerKey", triggerKey.toString()},
        {"asrBackend", asrBackend},
        {"openaiEndpoint", openaiEndpoint},
        {"openaiApiKey", openaiApiKey},
        {"openaiModel", openaiModel},
        {"openaiLanguage", openaiLanguage},
        {"modelPath", modelPath},
        {"modelName", modelName},
        {"numThreads", numThreads},
        {"silenceThresholdMs", silenceThresholdMs},
        {"vadThreshold", vadThreshold},
        {"activeScene", activeScene},
        {"llmEndpoint", llmEndpoint},
        {"llmApiKey", llmApiKey},
    };
}

VoiceInputConfig VoiceInputConfig::FromJson(const nlohmann::json& j) {
    VoiceInputConfig cfg;
    try {
        if (j.contains("triggerKey"))
            cfg.triggerKey = fcitx::Key(j["triggerKey"].get<std::string>());
        if (j.contains("asrBackend"))
            cfg.asrBackend = j["asrBackend"].get<std::string>();
        if (j.contains("openaiEndpoint"))
            cfg.openaiEndpoint = j["openaiEndpoint"].get<std::string>();
        if (j.contains("openaiApiKey"))
            cfg.openaiApiKey = j["openaiApiKey"].get<std::string>();
        if (j.contains("openaiModel"))
            cfg.openaiModel = j["openaiModel"].get<std::string>();
        if (j.contains("openaiLanguage"))
            cfg.openaiLanguage = j["openaiLanguage"].get<std::string>();
        if (j.contains("modelPath"))
            cfg.modelPath = j["modelPath"].get<std::string>();
        if (j.contains("modelName"))
            cfg.modelName = j["modelName"].get<std::string>();
        if (j.contains("numThreads"))
            cfg.numThreads = j["numThreads"].get<int>();
        if (j.contains("silenceThresholdMs"))
            cfg.silenceThresholdMs = j["silenceThresholdMs"].get<int>();
        if (j.contains("vadThreshold"))
            cfg.vadThreshold = j["vadThreshold"].get<float>();
        if (j.contains("activeScene"))
            cfg.activeScene = j["activeScene"].get<std::string>();
        if (j.contains("llmEndpoint"))
            cfg.llmEndpoint = j["llmEndpoint"].get<std::string>();
        if (j.contains("llmApiKey"))
            cfg.llmApiKey = j["llmApiKey"].get<std::string>();
    } catch (const std::exception& e) {
        // On parse error, keep defaults and log warning
        return Defaults();
    }
    return cfg;
}

} // namespace fcitx
