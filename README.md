# Embedded Linux Telemetry System

This project demonstrates a complete, from-scratch Embedded Linux environment designed for an ARM64 architecture (AArch64). It features a custom I2C character device driver, a high-performance IPC telemetry pipeline, a BusyBox-based minimal RootFS, and automated QEMU simulation.

Everything is containerized via Docker to ensure it compiles reliably on any host machine without dependency issues.

## Project Architecture
1. **Kernel Driver (`source_code/driver/`)**: A Linux Loadable Kernel Module (LKM) acting as an I2C character driver for a dummy telemetry sensor.
2. **IPC Applications (`source_code/ipc/`)**: 
   - `sensor_reader`: Reads data from the kernel driver and pushes it to POSIX Shared Memory.
   - `logger_app`: Securely logs the shared memory data to disk with power-loss resilience.
3. **RootFS (`rootfs/`)**: Scripts to cross-compile BusyBox and generate a minimal standard Linux filesystem hierarchy.
4. **QEMU Simulator (`qemu/`)**: Scripts to download and compile the Linux Kernel (v6.6) from source and boot the entire system virtually.

---

## 🚀 Quick Start Guide (For Evaluators/Teachers)

**No native ARM toolchains are required; Docker handles everything.**

### One-Command Run (Recommended)
Clone the repo and run a single command to compile everything from scratch and boot the simulation:
```bash
bash run.sh
```
This will automatically: build the Docker environment → cross-compile all C code → generate the RootFS → compile the Linux Kernel → create the disk image → boot QEMU.

---

### Manual Steps (If Preferred)

### Step 1: Prepare the Build Environment
Build the Docker image containing the cross-compilation toolchains:
```bash
make docker-build
```

### Step 2: Cross-Compile the Source Code
Enter the isolated Docker build environment:
```bash
make docker-run
```
Once inside the container (your prompt will change), compile all binaries and the kernel module:
```bash
make all
exit
```

### Step 3: Generate the Root Filesystem (RootFS)
Now that the binaries are compiled in the `build/` directory, create the Linux filesystem and inject the binaries into it:
```bash
bash rootfs/build_rootfs.sh
```

### Step 4: Build the Kernel and Ext4 Disk Image
Download the Linux kernel source, compile it for ARM64, and format the RootFS into an `.ext4` raw disk image.
*(Note: This step may take 5-15 minutes depending on CPU speed and will ask for sudo permissions to mount the loop device for ext4 creation).*
```bash
bash qemu/build_qemu_env.sh all
```

### Step 5: Boot the QEMU Simulation
Launch the virtual ARM computer! 
```bash
bash qemu/build_qemu_env.sh boot-docker
```

**What to expect on boot:**
When QEMU boots, the custom `rcS` script will automatically:
1. Load the `telemetry_sensor.ko` driver.
2. Start the `logger_app` in the background.
3. Start the `sensor_reader` in the background.
You will immediately see telemetry data successfully streaming and being logged via IPC.

To exit QEMU at any time: Press `Ctrl + A`, release, then press `X`.