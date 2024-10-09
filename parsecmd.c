#include "parsecmd.h"
#include "util.h"

#define FILECMD_DELI_START_INDEX 1

char *delimiter[] = {"|", "2>", "<", ">"};

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

void update_background_process(pid_t ret_child_pid, int status, char *cmd)
{
    if (WSTOPSIG(status) == SIGTSTP)
    {
#ifdef DEBUG
        printf("Caught SIGTSTP\n");
#endif
        struct background_process *bgprocess_info;
        bgprocess_info = (struct background_process *)malloc(sizeof(struct background_process));
        bgprocess_info->pid = ret_child_pid;
        bgprocess_info->pgid = ret_child_pid;
        bgprocess_info->status = STOPPED;
        strncpy(bgprocess_info->cmd, cmd, strlen(cmd));
        ll_add_front(background_process_list, (struct background_process *)bgprocess_info);
    }
}

void add_background_process(pid_t pid, pid_t pgid, int status, char *cmd)
{
    struct background_process *bgprocess_info;
    bgprocess_info = (struct background_process *)malloc(sizeof(struct background_process));
    memset(bgprocess_info, 0, sizeof(struct background_process));
    bgprocess_info->pid = pid;
    bgprocess_info->pgid = pgid;
    bgprocess_info->status = status;
    strncpy(bgprocess_info->cmd, cmd, strlen(cmd));
    ll_add_front(background_process_list, (struct background_process *)bgprocess_info);
    // printf("Now we have %d background process\n", ll_length(background_process_list));
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

static void parse_filecmd(struct file_cmd *fcmd, int delimiter_idx)
{
    char *token;
    char *delimiter_file = delimiter[delimiter_idx];
    // printf("delimiter file: %s\n", delimiter_file);
    char *cmd_exec = NULL;
    token = strtok(fcmd->cmd_file, delimiter_file);
    cmd_exec = token;
    memcpy(fcmd->cmd_exec, cmd_exec, strlen(cmd_exec));
    while (token)
    {
        // printf("file token: %s\n", token);
        memset(fcmd->file_name, 0, MAXLENGTH_FILENAME);
        strncpy(fcmd->file_name, token, strlen(token));
        token = strtok(NULL, delimiter_file);
    }
    // get file decriptor to redirect
    fcmd->fd = get_std_redirect(delimiter_idx);
    remove_spaces(fcmd->file_name);
}

void do_execcmd(struct exec_cmd *ecmd, pid_t pid, int cmd_type, int background)
{
    // printf("Do exec cmd\n");
    int status, do_fg_bg = 0, wait_flag = WUNTRACED;
    parse_execcmd(ecmd);

    if (!strcmp("fg", ecmd->cmd_exec))
    {
        do_fg_bg = 1;
    }
    else if (!strcmp("bg", ecmd->cmd_exec))
    {
        do_fg_bg = 2;
    }

    if (do_fg_bg)
    {
        if (ll_length(background_process_list) < 1)
        {
            printf("No background process is running...\n");
            return;
        }
    }
    // fork for child to execute command
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

        if (!strcmp("jobs", ecmd->cmd_exec))
        {
            print_bgprocess_list(background_process_list);
            exit(0);
        }
        else if (do_fg_bg != 0)
        {
            // printf("Do foreground/background job\n");
            int status;
            ll_node_t *fg_node = ll_front(background_process_list);
            fg_process_info = (struct background_process *)fg_node->object;
            //   printf("Now foreground/background job will execute cmd: %s  with pid: %d\n", fg_process_info->cmd, fg_process_info->pid);
            //   Send SIGCONT to continue execute and WAIT
            kill(-fg_process_info->pgid, SIGCONT);
            exit(0);
        }

        execvp(ecmd->argv[0], ecmd->argv);
        exit(0);
    }

    if (background)
    {
        add_background_process(pid, pid, RUNNING, ecmd->cmd_exec);
    }

    pid_t process_control_terminal_id;
    // struct background_process *fg_process_info;

    if (do_fg_bg == 0)
    {
        process_control_terminal_id = pid;
        setpgid(pid, pid);
        if (background)
            wait_flag = WNOHANG;
        else
            wait_flag = WUNTRACED | WCONTINUED;
    }
    else
    {
        ll_node_t *fg_node = ll_front(background_process_list);
        fg_process_info = (struct background_process *)fg_node->object;
        process_control_terminal_id = fg_process_info->pgid;
        // if run fg: Remove process running in foreground from list of background process.
        if (do_fg_bg == 1)
            ll_remove(background_process_list, fg_node);
        /* Do not used SIGCONT because we send SIGCONT to process control terminal for it to continue running in foreground */
        if (do_fg_bg == 1)
            wait_flag = WUNTRACED;
        else
        {
            wait_flag = WNOHANG;
        }
        fg_process_info->status = RUNNING;
    }
    tcsetpgrp(0, process_control_terminal_id);
    pid_t ret_child_pid = waitpid(-process_control_terminal_id, &status, wait_flag);
    tcsetpgrp(0, getpid());
    // printf("[%s-%d]Process %d is stop by signal: %d\n", __FUNCTION__, __LINE__, ret_child_pid, WSTOPSIG(status));
    if (do_fg_bg)
    {
        strncpy(ecmd->cmd_exec, fg_process_info->cmd, strlen(fg_process_info->cmd));
    }

    /* If receive SIGTSTP ==> Update background process */
    update_background_process(ret_child_pid, status, ecmd->cmd_exec);
    // printf("After terminal is control of process group: %d\n", tcgetpgrp(0));

    //  free execute cmd after execute
    free(ecmd);
}

