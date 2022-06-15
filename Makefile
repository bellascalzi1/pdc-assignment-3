# NOTE: Adapted from the Makefiles given to us in lectures.
CC=gcc
FLAGS= -Wall -Wextra -Wpedantic -std=c99

parallel: parallel.c
	$(CC) $(FLAGS) parallel.c -o parallel -l OpenCL

serial: serial.c
	$(CC) $(FLAGS) serial.c -o serial

clean:
	rm -f parallel serial
