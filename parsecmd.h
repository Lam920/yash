#define EXEC 1
#define FILE_REDIRECTION 2
#define PIPING 3
#define BACKGROUND 4
#define MAXLENGTH_CMD 2000

#define NUM_OF_PIPE 1

int paresecmd(char cmd[]);

int init_cmd(int type_cmd);

struct exec_cmd
{
    int type;
    char cmd_exec[MAXLENGTH_CMD];
    char **cmd_argv;
    int argc;
};

struct file_cmd
{
    int type;
    char file_name[16];
    char cmd_exec[MAXLENGTH_CMD];
    int fd; // stdin, stdout, stderr
};

struct pipe_cmd
{
    int type;
    char cmd[NUM_OF_PIPE + 1][MAXLENGTH_CMD / 2];
};

typedef struct file_cmd file_cmd;
typedef struct pipe_cmd pipe_cmd;
typedef struct exec_cmd exec_cmd;