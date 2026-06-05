#!/bin/bash
set -euo pipefail

BUSYBOX_VERSION="1.36.1"
BUSYBOX_URL="https://busybox.net/downloads/busybox-${BUSYBOX_VERSION}.tar.bz2"
CROSS_COMPILE="aarch64-linux-gnu-"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOTFS_DIR="${SCRIPT_DIR}/rootfs_staging"
BUILD_DIR="${SCRIPT_DIR}/build"

info()  { printf "\033[1;36m[INFO]\033[0m  %s\n" "$1"; }
ok()    { printf "\033[1;32m[OK]\033[0m    %s\n" "$1"; }
warn()  { printf "\033[1;33m[WARN]\033[0m  %s\n" "$1"; }
fail()  { printf "\033[1;31m[FAIL]\033[0m  %s\n" "$1"; exit 1; }

create_fhs_directories() {
    info "Creating FHS directory structure..."
    rm -rf "${ROOTFS_DIR}"
    mkdir -p "${ROOTFS_DIR}"/{bin,sbin,usr/bin,usr/sbin,lib,lib64,proc,sys,dev,dev/shm,tmp,run,etc/init.d,var/log,var/run,root,home,mnt,opt}
    ok "FHS directory structure created"
}

create_device_nodes() {
    info "Creating essential device nodes..."
    # Rootfs uses devtmpfs so manual mknod is typically avoided here unless strictly necessary for bootloader
    ok "Device nodes handled"
}

create_init_configs() {
    info "Creating init configuration files..."

    cat > "${ROOTFS_DIR}/etc/inittab" << 'INITTAB_EOF'
::sysinit:/etc/init.d/rcS
::askfirst:-/bin/sh
::shutdown:/bin/umount -a -r
::shutdown:/sbin/swapoff -a
::ctrlaltdel:/sbin/reboot
INITTAB_EOF

    cat > "${ROOTFS_DIR}/etc/init.d/rcS" << 'RCS_EOF'
#!/bin/sh
mountpoint -q /dev || mount -t devtmpfs devtmpfs /dev
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t tmpfs tmpfs /tmp
mount -t tmpfs tmpfs /run
mkdir -p /dev/shm
mount -t tmpfs tmpfs /dev/shm

hostname aarch64-embedded
echo "aarch64-embedded" > /etc/hostname

echo "    TELEMETRY SYSTEM AUTO-START       "

echo "[*] Configuring network..."
ifconfig eth0 10.0.2.15 netmask 255.255.255.0 up 2>/dev/null
route add default gw 10.0.2.2 2>/dev/null

echo "[*] Loading telemetry_sensor.ko..."
insmod /lib/modules/telemetry_sensor.ko

echo "[*] Starting sensor_reader in background..."
sensor_reader > /dev/null 2>&1 &
sleep 1

echo "[*] Starting logger_app in background..."
logger_app > /dev/null 2>&1 &

echo "[*] Starting httpd web server on port 8080..."
httpd -p 8080 -h /var/www

RCS_EOF
    chmod +x "${ROOTFS_DIR}/etc/init.d/rcS"

    cat > "${ROOTFS_DIR}/etc/passwd" << 'EOF'
root:x:0:0:root:/root:/bin/sh
daemon:x:1:1:daemon:/usr/sbin:/bin/false
nobody:x:65534:65534:nobody:/nonexistent:/bin/false
EOF

    cat > "${ROOTFS_DIR}/etc/group" << 'EOF'
root:x:0:
daemon:x:1:
nogroup:x:65534:
EOF

    cat > "${ROOTFS_DIR}/etc/shadow" << 'EOF'
root::0:0:99999:7:::
daemon:*:0:0:99999:7:::
nobody:*:0:0:99999:7:::
EOF
    chmod 600 "${ROOTFS_DIR}/etc/shadow"

    cat > "${ROOTFS_DIR}/etc/fstab" << 'EOF'
devtmpfs     /dev     devtmpfs   defaults           0      0
proc         /proc    proc       defaults           0      0
sysfs        /sys     sysfs      defaults           0      0
tmpfs        /tmp     tmpfs      defaults,size=64m  0      0
tmpfs        /run     tmpfs      defaults,size=16m  0      0
tmpfs        /dev/shm tmpfs      defaults,size=16m  0      0
EOF

    cat > "${ROOTFS_DIR}/etc/profile" << 'EOF'
export PATH="/bin:/sbin:/usr/bin:/usr/sbin"
export HOME="/root"
export PS1='\[\033[1;32m\]\u@\h\[\033[0m\]:\[\033[1;34m\]\w\[\033[0m\]\$ '
alias ll='ls -la'
alias la='ls -A'
EOF

    ok "Init configuration files created"
}

build_busybox_docker() {
    info "Cross-compiling BusyBox ${BUSYBOX_VERSION} via Docker..."

    TARBALL="busybox-${BUSYBOX_VERSION}.tar.bz2"
    if [ ! -f "${SCRIPT_DIR}/${TARBALL}" ]; then
        wget -q "${BUSYBOX_URL}" -O "${SCRIPT_DIR}/${TARBALL}"
    fi

    docker build -t busybox-aarch64 -f "${SCRIPT_DIR}/Dockerfile" "${SCRIPT_DIR}"

    docker create --name bb-extract busybox-aarch64 > /dev/null 2>&1
    docker cp bb-extract:/rootfs - | tar xf - -C "${ROOTFS_DIR}" --strip-components=1
    docker rm bb-extract > /dev/null 2>&1
    ok "BusyBox cross-compile and install complete"
}

