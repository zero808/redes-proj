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
	//serveraddr.sin_addr.s_addr=((struct in_addr*)(hostptr->h_addr_list[0]))->s_addr; //ECP server IP address
	serveraddr.sin_addr.s_addr=((struct in_addr*)(hostptr->h_addr_list[0]))->s_addr; //ECP server IP address
	serveraddr.sin_port=htons((u_short) ECPport);
	
	int action;
	char topic[4];
	int tnn; //desired questionnaire topic number (between 1 and 99)
	char answers[NB_ANSWERS*2];//sequence of answers values (V1 to V5) in the format "V1 V2 V3 V4 V5"
	char **T; //ECP's TQR reply into tokens
	int nt; //number of topics
	char ter_request[8];
	//const char iptes_addr[INET_ADDRSTRLEN];// 16 bytes (15 characters for IPv4 (xxx.xxx.xxx.xxx format, 12+3 separators) + '\0')
	struct in_addr IPTES;
	const char *iptes_addr;
	unsigned short int portTES;
	char rqt_request[8];
	char *QID; //unique transaction identifier string
	char rqs_request[128];
	
	
	while(1) {
		
		action = action_selector();
			    
	    switch(action) {
	    	
	    	case -1: 
	    			printf("error: Invalid instruction.\n");
	    			exit(0);
	    			 
	    	case  0: 
	    			/* list instruction */
	    			
	    			/* TQR - User–ECP Protocol (in UDP) */	    			
	    			n=sendto(fd_udp,"TQR\n",4,0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
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
					

					/* breaks the ECP's AWT reply into tokens */
  					n = parseString(buffer, &T); 
  					
  					/* verify received protocol message */
  					n=verifyAWT(n, &T);
  					if(n==-1)exit(1);
  					
  					/* questionnaire topics displayed as a numbered list */	
  					displayTopics(&T);
	    	
	    			break;
	    	
	    	case  1: 
	    			/* request instruction */
	    				    			    			
	    			getchar();
	    			ptr = fgets(topic, sizeof(topic), stdin);
	    			if (ptr == NULL) exit(1);	
	    			
	    			tnn = verifyTnn(ptr);
	    			if (tnn == -1) {
	    				printf("Invalid topic or format\n");
	    				exit(1);
	    			}
	    			
	    			//write at most 8 bytes, including the terminating null byte ('\0'), because tnn is composed of 1 or 2 digits
	    			snprintf(ter_request, 8, "TER %d\n", tnn);
	    			printf("%s", ter_request);
	    			
	    			/* TER Tnn - User–ECP Protocol (in UDP) */
				if(tnn > 9)
					n=sendto(fd_udp,ter_request,7,0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
				else
					n=sendto(fd_udp,ter_request,6,0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	    			if(n==-1)exit(1); //error
	    			
	    			/* AWTES IPTES portTES - User–ECP Protocol (in UDP) */
	    			addrlen=sizeof(serveraddr);
					n=recvfrom(fd_udp,buffer,128,0,(struct sockaddr*)&serveraddr,&addrlen);
					if(n==-1)exit(1);
	    			 
	    			/* breaks the ECP's AWTES reply into tokens */
  					n = parseString(buffer, &T);
  					
  					/* verify received protocol message */
  					n=verifyAWTES(n, &T);
  					if(n==-1)exit(1);
  					
  					iptes_addr = T[1];
  					portTES = atoi(T[2]);
	    			
	    			//printf("%s %hu\n", iptes_addr, portTES);
	    			
	    			/* converts IPv4 network address in dotted-decimal format into a struct in_addr */
	    			n = inet_pton(AF_INET, iptes_addr, &IPTES);
	    			if (n <= 0) {
	    				if (n == 0)
	    					/* iptes_addr does not point to a character string containing an IPv4 network address 
	    					in dotted-decimal format, "ddd.ddd.ddd.ddd", where ddd is a decimal number of up to 
	    					three digits in the range 0 to 255. */
	    					fprintf(stderr, "Not in presentation format\n");
	    				else 
	    					//AF_INET does not contain a valid address family
	    					perror("Error in inet_pton");
	    					
	    				exit(1);
	    			}
	    			
					hostptr=gethostbyaddr(&IPTES,sizeof(IPTES),AF_INET);
					if(hostptr==NULL)exit(1);
	    			else printf("%s %hu\n", hostptr->h_name, portTES); //expected output from ECP's AWTES reply
	    			
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
	    			//TODO
	    			
	    			/* ... */
	    			
	    			/* verify protocol message received */
  					//verifyAQT();
	    			
	    			
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
	    			
	    			getchar();
	    			ptr = fgets(answers, sizeof(answers), stdin); //answer values (V1 to V5) in the format "V1 V2 V3 V4 V5"
	    			if (ptr == NULL) exit(1);
	    			//printf("%s", answers);
  					
  					n = sscanf(answers, "%*c %*c %*c %*c %*c"); //FIXME I only checked the format...
  					if (n == -1) {
	    				printf("Invalid format\n");
	    				exit(1);
	    			}
	    			
  					/* RQS SID QID V1 V2 V3 V4 V5 - User–TES Protocol (in TCP) */
  					
  					/* just for testing... */
  					QID = "12345678"; //FIXME
  					
  					n=sprintf(rqs_request, "RQS %d %s %s\n", SID, QID, answers);
  					printf("%s", rqs_request);
  					
	    			ptr=rqs_request;
	    			nbytes=n+1;
	    			
	    			nleft=nbytes;
	    			while(nleft>0) {
	    				nwritten=write(fd_tcp,ptr,nleft);
	    				if(nwritten<=0) exit(1); //error
	    				nleft-=nwritten;
	    				ptr+=nwritten;
	    			}
  					
  					
  					/* AQS QID score - User–TES Protocol (in TCP) */
  					
  					/* ... */
  					
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
