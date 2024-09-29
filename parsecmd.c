#include "parsecmd.h"
#include "util.h"

#define FILECMD_DELI_START_INDEX 1

char *delimiter[] = {"|", "2>", "<", ">", "&"};

static int getcmd_type(char *delimiter[], char *input, int delimeter_num, int *delimiter_idx)
{
    int i = 0, type_idx = 99, type_cmd = EXEC;
    for (i = 0; i < delimeter_num; i++)
    {
        if (strstr(input, delimiter[i]))
        {
            type_idx = i;
            // printf("Get delimeter: %s\n", delimiter[i]);
            *delimiter_idx = type_idx;
            break;
        }
    }
    // printf("Get type_id: %d\n", type_idx);
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

static void parse_execcmd(struct exec_cmd *ecmd)
{
    char *token;
    char delimiter[] = " ";

    ecmd->argc = 0;
    token = strtok(ecmd->cmd_exec, delimiter);
    ecmd->argv[0] = token;
    ecmd->argc++;

    while (token)
    {
        token = strtok(NULL, delimiter);
        ecmd->argv[ecmd->argc] = token;
        ecmd->argc++;
    }

    ecmd->argc -= 1;
    ecmd->argv[ecmd->argc] = NULL;
}

void do_execcmd(struct exec_cmd *ecmd, pid_t pid, int cmd_type)
{
    int status;
    parse_execcmd(ecmd);
    // fork for child to execute command
    // printf("Enter do_execcmd\n");
    pid = fork();
    if (pid == -1)
    {
        printf("can't fork, error occurred\n");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        // child execute command
        execvp(ecmd->argv[0], ecmd->argv);
        exit(0);
    }

    check_process_status(pid, status);
    // free execute cmd after execute
    free(ecmd);
}

void do_filecmd(struct file_cmd *fcmd, pid_t pid, int delimiter_idx)
{
    printf("Enter do_filecmd with delimiter_idx: %d\n", delimiter_idx);
    char *token;
    char *delimiter_file = delimiter[delimiter_idx];
    printf("helimiter file: %s\n", delimiter_file);
    char *cmd_exec = NULL;
    int status;

    // We need a cmd_exec to exec before redirect
    struct exec_cmd *ecmd = (struct exec_cmd *)init_cmd(EXEC);

    token = strtok(fcmd->cmd_file, delimiter_file);
    cmd_exec = token;

    while (token)
    {
        printf("file token: %s\n", token);
        memset(fcmd->file_name, 0, MAXLENGTH_FILENAME);
        strncpy(fcmd->file_name, token, strlen(token));
        token = strtok(NULL, delimiter_file);
    }

    // get file decriptor to redirect
    fcmd->fd = get_std_redirect(delimiter_idx);

    printf("Get file to redirect: %s\n", fcmd->file_name);
    remove_spaces(fcmd->file_name);

    memcpy(ecmd->cmd_exec, cmd_exec, strlen(cmd_exec));

    pid = fork();
    if (pid == -1)
    {
        printf("can't fork, error occurred\n");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {

        int std_fno = dup(fcmd->fd);
        close(fcmd->fd);
        int fd = open(fcmd->file_name, O_CREAT | O_RDWR, 0666);
        parse_execcmd(ecmd);
        execvp(ecmd->argv[0], ecmd->argv);
        exit(0);
    }
    check_process_status(pid, status);

    free(ecmd);
    free(fcmd);
}

void do_pipecmd(struct pipe_cmd *pcmd, pid_t pid)
{
    char *token;
    char *delimiter = "|";
    char *cmd_exec = NULL;
    int status;

    pid_t cpid_left;
    pid_t cpid_right;
    int pipefd[2];

    pid_t parent_pid = getpid();

    struct exec_cmd *ecmd_left = (struct exec_cmd *)init_cmd(EXEC);
    struct exec_cmd *ecmd_right = (struct exec_cmd *)init_cmd(EXEC);

    token = strtok(pcmd->cmd_pipe, delimiter);
    strncpy(ecmd_left->cmd_exec, token, strlen(token));

    while (token)
    {
        token = strtok(NULL, delimiter);
        strncpy(ecmd_right->cmd_exec, token, strlen(token));
        break;
    }

    // create pipe
    if (pipe(pipefd) < 0)
    {
        printf("Cannot create pipe\n");
        exit(1);
    }

    cpid_left = fork();
    if (cpid_left == 0)
    {
        setpgid(0, 0);
        // printf("Get process group of left_child: %d\n", getpgrp());
        dup2(pipefd[1], STDOUT_FILENO);
        // close pipe unused
        close(pipefd[0]);
        close(pipefd[1]);
        parse_execcmd(ecmd_left);
        execvp(ecmd_left->argv[0], ecmd_left->argv);
        exit(0);
    }
    else
    {
        // waitpid(cpid_left, (int*)NULL, 0);
        cpid_right = fork();
        if (cpid_right == 0)
        {
            setpgid(0, cpid_left);
            // printf("Get process group of right_child: %d\n", getpgrp());
            //  get stdin from read end of pipe
            dup2(pipefd[0], STDIN_FILENO);
            // close pipe unused
            close(pipefd[1]);
            close(pipefd[0]);
            parse_execcmd(ecmd_right);
            execvp(ecmd_right->argv[0], ecmd_right->argv);
        }
        else
        {
            setpgid(0, cpid_left);
            printf("Get parent process id: %d\n", getpid());
            printf("Get process group of parent: %d\n", getpgrp());
            printf("Process control terminal: %d\n", tcgetpgrp(0));
            tcsetpgrp(0, cpid_left);
            //  printf("Now terminal is control of process group: %d\n", tcgetpgrp(0));
            //   always close unused pipe for prevent hanging
            close(pipefd[1]);
            close(pipefd[0]);
            waitpid(-cpid_left, (int *)NULL, WUNTRACED | WCONTINUED);
            waitpid(-cpid_left, (int *)NULL, WUNTRACED | WCONTINUED);
            // wait(NULL);
            // wait(NULL);
            tcsetpgrp(0, 0);
        }
    }
}

int runcmd(char *user_input, pid_t pid)
{
    int delimiter_idx = 0;
    int type_cmd = getcmd_type(delimiter, user_input, DELIMITER_NUM, &delimiter_idx);
    struct cmd *cmd = init_cmd(type_cmd);
    struct exec_cmd *ecmd;
    struct file_cmd *fcmd;
    struct pipe_cmd *pcmd;

    switch (type_cmd)
    {
    case EXEC:
        ecmd = (struct exec_cmd *)cmd;
        // printf("User input before exec: %s\n", user_input);
        strncpy(ecmd->cmd_exec, user_input, strlen(user_input));
        // printf("Command to exec: %s\n", ecmd->cmd_exec);
        do_execcmd(ecmd, pid, EXEC);
        break;
    case FILE_REDIRECTION:
        fcmd = (struct file_cmd *)cmd;
        memcpy(fcmd->cmd_file, user_input, strlen(user_input));
        do_filecmd(fcmd, pid, delimiter_idx);
        break;
    case PIPE:
        pcmd = (struct pipe_cmd *)cmd;
        memcpy(pcmd->cmd_pipe, user_input, strlen(user_input));
        do_pipecmd(pcmd, pid);
    default:
        break;
    }
    // free(cmd);
}

struct cmd *init_execcmd()
{
    struct exec_cmd *exec_cmd = (struct exec_cmd *)malloc(sizeof(struct exec_cmd));
    memset(exec_cmd, 0, sizeof(struct exec_cmd));
    exec_cmd->type = EXEC;
    return (struct cmd *)exec_cmd;
}

struct cmd *init_filecmd()
{
    struct file_cmd *file_cmd = (struct file_cmd *)malloc(sizeof(struct file_cmd));
    memset(file_cmd, 0, sizeof(struct file_cmd));
    file_cmd->type = FILE_REDIRECTION;
    return (struct cmd *)file_cmd;
}

struct cmd *init_pipecmd()
{
    struct pipe_cmd *pipe_cmd = (struct pipe_cmd *)malloc(sizeof(struct pipe_cmd));
    memset(pipe_cmd, 0, sizeof(struct pipe_cmd));
    pipe_cmd->type = FILE_REDIRECTION;
    return (struct cmd *)pipe_cmd;
}

struct cmd *init_cmd(int type)
{
    struct cmd *run_cmd = NULL;
    switch (type)
    {
    case EXEC:
        // printf("Init exec cmd\n");
        run_cmd = init_execcmd();
        break;
    case FILE_REDIRECTION:
        // printf("Init file redirection cmd\n");
        run_cmd = init_filecmd();
        break;
    case PIPE:
        // printf("Init pipe cmd\n");
        run_cmd = init_pipecmd();
        break;
    default:
        // printf("Init nothing cmd\n");
        break;
    }

    return run_cmd;
}