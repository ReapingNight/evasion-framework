ARCH=arm64
CROSS_COMPILE=aarch64-linux-gnu-

venus-wrapper-objs += wrapper.o

obj-m += venus-wrapper.o

all:
	$(MAKE) -C .. M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C .. M=$(PWD) ARCH=$(ARCH) clean
