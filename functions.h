#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "constants.h"

int cmd_manager(int index, char **argv);
int action_selector();
int parseString(char *line, char ***argv);
void displayTopics(char ***argv);
int verifyAWT(int toks, char ***argv);
int verifyTnn(char *p);
int verifyAWTES(int toks, char ***argv);
char* getTCPServerReply(int sockfd);
int verifyAQT(int toks, char ***argv);
int validTime(char *time);
int validTimeDate(int day, char* month, int year);
int validMonth(char* month);
int daysMonth(int month, int year);
int validTimeHour(int hour, int min, int sec);

#endif


