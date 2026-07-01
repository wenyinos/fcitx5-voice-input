<div align="center">

<p align="center">
  <img width="1774" alt="Banner" src="https://github.com/user-attachments/assets/d3fe33eb-6c91-4000-925b-99eb83a498a7" />
</p>

# fcitx5-voice-input

<p>
  <a href="https://github.com/devcxl/fcitx5-voice-input/actions/workflows/build.yml"><img src="https://img.shields.io/github/actions/workflow/status/devcxl/fcitx5-voice-input/build.yml?branch=main&logo=github&label=build" alt="Build"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-LGPL%20v3-blue.svg" alt="License"></a>
  <img src="https://img.shields.io/badge/platform-Linux-important" alt="Platform">
  <img src="https://img.shields.io/badge/fcitx5-%3E%3D5.1.19-blueviolet" alt="Fcitx5">
  <img src="https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus" alt="C++20">
</p>

[English](README.md) | **中文**

---

</div>

**fcitx5-voice-input** 是一个 Fcitx5 语音输入插件。通过 PulseAudio（或 PipeWire fallback）捕获音频，使用 Silero ONNX VAD 检测人声分段，通过 OpenAI 兼容 API 进行语音识别。

## 功能

- 通过 OpenAI 兼容 API 进行语音识别（Whisper、Groq、SiliconFlow 等）
- **小米 MiMo ASR**（`mimo-v2.5-asr`）原生支持
- **Chat Completions API 格式**（`/chat/completions` + JSON Base64），兼容不支持 Whisper 端点的服务商
- **两种录音模式**：
  - **VAD 自动分段**：Silero ONNX VAD 自动检测语音边界（免按键）
  - **Push-to-Talk**：按住热键（默认 Right Ctrl）录音，松开提交（隐私友好）
- 从 API 获取可用模型列表，下拉选择
- LLM 后处理（纠错 / 翻译 / 格式化）
- 通过 `fcitx5-configtool` 图形化配置
- 窗口快速切换自动延迟停止，防止误停

<p align="center">
  <img width="720" alt="演示" src="https://github.com/user-attachments/assets/48164962-deba-4328-bf26-70cd258f86a6" />
</p>

## 使用

### 1. 安装

#### 手动编译安装

