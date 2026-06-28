FROM archlinux:base-devel

RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm --needed \
        cmake \
        pkg-config \
        onnxruntime-cpu \
        sudo && \
    pacman -Scc --noconfirm

RUN useradd -m builder && \
    echo "builder ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

WORKDIR /build
CMD ["su", "builder", "-c", "makepkg -f --noconfirm"]
