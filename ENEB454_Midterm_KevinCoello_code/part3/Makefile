obj-m += video.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

part4: part4.c defines.h
	gcc -o part4 part4.c
