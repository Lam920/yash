#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "parsecmd.h"

#define MAXLENGTH_CMD 2000

char user_input[MAXLENGTH_CMD];
linked_list_t *background_process_list = NULL;

void handle_sigttou(int sig){
    printf("Caugh SIGTTOU\n");
}

//Dealing with Zombie process
void handle_sigchld(int sig)
{
    pid_t child_pid = 0;
    int status;
    printf("Caugh sigchld\n");
    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0){
        printf("Parent get SIGCHLD from child: %d\n", child_pid);
        //find pid and remove from linked list when it done
        ll_find_pid(background_process_list, child_pid);
        continue;
    }
}

void handle_sigtstp(int sig){
    // printf("Caught SIGTSTP from pid: %d\n", getpid());
    // raise(SIGSTOP);
}

static void handler(int sig){
    switch(sig){
        case SIGTSTP:
            printf("Caught SIGTSTP\n");
            handle_sigtstp(sig);
            break;
        case SIGCHLD:
            handle_sigchld(sig);
            break;
        default: 
            break;
    }
}

int main()
{
    signal(SIGTTOU, SIG_IGN);
    // signal(SIGTSTP, handle_sigtstp);
    // signal(SIGCHLD, handle_sigchld);
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handler;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
        return -1;
    if (sigaction(SIGTSTP, &sa, NULL) == -1)
        return -1;
    if (sigaction(SIGCONT, &sa, NULL) == -1)
        return -1;
    // if (sigaction(SIGQUIT, &sa, NULL) == -1)
    //     return -1;

    pid_t pid;
    printf("Create linked list for bgprocess\n");
    background_process_list = ll_create();
    while (1)
    {
        memset(user_input, 0, MAXLENGTH_CMD);
        printf("# ");
        fflush(stdin);
        fgets(user_input, MAXLENGTH_CMD, stdin);
        if (!strcmp(user_input, "\n"))
            continue;
        user_input[strlen(user_input) - 1] = '\0';
        runcmd(user_input, pid);
    }
    return 0;
}
