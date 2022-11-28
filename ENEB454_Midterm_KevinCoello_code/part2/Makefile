obj-m += video.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

part2: part2.c defines.h
	gcc -o part2 part2.c
