#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> /* for INET_ADDRSTRLEN */
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

#include "functions.h"

extern int errno;
extern char ECPname[];
extern unsigned short int ECPport;
extern char filename[];

int cmd_manager(int index, char **argv) {

	char c = argv[index][1];

	switch(c) {
		
		case 'n':	strcpy(ECPname, argv[index + 1]);
					break;
		case 'p':	ECPport = atoi(argv[index + 1]);
					break;
		default :	return -1;
	}
	
	return 0;
}

int action_selector() {
	char cmd[32];
	int action;
	
	printf("> ");
	
	scanf("%s", cmd);
	
	if (strcmp(cmd, "list") == 0)
		return action = 0;
	
	if (strcmp(cmd, "request") == 0)
		return action = 1;
	
	if (strcmp(cmd, "submit") == 0)
		return action = 2;
	
	if (strcmp(cmd, "exit") == 0)
		return action = 3;

	return action = -1;
}

void handler_alrm() {
	
	printf("socket timeout\n");
	exit(1);
}

int parseString(char *line, char ***argv, int max_toks) {
	char *buffer;
	int argc;
	const char delim[] = " \t";
	
	buffer = (char*) malloc((strlen(line) + 1) * sizeof(char));
	strcpy(buffer,line);
	
	*argv = (char**) malloc(max_toks * sizeof(char**));
	
	argc = 0;
	(*argv)[argc++] = strtok(buffer, delim);
	while ((((*argv)[argc] = strtok(NULL, delim)) != NULL) && (argc < max_toks))
		++argc;
	
	return argc;
}

void displayTopics(char ***argv) {
	int i, nt = atoi((*argv)[1]);
	
	/* display questionnaire topics as a numbered list */
	for (i = 0; i < nt; ++i) {
		printf("%d- %s\n",i+1, (*argv)[i+2]);
    }
}

int verifyAWT(int toks, char ***argv) {
	int i, nt, max_toks;
	
	max_toks = NB_TOPICS + 2;
	nt = atoi((*argv)[1]);
	
	if (toks <= max_toks && strcmp((*argv)[0], "AWT") == 0)
		;
	else
		return -1;
	
	if (nt < 1 || nt > NB_TOPICS)
		return -1;
	
	//detect mismatch between indicated number of topics and listed topics
	if(nt != toks-2)
		return -1;
	
	for (i = 2; i < toks; ++i)
		if(strlen((*argv)[i]) > TOPIC_NAME_SZ) 
			return -1;
    
	return 0;
}

int verifyAWTES(int toks, char ***argv) {
	int port;
	
	//INET_ADDRSTRLEN = 16 bytes (15 characters for IPv4 (xxx.xxx.xxx.xxx format, 12+3 separators) + '\0')
	if (toks == 3 && strcmp((*argv)[0], "AWTES") == 0 && (strlen((*argv)[1]) <= INET_ADDRSTRLEN-1))
		;
	else
		return -1;
	
	port = atoi((*argv)[2]);

	//The Dynamic and/or Private Ports are those from 49152 through 65535
	if(port < 0 || port > 65535)
		return -1;
	
	return 0;
}

int verifyTnn(char *p) {

	int tnn = 0;
	
	if(*p >= '1' && *p <= '9') {
		tnn = tnn * 10 + *p - '0';
		p++;
	}
	else
		return -1;
	
	if(isdigit(*p)) {// (*p >= '0' && *p <= '9')
		//tnn is composed of 2 digits
		tnn = tnn * 10 + *p - '0';
		p++;
	}
	    			
	if(*p != '\n')
		return -1;

	return tnn;
}


//validate time in format DDMMMYYYY_HH:MM:SS
int validTime(char *time) {

	int n, dd, yyyy, hh, mm, ss;
	char mmm[4];
	
	if(strlen(time) != 18)
		return 0;
	
	errno = 0;
	n = sscanf(time, "%2d%3s%4d_%2d:%2d:%2d", &dd, mmm, &yyyy, &hh, &mm, &ss); // '\0' is appended to mmm
	if (n == -1) {
		if (errno != 0) {
			perror("sscanf");
    	} else { 
    		printf("Invalid time format\n");
  			return 0;
    	}
    }

 	if (n != 6)
 		return 0;
 	
	if (validTimeDate(dd, mmm, yyyy) == 0)
		return 0;
	
	if (validTimeHour(hh, mm, ss) == 0)
		return 0;	
	
	return 1;
}

int validTimeDate(int day, char* month, int year) {
	int m;
	
	if((m = validMonth(month)) != 0) {
		if(day >= 1 && day <= daysMonth(m, year))
			return 1;
	}

	return 0;
}

int validMonth(char* m) {

	enum months { JAN = 1, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC };
	enum months month;
		
	//const char *monthName[] = { "", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
	const char *monthName[] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	
	for (month = JAN; month <= DEC; month++) {
		if (strcmp(m, monthName[month]) == 0)
			return month; // 1 <= month <= 12
	}
	
	return 0; //not valid
}

