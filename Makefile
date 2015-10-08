all: ECP TES user

ECP: ECP.o
	gcc -std=gnu99 -Wall -O0 -ggdb -o ECP ECP.o

ECP.o: ECP.c constants.h
	gcc -std=gnu99 -Wall -O0 -ggdb -c ECP.c

TES: TES.c
	gcc -Wall TES.c -o TES

user: user.o functions.o
	gcc -Wall -o user user.o functions.o

user.o: user.c constants.h
	gcc -Wall -c user.c

functions.o: functions.c functions.h
	gcc -Wall -c functions.c

clean:
	rm -f ECP TES user *.o
