obj-m += sane-dev.o

all:
	$(MAKE) -C $(KERNEL_SRC_PATH) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_SRC_PATH) M=$(PWD) clean


