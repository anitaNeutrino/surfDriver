# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
V=1
ifneq ($(KERNELRELEASE),)
	obj-m := surfDriver.o
        surfDriver-objs := sD_main.o sD_plx.o sD_char.o sD_transfer.o sD_sysfs.o

# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else

	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

endif
