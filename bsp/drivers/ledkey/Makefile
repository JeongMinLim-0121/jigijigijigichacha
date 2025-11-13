# ledkey/Makefile
MOD := ledkey_drv            
obj-m += ledkey_drv.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod $(MOD).ko || true
	dmesg | tail -n 30

unload:
	- sudo rmmod $(MOD) || true
	dmesg | tail -n 30

reload: unload all load
