#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "constants.h"

int main(int argc, char **argv) {

	int fd, addrlen, ret, nread;
	struct sockaddr_in serveraddr, clientaddr;
	char buffer[32];
	unsigned short int ECPport = PORT + GN;
	char usage[] = "usage: ./ECP [-p ECPport]";

	if(argc % 2 != 1 || argc > 3){
		printf("error: Incorrect number of arguments.\n%s\n", usage);
		return 1;
	}
	
	if(argc > 1) { //ou argv[1] != NULL
		if (!strcmp(argv[1], "-p")) {
			ECPport = atoi(argv[2]);
			//printf("%u\n", ECPport);
		}
		else {
			printf("error: Invalid option.\n%s\n", usage);
			return 1;
		}
	}

	if((fd=socket(AF_INET,SOCK_DGRAM,0))==-1)exit(1);//error

	memset((void*)&serveraddr,(int)'\0',sizeof(serveraddr));
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_addr.s_addr=htonl(INADDR_ANY); //Accept datagrams on any Internet interface on the system.
	serveraddr.sin_port=htons((u_short) ECPport);

	/* Use bind to register the server well-known address (and port) with the system. */
	ret=bind(fd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	if(ret==-1)exit(1);//error

	while(1){
		char tqr[] = "TQR\n";
		addrlen=sizeof(clientaddr);
		nread=recvfrom(fd,buffer,32,0,(struct sockaddr*)&clientaddr,&addrlen);
		if(nread==-1)exit(1);//error
		
		if(strncmp(buffer, tqr, 5) == 0) {
		
			/* this is just a test */
			ret=sendto(fd,"AWT...\n",7,0,(struct sockaddr*)&clientaddr,addrlen); 
			if(ret==-1)exit(1);
			
			/* 
			if the TQR cannot be answered, the reply will be the string "EOF" ... 
			
				ret=sendto(fd,"EOF\n",5,0,(struct sockaddr*)&clientaddr,addrlen); 
				if(ret==-1)exit(1);
			*/
		}
		else {
			ret=sendto(fd,"ERR\n",5,0,(struct sockaddr*)&clientaddr,addrlen); /* '\0' transmitted */
			if(ret==-1)exit(1);//error
		}
	}

	//close(fd);
	//exit(0);
}
