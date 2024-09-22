Feature must implement:
- File redirection:
    + with creation of files if they don't exist for output redirection
    + fail command if input redirection (a file) is not exist. Ex: ls < file.txt ==> failed if file.txt is not exist
    + <, >, 2>

- Piping:
    + "|" seperates two command 
    + Children within the same pipeline will be in one process group for the pipeline
    + Children within the same pipeline will be started and stopped simultaneously
    + Only one "|" for simplicity

- Signal(SIGINT, SIGTSTP, SIGCHLD):
    + Ctrl-c: quit current foreground process
    + Ctrl-z: send SIGTSTP to the current foreground process

- Job control:
    + Background jobs using "&"
    + "fg": send SIGCONT to the most recent background or stopped process, print the process to stdout, and wait for completion
    + "bg": send SIGCONT to the most recent stopped process, print the process to stdout, and not wait for completion
    + Terminated background jobs will be printed after the "\n" on stdin with a Done in place of the Stopped or Running
    + A Job is all childen within one pipeline as defined above

- Misc: 
    + Children must inherit the enviroment from the parent
    + MUST search the PATH environment variable for every executable: Must search cmd in PATH directory to check it exist or not.
    + All child proccess will be dead on exist
    + Print "#" before accepting user input

Restriction:
- Each line contains one command or two commands in one pipe. Ex: ls | grep "hi" | grep "hello" ==> Invalid
- Lines will not exceed 2000 chars
- All char will be ASCII
- Ctrl-d will exit the shell


