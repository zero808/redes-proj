#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "constants.h"
#include "functions.h"

/* I need this for getline() */
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

/* AWT -> 3
 * nT -> 2
 * 99 Topics * 25 chars max
 * 100 spaces in between them at most
 * 1 for '\n' */
#define REPLY_MAX_SIZE 2585

/* Maximum number of tokens that any request or reply can contain */
#define MAX_TOKENS 5
/* size of AWTES */
#define SIZE_AWTES 50
/* reply to TES server */
#define SIZE_AWI 30
/* buffer to read requests to the server */
#define BUFFER_SIZE 128
/* file names */
const char topics_file[11] = "topics.txt";
const char stats_file[10] = "stats.txt";

struct file_lines {
    /* Number of lines read from file*/
    unsigned int lines_used;
    /* the data from the file */
    char **lines;
};

/* we save the each average score here and then save
 * all the scores to stats.txt */
struct topic_score {
    char topic_name[TOPIC_NAME_SZ];
    float average_score;
    int submissions;
};

struct submission {
    int SID;
    char topic_name[TOPIC_NAME_SZ];
    int score;
    struct submission *next;
};

/* the caller must free all the memory */
struct file_lines* readTopics(const char *filename) {
    unsigned int lines = 0;
    int i = 0;
    FILE *fp = fopen(filename,"r");
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    struct file_lines *fl = NULL;
    fl = calloc(1, sizeof(struct file_lines));
    if(fl == NULL)
        exit(EXIT_FAILURE);

    fl->lines_used = 0; /* should be 0 anyway from the calloc... */
    fl->lines = calloc(NB_TOPICS, sizeof(char*));
    if(fl->lines  == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        len = strlen(line);
        fl->lines[i] = calloc(len, sizeof(char));
        if(fl->lines[i] == NULL)
            exit(EXIT_FAILURE);
        strncpy(fl->lines[i], line, len);
        fl->lines[i][len - 1] = '\0'; /* ditch the \n */
        ++(fl->lines_used);
    }
    return fl;
}

int parseRequest(char *line, char ***argv, int max_toks) {
    char *buffer = NULL;
    int argc = 0;
    size_t length = 0;;

    length = strlen(line);
    buffer = (char*) calloc(length, sizeof(char));
    strncpy(buffer,line, length);

    *argv = (char**) calloc(max_toks, sizeof(char**));

    (*argv)[argc++] = strtok(buffer, " ");
    while ((((*argv)[argc] = strtok(NULL, " ")) != NULL) && (argc < max_toks))
        ++argc;

    return argc;
}

int checkIfTopicExists(char* topic, struct file_lines* fl) {
    if((topic != NULL) && (fl != NULL)) {
        for(int i = 0; i < fl->lines_used; ++i) {
            /* FIXME we only want to compare the first word */
            if(strncmp(topic, fl->lines[i], TOPIC_NAME_SZ) == 0)
                return 0;
        }
    }
    return -1;
}

void initializeTopicScores(struct topic_score (*tsc)[NB_TOPICS]) {
    for (int i = 0; i < NB_TOPICS; i++) {
        strncpy((*tsc)[i].topic_name, "", TOPIC_NAME_SZ);
        (*tsc)[i].average_score = 0.0f;
        (*tsc)[i].submissions = 0;
    }
}

