#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "constants.h"

int cmd_manager(int index, char **argv);
int action_selector();
int parseString(char *line, char ***argv, int max_toks);
void displayTopics(char ***argv);
int verifyAWT(int toks, char ***argv);
int verifyTnn(char *p);
int verifyAWTES(int toks, char ***argv);
int verifyAQT(char *tok, int ntok, char* qid, char* time, size_t *size);
int verifyQuestAnswers(char *answers, char ***argv);
int verifyAQS(char *aqs_reply, char ***argv, char *qid);
int validTime(char *time);
int validTimeDate(int day, char* month, int year);
int validMonth(char* month);
int daysMonth(int month, int year);
int validTimeHour(int hour, int min, int sec);
int checkErrorMessages(char* reply, char* request);
int getAQTReply(int sockfd, char *qid, char* time, size_t *size);
char* getAQSReply(int sockfd);

#endif


