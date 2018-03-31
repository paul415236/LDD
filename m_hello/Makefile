# module
MODULE = hello

# kernel
KERNELVERSION = $(shell uname -r)
KERNELDIR := /usr/src/linux-headers-$(KERNELVERSION)

# other
PWD = $(shell pwd)

ifneq ($(KERNELRELEASE),)
	obj-m := $(MODULE).o
	#module-objs := 
else

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
