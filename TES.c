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
const char * QF[NUM_OF_FILES][NUM_OF_QUESTIONS] = { {"A","A","A","A","A"},{ "A","B","C","D","N"} };
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
					

int main(int argc, char **argv) {

	int fd, newfd, clientlen, n, nw, ret, num;
	struct sockaddr_in serveraddr, clientaddr;
	unsigned short int TESport = PORT;

	ssize_t rcvd_bytes;
	ssize_t send_bytes;

	char *SID, *QID, *deadline, *data, *filename;
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

			while ((rcvd_bytes = read(newfd, recv_str, MAX_BUF)) != 0) {
				if (rcvd_bytes = -1)exit(1);//error
			}

			char* arr[8];
			char * ch;
			ch = strtok(recv_str, " ");
			int i = 0;
			while (ch != NULL) {
				arr[i] = ch;
				i++;
				ch = strtok(NULL, " ,");
			}

			/* USER-TES: RQT SID
			TES-USER: AQT QID time size data */

			if (strcmp(arr[0], "RQT") == 0 && atoi(arr[1]) < 100000 && atoi(arr[1]) > 9999) {

				strcpy(SID, arr[1]);
				
				num = rand() % (NUM_OF_FILES + 1);//choose randomly 1 of available questionnaires

				sprintf(filename, "/home/User/Desktop/T%dQF%d.pdf", TES_NUMBER, num);

				//todo data em binario conteudo do questionario 
				/*char sdbuf[MAX_BUF]; // Send buffer
				printf("[TES] Sending %s to the Student...", filename);
				FILE *fs = fopen(filename, "r");
				if (fs == NULL){
				printf("ERROR: File %s not found on server.\n", filename);
				exit(1);
				}

				bzero(sdbuf, MAX_BUF);
				int fs_block_sz;
				while ((fs_block_sz = fread(sdbuf, sizeof(char), MAX_BUF, fs))>0){
				if (send(newfd, sdbuf, fs_block_sz, 0) < 0){
				printf("ERROR: Failed to send file %s.\n", filename);
				exit(1);
				}
				bzero(sdbuf, MAX_BUF);
				}
				printf("Ok sent to client!\n");
				*/
				time(&rawtime);
				deadlinetime = localtime(&rawtime);
				//QID: unique transaction identifier (current time)
				sprintf(QID, "%d%d%d%d%d%d", deadlinetime->tm_mday,
					deadlinetime->tm_mon + 1,
					deadlinetime->tm_year + 1900,
					deadlinetime->tm_hour,
					deadlinetime->tm_min,
					deadlinetime->tm_sec);
				//deadline: current time + 15min
				deadlinetime->tm_min += 15;
				sprintf(deadline, "%d%d%d_%d:%d:%d", deadlinetime->tm_mday,
					deadlinetime->tm_mon + 1,
					deadlinetime->tm_year + 1900,
					deadlinetime->tm_hour,
					deadlinetime->tm_min,
					deadlinetime->tm_sec);

				/* file size in bytes */
				struct stat st;
				stat(filename, &st);
				size = st.st_size;

				//todo data
				sprintf(send_str, "AQT %s %s %d %s\n", QID, deadline, size, data);

				while ((send_bytes = write(newfd, send_str, MAX_BUF)) != 0) {
					if (send_bytes = -1)exit(1);//error
				}
			}

			/* USER-TES: RQS SID QID V1 V2 V3 V4 V5
			TES-USER: AQS QID score */

			if (strcmp(arr[0], "RQS") == 0 && strcmp(arr[1], SID) == 0 && strcmp(arr[2], QID)) {
				int j = 0;
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
						if (strcmp(QF[num][i], arr[i + 3]) == 0)score += 20;
					}
				}
				sprintf(send_str, "AQS %s %d\n", QID, score);
				printf("%s Score: %d%\n", SID, score);

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
				
				sprintf(send_str, "IQR %s %s %s %d\n\0", SID, QID, TOPIC_NAME, score);

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
					chh = strtok(NULL, " ,");
				}
				if (strcmp(array[0], "AWI") == 0 && strcmp(array[1], QID) == 0) {
					close(fd); exit(0);
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
