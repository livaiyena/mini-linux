FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        gcc \
        libc6-dev \
        gcc-aarch64-linux-gnu \
        libc6-dev-arm64-cross \
        make \
        bc \
        bison \
        flex \
        libssl-dev \
        libelf-dev \
        wget \
        xz-utils \
        file \
        e2fsprogs \
        cpio \
        qemu-system-arm \
        ipxe-qemu \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