void do_filecmd(struct file_cmd *fcmd, pid_t pid, int delimiter_idx, int background)
{
    // printf("Enter do_filecmd with delimiter_idx: %d\n", delimiter_idx);

    int status, wait_flag;

    if (background)
    {
        wait_flag = WNOHANG;
    }
    else
    {
        wait_flag = WCONTINUED | WUNTRACED;
    }

    // We need a cmd_exec to exec before redirect
    parse_filecmd(fcmd, delimiter_idx);

    struct exec_cmd *ecmd = (struct exec_cmd *)init_cmd(EXEC);
    memcpy(ecmd->cmd_exec, fcmd->cmd_exec, strlen(fcmd->cmd_exec));

    pid = fork();
    if (pid == -1)
    {
        printf("can't fork, error occurred\n");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        // close stdin/stdout/stderr base on redirection parameter
        setpgid(0, 0);
        close(fcmd->fd);
        int fd = open(fcmd->file_name, O_CREAT | O_RDWR, 0666);
        parse_execcmd(ecmd);
        execvp(ecmd->argv[0], ecmd->argv);
        exit(0);
    }
    setpgid(pid, pid); // set pgid for child process
    tcsetpgrp(0, pid);
    if (background)
    {
        add_background_process(pid, pid, RUNNING, ecmd->cmd_exec);
    }
    // check_process_status(pid, status);
    pid_t ret_child_pid = waitpid(-pid, &status, wait_flag);
    tcsetpgrp(0, getpid());
    free(ecmd);
    free(fcmd);
}

