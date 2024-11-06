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

int getCmd(char *input)
{
    unsigned char parsePos = 0;
    unsigned char wordPos = 0;

    while(!isalpha(input[parsePos]) && input[parsePos]!='\0') parsePos++;
        if(isalpha(input[parsePos]))
        {
            while(isalpha(input[parsePos]))
            {
                cmd[wordPos] = input[parsePos];
                wordPos++;
                parsePos++;
            }
        }        

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

    while(strcmp(lowercase(cmd), "exit"))
    {
        cmdPtr = NULL;
        printf("%s>", currentPath);
        fgets(input, MAX_INPUT_LEN, stdin);
        if(strlen(input))
        {
            int len = getCmd(input);
            printf("%d %s\n", len, cmd);

            if(strcmp(lowercase(cmd), "pwd") == 0)  cmdPtr = pwd;

            if (cmdPtr != NULL) cmdPtr(&input[len]);
        }
    }
}
