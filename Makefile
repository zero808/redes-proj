all: ECP TES user

ECP: ECP.o
	gcc -std=gnu99 -O0 -ggdb -fdiagnostics-color=always -o ECP ECP.o

ECP.o: ECP.c constants.h
	gcc -std=gnu99 -O0 -ggdb -fdiagnostics-color=always -c ECP.c

TES: TES.c
	gcc TES.c -o TES

user: user.o functions.o
	gcc -o user user.o functions.o

user.o: user.c constants.h
	gcc -c user.c

functions.o: functions.c functions.h
	gcc -c functions.c

clean:
	rm -f ECP TES user *.o
