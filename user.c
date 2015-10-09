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

extern int errno;
int SID;
char ECPname[STRING_SZ];
unsigned short int ECPport = PORT + GN; /* default port number */
char filename[QID_SZ + 6]; //+6 for file extension and '\0'

/*-------------------------------------------------------------------------------------
 | main
 +-------------------------------------------------------------------------------------*/

int main(int argc, char **argv) {

	int fd_tcp, fd_udp, i, n, nbytes, nleft, nwritten, addrlen;	
	struct hostent *hostptr;
	struct sockaddr_in serveraddr;
	char *ptr, buffer[BUFFER_SZ];
	char usage[] = "usage: ./user SID [-n ECPname] [-p ECPport]";
	void (*old_handler)(int);//interrupt handler
	
	if ((old_handler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) {
		perror("Error in signal");
		exit(1);
	}
	
	if (signal(SIGALRM, handler_alrm) == SIG_ERR) { 
		perror("Error in signal");
		exit(1); 
	}

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
	if ((hostptr = gethostbyname(ECPname)) == NULL) exit(1); //error

	fd_tcp = socket(AF_INET, SOCK_STREAM, 0); //TCP socket
	if (fd_tcp == -1) exit(1); //error

	fd_udp = socket(AF_INET, SOCK_DGRAM, 0); //UDP socket
	if(fd_udp == -1) exit(1); //error

	memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = ((struct in_addr*)(hostptr->h_addr_list[0]))->s_addr; //ECP server IP address
	serveraddr.sin_port = htons((u_short) ECPport);
	
	int action, len;
	size_t size;
	int tnn; //questionnaire topic number (1 <= tnn <= 99)
	char **T; //array of tokens
	char topic[4], ter_request[8], rqt_request[8], rqs_request[STRING_SZ], QID[QID_SZ + 1], time[TIME_SZ + 1], answers[STRING_SZ];
	const char *iptes_addr; //TES IP address for the transfer of a questionnaire file
	unsigned short int portTES; //TCP port number for the transfer of a questionnaire file
	struct in_addr IPTES;
	
	while(1) {
		
		action = action_selector();
			    
	    switch(action) {
	    	
	    	case -1: 
	    			printf("error: Invalid instruction.\n");
	    			exit(0);
	    					 
	    	case  0: 
	    			/* list instruction */
	    			
	    			/*---------------------------------------------------------------------------
  					 | TQR - User–ECP Protocol (in UDP)
					 +--------------------------------------------------------------------------*/
	    				    			
	    			n = sendto(fd_udp, "TQR\n", 4, 0, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	    			if (n == -1) exit(1); //error
 					
 					/*---------------------------------------------------------------------------
  					 | AWT nT T1 T2 ... TnT - User–ECP Protocol (in UDP)
					 +--------------------------------------------------------------------------*/
 					
					addrlen = sizeof(serveraddr);
					
					alarm(10);//generates the SIGALRM signal when the specified time has expired
					if ((n = recvfrom(fd_udp, buffer, BUFFER_SZ, 0, (struct sockaddr*)&serveraddr, &addrlen)) == -1) {
						perror("Error in recvfrom");
						exit(1);
					}
					else
						alarm(0); //cancel currently active alarm
					
					
					if (n > 0) {
	    				if (buffer[n - 1] == '\n')
	    					buffer[n - 1] = '\0'; //replace '\n' with '\0'
	    				else
	    					exit(1);
	    			}
	    			else
	    				exit(1);
					
					n = checkErrorMessages(buffer, "TQR");
					if(n == -1) exit(1);
				
					/* break the ECP's AWT reply into tokens (a valid AWT reply can be divided at most 
					into NB_TOPICS + 2 tokens, so add +1 to detect an invalid AWT reply) */
  					n = parseString(buffer, &T, NB_TOPICS + 3); 
  					
  					/* verify received protocol message */
  					n = verifyAWT(n, &T);
  					if(n == -1) exit(1);
  					
  					/* display questionnaire topics as a numbered list */	
  					displayTopics(&T);
	    	
	    			break;
	    			
	    	case  1: 
	    			/* request instruction */
	    			
	    			/*---------------------------------------------------------------------------
  					 | TER Tnn - User–ECP Protocol (in UDP)
					 +--------------------------------------------------------------------------*/
	    				    			    			
	    			getchar();
	    			ptr = fgets(topic, sizeof(topic), stdin);
	    			if (ptr == NULL) exit(1);	
	    			
	    			tnn = verifyTnn(ptr); // 1 <= tnn <= 99
	    			if (tnn == -1) {
	    				printf("Invalid topic or format\n");
	    				exit(1);
	    			}
	    			
	    			//write at most 7 bytes because tnn is composed of 1 or 2 digits ('\0' is not trasmitted) 
	    			snprintf(ter_request, 7, "TER %d\n", tnn);
	    			
	    			if (tnn > 9)
	    				n = sendto(fd_udp, ter_request, 7, 0, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	    			else
	    				n = sendto(fd_udp, ter_request, 6, 0, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	    			if (n == -1) exit(1); //error    			
	    			
	    			/*---------------------------------------------------------------------------
  					 | AWTES IPTES portTES - User–ECP Protocol (in UDP)
					 +--------------------------------------------------------------------------*/

	    			addrlen = sizeof(serveraddr);
					
					alarm(10);//generates the SIGALRM signal when the specified time has expired
					if ((n = recvfrom(fd_udp, buffer, STRING_SZ, 0, (struct sockaddr*)&serveraddr, &addrlen)) == -1) {
						perror("Error in recvfrom");
						exit(1);
					}
					else
						alarm(0); //cancel currently active alarm						
	    			
	    			if (n > 0) {
	    				if (buffer[n - 1] == '\n')
	    					buffer[n - 1] = '\0'; //replace '\n' with '\0'
	    				else
	    					exit(1);
	    			}
	    			else
	    				exit(1);
	    			
					n = checkErrorMessages(buffer, "TER");
					if (n == -1) exit(1);
	    			
	    			/* break the ECP's AWTES reply into tokens (a valid AWTES reply can be divided at most 
					into 3 tokens, so add +1 to detect an invalid AWTES reply)*/
  					n = parseString(buffer, &T, 4);
  					
  					/* verify received protocol message */
  					n = verifyAWTES(n, &T);
  					if (n == -1) exit(1);
  					
  					iptes_addr = T[1]; //TES IP address
  					portTES = atoi(T[2]); //TCP port number
	    			
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
	    			
					hostptr = gethostbyaddr(&IPTES, sizeof(IPTES), AF_INET);
					if (hostptr == NULL) exit(1);
	    			else printf("%s %hu\n", hostptr->h_name, portTES); //expected output from ECP's AWTES reply
	    			
	    			/*---------------------------------------------------------------------------
  					 | RQT SID - User–TES Protocol (in TCP)
					 +--------------------------------------------------------------------------*/
	    			
	    			serveraddr.sin_addr.s_addr = IPTES.s_addr; //TES IP address
	    			serveraddr.sin_port = htons((u_short) portTES); //TCP port number
	    			
	    			n = connect(fd_tcp, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
					if(n == -1) exit(1); //error
	    			
	    			// n does not include byte '\0' automatically appended at the end of the string 
	    			n = sprintf(rqt_request, "RQT %d\n", SID);
	    			if (n < 0) { printf("Error in sprintf"); exit(1); }
	    			ptr = rqt_request;
	    			nbytes = n; //10 ('\0' is not transmitted)
	    			
	    			/* write() may write a smaller number of bytes than solicited */
	    			nleft = nbytes;
	    			while (nleft > 0) {
	    				nwritten = write(fd_tcp, ptr, nleft);
	    				if (nwritten <= 0) exit(1); //error
	    				nleft -= nwritten;
	    				ptr += nwritten;
	    			}
	    			
	    			/*---------------------------------------------------------------------------
  					 | AQT QID time size data - User–TES Protocol (in TCP)
					 +--------------------------------------------------------------------------*/
	    			
	    			n = getAQTReply(fd_tcp, QID, time, &size);
	    			if (n == -1) {
	    				printf("Invalid AQT reply\n");
	    				exit(1);
	    			}
	    		
	    			printf("received file %s\n", filename);
	    			
	    			close(fd_tcp);

	    			break;
	    			
	    	case  2: 
	    			/* submit instruction */
	    			
	    			fd_tcp = socket(AF_INET, SOCK_STREAM, 0); //TCP socket
					if (fd_tcp == -1) exit(1); //error
					
					memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));
					serveraddr.sin_family = AF_INET;
	    			serveraddr.sin_addr.s_addr = IPTES.s_addr; //TES IP address
	    			serveraddr.sin_port = htons((u_short) portTES); //TCP port number
	    			
	    			n = connect(fd_tcp, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
					if(n == -1) exit(1); //error
	    			
	    			/*---------------------------------------------------------------------------
  					 | RQS SID QID V1 V2 V3 V4 V5 - User–TES Protocol (in TCP)
					 +--------------------------------------------------------------------------*/
	    			
	    			getchar();
	    			ptr = fgets(answers, sizeof(answers), stdin);
	    			if (ptr == NULL) exit(1);
	    			
	    			len = strlen(ptr) - 1;
	    			if (ptr[len] == '\n') ptr[len] = '\0';
            		else ptr[len + 1] = '\0';
  					
  					/* verify questionnaire answers */
  					n = verifyQuestAnswers(ptr, &T);
  					if(n == -1)	break;
  					
  					n = sprintf(rqs_request, "RQS %d %s %s\n", SID, QID, answers);
  					if (n < 0) { printf("Error in sprintf"); exit(1); }
  					
	    			ptr = rqs_request;
	    			nbytes = n; //21+strlen(QID) ('\0' is not transmitted)
	    			nleft = nbytes;
	    			
	    			while (nleft > 0) {
	    				nwritten = write(fd_tcp, ptr, nleft);
	    				if (nwritten <= 0) exit(1); //error
	    				nleft -= nwritten;
	    				ptr += nwritten;
	    			}
  					
  					/*---------------------------------------------------------------------------
  					 | AQS QID score - User–TES Protocol (in TCP)
					 +--------------------------------------------------------------------------*/
  					
  					ptr = getAQSReply(fd_tcp);  					
  					
  					if (strlen(ptr) > 0) {
  						len = strlen(ptr) - 1;
  						if (ptr[len] == '\n')
  							ptr[len] = '\0'; //replace '\n' with '\0'
  						else
  							exit(1);
  					}
  					else
  						exit(1);
  					
  					
  					printf("AQS reply =«%s»\n", ptr);
  					
  					n = checkErrorMessages(ptr, "AQS");
					if (n == -1) exit(1);
  					
  					//verify received protocol message
  					n = verifyAQS(ptr, &T, QID);
					if (n == -1) {
						printf("Invalid AQS reply from TES\n");
						exit(1);
					}
					
					printf("Obtained score: %d%%\n", atoi(T[2]));
	
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
