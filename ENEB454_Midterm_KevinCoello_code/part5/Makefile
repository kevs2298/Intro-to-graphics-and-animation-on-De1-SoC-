obj-m += video.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

part5: part5.c pushbutton.c defines.h
	gcc -o part5 part5.c pushbutton.c
