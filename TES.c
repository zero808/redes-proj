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

#define PORT 59000
#define MAX_BUF 128
#define NUM_OF_FILES 2
#define NUM_OF_QUESTIONS 5
#define TES_NUMBER 1

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

/*char * createQID() {
	//QID: DDMMHHMMSS ex: 2609195001 (10 numbers)
	char * qid;
	time_t rawtime;
	struct tm *currenttime;
	time(&rawtime);
	currenttime = localtime(&rawtime);
	char* day, *mon, *hour, *min, *sec;

	if (currenttime->tm_mday < 10)sprintf(day, "0%d", currenttime->tm_mday);
	if (currenttime->tm_mon + 1 < 10)sprintf(mon, "0%d", currenttime->tm_mon + 1);
	if (currenttime->tm_hour < 10)sprintf(hour, "0%d", currenttime->tm_hour);
	if (currenttime->tm_min < 10)sprintf(min, "0%d", currenttime->tm_min);
	if (currenttime->tm_sec < 10)sprintf(sec, "0%d", currenttime->tm_sec);

	sprintf(day, "%d", currenttime->tm_mday);
	sprintf(mon, "%d", currenttime->tm_mon + 1);
	sprintf(hour, "%d", currenttime->tm_hour);
	sprintf(min, "%d", currenttime->tm_min);
	sprintf(sec, "%d", currenttime->tm_sec);

	sprintf(qid, "%s%s%s%s%s", day, mon, hour, min, sec);
	return qid;
}*/

			
char *readTCPclient(int sockfd) {
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

	return message;
}




