#include "pipewire_capture.h"

#include <cstring>
#include <spa/param/audio/format-utils.h>

namespace fcitx {

PipeWireCapture::PipeWireCapture()
    : ringBuffer_(std::make_unique<AudioRingBuffer>(65536))
{
}

PipeWireCapture::~PipeWireCapture() {
    Stop();
}

bool PipeWireCapture::Start() {
    if (running_) return true;

    pw_init(nullptr, nullptr);

    // ── Create main loop ──────────────────────────────────────────────
    loop_ = pw_thread_loop_new("voice-input-capture", nullptr);
    if (!loop_) return false;

    context_ = pw_context_new(pw_thread_loop_get_loop(loop_), nullptr, 0);
    if (!context_) {
        pw_thread_loop_destroy(loop_);
        loop_ = nullptr;
        return false;
    }

    core_ = pw_context_connect(context_, nullptr, 0);
    if (!core_) {
        pw_context_destroy(context_);
        context_ = nullptr;
        pw_thread_loop_destroy(loop_);
        loop_ = nullptr;
        return false;
    }

    // ── Create stream ──────────────────────────────────────────────────
    // Note: pw_properties_new returns non-const in PipeWire 1.0.5.
    // PW_KEY_ADAPTIVE_API was added in 1.2.x; omit on older versions.
    struct pw_properties* props =
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Capture",
            PW_KEY_MEDIA_ROLE, "Communication",
            PW_KEY_NODE_NAME, "voice-input-capture",
            PW_KEY_NODE_DESCRIPTION, "Voice Input Audio Capture",
            nullptr
        );

    stream_ = pw_stream_new(core_, "voice-input-capture", props);
    if (!stream_) {
        pw_core_disconnect(core_);
        core_ = nullptr;
        pw_context_destroy(context_);
        context_ = nullptr;
        pw_thread_loop_destroy(loop_);
        loop_ = nullptr;
        return false;
    }

    // ── Configure audio format ────────────────────────────────────────
    static const struct pw_stream_events stream_events = {
        .version = PW_VERSION_STREAM_EVENTS,
        .process = &PipeWireCapture::OnProcess,
    };

    pw_stream_add_listener(stream_, &streamListener_, &stream_events, this);

    uint8_t buffer[1024];

    spa_audio_info_raw audio_info = {};
    audio_info.format = SPA_AUDIO_FORMAT_F32;
    audio_info.channels = 1;
    audio_info.rate = 16000;

    struct spa_pod_builder podBuilder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    const spa_pod* params[1];
    params[0] = spa_format_audio_raw_build(&podBuilder,
                                           SPA_PARAM_EnumFormat,
                                           &audio_info);

    pw_stream_connect(stream_,
                      PW_DIRECTION_INPUT,
                      PW_ID_ANY,
                      static_cast<pw_stream_flags>(
                          PW_STREAM_FLAG_AUTOCONNECT |
                          PW_STREAM_FLAG_MAP_BUFFERS |
                          PW_STREAM_FLAG_RT_PROCESS),
                      params, 1);

    // ── Start loop ────────────────────────────────────────────────────
    pw_thread_loop_start(loop_);
    running_ = true;
    return true;
}

void PipeWireCapture::Stop() {
    if (!running_) return;
    running_ = false;

    if (loop_) {
        pw_thread_loop_stop(loop_);
    }
    if (stream_) {
        pw_stream_destroy(stream_);
        stream_ = nullptr;
    }
    if (core_) {
        pw_core_disconnect(core_);
        core_ = nullptr;
    }
    if (context_) {
        pw_context_destroy(context_);
        context_ = nullptr;
    }
    if (loop_) {
        pw_thread_loop_destroy(loop_);
        loop_ = nullptr;
    }
}

void PipeWireCapture::OnProcess(void* userdata) {
    auto* self = static_cast<PipeWireCapture*>(userdata);
    self->OnProcessImpl();
}

void PipeWireCapture::OnProcessImpl() {
    // ══════════════════════════════════════════════════════════════════
    // THIS RUNS INSIDE PW_THREAD_LOOP LOCK
    //   - Do NOT allocate large objects
    //   - Do NOT run ASR / VAD / JSON serialization
    //   - Do NOT hold std::mutex
    //   - Do NOT call Fcitx UI functions
    // ══════════════════════════════════════════════════════════════════

    pw_buffer* buf = pw_stream_dequeue_buffer(stream_);
    if (!buf) return;

    struct spa_buffer* spa_buf = buf->buffer;
    if (spa_buf->n_datas == 0) return;

    void* src = spa_buf->datas[0].data;
    uint32_t size = spa_buf->datas[0].chunk->size;

    if (!src || size == 0) {
        pw_stream_queue_buffer(stream_, buf);
        return;
    }

    size_t frames = size / sizeof(float);
    const float* pcm = static_cast<const float*>(src);

    // Only operation: PCM frame → ring buffer
    ringBuffer_->Write(pcm, frames);

    // Optional raw callback (for testing/monitoring only)
    if (rawCallback_) {
        rawCallback_(pcm, frames);
    }

    pw_stream_queue_buffer(stream_, buf);
}

} // namespace fcitx
