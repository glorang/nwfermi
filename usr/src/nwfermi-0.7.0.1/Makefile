# build modules
obj-m := nw-fermi.o

# use the kernel build system
KERNEL_VERSION := $(shell uname -r)
KERNEL_SOURCE ?= /lib/modules/$(KERNEL_VERSION)/build

# build
PWD := $(shell pwd)
all module:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) clean