int daysMonth(int month, int year) {
	if (month == 4 || month == 6 || month == 9 || month == 11)
		return 30;
	
	if (month == 2)
		return ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) ? 29 : 28;
	
	return 31;  
}

int validTimeHour(int hour, int min, int sec) {

	if (hour < 0 || hour > 23)
		return 0;
		
	if (min < 0 || min > 59)
		return 0;
		
	if (sec < 0 || sec > 59)
		return 0;
		
	return 1;
}

int verifyQuestAnswers(char *answers, char ***argv) {
	
	int i, n, ch;
	
  	n = parseString(answers, argv, 5+1);
  	
  	if (n == NB_ANSWERS) {
  	
  		//check format
  		char str[STRING_SZ];
  		n = sprintf(str, "%c %c %c %c %c", *(*argv)[0], *(*argv)[1], *(*argv)[2], *(*argv)[3], *(*argv)[4]);
  		if (n < 0) { printf("Error in sprintf"); exit(1); }
  		
  		if (strcmp(answers, str) != 0) {
  			printf("Invalid format for questionnaire answers\n");
  			return -1;
  		}
  		
  		//check answer values
  		for (i = 0; i < NB_ANSWERS; i++) {
  			ch = *(*argv)[i];
  			if (!((ch >= 'A' && ch <= 'D') || ch == 'N')) {
  				printf("Invalid questionnaire answers\n");
  				return -1;
  			}
  		}	
	}
	else {
		printf("Invalid format for questionnaire answers\n");
		return -1;
	}
	
	return 0;
}

int verifyAQS(char *aqs_reply, char ***argv, char *qid) { //AQS QID score

	int n;
	
	//breaks the AQS reply into tokens
  	n = parseString(aqs_reply, argv, 3+1);
	if (n == 3) {
		char str[STRING_SZ];
		sprintf(str, "%s %s %s", (*argv)[0], (*argv)[1], (*argv)[2]);
		
		if (strcmp(aqs_reply, str) != 0)
			return -1; //invalid format
		
		int score;
		score = atoi((*argv)[2]); //last token (score) does NOT include byte '\n'
	
		if (strcmp((*argv)[0], "AQS") == 0  
			&& strcmp((*argv)[1], qid) == 0
			&& (score >= 0 && score <= 100)) ;
		else
			return -1;
	}
	else 
		return -1;
	
	return 0;
}

int checkErrorMessages(char* reply, char* request) {

	if (strcmp(reply, "ERR") == 0) {
		printf("The %s request is not correctly formulated\n", request);
		return -1;
	}
		
	if (strcmp(request, "TQR") == 0 || strcmp(request, "TER") == 0) {
		if (strcmp(reply, "EOF") == 0) {
			printf("The %s request cannot be answered\n", request);
			return -1;
		}
	}

	if (strcmp(request, "AQS") == 0) {
		if (strcmp(reply, "-1") == 0) {
			printf("Questionnaire submitted after the deadline\n");
			return -1;
		}
		else if (strcmp(reply, "-2") == 0) {
			printf("SID-QID pair does not match\n");
			return -1;
		}
		else if (strncmp(reply, "AQS ", 4) != 0) {
			printf("Invalid AQS reply\n");
			return -1;
		}
	}
	
	if (strcmp(request, "TQR") == 0) {
		if (strncmp(reply, "AWT ", 4) != 0)
			return -1;
	}
	
	if (strcmp(request, "TER") == 0) {
		if (strncmp(reply, "AWTES ", 6) != 0)
			return -1;	
	}
	
	return 0;
}


int verifyAQT(char *tok, int ntok, char* qid, char* time, size_t *size) {
	int len;
	
	if (ntok == 0) { //AQT
	
		if (strncmp(tok, "ERR", 3) == 0) {
			printf("The RQT request is not correctly formulated\n");
			return -1;
		}
	
		if (strncmp(tok, "AQT", 3) != 0)
			return -1;
	}
	
	if (ntok == 1) { //QID
		len = strlen(tok);	
		if (len < 1 || len > QID_SZ)
			return -1;
		else
			strcpy(qid, tok);
	}

	if (ntok == 2) { //time
		if (!validTime(tok))
			return -1;
		else
			strcpy(time, tok);
	}

	if (ntok == 3) { //size
		*size = (size_t)atoi(tok); 
		if (size == 0)
			return -1;
	}

	return 0;
}


