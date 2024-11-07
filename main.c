#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include "cmd/cmd.h"

#define MAX_INPUT_LEN   255
#define MAX_ARGS        10

char currentPath[PATH_MAX];
char input[MAX_INPUT_LEN];
char cmd[MAX_INPUT_LEN];

int (*cmdPtr)(char *p);

char *lowercase(char *p)
{
    char *ptr = p;
    for ( ; *p; ++p) *p = tolower(*p);
    return ptr;
}

int getCmd(char *inputStr)
{
    unsigned char parsePos = 0;
    unsigned char wordPos = 0;
    memset(cmd, 0, sizeof(cmd));

    while(!isalpha(inputStr[parsePos])) parsePos++;
    if(isalpha(inputStr[parsePos]))
    {
        while(isalpha(inputStr[parsePos]))
        {
            cmd[wordPos] = inputStr[parsePos];
            wordPos++;
            parsePos++;
        }
    }

    if(inputStr[parsePos] != ' ' && inputStr[parsePos] != '\0')
        return 0;           //more chars following and no delimiter: invalid command

    cmd[wordPos] = '\0';
    return wordPos;
}

int main(int, char**){
    
    if (getcwd(currentPath, sizeof(currentPath)) == NULL)
    {
        perror("getcwd() error");
        return 1;
    }

    printf("Hello, from mybash!\n");

    do
    {
        cmdPtr = NULL;
        memset(input, 0, sizeof(input));
        printf(">");
        fgets(input, MAX_INPUT_LEN, stdin);
        input[strlen(input)-1] = '\0';    //strip CR
        if(strlen(input))
        {
            int cmdLen = getCmd(input);
            if(cmdLen)
            {
                if(strcmp(lowercase(cmd), "pwd") == 0)  cmdPtr = pwd;
                else if(strcmp(lowercase(cmd), "cd") == 0)  cmdPtr = cd;
                else if(strcmp(lowercase(cmd), "ls") == 0)  cmdPtr = ls;
                else if(strcmp(lowercase(cmd), "cp") == 0)  cmdPtr = cp;
            }
            if (cmdPtr != NULL) cmdPtr(&input[cmdLen]);
            else if(strcmp(lowercase(cmd), "exit")) printf("Invalid command.\n");
        }
    }while(strcmp(lowercase(cmd), "exit"));
}
