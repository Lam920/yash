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

void update_background_process(pid_t ret_child_pid, int status, struct exec_cmd *ecmd){
    if (WSTOPSIG(status) == SIGTSTP){
        struct background_process *bgprocess_info;
        bgprocess_info = (struct background_process *)malloc(sizeof(struct background_process));
        bgprocess_info->pid = ret_child_pid;
        bgprocess_info->pgid = ret_child_pid;
        bgprocess_info->status = STOPPED;
        strncpy(bgprocess_info->cmd, ecmd->cmd_exec, strlen(ecmd->cmd_exec));
        ll_add_front(background_process_list, (struct background_process *)bgprocess_info);
    }
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
    //printf("Do exec cmd\n");
    int status;
    parse_execcmd(ecmd);
    // fork for child to execute command
    // printf("Enter do_execcmd\n");
    pid_t parent_pid = getpid();
    pid = fork();
    if (pid == -1)
    {
        printf("can't fork, error occurred\n");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        // child execute command
        setpgid(0, 0);

        if (!strcmp("jobs", ecmd->cmd_exec)){
            print_bgprocess_list(background_process_list);
            exit(0);
        }
        else if (!strcmp("fg", ecmd->cmd_exec)){
            printf("Do foreground job\n");
            int status;
            ll_node_t *fg_node = ll_front(background_process_list);
            struct background_process *fg_process_info = (struct background_process *)fg_node->object;
            printf("Now foreground job will execute cmd: %s\n", fg_process_info->cmd);
            // Send SIGCONT to continue execute and WAIT
            ll_remove(background_process_list, fg_node);
            printf("Before send signal: %d\n", tcgetpgrp(0));
            tcsetpgrp(0, fg_process_info->pgid);
            printf("After send signal: %d\n", tcgetpgrp(0));
            kill(fg_process_info->pid, SIGCONT);
            //pid_t ret_child_pid = waitpid(fg_process_info->pid, &status, WCONTINUED | WUNTRACED);
            //tcsetpgrp(0, parent_pid);
            //printf("Later send signal: %d\n", tcgetpgrp(0));
            //printf("Process %d is stop/terminate by signal: %d\n",  ret_child_pid, WSTOPSIG(status));
        }

        execvp(ecmd->argv[0], ecmd->argv);
        exit(0);
    }
    setpgid(pid, pid);
    printf("Do cmd for pid: %d of parent: %d\n", pid, getpid());

    //printf("Process control terminal: %d\n", tcgetpgrp(0));
    
    tcsetpgrp(0, pid);
    //printf("Before terminal is control of process group: %d\n", tcgetpgrp(0));
    //check_process_status(pid, status);
    pid_t ret_child_pid = waitpid(pid, &status, WUNTRACED | WCONTINUED);
    tcsetpgrp(0, getpid());
    printf("Process %d is stop by signal: %d\n",  ret_child_pid, WSTOPSIG(status));
    update_background_process(ret_child_pid, status, ecmd);
    //printf("After terminal is control of process group: %d\n", tcgetpgrp(0));
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
        //close stdin/stdout/stderr base on redirection parameter
        setpgid(0, 0);
        close(fcmd->fd);
        int fd = open(fcmd->file_name, O_CREAT | O_RDWR, 0666);
        parse_execcmd(ecmd);
        execvp(ecmd->argv[0], ecmd->argv);
        exit(0);
    }
    setpgid(pid, pid); //set pgid for child process
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
    pid_t parent_pgid = getpgrp();

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
        printf("Get process group of left_child: %d\n", getpgrp());
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
        setpgid(cpid_left, cpid_left);
        cpid_right = fork();
        if (cpid_right == 0)
        {
            setpgid(0, cpid_left);
            printf("Get process group of right_child: %d\n", getpgrp());
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
            setpgid(cpid_right, cpid_left);
            printf("Get parent process id: %d\n", getpid());
            printf("Get process group of parent: %d\n", getpgrp());
            printf("Process control terminal: %d\n", tcgetpgrp(0));
            printf("Before terminal is control of process group: %d\n", tcgetpgrp(0));
            tcsetpgrp(0, cpid_left);
            printf("Now terminal is control of process group: %d\n", tcgetpgrp(0));
            //   always close unused pipe for prevent hanging
            close(pipefd[1]);
            close(pipefd[0]);
            int status;
            pid_t first_end = waitpid(-cpid_left, &status, WUNTRACED | WCONTINUED);
            printf("Process %d exit with status: %d\n", first_end, status);
            // waitpid(-cpid_left, (int *)NULL, WUNTRACED | WCONTINUED);
            pid_t second_end = waitpid(-cpid_left, (int *)NULL, WUNTRACED | WCONTINUED);
            printf("Process2 %d exit with status: %d and WEXITSTATUS: %d\n", second_end, status, WEXITSTATUS(status));
            tcsetpgrp(0, parent_pgid);
            printf("After terminal is control of process group: %d\n", tcgetpgrp(0));
        }
    }
}

