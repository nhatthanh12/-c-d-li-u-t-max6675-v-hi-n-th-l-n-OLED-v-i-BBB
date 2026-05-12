obj-m += nhatthanh_gpio.o

# Khai báo đường dẫn trỏ đến Kernel 5.10 vừa build hôm qua
LINUX_DIR := /mnt/eos_build/EOS_BuiltRoot/buildroot/output/build/linux-5.10.162
PWD := $(shell pwd)

# Ép kiến trúc ARM và trỏ đến Toolchain xịn của Buildroot
ARCH ?= arm
CROSS_COMPILE ?= /mnt/eos_build/EOS_BuiltRoot/buildroot/output/host/bin/arm-linux-

all:
	$(MAKE) -C $(LINUX_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(LINUX_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
