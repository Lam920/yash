#include "parsecmd.h"

char delimiter[DELIMITER_NUM][DELIMITER_MAXLENGTH] = {"|", "2>", "<", ">", "&"};

static int getcmd_type(char delimiter[DELIMITER_NUM][DELIMITER_MAXLENGTH], char *input, int delimeter_num)
{
    int i = 0, type_idx = 99, type_cmd = EXEC;
    for (i = 0; i < delimeter_num; i++)
    {
        if (strstr(input, delimiter[i]))
        {
            type_idx = i;
            //printf("Get delimeter: %s\n", delimiter[i]);
            break;
        }
    }

    //printf("Get type_id: %d\n", type_idx);

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

void do_execcmd(struct exec_cmd *ecmd, pid_t pid){
    char *token;
    char delimiter[] = " ";
    int status;

    ecmd->argc = 0;
    token = strtok(ecmd->cmd_exec, delimiter);
    ecmd->argv[0] = token;
    ecmd->argc++;

    while(token){
        token = strtok(NULL, delimiter);
        ecmd->argv[ecmd->argc] = token;
        ecmd->argc++;
    }

    ecmd->argc -= 1;

    ecmd->argv[ecmd->argc] = NULL;

        //fork for child to execute command

        pid = fork();
        if (pid == -1){
            printf("can't fork, error occurred\n");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0){
            //child execute command
            execvp(ecmd->argv[0], ecmd->argv);
            exit(0);
        }

        if (waitpid(pid, &status, 0) > 0) {
             
            if (WIFEXITED(status) && !WEXITSTATUS(status)){} 
            else if (WIFEXITED(status) && WEXITSTATUS(status)) {
                if (WEXITSTATUS(status) == 127) {
 
                    // execv failed
                    printf("execv failed\n");
                }
                else
                    printf("program terminated normally,"
                    " but returned a non-zero status\n");             
            }
            else
            printf("program didn't terminate normally\n");         
        } 
        else {
            printf("waitpid() failed\n");
        }
        //free execute cmd after execute
        free(ecmd);

}


int runcmd(char *user_input, pid_t pid)
{
    printf("Start to getcmd_type\n");
    int type_cmd = getcmd_type(delimiter, user_input, DELIMITER_NUM);
    struct cmd *cmd = init_cmd(type_cmd);
    struct exec_cmd *ecmd;
    struct file_cmd *fcmd;
    struct pipe_cmd *pcmp;

    switch (type_cmd)
    {
    case EXEC:
        ecmd = (struct exec_cmd*)cmd;
        memcpy(ecmd->cmd_exec, user_input, strlen(user_input));
        //printf("Command to exec: %s\n", ecmd->cmd_exec);
        do_execcmd(ecmd, pid);
        break;
    default:
        break;
    }
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
    struct pipe_cmd *pipe_cmd = (struct pipe_cmd *)malloc(sizeof(struct pipe_cmd));
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