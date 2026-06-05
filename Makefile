CROSS_COMPILE ?= aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -O2 -I./source_code/ipc/includes
LDFLAGS = -lpthread -lrt -static

# Look for the custom compiled ARM64 kernel first, then fallback to host kernel
CUSTOM_KERNEL := $(shell ls -d $(CURDIR)/qemu/linux-* 2>/dev/null | head -n 1)
KERNEL_DIR_UNAME := /lib/modules/$(shell uname -r)/build
KERNEL_DIR ?= $(if $(wildcard $(KERNEL_DIR_UNAME)),$(KERNEL_DIR_UNAME),$(shell ls -d /lib/modules/*/build 2>/dev/null | head -n 1))
ARCH ?= arm64

IPC_DIR = source_code/ipc/srcs
DRIVER_DIR = source_code/driver
BUILD_DIR = build
ROOTFS_DIR = $(BUILD_DIR)/rootfs

LOGGER_BIN = $(BUILD_DIR)/logger_app
SENSOR_BIN = $(BUILD_DIR)/sensor_reader

DOCKER_IMAGE = embedded-linux-env

all: ipc driver rootfs

ipc: $(LOGGER_BIN) $(SENSOR_BIN)

$(LOGGER_BIN): $(IPC_DIR)/logger_app.c $(IPC_DIR)/ipc_utils.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SENSOR_BIN): $(IPC_DIR)/sensor_reader.c $(IPC_DIR)/ipc_utils.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

driver:
	@echo "obj-m += telemetry_sensor.o" > $(DRIVER_DIR)/Makefile
	@if grep -q "x86" $(KERNEL_DIR)/.config 2>/dev/null; then \
		echo "Warning: Compiling kernel module for x86 host to check syntax (ARM kernel not found in KERNEL_DIR)"; \
		make -C $(KERNEL_DIR) M=$(CURDIR)/$(DRIVER_DIR) ARCH=x86 CROSS_COMPILE= modules; \
	else \
		make -C $(KERNEL_DIR) M=$(CURDIR)/$(DRIVER_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules; \
	fi

rootfs: ipc
	mkdir -p $(ROOTFS_DIR)/bin $(ROOTFS_DIR)/sbin $(ROOTFS_DIR)/etc $(ROOTFS_DIR)/proc $(ROOTFS_DIR)/sys $(ROOTFS_DIR)/dev $(ROOTFS_DIR)/lib $(ROOTFS_DIR)/usr/bin $(ROOTFS_DIR)/usr/sbin
	cp $(LOGGER_BIN) $(ROOTFS_DIR)/bin/
	cp $(SENSOR_BIN) $(ROOTFS_DIR)/bin/

qemu:
	qemu-system-aarch64 -machine virt -cpu cortex-a57 -m 1G -nographic \
		-kernel $(BUILD_DIR)/Image \
		-append "console=ttyAMA0 root=/dev/vda rw" \
		-drive file=$(BUILD_DIR)/rootfs.ext4,format=raw,if=virtio

docker-build:
	docker build -t $(DOCKER_IMAGE) .

docker-run:
	docker run --rm -it -v $(PWD):/project $(DOCKER_IMAGE) /bin/bash

clean:
	rm -rf $(BUILD_DIR)
	-make -C $(KERNEL_DIR) M=$(CURDIR)/$(DRIVER_DIR) clean
	rm -f $(DRIVER_DIR)/Makefile

.PHONY: all ipc driver rootfs qemu docker-build docker-run clean
