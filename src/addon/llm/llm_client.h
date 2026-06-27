#pragma once

#include <string>
#include <functional>
#include <vector>

namespace fcitx {

class LLMClient {
public:
    struct Config {
        std::string endpoint;
        std::string apiKey;
        std::string model;
        std::string systemPrompt;
    };

    LLMClient(Config config);
    ~LLMClient();

    LLMClient(const LLMClient&) = delete;
    LLMClient& operator=(const LLMClient&) = delete;

    // Process text through LLM. Returns processed text on success,
    // or empty string on failure (caller should use original text).
    std::string Process(const std::string& text);

private:
    Config config_;
};

} // namespace fcitx
