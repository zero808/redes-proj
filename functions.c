#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> /* for INET_ADDRSTRLEN */

#include "functions.h"

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


int parseString(char *line, char ***argv) {
	char *buffer;
	int argc, max_toks;
	
	buffer = (char*) malloc(strlen(line) * sizeof(char));
	strcpy(buffer,line);
	
	max_toks = NB_TOPICS + 2;
	*argv = (char**) malloc(max_toks * sizeof(char**));
	
	argc = 0;
	(*argv)[argc++] = strtok(buffer, " ");
	while ((((*argv)[argc] = strtok(NULL, " ")) != NULL) && (argc < max_toks))
		++argc;
	
	return argc;
}

void displayTopics(char ***argv) {
	int i, nt = atoi((*argv)[1]);
	
	/* display questionnaire topics as a numbered list */
	for (i = 0; i < nt; ++i) {
		printf("%d- %s",i+1, (*argv)[i+2]);
		if(i < nt-1) //last token includes byte '\n'
		printf("\n");
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
