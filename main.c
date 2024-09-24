#include <stdio.h>
#include <stdlib.h>
#include "parsecmd.h"


#define MAXLENGTH_CMD 2000

char user_input[MAXLENGTH_CMD];

int main()
{
    pid_t pid;
    while(1){
        memset(user_input, 0, MAXLENGTH_CMD);
        printf("# ");
        fgets(user_input, MAXLENGTH_CMD, stdin);
        if (!strcmp(user_input, "\n")) continue;
        user_input[strlen(user_input) - 1] = '\0'; 
        runcmd(user_input, pid);
    }
    return 0;
}
