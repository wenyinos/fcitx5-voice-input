#pragma once

#include <string>
#include <vector>
#include <fcitx-utils/key.h>
#include <nlohmann/json.hpp>

namespace fcitx {

/**
 * Voice input addon configuration.
 *
 * Loaded from JSON config file, with defaults for everything.
 * Fcitx5 Config framework provides UI via fcitx5-configtool.
 */
struct VoiceInputConfig {
    // ── Keys ──────────────────────────────────────────────────────────
    fcitx::Key triggerKey{fcitx::Key("Alt_R")};

    // ── ASR ───────────────────────────────────────────────────────────
    std::string modelPath;          // Empty = use default search paths
    std::string modelName;          // e.g. "sherpa-onnx-zipformer-zh-14M"
    int numThreads = 4;

    // ── VAD ───────────────────────────────────────────────────────────
    int silenceThresholdMs = 500;   // Silence duration to trigger VAD stop
    float vadThreshold = 0.5f;      // VAD sensitivity

    // ── Scene ─────────────────────────────────────────────────────────
    std::string activeScene = "__raw__";
    std::string llmEndpoint;        // Empty = LLM disabled
    std::string llmApiKey;

    // ── Serialization ─────────────────────────────────────────────────
    nlohmann::json ToJson() const;
    static VoiceInputConfig FromJson(const nlohmann::json& j);
    static VoiceInputConfig Defaults() { return VoiceInputConfig{}; }
};

} // namespace fcitx
