obj-m += nhatthanh_oled.o

# Đường dẫn nhân Linux 5.10 trong Buildroot
LINUX_DIR ?= /mnt/eos_build/EOS_BuiltRoot/buildroot/output/build/linux-5.10.162
PWD := $(shell pwd)

# Kiến trúc ARM và Toolchain chính xác 100% vừa tìm được
ARCH ?= arm
CROSS_COMPILE ?= /mnt/eos_build/EOS_BuiltRoot/buildroot/output/host/bin/arm-linux-

all:
	$(MAKE) -C $(LINUX_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(LINUX_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
