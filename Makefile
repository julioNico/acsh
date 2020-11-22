all:
	gcc -g -Wall *.c -o acsh

clean:
	rm -f *.o acsh
