# fcitx5-voice-input

[English](#english) | [中文](#中文)

---

## English

**fcitx5-voice-input** is a Fcitx5 addon for voice input. It captures audio via PulseAudio (or PipeWire fallback), detects speech with a lightweight VAD (Voice Activity Detection), and transcribes via OpenAI-compatible API (Groq, OpenAI, Together AI, etc.).

**Features**
- Speaker-independent Chinese speech recognition
- VAD-based automatic recording segmentation
- Optional LLM post-processing to correct transcription errors
- Simple configuration via `fcitx5-configtool`

**Dependencies** `fcitx5`, `libpulse-simple`, `libpipewire-0.3`, `nlohmann-json`, `libcurl`

**ASR Backends** OpenAI-compatible API (Whisper) — use with Groq, OpenAI, DeepSeek, etc.

---

## 中文

**fcitx5-voice-input** 是一个 Fcitx5 语音输入插件。通过 PulseAudio（或 PipeWire fallback）捕获音频，使用轻量 VAD 检测人声，通过 OpenAI 兼容 API 进行语音识别。

**功能**
- 中文语音输入
- VAD 自动分段录音
- 可选的 LLM 后处理纠错
- 通过 `fcitx5-configtool` 简单配置

**依赖** `fcitx5`, `libpulse-simple`, `libpipewire-0.3`, `nlohmann-json`, `libcurl`

**ASR 后端** OpenAI 兼容 API（Whisper）— 可搭配 Groq、OpenAI、DeepSeek 等使用
