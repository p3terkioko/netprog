#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void handle_error(const char *msg);
void parse_command(char *input, char **command, char **arg1, char **arg2);

#endif // UTILS_H