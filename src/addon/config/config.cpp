#include "config.h"

namespace fcitx {

nlohmann::json VoiceInputConfig::ToJson() const {
    return {
        {"triggerKey", triggerKey.toString()},
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
        // (logging not available at this stage; caller should catch)
        return Defaults();
    }
    return cfg;
}

} // namespace fcitx
