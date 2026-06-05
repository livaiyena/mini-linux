#!/bin/bash
#fast build file for full system
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"

info()  { printf "\033[1;36m[INFO]\033[0m  %s\n" "$1"; }
ok()    { printf "\033[1;32m[OK]\033[0m    %s\n" "$1"; }
fail()  { printf "\033[1;31m[FAIL]\033[0m  %s\n" "$1"; exit 1; }

command -v docker >/dev/null 2>&1 || fail "Docker is not installed."

info "Step 1/6: Building Docker build environment..."
make -C "${ROOT_DIR}" docker-build
ok "Docker image ready"

info "Step 2/6: Compiling Linux Kernel (needed for Driver headers)..."
bash "${ROOT_DIR}/qemu/build_qemu_env.sh" kernel
ok "Kernel Image ready"

info "Step 3/6: Cross-compiling IPC binaries and kernel module..."
docker run --rm -v "${ROOT_DIR}":/project embedded-linux-env make all
ok "All binaries compiled"

info "Step 4/6: Generating BusyBox RootFS..."
bash "${ROOT_DIR}/rootfs/build_rootfs.sh"
ok "RootFS created"

info "Step 5/6: Creating ext4 disk image..."
bash "${ROOT_DIR}/qemu/build_qemu_env.sh" disk
ok "Disk image ready"

info "Step 6/6: Booting QEMU simulation..."
bash "${ROOT_DIR}/qemu/build_qemu_env.sh" boot-docker
