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

extern int errno;
const char * qf1[] = {"A","A","A","A","A"};
const char * qf2[] = { "A","B","C","D","N"};

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
					

void str_echo(int sock_fd) {
	ssize_t rcvd_bytes;

	char *SID;
	char *QID;
	char *deadline;
	int size;
	char *data;
	int score = 0;
	
	time_t rawtime;
	struct tm *deadlinetime;
	struct tm *currenttime;

	char recv_str[MAX_BUF];
	char send_str[MAX_BUF];

	while ((rcvd_bytes = read(sock_fd, recv_str, MAX_BUF)) > 0) {
		char *array[10];
		int i = 0;
		array[i] = strtok(recv_str, " ");
		while (array[i] != NULL){
			array[++i] = strtok(NULL, " ");
		}

		if (strcmp(array[0], "RQT") == 0) {
			//printf("RQT sent by [%s:%hu]\n",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));

			strcpy(SID, array[1]);
			
			//todo choose randomly the questionnaire
			char* filename = "/home/User/Desktop/TnnQFxxx.pdf";
			char sdbuf[MAX_BUF]; // Send buffer
			printf("[TES] Sending %s to the Student...", filename);
			FILE *fs = fopen(filename, "r");
			if (fs == NULL){
				printf("ERROR: File %s not found on server.\n", filename);
				exit(1);
			}

			bzero(sdbuf, MAX_BUF);
			int fs_block_sz;
			while ((fs_block_sz = fread(sdbuf, sizeof(char), MAX_BUF, fs))>0){
				if (send(sock_fd, sdbuf, fs_block_sz, 0) < 0){
					printf("ERROR: Failed to send file %s.\n", filename);
					exit(1);
				}
				bzero(sdbuf, MAX_BUF);
			}
			printf("Ok sent to client!\n");

			time(&rawtime);
			deadlinetime = localtime(&rawtime);
			//unique transaction identifier (current time)
			sprintf(QID, "%d%d%d%d%d%d", deadlinetime->tm_mday, 
						     deadlinetime->tm_mon + 1, 
						     deadlinetime->tm_year + 1900, 
						     deadlinetime->tm_hour, 
						     deadlinetime->tm_min, 
						     deadlinetime->tm_sec);
			//deadline (current time + 15min)
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
			sprintf(send_str, "AQT %s %s %d %s", QID, deadline, size, data);

			write(sock_fd, send_str, MAX_BUF);
		}
		
		if (strcmp(array[0], "RQS") == 0 && strcmp(array[1], SID) == 0 && strcmp(array[2], QID)) {
			//printf("RQS sent by [%s:%hu]\n",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
			i = 0;
			while(i < 5)  {
				if (strcmp(array[i + 3], "A") == 0 || 
				    strcmp(array[i + 3], "B") == 0 || 
				    strcmp(array[i + 3], "C") == 0 ||
				    strcmp(array[i + 3], "D") == 0 || 
				    strcmp(array[i + 3], "N") == 0)i++;
				else{
					write(sock_fd, "ERR", MAX_BUF);
				}
			}
			
			time(&rawtime);
			currenttime = localtime(&rawtime);
			if (comparedates(currenttime, deadlinetime)==-1) score=-1;
			else {
				for (i = 0; i < 5; i++) {
					if (strcmp(qf1[i], array[i + 3]) == 0)score += 20;
				}
			}
			sprintf(send_str, "AQS %s %d", QID, score);

			write(sock_fd, send_str, MAX_BUF);
			//todo comunication with ECP using UDP
		}
		else {
			write(sock_fd, "ERR", MAX_BUF);
		}

		if (rcvd_bytes < 0)
			perror("read error");
	}
}

	int main(int argc, char **argv) {

		int fd, newfd, clientlen, n, nw, ret;
		struct sockaddr_in serveraddr, clientaddr;
		//char *ptr, buffer[128];
		unsigned short int TESport = PORT;
		pid_t pid;
		char usage[] = "usage: ./TES [-p TESport]";
		void(*old_handler)(int);//interrupt handler

		/* Avoid zombies when child processes die. */
		if ((old_handler = signal(SIGCHLD, SIG_IGN)) == SIG_ERR)exit(1);//error

		if (argc % 2 != 1 || argc > 3) {
			printf("error: Incorrect number of arguments.\n%s\n", usage);
			return 1;
		}

		if (argc > 1) { //ou argv[1] != NULL
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
				str_echo(newfd);
				/*while((n=read(newfd,buffer,128))!=0){
				if(n==-1)exit(1);//error
				ptr=&buffer[0];
				while(n>0){
				if((nw=write(newfd,ptr,n))<=0)exit(1);//error
				n-=nw; ptr+=nw;
				}
				}*/
				//exit(0);
			}
			//close(newfd);

			//parent process
			do ret = close(newfd);
			while (ret == -1 && errno == EINTR);
			if (ret == -1)exit(1);//error
		}
		/* close(fd); exit(0); */
	}