void do_pipecmd(struct pipe_cmd *pcmd, pid_t pid, int delimiter_idx, int background)
{
    char *token;
    char *delimiter_pipe = "|";
    char *cmd_exec = NULL;
    int status, wait_flag;

    if (background)
    {
        wait_flag = WNOHANG;
    }
    else
    {
        wait_flag = WCONTINUED | WUNTRACED;
    }

    pid_t cpid_left, cpid_right;
    int pipefd[2];
    int type_cmd_left = 0, type_cmd_right = 0;
    int delimiter_left_idx = 0, delimiter_right_idx = 0;

    pid_t parent_pid = getpid();
    pid_t parent_pgid = getpgrp();

    /* Left and right cmd can be fcmd or ecmd */
    struct exec_cmd *ecmd_left = (struct exec_cmd *)init_cmd(EXEC);
    struct exec_cmd *ecmd_right = (struct exec_cmd *)init_cmd(EXEC);
    struct file_cmd *fcmd_left = (struct file_cmd *)init_cmd(FILE_REDIRECTION);
    struct file_cmd *fcmd_right = (struct file_cmd *)init_cmd(FILE_REDIRECTION);

    char cmd_pipe_back[MAXLENGTH_CMD] = {0};
    strncpy(cmd_pipe_back, pcmd->cmd_pipe, strlen(pcmd->cmd_pipe));
    token = strtok(pcmd->cmd_pipe, delimiter_pipe);
    // printf("first cmd: %s\n", token);
    /* Check whether left cmd is EXEC/FILE_REDIRECTION */
    type_cmd_left = getcmd_type(delimiter, token, DELIMITER_NUM, &delimiter_left_idx);
    type_cmd_right = EXEC;
    // printf("Hello\n");
    if (type_cmd_left == FILE_REDIRECTION)
    {
        strncpy(fcmd_left->cmd_file, token, strlen(token));
        parse_filecmd(fcmd_left, delimiter_left_idx);
        memcpy(ecmd_left->cmd_exec, fcmd_left->cmd_exec, strlen(fcmd_left->cmd_exec));
        // parse_execcmd(ecmd_left);
    }
    else
    {
        strncpy(ecmd_left->cmd_exec, token, strlen(token));
        // parse_execcmd(ecmd_left);
    }

    // printf("hi\n");

    /* Check whether right cmd is EXEC/FILE_REDIRECTION */
    while (token)
    {
        token = strtok(NULL, delimiter_pipe);
        // printf("Second cmd: %s\n", token);
        type_cmd_right = getcmd_type(delimiter, token, DELIMITER_NUM, &delimiter_right_idx);
        // type_cmd_right = EXEC;
        if (type_cmd_right == FILE_REDIRECTION)
        {
            strncpy(fcmd_right->cmd_file, token, strlen(token));
            // printf("Now parse filecmd2\n");
            parse_filecmd(fcmd_right, delimiter_right_idx);
            // printf("Done parse\n");
            memcpy(ecmd_right->cmd_exec, fcmd_right->cmd_exec, strlen(fcmd_right->cmd_exec));
            // parse_execcmd(ecmd_right);
        }
        else
        {
            strncpy(ecmd_right->cmd_exec, token, strlen(token));
            // parse_execcmd(ecmd_right);
        }
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
        if (type_cmd_left == FILE_REDIRECTION)
        {
            dup2(fcmd_left->fd, pipefd[1]);
        }
        // close pipe unused
        close(pipefd[0]);
        close(pipefd[1]);
        parse_execcmd(ecmd_left);
        execvp(ecmd_left->argv[0], ecmd_left->argv);
        exit(0);
    }
    else
    {
        setpgid(cpid_left, cpid_left);
        cpid_right = fork();
        if (cpid_right == 0)
        {
            setpgid(0, cpid_left);
            //   get stdin from read end of pipe
            dup2(pipefd[0], STDIN_FILENO);
            if (type_cmd_right == FILE_REDIRECTION)
            {
                // printf("fcmd_right fd to replace: %d\n", fcmd_right->fd);
                int fd = open(fcmd_right->file_name, O_CREAT | O_RDWR, 0666);
                dup2(fd, fcmd_right->fd);
            }
            // close pipe unused
            close(pipefd[1]);
            close(pipefd[0]);
            parse_execcmd(ecmd_right);
            execvp(ecmd_right->argv[0], ecmd_right->argv);
        }
        else
        {
            setpgid(cpid_right, cpid_left);

            if (background)
            {
                add_background_process(cpid_left, cpid_left, RUNNING, cmd_pipe_back);
            }

            tcsetpgrp(0, cpid_left);
            // always close unused pipe for prevent hanging
            close(pipefd[1]);
            close(pipefd[0]);
            pid_t first_end = waitpid(-cpid_left, &status, wait_flag);
            // printf("Process %d exit with status: %d\n", first_end, status);
            //  waitpid(-cpid_left, (int *)NULL, WUNTRACED | WCONTINUED);
            pid_t second_end = waitpid(-cpid_left, &status, wait_flag);
            // printf("Process2 %d exit with status: %d and WEXITSTATUS: %d\n", second_end, status, WEXITSTATUS(status));
            // printf("**Update background process list with pcmd: %s\n", cmd_pipe_back);
            update_background_process(cpid_left, status, cmd_pipe_back);
            tcsetpgrp(0, parent_pgid);
            // printf("After terminal is control of process group: %d\n", tcgetpgrp(0));
            free(ecmd_left);
            free(ecmd_right);
            free(fcmd_left);
            free(fcmd_right);
        }
    }
}

void do_backcmd(struct back_cmd *bcmd, pid_t pid)
{
    char *token;
    char *delimiter_back = "&";
    struct exec_cmd *back_cmd = (struct exec_cmd *)init_cmd(EXEC);
    int status, delimiter_idx = 0;
    token = strtok(bcmd->cmd_back, delimiter_back);

    int type_cmd = getcmd_type(delimiter, token, DELIMITER_NUM, &delimiter_idx);
    struct cmd *cmd = init_cmd(type_cmd);
    struct exec_cmd *ecmd;
    struct pipe_cmd *pcmd;
    switch (type_cmd)
    {
    case EXEC:
        ecmd = (struct exec_cmd *)cmd;
        // printf("User input before exec: %s\n", user_input);
        strncpy(ecmd->cmd_exec, token, strlen(token));
        do_execcmd(ecmd, pid, EXEC, 1);
        break;
    case PIPE:
        pcmd = (struct pipe_cmd *)cmd;
        memcpy(pcmd->cmd_pipe, token, strlen(token));
        do_pipecmd(pcmd, pid, delimiter_idx, 1);
        break;
    default:
        break;
    }
}

int runcmd(char *user_input, pid_t pid)
{
    int background, delimiter_idx = 0;
    char last_char = getLastNonSpaceChar(user_input);
    if (last_char == '&')
        background = 1;
    else
        background = 0;

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
        do_execcmd(ecmd, pid, EXEC, background);
        break;
    case FILE_REDIRECTION:
        fcmd = (struct file_cmd *)cmd;
        memcpy(fcmd->cmd_file, user_input, strlen(user_input));
        do_filecmd(fcmd, pid, delimiter_idx, background);
        break;
    case PIPE:
        pcmd = (struct pipe_cmd *)cmd;
        memcpy(pcmd->cmd_pipe, user_input, strlen(user_input));
        do_pipecmd(pcmd, pid, delimiter_idx, background);
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