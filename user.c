#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> /* socket() */
#include <sys/socket.h> /* socket() */
#include <netdb.h> /* gethostbyname() */
#include <arpa/inet.h> /* htons() */
#include <netinet/in.h>
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
	serveraddr.sin_addr.s_addr=((struct in_addr*)(hostptr->h_addr_list[0]))->s_addr; //ECP server IP address
	serveraddr.sin_port=htons((u_short) ECPport);
	
	int action;
	char v[NB_ANSWERS];//answer values (V1 to V5)
	char **T; //ECP's TQR reply into tokens
	int nt; //number of topics
	int tnn; //desired questionnaire topic number
	char ter_request[8];
	char iptes_addr[INET_ADDRSTRLEN];// 16 bytes (15 characters for IPv4 (xxx.xxx.xxx.xxx format, 12+3 separators) + '\0')
	struct in_addr IPTES;
	unsigned short int portTES;
	char rqt_request[8];
	//char *QID; //unique transaction identifier string
	//char *qfile;//questionnaire file
	//char *rqs_request;
	
	while(1) {
		
		action = action_selector();
			    
	    switch(action) {
	    	
	    	case -1: 
	    			printf("error: Invalid option.\n");
	    			break;
	    			 
	    	case  0: 
	    			/* list instruction */
	    			
	    			/* TQR - User–ECP Protocol (in UDP) */	    			
	    			n=sendto(fd_udp,"TQR\n",5,0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	    			if(n==-1)exit(1); //error
 
 					/* AWT nT T1 T2 ... TnT - User–ECP Protocol (in UDP) */
					addrlen=sizeof(serveraddr);
					n=recvfrom(fd_udp,buffer,4096,0,(struct sockaddr*)&serveraddr,&addrlen); //FIXME 4096
					if(n==-1)exit(1);
				
				
					//TEST
					printf("TEST ------------------------------------------------------------------\n");
					write(1,"echo from ECP: ",15);
					write(1,buffer,n); 
					
					/* check who sent the message */
					hostptr=gethostbyaddr((char*)&serveraddr.sin_addr,sizeof(struct in_addr),AF_INET);
					if(hostptr==NULL)
						printf("sent by [%s:%hu]\n",inet_ntoa(serveraddr.sin_addr),ntohs(serveraddr.sin_port));
					else 
						printf("sent by [%s:%hu]\n",hostptr->h_name,ntohs(serveraddr.sin_port));
					printf("----------------------------------------------------------- end of TEST\n");
					//end of TEST
					

					/* breaks the ECP's TQR reply into tokens */
  					n = parseString(buffer, &T); 
  					
  					/* verify received protocol message */
  					n=verifyAWT(n, &T);
  					if(n==-1)exit(1);
  					
  					/* questionnaire topics displayed as a numbered list */	
  					displayTopics(&T);
	    	
	    			break;
	    	
	    	case  1: 
	    			/* request instruction */
	    			
	    			scanf("%d", &tnn);
	    			
	    			//write at most 8 bytes, including the terminating null byte ('\0'), because tnn is composed of 1 or 2 digits
	    			snprintf(ter_request, 8, "TER %d\n", tnn);
	    			
	    			/* TER Tnn - User–ECP Protocol (in UDP) */
	    			n=sendto(fd_udp,ter_request,8,0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	    			if(n==-1)exit(1); //error
	    			
	    			/* AWTES IPTES portTES - User–ECP Protocol (in UDP) */
	    			addrlen=sizeof(serveraddr);
					n=recvfrom(fd_udp,buffer,128,0,(struct sockaddr*)&serveraddr,&addrlen);
					if(n==-1)exit(1);
	    			 
	    			/* breaks the ECP's AWTS reply into tokens */
  					n = parseString(buffer, &T);
  					
  					/* verify protocol message received */
  					//verifyAWTES(n, &T);//TODO
  					
  					strcpy(iptes_addr, T[1]);
  					portTES = atoi(T[2]);
	    			
	    			//printf("%s %hu\n", iptes_addr, portTES);
	    			
	    			/* 
	    			//inet_aton instead of inet_pton is also a valid solution
	    			if (inet_aton(iptes_addr, &IPTES) == 0) {
	    				fprintf(stderr, "Invalid address\n");
	    				exit(1);
	    			}
	    			*/
	    			
	    			n = inet_pton(AF_INET, iptes_addr, &IPTES);
	    			if (n <= 0) {
	    				if (n == 0)
	    					//iptes_addr does not contain a character string representing a valid network address in AF_INET
	    					fprintf(stderr, "Not in presentation format");
	    				else
	    				//AF_INET does not contain a valid address family
	    					perror("Error in inet_pton");
	    				exit(1);
	    			}
	    			
					hostptr=gethostbyaddr(&IPTES,sizeof(IPTES),AF_INET);
	    			printf("%s %hu\n", hostptr->h_name, portTES); //expected output from ECP's AWTES reply
	    			
	    			/* RQT SID - User–TES Protocol (in TCP) */
	    			serveraddr.sin_addr.s_addr=IPTES.s_addr; //TES server IP address
	    			serveraddr.sin_port=htons((u_short) portTES);
	    			
	    			n=connect(fd_tcp,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
					if(n==-1)exit(1); //error
	    			
	    			//write 11 bytes, including the terminating null byte ('\0'), because SID is composed of 5 digits
	    			n=sprintf(rqt_request, "RQT %d\n", SID);
	    			
	    			ptr=rqt_request;
	    			nbytes=n+1;//10+1
	    			
	    			//printf("%s", ptr);
	    			
	    			/* write() may write a smaller number of bytes than solicited */
	    			nleft=nbytes;
	    			while(nleft>0) {
	    				nwritten=write(fd_tcp,ptr,nleft);
	    				if(nwritten<=0) exit(1); //error
	    				nleft-=nwritten;
	    				ptr+=nwritten;
	    			}
	    			
	    			/* AQT QID time size data - User–TES Protocol (in TCP) */
	    			
	    			/* ... */
	    			
	    			/* verify protocol message received */
  					//verifyAQT();//TODO
	    			
	    			
	    			/*
	    			nleft=nbytes; ptr=&buffer[0];

					while(nleft>0){
						nread=read(fd_tcp,ptr,nleft);
						if(nread==-1)exit(1); //error
						else if(nread==0)break; //closed by peer
						nleft-=nread;
						ptr+=nread;
					}

					nread=nbytes-nleft;
	
					write(1,"echo from TES: ",15); //stdout
					write(1,buffer,nread);
	    			*/
	    			
	    			break;
	    			
	    	case  2: 
	    			/* submit instruction */
	    			
	    			/* answer values (V1 to V5) */
	    			for(i = 0; i<NB_ANSWERS; i++)
  						scanf(" %c", &v[i]);
  					
  					/* RQS SID QID V1 V2 V3 V4 V5 - User–TES Protocol (in TCP) */
  					
  					/*
	    			n=sprintf(rqs_request, "RQS %d %s %c %c %c %c %c\n", SID, QID, v[0], v[1], v[2], v[3], v[4]);
	    			
	    			ptr=rqs_request;
	    			nbytes=n+1;
	    			
	    			printf("%s", ptr);
	    			
	    			nleft=nbytes;
	    			while(nleft>0) {
	    				nwritten=write(fd_tcp,ptr,nleft);
	    				if(nwritten<=0) exit(1); //error
	    				nleft-=nwritten;
	    				ptr+=nwritten;
	    			}
	    			*/
  					
  					
  					break;
  						
	    	case  3: 	
	    			/* exit instruction */
	    			
	    			close(fd_tcp);
	    			close(fd_udp);
	    			
	    			/* the program does not return a value from main(): 
	    			control flow never falls off the end of main() because exit() never returns */
	    			exit(0);
	    }
	    
	}//end while	

}
