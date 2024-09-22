// C program to illustrate the use of strtok() in C
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LIMIT 1000
#define EXEC 1
#define FILE_REDIRECTION 2
#define PIPE 3
#define BACKGROUND 4
#define DELIMITER_NUM 5
#define DELIMITER_MAXLENGTH 3

int getcmd_type(char delimiter[DELIMITER_NUM][DELIMITER_MAXLENGTH], char *input, int delimeter_num)
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
        type_cmd = FILE_REDIRECTION;
        break;
    case 2:
        type_cmd = FILE_REDIRECTION;
        break;
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

int main()
{
    while (1)
    {
        char str[MAX_LIMIT];
        memset(str, 0, MAX_LIMIT);
        printf("Enter your cmd: # ");
        fgets(str, MAX_LIMIT, stdin);
        printf("%s", str);

        char delimiter[DELIMITER_NUM][DELIMITER_MAXLENGTH] = {"|", "2>", "<", ">", "&"};

        int cmd_type = getcmd_type(delimiter, str, DELIMITER_NUM);
        switch (cmd_type)
        {
        case EXEC:
            printf("Exec command\n");
            break;
        case PIPE:
            printf("Pipe command\n");
            break;
        case FILE_REDIRECTION:
            printf("File redirection command\n");
            break;
        case BACKGROUND:
            printf("Background command\n");
            break;
        default:
            break;
        }
    }
    return 0;
}