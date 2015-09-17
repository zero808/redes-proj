#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> /* pid_t, socket(), bind(), listen(), accept() */
#include <sys/socket.h> /* socket(), bind(), listen(), accept() */
#include <arpa/inet.h> /* htonl(), htons() */
#include <string.h> /* memset() */
#include <unistd.h> /* fork(), read(), write() */
#include <signal.h>
#include <errno.h>

#define PORT 59000

extern int errno;

int main(int argc, char **argv) {

	int fd, newfd, addrlen, n, nw, ret;
	struct sockaddr_in addr;
	char *ptr, buffer[128];
	unsigned short int TESport = PORT;
	pid_t pid;
	char usage[] = "usage: ./TES [-p TESport]";
	void (*old_handler)(int);//interrupt handler

	/* Avoid zombies when child processes die. */
	if((old_handler=signal(SIGCHLD,SIG_IGN))==SIG_ERR)exit(1);//error

	if(argc % 2 != 1 || argc > 3){
		printf("error: Incorrect number of arguments.\n%s\n", usage);
		return 1;
	}
	
	if(argc > 1) { //ou argv[1] != NULL
		if (!strcmp(argv[1], "-p")) {
			TESport = atoi(argv[2]);
			//printf("%u\n", ECPport);
		}
		else {
			printf("error: Invalid option.\n%s\n", usage);
			return 1;
		}
	}

	if((fd=socket(AF_INET,SOCK_STREAM,0))==-1)exit(1);//error
	
	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	addr.sin_port=htons((u_short) TESport);

	if(bind(fd,(struct sockaddr*)&addr,sizeof(addr))==-1)exit(1);//error

	if(listen(fd,5)==-1)exit(1);//error

	while(1){

		addrlen=sizeof(addr);

		do newfd=accept(fd,(struct sockaddr*)&addr,&addrlen);//wait for a connection
		while(newfd==-1&&errno==EINTR);

		if(newfd==-1)exit(1);//error
		
		/* Create a child process for each new connection. */
		if((pid=fork())==-1)exit(1);//error
		else if(pid==0) { //child process
				close(fd);
				while((n=read(newfd,buffer,128))!=0){
					if(n==-1)exit(1);//error
					ptr=&buffer[0];
					while(n>0){
						if((nw=write(newfd,ptr,n))<=0)exit(1);//error
						n-=nw; ptr+=nw;
					}
				}
				close(newfd); exit(0);
			}
		//parent process
		do ret=close(newfd);
		while(ret==-1&&errno==EINTR);
		if(ret==-1)exit(1);//error
	}
	/* close(fd); exit(0); */
}
