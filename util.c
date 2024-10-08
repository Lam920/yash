#include "util.h"
void remove_spaces(char* s) {
    char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
    } while (*s++ = *d++);
}

int check_process_status(pid_t pid, int status){
    if (waitpid(pid, &status, 0) > 0) {        
        if (WIFEXITED(status) && !WEXITSTATUS(status)){
            return 1;
        } 
        else if (WIFEXITED(status) && WEXITSTATUS(status)) {
            if (WEXITSTATUS(status) == 127) {
                // execv failed
                printf("execv failed\n");
            }
            else {
                printf("program terminated normally, but returned a non-zero status\n");        
            }
            return 0;          
        }
        else {
            printf("program didn't terminate normally\n");
            return 0;  
        }       
    } 
    else {
        printf("waitpid() failed\n");
        return 0;
    }
}

int get_std_redirect(int delimiter_idx){
    int fd;
    switch (delimiter_idx)
    {
    case 1:
        fd = STDERR_FILENO;
        break;
    case 2:
        fd = STDIN_FILENO;
        break;
    case 3:
        fd = STDOUT_FILENO;
    default:
        fd = STDOUT_FILENO;
        break;
    }
    return fd;
}

char getLastNonSpaceChar(char *str) {
    int i, length = strlen(str);
    char last_char;
    for (i = length - 1; i >= 0; i--) {
        if (str[i] != ' ') {
            last_char = str[i];
            if (str[i] == '&') str[i] = '\0';
            return last_char;
        }
    }
    return '\0';
}
