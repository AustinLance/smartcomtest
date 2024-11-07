#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <limits.h>
#include "cmd.h"
#include <bits/posix1_lim.h>

#define MAX_INPUT_LEN   255
#define MAX_ARGS        255
extern char currentPath[PATH_MAX];
extern char input[MAX_INPUT_LEN];

static inline bool validPathChar(char c)
{
    return isalnum(c) || c== '-' || c == '.' || c == '/';
}

static int getOneArg(char *result, const char *inputStr)
{
    unsigned char parsePos = 0;
    unsigned char wordPos = 0;

    memset(result, 0, sizeof(result));
    if(inputStr[parsePos] == '\0') return 0;

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

int pwd(char *argStr)
{
    printf("%s\n", currentPath);
}

int cd(char *argStr)
{
    char newPath[PATH_MAX];
    if(getOneArg(newPath, argStr))
    {
        if (chdir(newPath)) printf("Invalid path.\n");
        else if (getcwd(currentPath, sizeof(currentPath)) == NULL)
            {
                perror("getcwd() error");
                return 1;
            }
    }
    return 0;
}

int ls(char *argStr)
{
    char newPath[PATH_MAX];
    DIR *d;
    struct dirent *dir;

    if(!getOneArg(newPath, argStr)) strcpy(newPath, ".");
    d = opendir(newPath);
    if (d) 
    {
        while ((dir = readdir(d)) != NULL) printf("%s %s\n", dir->d_type==4 ? "dir " : "file" , dir->d_name);
        closedir(d);
    }
    else printf("%s is not a valid directory.\n", newPath);
    printf("\n");
    return 0;
}

static int GetFilename(char *filename, const char *path)
{
    int parsePos;
    memset(filename, 0, 256);
    for(parsePos=strlen(path); path[parsePos]!='/', parsePos; --parsePos);
    strcpy(filename, (const char *)(&path+parsePos+1));
    printf("filename is %s", filename);
    return 1;
}

static int CopyFile(const char *srcName, const char *dstName)
{
    int src, dst;                       //file descriptors
    int result = 0;

    if ((src = open(srcName, O_RDONLY)) == -1)
    {
        printf("Can't read source file.\n");
        return -1;
    }
    if ((dst = creat(dstName, 0660)) == -1)
    {
        close(src);
        printf("Can't write to destination.\n");
        return -1;
    }

    struct stat file_stat = {0};
    result = fstat(src, &file_stat);
    off_t copied = 0;
    while (result == 0 && copied < file_stat.st_size) {
        ssize_t written = sendfile(dst, src, &copied, SSIZE_MAX);
        copied += written;
        if (written == -1) {
            result = -1;
        }
    }

    close(src);
    close(dst);

    return result;
}

int cp(char *argStr)
{
    char args[MAX_ARGS][PATH_MAX];      //buff for arg parsing
    bool dirFlag[MAX_ARGS];
    int dirCount = 0;

    int parsePos = 0;
    int argCount = 0;
    int argLen = 1;
    
    DIR *d;
    struct stat file_stat = {0};

    //parse the args
    while(argLen && parsePos<strlen(argStr)-2 && argCount<=MAX_ARGS)
    {
        argLen = getOneArg(args[argCount], (char *)(argStr+parsePos+1));
        parsePos += argLen + 1;
        argCount++;
    }
    printf("arg count is %d\n", argCount);

    if(argCount < 2)
    {
        printf("Wrong usage cp\n");
        return 1;
    }

    //count and tag directories
    for(int i=0; i<argCount; ++i)
    {
        d = opendir(args[i]);
        if (d)
        {
            dirFlag[i] = true;
            ++dirCount;
        }
        else dirFlag[i] = false;
    }

    //Directory processing is somewhat complex and not required by the designation; skipping
    //Items beyond the first 2 discarded
    if(dirFlag[0] || dirFlag[1])
    {
        printf("Directory processing not implemented.\n");
        return 1;
    }

    if(CopyFile(args[0], args[1]) < 0) return 1;
    return 0;
}