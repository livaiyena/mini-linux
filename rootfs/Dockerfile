FROM ubuntu:22.04

# Install cross-compilation toolchain and dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        gcc \
        libc6-dev \
        gcc-aarch64-linux-gnu \
        libc6-dev-arm64-cross \
        make \
        bzip2 \
        file \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# The tarball is copied from the host to avoid DNS/network issues inside docker
# Make sure to place busybox-*.tar.bz2 in the build context
COPY busybox-*.tar.bz2 .
RUN tar xjf busybox-*.tar.bz2

# Build static BusyBox
# BusyBox version will be determined by the tarball name
RUN cd busybox-* && \
    make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig && \
    sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/' .config && \
    make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) && \
    make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- CONFIG_PREFIX=/rootfs install

CMD ["file", "/rootfs/bin/busybox"]
