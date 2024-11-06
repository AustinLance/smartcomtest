#include <stdio.h>
#include "cmd.h"

#define MAX_INPUT_LEN   255
#define MAX_ARGS        10
extern char input[MAX_INPUT_LEN];

int pwd(char *args)
{
    printf("args are: %s\n", args);
    //printf("lib function called\n");
}