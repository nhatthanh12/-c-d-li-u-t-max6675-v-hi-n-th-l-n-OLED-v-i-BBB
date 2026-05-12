obj-m += nhatthanh_max6675.o

# Đường dẫn đến bộ nguồn Kernel 5.10.162 trên máy của Thành
LINUX_DIR ?= /mnt/eos_build/EOS_BuiltRoot/buildroot/output/build/linux-5.10.162
PWD := $(shell pwd)

# Khai báo kiến trúc ARM và Toolchain "xịn" của Buildroot
ARCH ?= arm
CROSS_COMPILE ?= /mnt/eos_build/EOS_BuiltRoot/buildroot/output/host/bin/arm-linux-

all:
	$(MAKE) -C $(LINUX_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(LINUX_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
