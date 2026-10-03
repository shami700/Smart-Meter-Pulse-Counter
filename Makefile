# Makefile for the Smart Energy capstone project
#
# It builds:
#   1. The Linux kernel driver  -> smart_meter.ko
#   2. The C++ user program    -> smart_energy_agent
#
# The driver MUST be compiled against the kernel headers of the
# currently running kernel. That is what $(shell uname -r) gives us.
#
# Build everything :  make
# Build driver only :  make driver
# Build program only:  make app
# Clean everything  :  make clean

# ---- Paths ---------------------------------------------------------
KERNEL_DIR := /usr/src/WSL2-Linux-Kernel
PWD        := $(shell pwd)

# ---- Kernel module build flags ------------------------------------
# obj-m adds smart_meter.o to the module named smart_meter.ko
# obj-m is the official way to declare kernel modules.
obj-m    += smart_meter.o

# Our .c file is named smart_meter_driver.c, so we must tell the
# kernel build system that it produces smart_meter_driver.o,
# and that this object makes up the module smart_meter.
smart_meter-y := smart_meter_driver.o

# -Wall : show warnings
ccflags-y := -Wall -m64

all: driver app

# ---- 1. Build the kernel driver -----------------------------------
driver:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
	@echo ""
	@echo "Driver built successfully: smart_meter.ko"

# ---- 2. Build the C++ user-space program --------------------------
app:
	g++ -std=c++17 smart_energy_agent.cpp -o smart_energy_agent
	@echo "Application built successfully: smart_energy_agent"

# ---- 3. Remove all generated files -------------------------------
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f smart_energy_agent
	@echo "Project cleaned"

# ---- 4. Shortcuts for loading and removing the driver ------------
load:
	sudo insmod smart_meter.ko

unload:
	sudo rmmod smart_meter

.PHONY: all driver app clean load unload