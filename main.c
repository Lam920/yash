#include <stdio.h>
#include <stdlib.h>
#include "parsecmd.h"

#define MAXLENGTH_CMD 2000

char user_input[MAXLENGTH_CMD];

void handle_sigtstp(int sig)
{
    printf("Caught signal TSTP with id: %d\n", sig);
}

int main()
{
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, handle_sigtstp);
    pid_t pid;
    while (1)
    {
        memset(user_input, 0, MAXLENGTH_CMD);
        printf("# ");
        fflush(stdin);
        fgets(user_input, MAXLENGTH_CMD, stdin);
        if (!strcmp(user_input, "\n"))
            continue;
        user_input[strlen(user_input) - 1] = '\0';
        // printf("Get user input: %s\n", user_input);
        runcmd(user_input, pid);
    }
    return 0;
}