void do_backcmd(struct back_cmd *bcmd, pid_t pid){
    char *token;
    char *delimiter = "&";
    struct exec_cmd *back_cmd = (struct exec_cmd *)init_cmd(EXEC);
    int status;
    token = strtok(bcmd->cmd_back, delimiter);

    strncpy(back_cmd->cmd_exec, token, strlen(token));

/* Add new background process to linkedlist */
    struct background_process *bgprocess_info;
    bgprocess_info = (struct background_process *)malloc(sizeof(struct background_process));
    memset(bgprocess_info, 0, sizeof(struct background_process));

    parse_execcmd(back_cmd);
    int pid_back = fork();
    if (pid_back == 0)
    {
        setpgid(0, pid_back);
        //printf("Get process group of background child: %d\n", getpgrp());
        execvp(back_cmd->argv[0], back_cmd->argv);
    }
    else {
        /* Fill background process info */
        bgprocess_info->pid = pid_back;
        bgprocess_info->pgid = pid_back;
        bgprocess_info->status = RUNNING;
        strncpy(bgprocess_info->cmd, back_cmd->cmd_exec, strlen(back_cmd->cmd_exec));
        // printf("Add process with cmd: %s to list\n", bgprocess_info->cmd);

        ll_node_t *bg_node = NULL;
        bg_node = ll_add_front(background_process_list, (struct background_process *)bgprocess_info);
        
        // if (bg_node != NULL){
        //     struct background_process *process_add_info = (struct background_process *)bg_node->object;
        //     printf("Add process to list {pid = %d}, {status=%d}, {cmd=%s}\n", process_add_info->pid, process_add_info->status, process_add_info->cmd);
        // }
        printf("Run background process for pid: %d\n", pid_back);

        unsigned int bgprocess_num = ll_length(background_process_list);
        printf("Now we have %d background process\n", bgprocess_num);

        //no need to wait for child to complete
        waitpid(pid_back, &status, WNOHANG);
        // waitpid(pid_back, &status, WCONTINUED | WUNTRACED);
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
    struct back_cmd *bcmd;

    switch (type_cmd)
    {
    case EXEC:
        ecmd = (struct exec_cmd *)cmd;
        // printf("User input before exec: %s\n", user_input);
        strncpy(ecmd->cmd_exec, user_input, strlen(user_input));
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
        break;
    case BACKGROUND:
        printf("Do background job\n");
        bcmd = (struct back_cmd *)cmd;
        memcpy(bcmd->cmd_back, user_input, strlen(user_input));
        do_backcmd(bcmd, pid);
        break;
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

struct cmd *init_backcmd()
{
    struct back_cmd *back_cmd = (struct back_cmd *)malloc(sizeof(struct back_cmd));
    memset(back_cmd, 0, sizeof(struct back_cmd));
    back_cmd->type = BACKGROUND;
    return (struct cmd *)back_cmd;
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
    case BACKGROUND:
        printf("Init background cmd\n");
        run_cmd = init_backcmd();
        break;
    default:
        // printf("Init nothing cmd\n");
        break;
    }

    return run_cmd;
}