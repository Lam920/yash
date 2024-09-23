#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define EXEC 1
#define FILE_REDIRECTION 2
#define PIPE 3
#define BACKGROUND 4
#define DELIMITER_NUM 5
#define DELIMITER_MAXLENGTH 3
#define MAXLENGTH_CMD 2000
#define MAX_ARGC 10

#define NUM_OF_PIPE 1

int parsercmd(char *input_cmd);

struct cmd* init_cmd(int type_cmd);

struct cmd {
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
    char cmd_exec[MAXLENGTH_CMD];
    char file_name[16];
    int fd; // stdin, stdout, stderr
};

struct pipe_cmd
{
    int type;
    char cmd[NUM_OF_PIPE + 1][MAXLENGTH_CMD / 2];
};

typedef struct file_cmd file_cmd_t;
typedef struct pipe_cmd pipe_cmd_t;
typedef struct exec_cmd exec_cmd_t;