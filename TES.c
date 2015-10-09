#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> /* pid_t, socket(), bind(), listen(), accept() */
#include <sys/socket.h> /* socket(), bind(), listen(), accept() */
#include <arpa/inet.h> /* htonl(), htons() */
#include <string.h> /* memset() */
#include <unistd.h> /* fork(), read(), write() */
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <netdb.h>

#define TES_PORT 59000
#define ECP_PORT 58055
#define MAX_BUF 128
#define NUM_OF_FILES 2
#define TES_NUMBER 1
#define ERR_SIZE 4
#define DEADLINE_MIN 1

//------------------------------------------------------
extern int errno;
const char * TOPIC_NAME = "Networks";

//-------------------------------------------------------


void handler_alrm(){
	printf("Sockect timeout\n");
	exit(1);
}

int checkdeadline(char*qid,struct tm *t ){
				//N_N_DD_MM_YYYYY_HH_MM_SS			
	char* arr[10];
	char * ch;
	ch = strtok(qid, "_");
	int i = 0;
	while (ch != NULL) {
		arr[i] = ch;
		i++;
		ch = strtok(NULL, "_");
	}

	printf("dealine year->%d actual->%d\n",atoi(arr[4]),t->tm_year +1900  );
	printf("dealine mon->%d actual->%d\n",atoi(arr[3]), t->tm_mon +1 );
	printf("dealine day->%d actual->%d\n",atoi(arr[2]), t->tm_mday );
	printf("dealine hour->%d actual->%d\n",atoi(arr[5]),t->tm_hour );
	printf("dealine min->%d actual->%d\n",atoi(arr[6]),t->tm_min );
	printf("dealine sec->%d actual->%d\n",atoi(arr[7]),t->tm_sec );		
		
	if(t->tm_year +1900 > atoi(arr[4]))return -1;
	if(t->tm_year +1900 == atoi(arr[4])) if(t->tm_mon +1 > atoi(arr[3]))return -1;
	if(t->tm_mon +1 == atoi(arr[3])) if(t->tm_mday > atoi(arr[2]))return -1;
	if(t->tm_mday == atoi(arr[2])) if(t->tm_hour > atoi(arr[5]))return -1;
	if(t->tm_hour == atoi(arr[5])) if(t->tm_min > atoi(arr[6]))return -1;
	if(t->tm_min == atoi(arr[6])) if(t->tm_sec > atoi(arr[7]))return -1;
		
	return 0;			
}


char* giveanswersfilename(char*qid){
	//N_N_DD_MM_YYYYY_HH_MM_SS			
	char *filename=malloc(sizeof(char)* (13+1));
	char* arr[10];
	char * ch;
	ch = strtok(strdup(qid), "_");
	int i = 0;
	while (ch != NULL) {
		arr[i] = ch;
		i++;
		ch = strtok(NULL, "_");
	}
	sprintf(filename,"T%sQF%sA.txt",arr[0],arr[1]);
	
	return filename;
}


struct tm* deadline(struct tm *t){
	//deadline: current time + DEADLINE_MIN
	if ((t->tm_min += DEADLINE_MIN) > 59) {
		t->tm_min -= 60;
		t->tm_hour++;
	}
	return t;
}


char* timeformat(struct tm *t) {
	char *tf = malloc(sizeof(char)* (19));
	strftime(tf, 19,"%d%b%Y_%H:%M:%S", t);
	return tf;
}

char* timeformat1(struct tm *t) {
	char *tf = malloc(sizeof(char)* (20));
	strftime(tf, 20,"%d_%m_%Y_%H_%M_%S", t);
	return tf;
}


char* createQID(char* SID, struct tm *t) {
	char *QID = malloc(sizeof(char)* (23 + 1));
	sprintf(QID,"%s%s", SID, timeformat(t));
	return QID;
}

		
char* readTCPclient(int sockfd) {
	char buffer[MAX_BUF];
	int nread = 0;
	int message_size = 0;
	int i, message_capacity = MAX_BUF;
	//allocates memory for an array of 1 element of BUFFER_SIZE bytes, and set to zero the allocated memory 
	char *message = calloc(1, sizeof(char) * MAX_BUF); 

	while ((nread = read(sockfd, buffer, MAX_BUF)) != 0) {
		if(nread==-1) {	//error
			free(message);
			exit(1);
		} 
		for (i = 0; i < nread; i++) {
			message[message_size + i] = buffer[i];
		}
		memset(buffer, 0, MAX_BUF);
		message_size += nread;
		if (message_size >= message_capacity) {
			message_capacity += MAX_BUF;
			message = realloc(message, message_capacity);
		}
		//message ends with the character '\n'
		if(message[strlen(message)-1] == '\n')
			break;
	}
	message[strlen(message) - 1] = '\0';

	return message;
}