见下方 [编译构建](#编译构建)。

#### Fedora RPM

```bash
rpmbuild -ba fcitx5-voice-input.spec
sudo dnf install ~/rpmbuild/RPMS/x86_64/fcitx5-voice-input-*.rpm
```

### 2. 配置

安装后，打开 `fcitx5-configtool`，在 Input Method 列表中找到 **Voice Input** 并添加到输入法列表。

然后在 Addon 配置中找到 **VoiceInput**，配置以下关键项：

| 配置项 | 说明 | 默认值 |
|--------|------|--------|
| `ASR Backend` | ASR 后端（`openai` / `mimo`） | `openai` |
| `API Format` | API 格式：Multipart Form 或 JSON Base64 | `whisper` |
| `Recording Mode` | 录音模式：VAD 自动分段或 Push-to-Talk | `vad` |
| `Push-to-Talk Hotkey` | PTT 模式热键 | Right Ctrl |
| `OpenAI API Endpoint` | API 地址 | `https://api.openai.com/v1` |
| `OpenAI API Key` | API Key | **（必填）** |
| `Voice Model` | 模型名 | `whisper-1` |
| `Output Language` | 输出语言，留空自动检测 | （空） |
| `VAD Threshold (%)` | VAD 灵敏度 (0-100)，越高越不易触发 | `50` |
| `Silence Threshold (ms)` | 静音多久结束说话 (ms) | `800` |

**API Key 配置**：在 `OpenAI API Key` 中填入你的 API Key。支持所有 OpenAI 兼容服务，如：

- [OpenAI](https://platform.openai.com/) — `https://api.openai.com/v1`
- [Groq](https://console.groq.com/) — `https://api.groq.com/openai/v1`
- [硅基流动 (SiliconFlow)](https://siliconflow.cn/) — `https://api.siliconflow.cn/v1`
- [小米 MiMo](https://mimo.mi.com/) — ASR Backend 选 `Xiaomi MiMo ASR`

**获取可用模型**：填写 Endpoint 和 API Key 后，勾选 **Fetch Available Models** → Apply → 重新打开配置即可从下拉框选择模型。

### 3. 使用

**VAD 模式（默认）：**
1. 切换到 **Voice Input** 输入法
2. 开始说话，VAD 自动检测人声并录音
3. 停止说话（默认 800ms 静音超时），自动发送 ASR 识别
4. 识别结果自动上屏
5. 保持语音输入模式，继续说话可连续识别

**Push-to-Talk 模式：**
1. 切换到 **Voice Input** 输入法
2. 按住热键（默认 Right Ctrl）— 开始录音
3. 松开热键 — 音频发送 ASR 识别
4. 识别结果自动上屏

切换窗口时插件会自动延迟 200ms 停止，快速切回会取消停止，避免不必要的重启。

## 编译构建

### 依赖

- `fcitx5` — 输入法框架
- `libpulse-simple` — PulseAudio 音频捕获（优先）
- `libpipewire-0.3` — PipeWire 音频捕获（fallback）
- `jsoncpp` — JSON 解析
- `libcurl` — HTTP 客户端（ASR 必需）
- `onnxruntime` — Silero VAD ONNX Runtime

> **Arch Linux:** `sudo pacman -S fcitx5 pulseaudio pipewire jsoncpp curl onnxruntime-cpu`
>
> **Debian/Ubuntu:** `sudo apt install fcitx5 libpulse-dev libpipewire-0.3-dev libjsoncpp-dev libcurl4-openssl-dev libonnxruntime-dev`
>
> **Fedora:** `sudo dnf install fcitx5-devel pipewire-devel pulseaudio-libs-devel jsoncpp-devel libcurl-devel onnxruntime-devel`

### 编译

```bash
# 克隆并初始化子模块（获取 Silero VAD 模型）
git clone https://github.com/devcxl/fcitx5-voice-input.git
cd fcitx5-voice-input
git submodule update --init --recursive

# 配置
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr

# 编译
cmake --build build -j"$(nproc)"

# 安装
sudo cmake --install build --prefix /usr
```

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_TESTS` | `OFF` | 构建测试 |
| `ONNXRUNTIME_ROOT` | — | ONNX Runtime 自定义安装路径 |


## 注意事项

- **API Key 安全**：API Key 明文存储在 fcitx5 配置文件中（`~/.config/fcitx5/conf/voiceinput.conf`），请注意文件权限
- **网络要求**：云端 ASR 后端需要网络连接。本地 ASR 可通过 AsrEngine 接口后续扩展
- **音频设备**：默认自动选择系统音频输入设备。如需指定，在下拉框中选择。仅支持输入源（Source），不支持 Monitor 源
- **VAD 模型**：Silero VAD 模型通过 git submodule 分发（`third_party/silero-vad/`），编译时自动复制到安装目录。Push-to-Talk 模式不需要此模型
- **PipeWire 用户**：PulseAudio 后端也能在 pipewire-pulse 下正常工作，仅在 PulseAudio 完全不可用时 fallback 到 PipeWire 直连
- **窗口切换**：快速切换窗口时插件使用延迟停止机制（200ms），不会频繁重启流水线。长时间切出后会自动停止

## 架构简介

```
音频捕获线程 → FrameQueue → VAD Worker 线程 → UtteranceQueue → ASR Worker 线程 → ResultQueue → EventDispatcher → commitString
```

三个工作线程 + 主线程，通过 `ThreadSafeQueue` 连接各阶段。详见 [ARCHITECTURE.md](ARCHITECTURE.md)。

## 许可证

GNU Lesser General Public License v3.0
