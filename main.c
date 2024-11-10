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


//add new commands here
struct
{
    char cmdName[MAX_INPUT_LEN];
    int (*cmdPtr)(char *p);
}cmdTable[] =
{
    {"pwd",     pwd},
    {"cd",      cd},
    {"ls",      ls},
    {"cp",      cp},
    {"grep",    grep},
    {"ping",    ping},
};

int (*lookupCmd())(char *cmd)
{
    for(int i=0; i<sizeof cmdTable; ++i) if(strcmp(cmd, cmdTable[i].cmdName) == 0) return cmdTable[i].cmdPtr;
}

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
                if(lookupCmd(cmd))
                   lookupCmd(cmd)(&input[cmdLen]);
            else if(strcmp(lowercase(cmd), "exit")) printf("Invalid command.\n");
        }
    }while(strcmp(lowercase(cmd), "exit"));
}
