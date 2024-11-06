#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/limits.h>
#include "cmd.h"

#define MAX_INPUT_LEN   255
#define MAX_ARGS        10
extern char currentPath[PATH_MAX];
extern char input[MAX_INPUT_LEN];

static inline bool validPathChar(char c)
{
    return isalnum(c) || c == '.' || c == '/';
}

static int getOneArg(char *result, const char *inputStr)
{
    unsigned char parsePos = 0;
    unsigned char wordPos = 0;

    while(!validPathChar(inputStr[parsePos])) parsePos++;
    if(validPathChar(inputStr[parsePos]))
    {
        while(validPathChar(inputStr[parsePos]))
        {
            result[wordPos] = inputStr[parsePos];
            wordPos++;
            parsePos++;
        }
    }
    result[wordPos] = '\0';
    return wordPos;
}

int pwd(char *args)
{
    printf("%s\n", currentPath);
}

int cd(char *args)
{
    char newPath[PATH_MAX];
    if(getOneArg(newPath, args))
    {
        if (chdir(newPath)) printf("Invalid path.\n");
        else if (getcwd(currentPath, sizeof(currentPath)) == NULL)
            {
                perror("getcwd() error");
                return 1;
            }
    }
}

int ls(char *args)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d) 
    {
        while ((dir = readdir(d)) != NULL) printf("%s %s\n", dir->d_type==4 ? "dir " : "file" , dir->d_name);
        closedir(d);
    }
    return(0);
}