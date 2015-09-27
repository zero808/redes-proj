#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> /* for INET_ADDRSTRLEN */
#include <errno.h>

#include "functions.h"

extern int errno;
extern char ECPname[];
extern unsigned short int ECPport;

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

char* getTCPServerReply(int sockfd) {
	char buffer[BUFFER_SIZE];
	int nread = 0, message_size = 0;
	int i, message_capacity = BUFFER_SIZE;
	
	//allocates memory for an array of BUFFER_SIZE bytes, and set to zero the allocated memory 
	char *message = calloc(1, sizeof(char) * BUFFER_SIZE); 
	if (message == NULL) exit(1);
	
	do {
		
		if((nread = read(sockfd, buffer, BUFFER_SIZE)) == -1) {	
			free(message);
			exit(1);//error
		}
		
		if (nread > 0) {
			for (i = 0; i < nread; i++) {
				message[message_size + i] = buffer[i];
			}
			memset(buffer, 0, BUFFER_SIZE); //clear the buffer

			message_size += nread;
			if (message_size >= message_capacity) {
				message_capacity += BUFFER_SIZE;
				message = realloc(message, message_capacity);
				if (message == NULL) exit(1);
			}
		}

	//message ends with the character '\n', or when nread = 0 (closed by peer)
	} while ((message[strlen(message)-1] != '\n') && nread > 0);

	return message;
}

int verifyAQT(char *aqt_reply, char ***argv) { //AQT QID time size data
	int n;
	
	//breaks the AQT reply into tokens
	n = parseString(aqt_reply, argv, 5+1);
	if (n == 5) {
		int size;
		size = atoi((*argv)[3]); //size of data
	
		if (strcmp((*argv)[0], "AQT") == 0  
			&& (strlen((*argv)[1]) <= QID_SZ)
			&& validTime((*argv)[2]) //validate time in format DDMMMYYYY_HH:MM:SS 
			&& strlen((*argv)[4]) == size) //last token (data) does NOT include byte '\n'
			;
		else
			return -1;
	}
	else 
		return -1;
	
	return 0;
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
		
	const char *monthName[] = { "", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
	
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
	
	int n;
	
	//check only format (answers values will be checked by TES)
  	n = parseString(answers, argv, 5+1);
  	if (n == 5) {
  		char str[128];
  		sprintf(str, "%s %s %s %s %s", (*argv)[0], (*argv)[1], (*argv)[2], (*argv)[3], (*argv)[4]);
  		//printf("str:%s", str);
  		if (strcmp(answers, str) != 0) {
  			return -1; //invalid format
  		}
	}
	else
		return -1;
	
	return 0;
}

int verifyAQS(char *aqs_reply, char ***argv, char *qid) { //AQS QID score

	int n;
	
	//breaks the AQS reply into tokens
  	n = parseString(aqs_reply, argv, 3+1);
	if (n == 3) {
		char str[128];
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

	if (strcmp(request, "RQS") == 0 && strcmp(reply, "-1") == 0) {
		printf("Questionnaire submitted after the deadline\n");
		return -1;
	}
	
	return 0;
}
