obj-m += video.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

part1: part1.c defines.h
	gcc -o part1 part1.c
