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

#define PORT 59000
#define MAX_BUF 128

extern int errno;


void str_echo(int sock_fd) {
	ssize_t rcvd_bytes;

	char *SID;
	char *QID;
	char *deadline;
	int size;
	char *data;
	int score;
	
	time_t rawtime;
	struct tm *timeinfo;

	char recv_str[MAX_BUF];
	char send_str[MAX_BUF];

	while ((rcvd_bytes = read(sock_fd, recv_str, MAX_BUF)) > 0) {
		char *array[10];
		int i = 0;
		array[i] = strtok(recv_str, " ");
		while (array[i] != NULL)
		{
			array[++i] = strtok(NULL, " ");
		}

		if (strcmp(array[0], "RQT") == 0) {
			//printf("RQT sent by [%s:%hu]\n",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));

			strcpy(SID, array[1]);
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			//current+15min
			sprintf(deadline, "%d%d%d_%d:%d:%d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min + 15, timeinfo->tm_sec);

			//todo send file!
			sprintf(send_str, "AQT %s %s %d %s", QID, deadline, size, data);

			write(sock_fd, send_str, MAX_BUF);
		}
		if (strcmp(array[0], "RQS") == 0 && strcmp(array[1], SID) == 0 && strcmp(array[2], QID)) {
			//printf("RQS sent by [%s:%hu]\n",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
			//todo compare current date with deadline
			//todo calculate score

			sprintf(send_str, "AQS %s %d\%", QID, score);

			write(sock_fd, send_str, MAX_BUF);

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
				exit(0);
			}
			close(newfd);

			//parent process
			do ret = close(newfd);
			while (ret == -1 && errno == EINTR);
			if (ret == -1)exit(1);//error
		}
		/* close(fd); exit(0); */
	}