int getAQTReply(int sockfd, char* qid, char* time, size_t* size) {
	char buffer[BUFFER_SZ];
	int nread = 0, message_size = 0, message_capacity = BUFFER_SZ;
	int i, k = 0, n, ndelim = 0, nleft, nbytes, nwritten, nlastread;
	const char delim = ' ';
	char* token;
	char* ptr;
	size_t sz;
	
	FILE* fd;
	
	/* set to zero the allocated memory */ 
	char* message = (char*) calloc(BUFFER_SZ, sizeof(char)); 
	if (message == NULL) { 
		printf("Error allocating memory\n");
		exit(1);
	} 
	
	/* get AQT, QID, time and size from AQT reply */
	
	do {
	
		if((nread = read(sockfd, buffer, BUFFER_SZ)) == -1) {	
			free(message);
			exit(1);//error
		}
		
		if (nread > 0) {
		
			for (i = 0; i < nread; ++i) {
				message[message_size + i] = buffer[i];
				if (buffer[i] == delim) {
					sz = (message_size + i) - k; //length of the token
					token = (char*) calloc((sz + 1), sizeof(char));
					memcpy(token, message + k, sz); //message + k points to the token's first char
					
					n = verifyAQT(token, ndelim, qid, time, size); //validate token
					if (n == -1)
	    				return -1;
					
					++ndelim;
					k = (message_size + i) + 1; //index of next token's first char 
				}
				
				if (ndelim == 4) break; //all tokens processed
			}
				
			if (ndelim < 4) {
				memset(buffer, 0, BUFFER_SZ); //clear the buffer
				message_size += nread;
			
				if (message_size >= message_capacity) {
					message_capacity += BUFFER_SZ;
					message = realloc(message, message_capacity);
					if (message == NULL) { 
						printf("Error allocating memory\n");
						exit(1);
					}
				}		
			}
			else {
				nlastread = i + 1; //number of bytes read and processed from last call to read()
				message_size += nlastread; //index of first data byte
			}
				
		}
		
	//message ends when the 4th delim is reached, or when nread = 0 (closed by peer)
	} while (ndelim < 4 && nread > 0);

	/* process data from AQT reply */

	int data_read = nread - nlastread; //data bytes from last read()
	
	n = sprintf(filename, "%s.pdf", qid);
	if (n < 0) { printf("Error in sprintf"); exit(1); }
	fd = fopen(filename, "wb");
	if (fd == NULL) exit(1);
	
	size_t data_size = *size; //file size (in Bytes)
	
	if (data_read > data_size) { //end message character ('\n') already read
	
		/* process data_size (i.e., all) data bytes */
		ptr = &buffer[nlastread]; //ptr points to first unprocessed byte
		nwritten = fwrite(ptr, sizeof(char), data_size, fd);
		if (nwritten != data_size) { printf("Error in fwrite");	exit(1); }
		
		if (buffer[nlastread + data_size] == '\n') {
			fclose(fd);
			return 0;
		}
		else { //AQT reply is not valid
			fclose(fd);
			n = remove(filename); //delete file
			if (n == 0)
				printf("%s file deleted\n", filename);
   			else 
   				perror("Error in remove");
   		
			return -1;	
		}
	}
	
	if (data_read > 0 && data_read <= data_size) { //process first (or all) data bytes from last read()
		ptr = &buffer[nlastread]; //ptr points to first unprocessed byte
		nwritten = fwrite(ptr, sizeof(char), data_read, fd);
		if (nwritten != data_read) {
			printf("Error in fwrite()\n");
			exit(1);
		}
	}
	
	/* read and process all the data bytes if data_read == 0 */
	nbytes = data_size - data_read;
	nleft = nbytes; //bytes left to read
	
	char data[data_size];
	memset(data, 0, data_size);
	ptr = &data[0];
	
	while(nleft > 0) {
		nread = read(sockfd, ptr, nleft);
		if (nread == -1) { perror("Error in read"); exit(1);} //error
		else if(nread == 0) break; //closed by peer

		nwritten = fwrite(ptr, sizeof(char), nread, fd);
		if (nwritten != nread) {
			printf("Error in fwrite\n");
			exit(1);
		}
	
		nleft -= nread;
		ptr += nread;
	}

	nread = nbytes - nleft;
	if (nread != nbytes)
		printf("Closed by peer\n");
	
	fclose(fd);
	
	/* read last byte (AQT reply ends with the character '\n') */
	
	memset(buffer, 0, BUFFER_SZ); //clear the buffer
	
	nread = read(sockfd, buffer, BUFFER_SZ);
	if (nread == -1) exit(1); //error
	else if (nread == 0) exit(1); //closed by peer
	
	if (nread == 1 && buffer[0] == '\n') {
		return 0;
	}
	
	return -1;	
}


char* getAQSReply(int sockfd) {
	char buffer[STRING_SZ];
	int nread = 0, message_size = 0;
	int i, message_capacity = STRING_SZ;
	
	//allocates memory for an array of BUFFER_SIZE bytes, and set to zero the allocated memory 
	char *message = calloc(1, sizeof(char) * STRING_SZ); 
	if (message == NULL) exit(1);
	
	do {
		
		if((nread = read(sockfd, buffer, STRING_SZ)) == -1) {	
			free(message);
			exit(1);//error
		}
		
		if (nread > 0) {
			for (i = 0; i < nread; i++) {
				message[message_size + i] = buffer[i];
			}
			memset(buffer, 0, STRING_SZ); //clear the buffer

			message_size += nread;
			if (message_size >= message_capacity) {
				message_capacity += STRING_SZ;
				message = realloc(message, message_capacity);
				if (message == NULL) exit(1);
			}
		}

	//message ends with the character '\n', or when nread = 0 (closed by peer)
	} while ((message[strlen(message)-1] != '\n') && nread > 0);

	return message;
}

