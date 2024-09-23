#include "parsecmd.h"

char delimiter[DELIMITER_NUM][DELIMITER_MAXLENGTH] = {"|", "2>", "<", ">", "&"};

static int getcmd_type(char delimiter[DELIMITER_NUM][DELIMITER_MAXLENGTH], char *input, int delimeter_num)
{
    int i = 0, type_idx, type_cmd;
    for (i = 0; i < delimeter_num; i++)
    {
        if (strstr(input, delimiter[i]))
        {
            type_idx = i;
            printf("Get delimeter: %s\n", delimiter[i]);
            break;
        }
    }

    switch (type_idx)
    {
    case 0:
        type_cmd = PIPE;
        break;
    case 1:
    case 2:
    case 3:
        type_cmd = FILE_REDIRECTION;
        break;
    case 4:
        type_cmd = BACKGROUND;
        break;
    default:
        type_cmd = EXEC;
        break;
    }

    return type_cmd;
}


int parsercmd(char *input_cmd)
{
    int type_cmd = getcmd_type(delimiter, input_cmd, DELIMITER_NUM);
    struct cmd *cmd = init_cmd(type_cmd);
    struct exec_cmd *ecmd;
    struct file_cmd *fcmd;
    struct pipe_cmd *pcmp;

    switch (type_cmd)
    {
    case EXEC:
        ecmd = (struct exec_cmd*)cmd;
        do_execcmd(ecmd);
        break;
    default:
        break;
    }
}

void do_execcmd(struct exec_cmd *ecmd){
    char binexec[MAXLENGTH_CMD] = {0};

}

struct cmd* init_execcmd()
{
    struct exec_cmd *exec_cmd = (struct exec_cmd *)malloc(sizeof(struct exec_cmd));
    
    exec_cmd->type = EXEC;
    return (struct cmd*)exec_cmd;
}

struct cmd* init_filecmd()
{
    struct file_cmd *file_cmd = (struct file_cmd *)malloc(sizeof(struct file_cmd));
    memset(file_cmd, 0, sizeof(struct file_cmd));
    file_cmd->type = FILE_REDIRECTION;
    return (struct cmd*)file_cmd;
}

struct cmd* init_pipecmd()
{
    struct pipe_cmd *pipe_cmd = (struct file_cmd *)malloc(sizeof(struct pipe_cmd));
    memset(pipe_cmd, 0, sizeof(struct pipe_cmd));
    pipe_cmd->type = FILE_REDIRECTION;
    return (struct cmd*)pipe_cmd;
}

struct cmd* init_cmd(int type){
    struct cmd *run_cmd = NULL;
    switch (type)
    {
    case EXEC:
        printf("Init exec cmd\n");
        run_cmd = init_execcmd();
        break;
    case FILE_REDIRECTION:
        printf("Init file redirection cmd\n");
        run_cmd = init_filecmd();
        break;
    case PIPE:
        printf("Init pipe cmd\n");
        run_cmd = init_pipecmd();
        break;
    default:
        printf("Init nothing cmd\n");
        break;
    }

    return run_cmd;
}