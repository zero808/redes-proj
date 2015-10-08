CC=gcc
# CFLAGS=-std=gnu99 -O3
CFLAGS=-std=gnu99 -Wall -O0 -ggdb
all: ECP TES user

ECP: ECP.o
	$(CC) $(CLFAGS) -o ECP ECP.o

ECP.o: ECP.c constants.h
	$(CC) $(CFLAGS) -c ECP.c

TES: TES.o
	$(CC) $(CFLAGS) -o TES TES.o

TES.o: TES.c
	$(CC) $(CFLAGS) -c TES.c

user: user.o functions.o
	$(CC) $(CFLAGS) -o user user.o functions.o

user.o: user.c constants.h
	$(CC) $(CFLAGS) -c user.c

functions.o: functions.c functions.h
	$(CC) $(CFLAGS) -c functions.c

clean:
	rm -f ECP TES user *.o
