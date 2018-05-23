# module
MODULE = scull

# kernel
KERNELVERSION = $(shell uname -r)
#KERNELDIR := /usr/src/linux-headers-$(KERNELVERSION)
KERNELDIR := /lib/modules/$(KERNELVERSION)/build

# other
PWD = $(shell pwd)

# debug
DEBUG_MSG_ENABLE = y

ifeq ($(DEBUG_MSG_ENABLE), y)
	CCFLAGS = -O -g -DSCULL_DEBUG	
else
	CCFLAGS = -O2
endif

EXTRA_CFLAGS += $(CCFLAGS)


ifneq ($(KERNELRELEASE),)
	obj-m := $(MODULE).o
	#scull-objs := proc.o
else

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif
