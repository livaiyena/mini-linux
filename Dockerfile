FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-aarch64-linux-gnu \
    binutils-aarch64-linux-gnu \
    qemu-system-arm \
    qemu-user-static \
    bc \
    bison \
    flex \
    libssl-dev \
    make \
    libc6-dev-arm64-cross \
    linux-headers-generic \
    cpio \
    rsync \
    kmod \
    file \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /project

ENV CROSS_COMPILE=aarch64-linux-gnu-
ENV ARCH=arm64
ENV CC=${CROSS_COMPILE}gcc

CMD ["make", "all"]