/* }; */
/* struct topic_score { */
/*     char topic_name[TOPIC_NAME_SZ]; */
/*     float average_score; */
/*     int sumbmissions; */
/* }; */
int saveScore(struct topic_score (*tsc)[NB_TOPICS], struct submission **sb, char *sid, char *topic, char *score) {
    int sco = 0;
    int index = 0;
    int SID = 0;
    float temp = 0.0;
    struct submission **current = NULL;
    FILE *fp = NULL;
    if((tsc == NULL) || (sb == NULL) || (sid == NULL) || (topic == NULL) || (score == NULL))
        return -1;
    sco = atoi(score);
    SID = atoi(sid);
    current = sb;

    /* FIXME add mutexes */

    for (index = 0; index < NB_TOPICS; ++index) {
        /* the topic is not already present in the array */
        if(strncmp((*tsc)[index].topic_name, "", TOPIC_NAME_SZ) == 0) {
            /* copy the topic name to its place */
            strncpy((*tsc)[index].topic_name, topic, TOPIC_NAME_SZ);
            break;
        }
        /* if it is we are just interested in its position */
        if(strncmp((*tsc)[index].topic_name, topic, TOPIC_NAME_SZ) == 0) {
            break;
        }
        /* if not keep looking for it */
    }
    /* now that we have the postion */
    /* save the number of submissions */
    temp = (*tsc)[index].submissions;
    (*tsc)[index].submissions += 1;
    /* this way we have the sum of the scores */
    (*tsc)[index].average_score *= temp;
    /* and add this new one */
    (*tsc)[index].average_score += (float) sco;
    /* and now we can calculate the average score */
    (*tsc)[index].average_score *= (*tsc)[index].submissions;

    /* now do the same for the other structure */
    while((*current)->next != NULL) {
        (*current) = (*current)->next;
    }
    (*current)->next = calloc(1, sizeof(struct submission));
    (*current)->next->SID = SID;
    strncpy((*current)->next->topic_name, topic, TOPIC_NAME_SZ);
    (*current)->next->score = sco;
    (*current)->next->next = NULL;

    /* open the file with write permission and create it if it doesn't exist */
    fp = fopen(stats_file, "w+");
    if(fp == NULL)
        exit(EXIT_FAILURE);

    for(index = 0; index < NB_TOPICS; ++index) {
        fprintf(fp, "%s %d\n", (*tsc)[index].topic_name, (*tsc)[index].average_score);
    }
    for(current = sb; (*current) != NULL; (*current) = (*current)->next) {
        fprintf(fp, "%d %s %d\n", (*current)->SID, (*current)->topic_name, (*current)->score);
    }
    if(fclose(fp))
        exit(EXIT_FAILURE);


    return 0;
}

