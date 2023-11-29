
obj-m += the_bqs.o
the_bqs-objs += bqs.o lib/scth.o
#the_virtual-to-physical-memory-mapper-objs += virtual-to-physical-memory-mapper.o 


A = $(shell cat /sys/module/the_usctm/parameters/sys_call_table_address)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mount:
	insmod the_bqs.ko the_syscall_table=$(A)
remove:
	rmmod the_bqs.ko
