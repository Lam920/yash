#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "util.h"

#define EXEC 1
#define FILE_REDIRECTION 2
#define PIPE 3
#define BACKGROUND 4
#define DELIMITER_NUM 5
#define DELIMITER_MAXLENGTH 3
#define MAXLENGTH_CMD 2000
#define MAX_ARGC 10
#define MAXLENGTH_FILENAME 64

#define WCONTINUED 8 /* Report continued child.  */

#define NUM_OF_PIPE 1

int runcmd(char *user_input, pid_t pid);

struct cmd *init_cmd(int type_cmd);

struct cmd
{
    int type;
};

typedef struct cmd cmd;

struct exec_cmd
{
    int type;
    char cmd_exec[MAXLENGTH_CMD];
    char *argv[MAX_ARGC];
    char *eargv[MAX_ARGC];
    int argc;
};

struct file_cmd
{
    int type;
    char cmd_file[MAXLENGTH_CMD];
    char file_name[MAXLENGTH_FILENAME];
    int fd;          // stdin, stdout, stderr
    struct cmd *cmd; // point to cmd to execute
};

struct pipe_cmd
{
    int type;
    char cmd_pipe[MAXLENGTH_CMD];
    struct cmd *cmd_left;
    struct cmd *cmd_right;
};

typedef struct file_cmd file_cmd_t;
typedef struct pipe_cmd pipe_cmd_t;
typedef struct exec_cmd exec_cmd_t;