obj-m += video.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

part6: part6.c wrappers.c pushbutton.c defines.h wrappers.h
	gcc -Wall -o part6 part6.c wrappers.c pushbutton.c
