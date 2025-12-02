KERNEL_DIR := ..//linux-6.15.6

obj-m += tsune.o

all:
		make -C $(KERNEL_DIR) M=$(PWD) modules

.PHONY: all

