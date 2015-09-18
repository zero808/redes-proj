#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "constants.h"

int cmd_manager(int index, char **argv);
int action_selector();
int parseString(char *line, char ***argv);

#endif


