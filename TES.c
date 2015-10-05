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
#define NUM_OF_QUESTIONS 5
#define TES_NUMBER 1
#define ERR_SIZE 4

extern int errno;
const char * CORRECT_ANSWERS[NUM_OF_FILES][NUM_OF_QUESTIONS] = { {"A","A","A","A","A"},{ "A","B","C","D","N"} };
const char * TOPIC_NAME = "Network Programming";

int comparedates(struct tm *actual, struct tm *deadline) {
	if (actual->tm_year > deadline->tm_year)return (-1);
	if (actual->tm_year == deadline->tm_year) {
		if (actual->tm_mon > deadline->tm_mon)return (-1);
		if (actual->tm_mon == deadline->tm_mon) {
			if (actual->tm_mday > deadline->tm_mday)return (-1);
			if (actual->tm_mday == deadline->tm_mday) {
				if (actual->tm_hour > deadline->tm_hour)return (-1);
				if (actual->tm_hour == deadline->tm_hour) {
					if (actual->tm_min > deadline->tm_min)return (-1);
					if (actual->tm_min == deadline->tm_min) {
						if (actual->tm_sec > deadline->tm_sec)return (-1);
					}
				}
			}
		}
	}
	return (0);
}

char*createQID() {
	time_t rawtime;
	struct tm *t;
	time(&rawtime);
	t = localtime(&rawtime);
	char day[2], mon[2], hour[2], min[2], sec[2];
	char *QID=malloc(sizeof (char)*(10+1));

	if (t->tm_mday < 10)sprintf(day, "0%d", t->tm_mday);
	else sprintf(day, "%d", t->tm_mday);
	if ((t->tm_mon + 1) < 10)sprintf(mon, "0%d", t->tm_mon + 1);
	else sprintf(mon, "%d", t->tm_mon + 1);
	if (t->tm_hour < 10)sprintf(hour, "0%d", t->tm_hour);
	else sprintf(hour, "%d", t->tm_hour);
	if (t->tm_min < 10)sprintf(min, "0%d", t->tm_min);
	else sprintf(min, "%d", t->tm_min);
	if (t->tm_sec < 10)sprintf(sec, "0%d", t->tm_sec);
	else sprintf(sec, "%d", t->tm_sec);
	//QID: unique transaction identifier (current time)
	sprintf(QID, "%s%s%s%s%s", day, mon, hour, min, sec);
	return QID;
}

