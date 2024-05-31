dev_driver-objs := device.o timer.o
obj-m	:= dev_driver.o

default:
	$(MAKE) -C /work/achroimx_kernel SUBDIRS=$(shell pwd) modules ARCH=arm

push: default
	@echo ">> Pushing the module to /data/local/tmp on the device..."
	make clean
	make
	adb push -p $(subst .o,.ko,$(obj-m)) /data/local/tmp
	@echo ">> The module has been pushed to /data/local/tmp on the device."

clean:
	rm -rf *.o
	rm -rf *.ko
	rm -rf *.mod.c
	rm -rf *.order
	rm -rf *.symvers
	rm -rf .*.cmd
	rm -rf .tmp_versions
