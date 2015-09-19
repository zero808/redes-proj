#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