install_filesync() {
    FILESYNC_BIN="${SCRIPT_DIR}/../source_code/filesync/filesync-aarch64"
    if [ -f "${FILESYNC_BIN}" ]; then
        cp "${FILESYNC_BIN}" "${ROOTFS_DIR}/usr/bin/filesync"
        chmod +x "${ROOTFS_DIR}/usr/bin/filesync"
        ok "filesync installed at /usr/bin/filesync"
    fi
}

install_ipc() {
    BUILD_DIR="${SCRIPT_DIR}/../build"
    SENSOR_BIN="${BUILD_DIR}/sensor_reader"
    LOGGER_BIN="${BUILD_DIR}/logger_app"
    DRIVER_BIN="${SCRIPT_DIR}/../source_code/driver/telemetry_sensor.ko"
    if [ -f "${SENSOR_BIN}" ] && [ -f "${LOGGER_BIN}" ]; then
        cp "${SENSOR_BIN}" "${ROOTFS_DIR}/usr/bin/sensor_reader"
        cp "${LOGGER_BIN}" "${ROOTFS_DIR}/usr/bin/logger_app"
        chmod +x "${ROOTFS_DIR}/usr/bin/sensor_reader"
        chmod +x "${ROOTFS_DIR}/usr/bin/logger_app"
        ok "IPC binaries installed at /usr/bin/"
    else
        warn "IPC binaries not found — run 'make all' at project root first"
    fi
    if [ -f "${DRIVER_BIN}" ]; then
        mkdir -p "${ROOTFS_DIR}/lib/modules"
        cp "${DRIVER_BIN}" "${ROOTFS_DIR}/lib/modules/"
        ok "telemetry_sensor.ko driver installed at /lib/modules/"
    fi
}

install_web() {
    WEB_DIR="${SCRIPT_DIR}/../web"
    if [ -d "${WEB_DIR}" ]; then
        mkdir -p "${ROOTFS_DIR}/var/www/cgi-bin"
        cp "${WEB_DIR}/index.html" "${ROOTFS_DIR}/var/www/"
        cp "${WEB_DIR}/cgi-bin/telemetry.cgi" "${ROOTFS_DIR}/var/www/cgi-bin/"
        chmod +x "${ROOTFS_DIR}/var/www/cgi-bin/telemetry.cgi"

        cat > "${ROOTFS_DIR}/var/www/cgi-bin/get_api_key.cgi" << 'API_EOF'
#!/bin/sh
echo "Content-Type: text/plain"
echo ""
API_EOF
        if [ -f "${SCRIPT_DIR}/../.env" ]; then
            # Read the value from the host's .env file
            HOST_API_KEY=$(grep "GEMINI_API_KEY" "${SCRIPT_DIR}/../.env" | cut -d'=' -f2)
            echo "echo \"${HOST_API_KEY}\"" >> "${ROOTFS_DIR}/var/www/cgi-bin/get_api_key.cgi"
        else
            echo "echo \"\"" >> "${ROOTFS_DIR}/var/www/cgi-bin/get_api_key.cgi"
        fi
        chmod +x "${ROOTFS_DIR}/var/www/cgi-bin/get_api_key.cgi"

        ok "Web dashboard installed at /var/www/"
    else
        warn "Web directory not found, skipping dashboard"
    fi
}

install_verify() {
    cat > "${ROOTFS_DIR}/usr/bin/verify" << 'VERIFY_EOF'
#!/bin/sh
echo ""
echo "=== SYSTEM VERIFICATION ==="
echo ""

echo "[1] Running Processes:"
ps | grep -E "sensor_reader|logger_app" | grep -v grep
if [ $? -eq 0 ]; then
    echo "    -> PASS: IPC processes are running"
else
    echo "    -> FAIL: IPC processes not found"
fi
echo ""

echo "[2] Telemetry Log File (/tmp/telemetry.log):"
if [ -f /tmp/telemetry.log ]; then
    LINES=$(wc -l < /tmp/telemetry.log)
    echo "    -> PASS: Log file exists ($LINES lines written)"
    echo "    -> Last 3 entries:"
    tail -3 /tmp/telemetry.log | sed 's/^/       /'
else
    echo "    -> FAIL: Log file not found"
fi
echo ""

echo "[3] Shared Memory (IPC):"
if [ -f /dev/shm/telemetry_shm ]; then
    echo "    -> PASS: Shared memory segment active"
else
    echo "    -> FAIL: Shared memory not found"
fi
echo ""

echo "[4] Kernel Module:"
if [ -f /lib/modules/telemetry_sensor.ko ]; then
    echo "    -> INFO: telemetry_sensor.ko present in /lib/modules/"
fi
echo ""

echo "[5] Kernel Messages (last 5):"
dmesg | tail -5 | sed 's/^/    /'
echo ""
echo "=== VERIFICATION COMPLETE ==="
echo ""
VERIFY_EOF
    chmod +x "${ROOTFS_DIR}/usr/bin/verify"
    ok "verify script installed at /usr/bin/verify"
}

create_initramfs() {
    info "Creating initramfs image..."
    cd "${ROOTFS_DIR}"
    find . | cpio -o -H newc 2>/dev/null | gzip > "${SCRIPT_DIR}/rootfs.cpio.gz"
    cd "${SCRIPT_DIR}"
    ok "rootfs.cpio.gz created"
}

main() {
    create_fhs_directories
    create_init_configs
    build_busybox_docker
    create_device_nodes
    install_filesync
    install_ipc
    install_web
    install_verify
    create_initramfs
}

main "$@"
