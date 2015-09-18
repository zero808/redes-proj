#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> /* socket() */
#include <sys/socket.h> /* socket() */
#include <netdb.h> /* gethostbyname() */
#include <arpa/inet.h> /* htons() */
#include <string.h> /* memset() */
#include <unistd.h> /* write(), read() */
#include <signal.h> /* signal() */
#include <errno.h>

#include "constants.h"
#include "functions.h"

#define NB_ANSWERS 5

extern int errno;
int SID;
char ECPname[STRING_SZ];
unsigned short int ECPport = PORT + GN; /* default port number */


/*-------------------------------------------------------------------------------------
| main
+-------------------------------------------------------------------------------------*/

int main(int argc, char **argv) {

	int fd_tcp, fd_udp, i, n, nbytes, nleft, nwritten, nread, addrlen;
	struct hostent *hostptr;
	struct sockaddr_in serveraddr;
	char *ptr, buffer[4096];
	void (*old_handler)(int);//interrupt handler
	char usage[] = "usage: ./user SID [-n ECPname] [-p ECPport]";
	
	if((old_handler=signal(SIGPIPE,SIG_IGN))==SIG_ERR) exit(1);//error

	if(gethostname(ECPname, STRING_SZ) == -1)
		printf("error: %s\n", strerror(errno));

	if(argc % 2 != 0 || argc < 2 || argc > 6){
		printf("error: Incorrect number of arguments.\n%s\n", usage);
		return 1;
	}
	
	SID = atoi(argv[1]);

	for(i = 2; i <= argc - 1; i += 2){
		if(argv[i][0] != '-'){
			printf("error: Incorrect arguments.\n%s\n", usage);
			return 1;
		}
		if(cmd_manager(i, argv) == -1){
			printf("error: Invalid option.\n%s\n", usage);
			return 1;
		}
	}

	// Use gethostbyname to get the server IP address
	if((hostptr=gethostbyname(ECPname))==NULL) exit(1); //error

	fd_tcp=socket(AF_INET,SOCK_STREAM,0); //TCP socket
	if(fd_tcp==-1) exit(1); //error

	fd_udp=socket(AF_INET,SOCK_DGRAM,0); //UDP socket
	if(fd_udp==-1) exit(1); //error

	memset((void*)&serveraddr,(int)'\0',sizeof(serveraddr));
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_addr.s_addr=((struct in_addr*)(hostptr->h_addr_list[0]))->s_addr; //server IP address
	serveraddr.sin_port=htons((u_short) ECPport);

	
	while(1) {
		int action, i;
		char answers[NB_ANSWERS];
		action = action_selector();
		char **T; //ECP's TQR reply into tokens
		int nt; //number of topics
	    
	    switch(action) {
	    	
	    	case -1: 
	    			printf("error: Invalid option.\n");
	    			break;
	    			 
	    	case  0: 
	    			/* TQR - User–ECP Protocol (in UDP) */	    			
	    			n=sendto(fd_udp,"TQR\n",5,0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	    			if(n==-1)exit(1); //error
 
					addrlen=sizeof(serveraddr);
					n=recvfrom(fd_udp,buffer,4096,0,(struct sockaddr*)&serveraddr,&addrlen);
					if(n==-1)exit(1);
				
					/* this is just a test */
					write(1,"echo: ",6);
					write(1,buffer,n); 
					
					/* breaks the ECP's TQR reply into tokens */
  					n = parseString(buffer, &T); 
  					
  					nt = atoi(T[1]);
  					
  					/* questionnaire topics displayed as a numbered list */
  					for (i = 0; i < nt; ++i) {
    					printf("%d- %s",i+1, T[i+2]);
    					if(i < nt-1) //o ultimo token tem '\n'
    						printf("\n");
    				}

					hostptr=gethostbyaddr((char*)&serveraddr.sin_addr,sizeof(struct in_addr),AF_INET);
					if(hostptr==NULL)
						printf("sent by [%s:%hu]\n",inet_ntoa(serveraddr.sin_addr),ntohs(serveraddr.sin_port));
					else 
						printf("sent by [%s:%hu]\n",hostptr->h_name,ntohs(serveraddr.sin_port));
	    	
	    			break;
	    	
	    	case  1: 
	    			/* TER Tnn - User–ECP Protocol (in UDP) */
	    			/* ... */
	    			
	    			/* RQT SID - User–TES Protocol (in TCP) */
	    			/* ... */
	    			
	    			break;
	    			
	    	case  2: 
	    			/* answer values (V1 to V5) */
	    			for(i = 0; i<NB_ANSWERS; i++)
  						scanf(" %c", &answers[i]);
  					
  					/* RQS SID QID V1 V2 V3 V4 V5 - User–TES Protocol (in TCP) */
  					/* ... */
  					
  					break;
  						
	    	case  3: 	
	    			/* close(fd_tcp); */
	    			/* close(fd_udp); */
	    			exit(0);
	    	
	    }
	    
	}
		
		
	/* ---------------- */	
	

	n=connect(fd_tcp,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	if(n==-1)exit(1); //error
	
	ptr=strcpy(buffer,"Hello!\n");
	nbytes=7;

	nleft=nbytes;
	
	while(nleft>0){
		nwritten=write(fd_tcp,ptr,nleft);
		if(nwritten<=0)exit(1); //error
		nleft-=nwritten;
		ptr+=nwritten;
	}
	
	nleft=nbytes; ptr=&buffer[0];

	while(nleft>0){
		nread=read(fd_tcp,ptr,nleft);
		if(nread==-1)exit(1); //error
		else if(nread==0)break; //closed by peer
		nleft-=nread;
		ptr+=nread;
	}

	nread=nbytes-nleft;
	
	close(fd_tcp);
	close(fd_udp);
	
	write(1,"echo: ",6); //stdout
	write(1,buffer,nread);
	exit(0);
}
