# module
MODULE = hello

# kernel
KERNELVERSION = $(shell uname -r)
KERNELDIR := /usr/src/linux-headers-$(KERNELVERSION)

# other
PWD = $(shell pwd)

# flags
DEBUG = y

ifeq ($(DEBUG), y)
	DEBFLAGS = -O -g -DHELLO_DEBUG
else
	DEBFLAGS = -O2
endif

EXTRA_CFLAGS += $(DEBFLAGS)
#ccflags-y += $(CFLAGS)

ifneq ($(KERNELRELEASE),)
	obj-m := $(MODULE).o
	#module-objs := 
else

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