int main(int argc, char **argv) {

	int fd, newfd, clientlen, n, nw, ret, num, nbytes, nleft;
	struct sockaddr_in serveraddr, clientaddr;
	unsigned short int TESport = PORT;

	ssize_t rcvd_bytes;
	ssize_t send_bytes;

	//char *SID, *QID, *deadline, *data=NULL, *filename;
	char SID[5+1], QID[10+1], deadline[18+1], *data=NULL, filename[12+1];
	int size, score = 0;

	time_t rawtime;
	struct tm *deadlinetime, *currenttime;

	char recv_str[MAX_BUF], send_str[MAX_BUF];
	char *ptr;
	
	pid_t pid;
	char usage[] = "usage: ./TES [-p TESport]";
	void(*old_handler)(int);//interrupt handler

	/* Avoid zombies when child processes die. */
	if ((old_handler = signal(SIGCHLD, SIG_IGN)) == SIG_ERR)exit(1);//error

	if (argc % 2 != 1 || argc > 3) {
		printf("error: Incorrect number of arguments.\n%s\n", usage);
		return 1;
	}

	if (argc > 1) { 
		if (!strcmp(argv[1], "-p")) {
			TESport = atoi(argv[2]);
			//printf("%u\n", ECPport);
		}
		else {
			printf("error: Invalid option.\n%s\n", usage);
			return 1;
		}
	}

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)exit(1);//error

	memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((u_short)TESport);

	if (bind(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1)exit(1);//error

	if (listen(fd, 5) == -1)exit(1);//error

	while (1) {

		clientlen = sizeof(clientaddr);

		do newfd = accept(fd, (struct sockaddr*)&clientaddr, &clientlen);//wait for a connection
		while (newfd == -1 && errno == EINTR);
		if (newfd == -1)exit(1);//error

								/* Create a child process for each new connection. */
		if ((pid = fork()) == -1)exit(1);//error
		else if (pid == 0) { //child process
			close(fd);
			printf("Sent by [%s :%hu]\n",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
									
								/* USER-TES Protocol (in TCP) */


			/*-------------------------------------------------------*/
			
			// doesn't work...
			/*
			while ((rcvd_bytes = read(newfd, recv_str, MAX_BUF)) != 0) {
				if (rcvd_bytes = -1)exit(1);//error
			}
			*/


			ptr = readTCPclient(newfd);
			printf("TEST: received from user: «%s»\n", ptr);

			ptr[strlen(ptr)-1] = '\0'; //replace '\n' with '\0'
			printf("TEST: after replacing newline with NULL char: «%s»\n", ptr);
			/*-------------------------------------------------------*/

			//we can use parseString() from functions.c...
			char* arr[8];
			char * ch;
			ch = strtok(strdup(ptr), " ");
			int i = 0;
			while (ch != NULL) {
				arr[i] = ch;
				i++;
				ch = strtok(NULL, " ");
			}
			printf("arr[0]:%s\narr[1]:%d\n", arr[0], atoi(arr[1]));

			/* USER-TES: RQT SID
			TES-USER: AQT QID time size data */

			if (strcmp(arr[0], "RQT") == 0 && atoi(arr[1]) < 100000 && atoi(arr[1]) > 9999) {
				strcpy(SID, arr[1]);
				printf("TEST: received SID «%s»\n", SID);
				num = rand() % (NUM_OF_FILES + 1);//choose randomly 1 of available questionnaires

				//sprintf(filename, "/home/User/Desktop/T%dQF%d.pdf", TES_NUMBER, num);
				sprintf(filename, "T%dQF%d.pdf", TES_NUMBER, num);
				
				printf("TEST filename to send: «%s»\n", filename);
				
				
			
				
				//createQID() working...
				
	struct tm *t;
	time(&rawtime);
	t = localtime(&rawtime);
	char day[2], mon[2], hour[2], min[2], sec[2];

	if (t->tm_mday < 10)sprintf(day, "0%d", t->tm_mday);
	else sprintf(day, "%d", t->tm_mday);
	printf("%s", day);
	if ((t->tm_mon + 1) < 10)sprintf(mon, "0%d", t->tm_mon + 1);
	else sprintf(mon, "%d", t->tm_mon + 1);
	printf("%s", mon);

	if (t->tm_hour < 10)sprintf(hour, "0%d", t->tm_hour);
	else sprintf(hour, "%d", t->tm_hour);
	printf("%s", hour);

	if (t->tm_min < 10)sprintf(min, "0%d", t->tm_min);
	else sprintf(min, "%d", t->tm_min);
	printf("%s", min);

	if (t->tm_sec < 10)sprintf(sec, "0%d", t->tm_sec);
	else sprintf(sec, "%d", t->tm_sec);
	printf("%s", sec);


	sprintf(QID, "%s%s%s%s%s", day, mon, hour, min, sec);
				
				
				//QID: unique transaction identifier (current time)
				//sprintf(QID,"%s\n", ); //FIXME
				
				//createQID() is not working...
				printf("TEST QID creation: QID=<%s>\n", QID);
					time(&rawtime);
				deadlinetime = localtime(&rawtime);
				//deadline: current time + 15min
				deadlinetime->tm_min += 15;
				
				sprintf(deadline, "%d%d%d_%d:%d:%d", deadlinetime->tm_mday,
					deadlinetime->tm_mon + 1,
					deadlinetime->tm_year + 1900,
					deadlinetime->tm_hour,
					deadlinetime->tm_min,
					deadlinetime->tm_sec);

				//just a test...
				printf("TEST time to send: %s\n", deadline);

				FILE *handler = fopen(filename, "r");
				if (handler == NULL)exit(1);
				char *data = NULL;
				int read_size;
				
				fseek(handler, 0, SEEK_END);
				size = ftell(handler);
				rewind(handler);

				//allocate a string that can hold it all
				data = (char*)malloc(sizeof(char) * (size + 1));
				//read it all in one operation
				read_size = fread(data, sizeof(char), size, handler);
				//data[size] = '\n';

				if (size != read_size) {
					//something went wrong, throw away the memory and set
					//the buffer to NULL
					free(data);
					data = NULL;
				}
				
				printf("TEST data from pdf: «%s»", data);
				
				//sprintf(send_str, "AQT %s %s %d %s ", QID, deadline, size, data);
				
				
				
				//each message ends with the character '\n'
				n=sprintf(send_str, "AQT %s %s %d %s", QID, deadline, size, data);

							
				
				//FIXME
				//n = sprintf(send_str, "AQT 12345678 30NOV2015_11:55:00 %d %s\n", size, data);
				
				printf("TEST AQT to send: «%s»", send_str);				


				// works... but cannot be done this way...
				//FIXME
				/*
				while ((send_bytes = write(newfd, send_str, MAX_BUF)) != 0) {
					if (send_bytes = -1)exit(1);//error
				}
				*/
				
				ptr=send_str;
	    		nbytes=n; // '\0' is not transmitted
				
				/* write() may write a smaller number of bytes than solicited */
	    		nleft=nbytes;
	    		while(nleft>0) {
	    			send_bytes=write(newfd,ptr,nleft);
	    			if(send_bytes<=0) exit(1); //error
	    			nleft-=send_bytes;
	    			ptr+=send_bytes;
	    		}
				
			}

			/* USER-TES: RQS SID QID V1 V2 V3 V4 V5
			TES-USER: AQS QID score */

			if (strcmp(arr[0], "RQS") == 0 && strcmp(arr[1], SID) == 0 && strcmp(arr[2], QID)) {
				int i, j = 0;
				while (j < 5) {
					if (strcmp(arr[j + 3], "A") == 0 ||
						strcmp(arr[j + 3], "B") == 0 ||
						strcmp(arr[j + 3], "C") == 0 ||
						strcmp(arr[j + 3], "D") == 0 ||
						strcmp(arr[j + 3], "N") == 0)j++;
					else {
						if (send_bytes = write(newfd, "ERR\n", MAX_BUF) <= 0)exit(1);
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
				sprintf(send_str, "AQS %s %d\n", QID, score);
				//printf("%s Score: %d%\n", SID, score);

				while ((send_bytes = write(newfd, send_str, MAX_BUF)) != 0) {
					if (send_bytes = -1)exit(1);//error
				}
				close(newfd);
				
				/* TES-ECP Protocol (in UDP) */

				if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)exit(1);//error

				memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));
				serveraddr.sin_family = AF_INET;
				serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
				serveraddr.sin_port = htons((u_short)TESport);

				if (bind(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1)exit(1);//error

				clientlen = sizeof(clientaddr);

				/* TES-ECP: IQR SID QID topic_name score
			       ECP-TES: AWI QID */
				
				sprintf(send_str, "IQR %s %s %s %d\n", SID, QID, TOPIC_NAME, score);

				while ((send_bytes = sendto(fd, send_str, strlen(send_str) + 1, 0, (struct sockaddr*)&clientaddr, clientlen)) != 0) {
					if (send_bytes = -1)exit(1);//error
				}
				
				while ((rcvd_bytes = recvfrom(fd, recv_str, strlen(recv_str), 0, (struct sockaddr*)&clientaddr, &clientlen)) != 0) {
					if (rcvd_bytes = -1)exit(1);//error
				}

				char* array[8];
				char * chh;
				chh = strtok(recv_str, " ");
				int k = 0;
				while (chh != NULL) {
					array[k] = chh;
					k++;
					chh = strtok(NULL, " ");
				}
				if (strcmp(array[0], "AWI") == 0 && strcmp(array[1], QID) == 0) {
					close(fd); 
				}
				else {
					while ((send_bytes = sendto(fd,"ERR\n", 4, 0, (struct sockaddr*)&clientaddr, clientlen)) != 0) {
						if (send_bytes = -1)exit(1);//error
					}
				}
			}
			else {
				if (send_bytes = write(newfd, "ERR\n", MAX_BUF) <= 0)exit(1);
			}
		}

		//parent process
		do ret = close(newfd);
		while (ret == -1 && errno == EINTR);
		if (ret == -1)exit(1);//error
	}
}