char* setdeadline() {
	time_t rawtime;
	struct tm *t;
	time(&rawtime);
	t = localtime(&rawtime);
	char day[2], mon[2], hour[2], min[2], sec[2];
	enum months { JAN = 1, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC };
	enum months month;
	const char *monthName[] = { "", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
	char *deadline = malloc(sizeof(char)* (16 + 1));

	//deadline: current time + 15min
	if ((t->tm_min += 15) > 59) {
		t->tm_min -= 60;
		t->tm_hour++;
	}
	
	if (t->tm_mday < 10)sprintf(day, "0%d", t->tm_mday);
	else sprintf(day, "%d", t->tm_mday);
	if (t->tm_hour < 10)sprintf(hour, "0%d", t->tm_hour);
	else sprintf(hour, "%d", t->tm_hour);
	if (t->tm_min < 10)sprintf(min, "0%d", t->tm_min);
	else sprintf(min, "%d", t->tm_min);
	if (t->tm_sec < 10)sprintf(sec, "0%d", t->tm_sec);
	else sprintf(sec, "%d", t->tm_sec);
	
	month = t->tm_mon + 1;

	sprintf(deadline, "%s%s%d_%s:%s:%s", day, monthName[month], t->tm_year + 1900, hour, min, sec);

	return deadline;
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

int main(int argc, char **argv) {

	int fd_tcp, fd_ecp, newfd, clientlen, ret, num, nbytes, nleft, read_size, size, score=0;
	
	struct sockaddr_in serveraddr, clientaddr;
	struct hostent *hostptr;
	
	unsigned short int TESport = TES_PORT, ECPport = ECP_PORT;
	
	ssize_t rcvd_bytes, send_bytes, read_file_bytes, send_bytes_data, AQT_bytes, AQS_bytes;

	char SID[5+1], QID[10+1], deadline[18+1], *data, filename[12+1], *ptr1, *ptr2, *ptr3, *ptr4, *ECPname, recv_str[MAX_BUF], send_str[MAX_BUF];
	char usage[] = "usage: ./TES [-p TESport] [-n ECPname] [-e ECPport]";

	time_t rawtime;
	struct tm *currenttime, *deadlinetime;
	
	pid_t pid;
	void(*old_handler)(int);//interrupt handler

							/* Avoid zombies when child processes die. */
	if ((old_handler = signal(SIGCHLD, SIG_IGN)) == SIG_ERR) exit(1);


	if (argc % 2 != 1 || argc > 7) {
		printf("error: Incorrect number of arguments.\n%s\n", usage);
		return 1;
	}

	if (argc == 3) {
		if (!strcmp(argv[1], "-p")) TESport = atoi(argv[2]);
		else if (!strcmp(argv[1], "-n")) strcpy(ECPname, argv[4]);
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
			else if (!strcmp(argv[3], "-e")) ECPport = atoi(argv[2]);
			else {
				printf("error: Invalid option.\n%s\n", usage);
				return 1;
			}
		}else {
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
	if ((fd_ecp = socket(AF_INET, SOCK_STREAM, 0)) == -1){
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
				num = rand() % (NUM_OF_FILES + 1);//choose randomly 1 of available questionnaires

				sprintf(filename, "T%dQF%d.pdf", TES_NUMBER, num);
				
				sprintf(QID, "%s", createQID()); 

				time(&rawtime);
				deadlinetime = localtime(&rawtime);
				if ((deadlinetime->tm_min += 15) > 59) {
					deadlinetime->tm_min -= 60;
					deadlinetime->tm_hour++;
				}
				sprintf(deadline, "%s", setdeadline()); 

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
				printf("\nData to be sent: «%s»\n", data);

				//each message ends with the character '\n'
				AQT_bytes = sprintf(send_str, "AQT %s %s %d ", QID, deadline, size);


				ptr2 = send_str;

				/* write() may write a smaller number of bytes than solicited */
				nleft = AQT_bytes + read_file_bytes; // '\0' is not transmitted

				while (nleft > 0) {
					if (nleft > read_file_bytes) {
						send_bytes = write(newfd, ptr2, AQT_bytes);
						if (send_bytes <= 0) {
							perror("Error: write().\n");
							exit(1);
						}
						nleft -= send_bytes;
						printf("\nnsend first bytes =%d\n", send_bytes);
						ptr2 += send_bytes;
					}
					else {
						send_bytes_data = write(newfd, data, MAX_BUF);
						if (send_bytes_data <= 0) {
							perror("Error: data write().\n");
							exit(1);
						}
						nleft -= send_bytes_data;
						data += send_bytes_data;
					}
					printf("\nnleft data bytes=%d\n", nleft);
				}


				/* USER-TES: RQS SID QID V1 V2 V3 V4 V5
				TES-USER: AQS QID score */

				ptr3 = readTCPclient(newfd);
				ptr3[strlen(ptr3) - 1] = '\0'; //replace '\n' with '\0'
				printf("\nReceived message:«%s»\n", ptr3);

				char* arr[8];
				char * c;
				c = strtok(strdup(ptr3), " ");
				int i = 0;
				while (c != NULL) {
					arr[i] = c;
					i++;
					c = strtok(NULL, " ");
				}

				if (strcmp(arr[0], "RQS") == 0 && atoi(arr[1]) == atoi(SID) && atoi(arr[2]) == atoi(QID)) {
					int i, j;
					for (j = 0; j < 5; j++) {
						if (strcmp(arr[j + 3], "A") == 0 ||
							strcmp(arr[j + 3], "B") == 0 ||
							strcmp(arr[j + 3], "C") == 0 ||
							strcmp(arr[j + 3], "D") == 0 ||
							strcmp(arr[j + 3], "N") == 0)continue;

						else {
							printf("\n----->invalid answers\n");
							if (send_bytes = write(newfd, "ERR\n", ERR_SIZE) <= 0)exit(1);
						}
					}

					time(&rawtime);
					currenttime = localtime(&rawtime);
					if (comparedates(currenttime, deadlinetime) == -1) score = -1;
					else {
						for (i = 0; i < 5; i++) {
							if (strcmp(CORRECT_ANSWERS[num][i], arr[i + 3]) == 0)score += 20;
						}
					}
					AQS_bytes = sprintf(send_str, "AQS %s %d\n", QID, score);
					printf("%s Score: %d%\n", SID, score);

					ptr4 = send_str;
					nbytes = AQS_bytes; // '\0' is not transmitted

				/* write() may write a smaller number of bytes than solicited */
					nleft = nbytes;
					while (nleft > 0) {
						send_bytes = write(newfd, ptr4, nleft);
						if (send_bytes <= 0) exit(1); //error
						nleft -= send_bytes;
						ptr4 += send_bytes;
					}
					printf("TEST AQS sent : «%s»", send_str);

					close(newfd);

					/* TES-ECP Protocol (in UDP) */

					
					memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));
					serveraddr.sin_family = AF_INET;
					if (ECPname == NULL)serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
					else {
						if ((hostptr = gethostbyname(ECPname)) == NULL) exit(1);
						serveraddr.sin_addr.s_addr = ((struct in_addr*)(hostptr->h_addr_list[0]))->s_addr; //ECP server IP address
					}
					serveraddr.sin_port = htons((u_short)ECPport);
					if (bind(fd_ecp, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1)exit(1);//error
					clientlen = sizeof(clientaddr);

					/* TES-ECP: IQR SID QID topic_name score
					ECP-TES: AWI QID */

					sprintf(send_str, "IQR %s %s %s %d\n", SID, QID, TOPIC_NAME, score);


					while ((send_bytes = sendto(fd_ecp, send_str, strlen(send_str) + 1, 0, (struct sockaddr*)&clientaddr, clientlen)) != 0) {
						if (send_bytes = -1)exit(1);//error
					}

					while ((rcvd_bytes = recvfrom(fd_ecp, recv_str, strlen(recv_str), 0, (struct sockaddr*)&clientaddr, &clientlen)) != 0) {
						if (rcvd_bytes = -1)exit(1);//error
					}

					char* arr[8];
					char * ch;
					ch = strtok(recv_str, " ");
					int k = 0;
					while (ch != NULL) {
						arr[k] = ch;
						k++;
						ch = strtok(NULL, " ");
					}
					if (strcmp(arr[0], "AWI") == 0 && strcmp(arr[1], QID) == 0)close(fd_ecp);
					else {
						while ((send_bytes = sendto(fd_ecp, "ERR\n", ERR_SIZE, 0, (struct sockaddr*)&clientaddr, clientlen)) != 0) {
							if (send_bytes = -1)exit(1);//error
						}
					}

				}
			}else {
				if (send_bytes = write(newfd, "ERR\n", ERR_SIZE) <= 0)exit(1);
			}
		}

		//parent process
		do ret = close(newfd);
		while (ret == -1 && errno == EINTR);
		if (ret == -1)exit(1);//error
	}
}
