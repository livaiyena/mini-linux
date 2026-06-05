#!/bin/bash
set -euo pipefail

KERNEL_VERSION="6.6.87"
KERNEL_URL="https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${KERNEL_VERSION}.tar.xz"
DISK_SIZE_MB=50

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOTFS_STAGING="${SCRIPT_DIR}/../rootfs/rootfs_staging"

info()  { printf "\033[1;36m[INFO]\033[0m  %s\n" "$1"; }
ok()    { printf "\033[1;32m[OK]\033[0m    %s\n" "$1"; }
fail()  { printf "\033[1;31m[FAIL]\033[0m  %s\n" "$1"; exit 1; }

download_kernel() {
    TARBALL="linux-${KERNEL_VERSION}.tar.xz"
    if [ ! -f "${SCRIPT_DIR}/${TARBALL}" ]; then
        info "Downloading Linux kernel ${KERNEL_VERSION}..."
        wget -q --show-progress "${KERNEL_URL}" -O "${SCRIPT_DIR}/${TARBALL}"
        ok "Kernel tarball downloaded"
    else
        ok "Kernel tarball already exists, skipping download"
    fi
}

build_kernel_docker() {
    info "Building kernel image via Docker..."

    docker build -t qemu-aarch64-env -f "${SCRIPT_DIR}/Dockerfile" "${SCRIPT_DIR}"

    docker run --rm \
        -v "${SCRIPT_DIR}:/workspace" \
        -v "${SCRIPT_DIR}/linux-${KERNEL_VERSION}.tar.xz:/workspace/linux-${KERNEL_VERSION}.tar.xz:ro" \
        qemu-aarch64-env \
        bash -c "
            cd /workspace
            if [ ! -d linux-${KERNEL_VERSION} ]; then
                tar xf linux-${KERNEL_VERSION}.tar.xz
            fi
            cd linux-${KERNEL_VERSION}
            make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
            make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j\$(nproc) Image
            cp arch/arm64/boot/Image /workspace/Image
            file /workspace/Image
        "

    ok "Kernel Image built: ${SCRIPT_DIR}/Image"
}

create_ext4_image() {
    info "Creating ${DISK_SIZE_MB}MB ext4 disk image..."

    if [ ! -d "${ROOTFS_STAGING}" ] || [ -z "$(ls -A "${ROOTFS_STAGING}" 2>/dev/null)" ]; then
        fail "rootfs_staging not found. Run rootfs/build_rootfs.sh first."
    fi

    REAL_USER="$(id -un)"
    REAL_GROUP="$(id -gn)"

    dd if=/dev/zero of="${SCRIPT_DIR}/rootfs.ext4" bs=1M count=${DISK_SIZE_MB}
    mkfs.ext4 -F -L rootfs "${SCRIPT_DIR}/rootfs.ext4"

    MOUNT_DIR=$(mktemp -d)
    sudo mount -o loop "${SCRIPT_DIR}/rootfs.ext4" "${MOUNT_DIR}"
    sudo cp -a "${ROOTFS_STAGING}"/* "${MOUNT_DIR}"/
    sudo sync
    sudo umount "${MOUNT_DIR}"
    rmdir "${MOUNT_DIR}"
    sudo chown "${REAL_USER}:${REAL_GROUP}" "${SCRIPT_DIR}/rootfs.ext4"

    ok "rootfs.ext4 created (${DISK_SIZE_MB}MB)"
}

boot_qemu() {
    info "Launching QEMU..."

    KERNEL_IMAGE="${SCRIPT_DIR}/Image"
    DISK_IMAGE="${SCRIPT_DIR}/rootfs.ext4"

    [ -f "${KERNEL_IMAGE}" ] || fail "Kernel Image not found. Run build first."
    [ -f "${DISK_IMAGE}" ]   || fail "rootfs.ext4 not found. Run build first."

    echo ""
    echo "========================================"
    echo "  QEMU AArch64 Boot"
    echo "  Kernel: linux-${KERNEL_VERSION}"
    echo "  RootFS: rootfs.ext4 (ext4, ${DISK_SIZE_MB}MB)"
    echo "  Web:    http://localhost:8080"
    echo "  Exit:   Ctrl-A then X"
    echo "========================================"
    echo ""

    qemu-system-aarch64 \
        -machine virt \
        -cpu cortex-a72 \
        -m 256M \
        -nographic \
        -kernel "${KERNEL_IMAGE}" \
        -drive file="${DISK_IMAGE}",format=raw,if=virtio \
        -netdev user,id=net0,hostfwd=tcp::8080-:8080 \
        -device virtio-net-pci,netdev=net0 \
        -append "root=/dev/vda rw console=ttyAMA0 earlycon=pl011,0x09000000 panic=5"
}

boot_qemu_docker() {
    info "Launching QEMU inside Docker..."

    KERNEL_IMAGE="${SCRIPT_DIR}/Image"
    DISK_IMAGE="${SCRIPT_DIR}/rootfs.ext4"

    [ -f "${KERNEL_IMAGE}" ] || fail "Kernel Image not found. Run build first."
    [ -f "${DISK_IMAGE}" ]   || fail "rootfs.ext4 not found. Run build first."

    echo ""
    echo "========================================"
    echo "  QEMU AArch64 Boot (Docker)"
    echo "  Kernel: linux-${KERNEL_VERSION}"
    echo "  RootFS: rootfs.ext4 (ext4, ${DISK_SIZE_MB}MB)"
    echo "  Web:    http://localhost:8080"
    echo "  Exit:   Ctrl-A then X"
    echo "========================================"
    echo ""

    docker run --rm -it \
        -v "${SCRIPT_DIR}:/workspace" \
        -p 8080:8080 \
        qemu-aarch64-env \
        qemu-system-aarch64 \
            -machine virt \
            -cpu cortex-a72 \
            -m 256M \
            -nographic \
            -kernel /workspace/Image \
            -drive file=/workspace/rootfs.ext4,format=raw,if=virtio \
            -netdev user,id=net0,hostfwd=tcp::8080-:8080 \
            -device virtio-net-pci,netdev=net0 \
            -append "root=/dev/vda rw console=ttyAMA0 earlycon=pl011,0x09000000 panic=5"
}

usage() {
    echo "Usage: $0 <command>"
    echo ""
    echo "Commands:"
    echo "  download    Download Linux kernel source"
    echo "  kernel      Cross-compile kernel via Docker"
    echo "  disk        Create ext4 rootfs disk image"
    echo "  boot        Launch QEMU (host qemu-system-aarch64)"
    echo "  boot-docker Launch QEMU inside Docker container"
    echo "  all         download + kernel + disk"
    echo ""
}

case "${1:-}" in
    download)   download_kernel ;;
    kernel)     download_kernel; build_kernel_docker ;;
    disk)       create_ext4_image ;;
    boot)       boot_qemu ;;
    boot-docker) boot_qemu_docker ;;
    all)        download_kernel; build_kernel_docker; create_ext4_image ;;
    *)          usage; exit 1 ;;
esac
