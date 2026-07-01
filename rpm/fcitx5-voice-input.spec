Name:           fcitx5-voice-input
Version:        0.1.0
Release:        1%{?dist}
Summary:        Fcitx5 voice input addon with ASR via OpenAI-compatible API
License:        LGPL-3.0-or-later
URL:            https://github.com/devcxl/fcitx5-voice-input
Source0:        %{name}-%{version}.tar.gz

%define debug_package %{nil}

BuildRequires:  cmake >= 3.20
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig(Fcitx5Core)
BuildRequires:  pkgconfig(libpipewire-0.3)
BuildRequires:  pkgconfig(libpulse-simple)
BuildRequires:  pkgconfig(libpulse)
BuildRequires:  pkgconfig(jsoncpp)
BuildRequires:  pkgconfig(libcurl)
BuildRequires:  pkgconfig(libonnxruntime)
BuildRequires:  gettext

Requires:       fcitx5-libs
Requires:       pipewire-libs
Requires:       pulseaudio-libs
Requires:       jsoncpp
Requires:       libcurl
Requires:       onnxruntime

%description
fcitx5-voice-input is a Fcitx5 addon for voice input. It captures audio via
PulseAudio (or PipeWire fallback), detects speech segments with Silero ONNX
VAD, and transcribes via OpenAI-compatible API (OpenAI Whisper, Groq,
SiliconFlow, Xiaomi MiMo ASR, etc.).

%prep
%autosetup -n %{name}-%{version}

%build
%cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%{_prefix}
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%{_libdir}/fcitx5/voice-input-addon.so
%{_datadir}/fcitx5/addon/voiceinput.conf
%{_datadir}/fcitx5/voice-input/models/silero_vad.onnx
%{_datadir}/icons/hicolor/scalable/apps/fcitx5-voice-input.svg
%{_datadir}/locale/*/LC_MESSAGES/fcitx5-voice-input.mo

%changelog
* Wed Jul 01 2026 Wenyin Root <64475363+devcxl@users.noreply.github.com> - 0.1.0-1
- Initial RPM package
- PulseAudio / PipeWire audio capture
- Silero ONNX VAD speech segmentation
- OpenAI-compatible ASR backend
- Xiaomi MiMo ASR backend
- LLM post-processing support
- fcitx5-configtool graphical configuration
