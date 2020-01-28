.PHONY: all clean

all:
	gcc -Wall server3.c -o server3
	gcc -Wall client3.c -o client3
	gcc -lrt -Wall server4.c -o server4
	gcc -Wall client4.c -o client4
	


clean:
	rm -f client3 server3 client4 server4