//--------------------------------------------------------------------------------------------

int main(int argc, char **argv) {

	int fd_tcp, fd_ecp, newfd, clientlen, ret, questionnaire_number, nbytes, nleft, size, score=0;
	
	struct sockaddr_in serveraddr, clientaddr;
	struct hostent *hostptr;
	
	unsigned short int TESport = TES_PORT, ECPport = ECP_PORT;
	
	ssize_t rcvd_bytes, send_bytes, read_file_bytes, send_bytes_data, AQT_bytes, AQS_bytes;

	char *SID, *QID, dtime[18+1], *data, filename[12+1], *correctanswers, *ptr1, ECPname[MAX_BUF], recv_str[MAX_BUF], send_str[MAX_BUF];
		 SID = (char*)malloc(sizeof(char) * (5 + 1));
		 QID = (char*)malloc(sizeof(char) * (23 + 1));
		 
		 
	char usage[] = "usage: ./TES [-p TESport] [-n ECPname] [-e ECPport]";

	time_t rawtime;
  	struct tm * current_time;
  	time (&rawtime);
  	current_time = localtime (&rawtime);
 	
	pid_t pid;
	void(*old_handler)(int);//interrupt handler

							/* Avoid zombies when child processes die. */
	if ((old_handler = signal(SIGCHLD, SIG_IGN)) == SIG_ERR) exit(1);
	
	if (signal(SIGALRM, handler_alrm) == SIG_ERR) { 
		perror("Error in signal");
		exit(1); 
	}
	

	if (argc % 2 != 1 || argc > 7) {
		printf("error: Incorrect number of arguments.\n%s\n", usage);
		return 1;
	}

	if (argc == 3) {
		if (!strcmp(argv[1], "-p")) TESport = atoi(argv[2]);
		else if (!strcmp(argv[1], "-n")) strcpy(ECPname, argv[2]);
		else if (!strcmp(argv[1], "-e")) ECPport = atoi(argv[2]);
		else {
			printf("error: Invalid option.\n%s\n", usage);
			return 1;
		}
	}

	if (argc == 5) {
		if (!strcmp(argv[1], "-p")) {
			TESport = atoi(argv[2]);
			if (!strcmp(argv[3], "-n")) strcpy(ECPname, argv[4]);
			else if (!strcmp(argv[3], "-e")) ECPport = atoi(argv[4]);
			else {
				printf("error: Invalid option.\n%s\n", usage);
				return 1;
			}
		}
		if (!strcmp(argv[1], "-n")) {
			strcpy(ECPname, argv[2]);
			if (!strcmp(argv[3], "-e")) ECPport = atoi(argv[4]);
			else {
				printf("error: Invalid option.\n%s\n", usage);
				return 1;
			}
		}
		else {
			printf("error: Invalid option.\n%s\n", usage);
			return 1;
		}
	}

	if (argc == 7) {
		if (!strcmp(argv[1], "-p")) {
			TESport = atoi(argv[2]);
			if (!strcmp(argv[3], "-n")) {
				strcpy(ECPname, argv[4]);
				if (!strcmp(argv[1], "-e")) ECPport = atoi(argv[2]);
				else {
					printf("error: Invalid option.\n%s\n", usage);
					return 1;
				}
			}else {
				printf("error: Invalid option.\n%s\n", usage);
				return 1;
			}
		}else {
			printf("error: Invalid option.\n%s\n", usage);
			return 1;
		}
	}


	if ((fd_tcp = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Error creating tcp socket.\n");
		exit(1);
	}
	if ((fd_ecp = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		perror("Error creating ecp socket.\n");
		exit(1);
	}
	memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((u_short)TESport);
	if (bind(fd_tcp, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
		perror("Error: bind().\n");
		exit(1);
	}
	if (listen(fd_tcp, 5) == -1){
		perror("Error: listen().\n");
		exit(1);
	}

	while (1) {

		clientlen = sizeof(clientaddr);

		do newfd = accept(fd_tcp, (struct sockaddr*)&clientaddr, &clientlen);//wait for a connection
		while (newfd == -1 && errno == EINTR);
		if (newfd == -1){
			perror("Error: accept().\n");
			exit(1);
		}

								/* Create a child process for each new connection. */
		if ((pid = fork()) == -1) {
			perror("Error: fork().\n");
			exit(1);
		}
		else if (pid == 0) { //child process
			close(fd_tcp);
			printf("Sent by [%s :%hu]\n",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
									
								/* USER-TES Protocol (in TCP) */
		
			ptr1 = readTCPclient(newfd);
			printf("\nReceived message:«%s»\n", ptr1);
		
			//we can use parseString() from functions.c...
			char* arr[8];
			char * ch;
			ch = strtok(strdup(ptr1), " ");
			int i = 0;
			while (ch != NULL) {
				arr[i] = ch;
				i++;
				ch = strtok(NULL, " ");
			}

			/* USER-TES: RQT SID
			TES-USER: AQT QID time size data */

			if (strcmp(arr[0], "RQT") == 0 && atoi(arr[1]) < 100000 && atoi(arr[1]) > 9999) {
				
				strcpy(SID, arr[1]);
			
				questionnaire_number = rand() % (NUM_OF_FILES + 1);//choose randomly 1 of available questionnaires
				sprintf(filename, "T%dQF%d.pdf", TES_NUMBER, questionnaire_number);
				
				sprintf(dtime, "%s",timeformat(deadline(current_time)));
				//generate QID: TES_NUMBER+questionnaire_number+deadline
				sprintf(QID, "%d_%d_%s",  TES_NUMBER, questionnaire_number, timeformat1(deadline(current_time))); 

				FILE *handler = fopen(filename, "rb");
				if (handler == NULL){
					perror("Error: fopen().\n");
					exit(1);
				}

				fseek(handler, 0, SEEK_END);
				size = ftell(handler);
				rewind(handler);

				//allocate a string that can hold it all
				data = (char*)malloc(sizeof(char) * (size + 1));
				//read it all in one operation
				read_file_bytes = fread(data, sizeof(char), size, handler);

				if (size != read_file_bytes) {
					free(data);
					perror("Error: fread().\n");
					exit(1);
				}

				//each message ends with the character '\n'
				AQT_bytes = sprintf(send_str, "AQT %s %s %d ", QID, dtime, size);

				ptr1 = send_str;

				/* write() may write a smaller number of bytes than solicited */
				nleft = AQT_bytes + read_file_bytes; // '\0' is not transmitted

				while (nleft > 0) {
					if (nleft > read_file_bytes) {
						send_bytes = write(newfd, ptr1, AQT_bytes);
						if (send_bytes <= 0) {
							perror("Error: write().\n");
							exit(1);
						}
						nleft -= send_bytes;
						ptr1 += send_bytes;
					}
					else {
						send_bytes_data = write(newfd, data, nleft);
						if (send_bytes_data <= 0) {
							perror("Error: data write().\n");
							exit(1);
						}
						nleft -= send_bytes_data;
						data += send_bytes_data;
					}
					
				}
				send_bytes_data = write(newfd, "\n", 1);
				if (send_bytes_data <= 0) {
					perror("Error: data write().\n");
					exit(1);
				}

				close (newfd); exit(0);
			}
			
			else if (strcmp(arr[0], "RQS") == 0 ) {
			  	
			  if (atoi(arr[1]) > 99999 ||  atoi(arr[1]) < 10000 || sizeof(arr[2]) > 24){
				  if ((send_bytes = write(newfd, "-2\n", 4)) <= 0)exit(1);
				  close(newfd); exit(1);
			  }
			  printf("SID, QID checked\n");
			  
			  strcpy(SID, arr[1]);
			  strcpy(QID, arr[2]);
			  printf("%s\n", QID);
			
			   if (checkdeadline(arr[2],current_time) == -1){
				  if ((send_bytes = write(newfd, "-1\n", 4)) <= 0)exit(1);
				  close(newfd); exit(1);
				  }
			  
			  
			  for (i = 0; i < 5; i++) {	  
				  if (strcmp(arr[i + 3], "A") == 0 ||
					  strcmp(arr[i + 3], "B") == 0 ||
					  strcmp(arr[i + 3], "C") == 0 ||
					  strcmp(arr[i + 3], "D") == 0 ||
					  strcmp(arr[i + 3], "N") == 0)continue;
				  else {
					  printf("\n----->invalid answers\n");
					  if ((send_bytes = write(newfd, "ERR\n", ERR_SIZE)) <= 0)exit(1);
				  }
			  }
			  
			  FILE *a = fopen(giveanswersfilename(QID), "r");
			  if (a == NULL) {
				  perror("Error: fopen().\n");
				  exit(1);
			  }
			  fseek(a, 0, SEEK_END);
			  size = ftell(a);
			  rewind(a);


			  //allocate a string that can hold it all
			  correctanswers = (char*)malloc(sizeof(char) * (size));
			  //read it all in one operation
			  if (fread(correctanswers, sizeof(char), size, a) <= 0) {
				  perror("Error: fread().\n");
				  exit(1);
			  }
			  

			  for (i = 0; i < 5; i++) {
				  if (*correctanswers == arr[i + 3][0]) {
					  score += 20;
					  correctanswers+=3;
				  }
			  }
			  
			  AQS_bytes = sprintf(send_str, "AQS %s %d\n", QID, score);


			  ptr1 = send_str;
			  nbytes = AQS_bytes; // '\0' is not transmitted

			  /* write() may write a smaller number of bytes than solicited */
			  nleft = nbytes;
			  while (nleft > 0) {
				  send_bytes = write(newfd, ptr1, nleft);
				  if (send_bytes <= 0) exit(1); //error
				  nleft -= send_bytes;
				  ptr1 += send_bytes;
			  }
			  printf("TEST AQS sent : «%s»", send_str);
			  close(newfd);
			  
			  /* TES-ECP Protocol (in UDP) */
				printf("tcp closed");
				printf("%s", ECPname);  
			  memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));
			  serveraddr.sin_family = AF_INET;
			 if ((hostptr = gethostbyname(ECPname)) == NULL) exit(1);
				printf("%s", ECPname);
			 serveraddr.sin_addr.s_addr = ((struct in_addr*)(hostptr->h_addr_list[0]))->s_addr; //ECP server IP address
			 printf("%s", hostptr->h_name);
			  serveraddr.sin_port = htons((u_short)ECPport);
			 
			  clientlen = sizeof(clientaddr);
				printf("ligacao ucp estabelecida\n");

			  /* TES-ECP: IQR SID QID topic_name score
			  ECP-TES: AWI QID */

			  sprintf(send_str, "IQR %s %s %s %d\n", SID, QID, TOPIC_NAME, score);
				printf("%s\n",send_str);

			  send_bytes = sendto(fd_ecp, send_str, strlen(send_str), 0, (struct sockaddr*)&clientaddr, clientlen);
			  if (send_bytes == -1)exit(1);//error
			 
			  alarm(10);//generates the SIGALRM signal when the specified time has expired

			  rcvd_bytes = recvfrom(fd_ecp, recv_str, strlen(recv_str), 0, (struct sockaddr*)&clientaddr, &clientlen);
			  if (rcvd_bytes == -1)exit(1);//error
			 else alarm(0); //cancel currently active alarm

			  char *array[8];
			  char *c = strtok(recv_str, " ");
			  int j = 0;
			  while (c != NULL) {
				  array[j] = c;
				  j++;
				  c = strtok(NULL, " ");
			  }
			  if (strcmp(array[0], "AWI") == 0 && strcmp(array[1], QID) == 0) {
				  close(fd_ecp);
				  exit(0);
			  }
			
			  else {
				  send_bytes = sendto(fd_ecp, "ERR\n", ERR_SIZE, 0, (struct sockaddr*)&clientaddr, clientlen);
				  if (send_bytes == -1)exit(1);//error
				  exit(1);
			  }
		    }
		      
		      else {
			printf("\n----->invalid answers\n");
			send_bytes = write(newfd, "ERR\n", ERR_SIZE);
			if (send_bytes <= 0)exit(1);
				close(newfd); exit(1);
		     }
			
		}
		
		//parent process
		do ret = close(newfd);
		while (ret == -1 && errno == EINTR);
		if (ret == -1)exit(1);//error
	}
}