int main(int argc, char **argv) {

    int fd, addrlen, ret, nread;
    struct sockaddr_in serveraddr, clientaddr;

    /* buffer to save incoming requests */
    char buffer[BUFFER_SIZE];
    /* to tokenize a request */
    char **toks = NULL;
    /* to use when replying to requests from user */
    char awt[REPLY_MAX_SIZE] = "AWT ";
    /* use to reply to the TES */
    char awi[SIZE_AWI] = "AWI ";
    /* same for AWTES */
    char awtes[SIZE_AWTES] = "AWTES ";
    /* save the scores for each topic in memory */
    struct topic_score scores[NB_TOPICS];
    /* each student submission */
    struct submission *submissions = NULL;

    int qid;
    /* content of topics.txt. */
    struct file_lines *topics = NULL;
    unsigned short int ECPport = PORT + GN;
    char usage[] = "usage: ./ECP [-p ECPport]";
    pid_t pid;
    void(*old_handler)(int);//interrupt handler

    /* Avoid zombies when child processes die. */
    if ((old_handler = signal(SIGCHLD, SIG_IGN)) == SIG_ERR)exit(1);//error

    if(argc % 2 != 1 || argc > 3){
        printf("error: Incorrect number of arguments.\n%s\n", usage);
        return 1;
    }

    if(argc > 1) { //ou argv[1] != NULL
        if (!strcmp(argv[1], "-p")) {
            ECPport = atoi(argv[2]);
        }
        else {
            printf("error: Invalid option.\n%s\n", usage);
            return 1;
        }
    }

    if((fd=socket(AF_INET,SOCK_DGRAM,0))==-1)exit(1);//error

    memset((void*)&serveraddr,(int)'\0',sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_addr.s_addr=htonl(INADDR_ANY); //Accept datagrams on any Internet interface on the system.
    serveraddr.sin_port=htons((u_short) ECPport);

    /* Use bind to register the server well-known address (and port) with the system. */
    ret=bind(fd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
    if(ret==-1)exit(1);//error

    /* read topics.txt and keep it in memory to avoid reading it everytime */
    topics = readTopics(topics_file);
    /* FIXME were copying strings twice, just create the final string when we read from file
     * and do it in its own thread */
    unsigned int j = 5; /* 'a' 'w' 't' ' ' */
    /* 25 do nome do topico + '\0' */
    char temp[26] = "";
    /* put the number on a string */
    snprintf(temp, 3*sizeof(char), "%d", topics->lines_used);
    j += strlen(temp);
    printf("len do temp: %d\n", j);
    strncat(&awt[j], temp, strlen(temp));

    for(unsigned int i = 0; i < topics->lines_used; i = i){
        /* FIXME
         * we only want the topic name, so we have to remove
         * the IP and port of the server */
        strncat(&awt[j], topics->lines[i], REPLY_MAX_SIZE);
        if(++i < topics->lines_used) {
            /* update j */
            j += strlen(topics->lines[i]);
            /* add a separator */
            strncat(&awt[j], " ", 2*sizeof(char));
            ++j; /* +1 for the whitespace */
        }
    }
    /* add the \n to the end of the string */
    strncat(awt, "\n", REPLY_MAX_SIZE * sizeof(char));

    while(1){

        addrlen=sizeof(clientaddr);
        nread=recvfrom(fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &clientaddr, &addrlen);
        if(nread==-1)exit(1);//error
        buffer[nread] = '\0';

        pid = fork();
        if(pid == -1)
            exit(EXIT_FAILURE);
        /* Child process */
        if(pid == 0) {


            /* divide what we read into tokens */
            parseRequest(buffer, &toks, MAX_TOKENS);

            /* User - ECP */
            /* TQR */
            if(strncmp(toks[0], "TQR\n", 5* sizeof(char)) == 0) {
                /* FIXME */
                if(topics->lines_used > 0) {
                    ret=sendto(fd,awt, strlen(awt)*sizeof(char),0,(struct sockaddr*)&clientaddr,addrlen);
                }
                else
                    ret=sendto(fd,"EOF\n", 4*sizeof(char),0,(struct sockaddr*)&clientaddr,addrlen);
                if(ret==-1)exit(1);
            }

            /* TER Tnn */
            if(strncmp(toks[0], "TER\n", 5* sizeof(char)) == 0) {
                qid =strtol(toks[1], NULL, 10);

                if(qid <= topics->lines_used) {
                /* awtes iptes portTes */
                    /* awtes = calloc(strlen(topics->lines[qui-1] + 2), sizeof(char)); */
                    /* if(awtes == NULL) exit(EXIT_FAILURE); */
                    strncat(awtes, topics->lines[qid-1], SIZE_AWTES);
                    strncat(awtes, "\n", SIZE_AWTES);
                    ret=sendto(fd, awtes, strlen(topics->lines[qid -1]) *sizeof(char),0,(struct sockaddr*)&clientaddr,addrlen);
                    strncpy(awtes, "AWTES ", 7 * sizeof(char)); /* reset it to the original string */
                    /* free(awtes); awtes = NULL; */

                }
                else
                    ret=sendto(fd,"EOF\n", 4*sizeof(char),0,(struct sockaddr*)&clientaddr,addrlen);
                if(ret==-1)exit(1);

            }

            /* TES - ECP */
            /* IQR */
            if(strncmp(toks[0], "IQR\n", 5*sizeof(char)) &&
                /* verify that SID is a 5 digit number */
                (strlen(toks[1]) == 5) &&
                /* verify that QID has at most 24 characters*/
                (strlen(toks[2]) <= QID_SZ) &&
                /* verify that topic name is valid */
                (checkIfTopicExists(toks[3], topics) == 0) &&
                /* verify that 0 <= score <= 100 */
                (atoi(toks[4]) >= 0) && (atoi(toks[4]) <= 100)) {

                strncat(&(awi[5]), toks[2], SIZE_AWI); /* starts at the 5th char */
                awi[strlen(awi)] = '\n';
                ret = sendto(fd, awi, SIZE_AWI * sizeof(char), 0,(struct sockaddr*)&clientaddr,addrlen);
                strncpy(awi, "AWI ", 5 * sizeof(char)); /* reset it to the original string */
                /* actually save the score */
                saveScore(&scores, &submissions, toks[1], toks[3], toks[4]);
            }
            else {
                ret = sendto(fd, "ERR\n", 4 * sizeof(char), 0,(struct sockaddr*)&clientaddr,addrlen);
            }
            if(ret==-1)exit(1);
            memset(buffer, 0, BUFFER_SIZE);

        }
        else {
            /* Parent process */
            do
                ret = close(fd);
            while((ret == -1) && (errno == EINTR));

            if(ret == -1)
                exit(EXIT_FAILURE);
        }
    }

    //close(fd);
    //exit(EXIT_SUCCESS);
}
