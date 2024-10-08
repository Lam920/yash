#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h> 
#include <errno.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define _GNU_SOURCE
void remove_spaces(char* s);

int check_process_status(pid_t pid, int status);

int get_std_redirect(int delimiter_idx);
char getLastNonSpaceChar(char *str);