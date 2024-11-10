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
#include <bits/posix1_lim.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
//#include <signal.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include "cmd.h"
#include <errno.h>

#define MAX_INPUT_LEN           255
#define MAX_ARGS                255
#define GREP_PATTERN_MAX_LEN    255

#define PING_PORT               0
#define PING_LEN                64
#define PING_HOPS               64
#define PING_TIMEOUT            1
#define PING_COOLDOWN           1000000
#define PING_COUNT              4

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

    while(!validPathChar(inputStr[parsePos]) && inputStr[parsePos] != '"') ++parsePos;

    if(inputStr[parsePos] == '"')           //opening quote found
    {
        ++parsePos;
        while (inputStr[parsePos] != '"' && inputStr[parsePos] != '\0')
        {
            result[wordPos] = inputStr[parsePos];
            wordPos++;
            parsePos++;
        }
        if(inputStr[parsePos] != '"')       //no closing quote found
        {
            memset(result, 0, sizeof(result));
            return parsePos;
        }
        result[wordPos] = '\0';
        wordPos+=2;
    }
    else if(validPathChar(inputStr[parsePos]))      //no quotes
    {
        while(validPathChar(inputStr[parsePos]))
        {
            result[wordPos] = inputStr[parsePos];
            wordPos++;
            parsePos++;
        }
        result[wordPos] = '\0';
    }
    
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

    int charsProcessed = getOneArg(newPath, argStr);
    if (charsProcessed && !strlen(newPath))
    {
        printf("Wrong usage ls.\n");
        return 1;
    }

    if(!strlen(newPath)) strcpy(newPath, ".");
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

int grep(char *argStr)
{
    char pattern[GREP_PATTERN_MAX_LEN];
    char fileName[NAME_MAX];
    char string[MAX_INPUT_LEN];
    char c;

    int strCnt = 0;
    int chrCnt = 0;
    int parsePos = 0;
    int strPos = 0;
    bool gotResult = false;

    parsePos += getOneArg(pattern, argStr);
    if(strlen(pattern)) 
    {
        parsePos += getOneArg(fileName, (char *)(argStr+parsePos+2));
        if(!strlen(fileName))
        {
            printf("Wrong usage grep.\n");
            return 1;
        }
    }
    else
    {
        printf("Wrong usage grep.\n");
        return 1;
    }

    printf("%s\n%s\n", pattern, fileName);

    FILE *f = fopen(fileName, "r");
    if(!f) 
    {
        printf("Can't open file.\n");
	    return(1);
    }

    parsePos = 0;
    while((c = fgetc(f)) != EOF)
    {
        string[strPos++] = c;
        if(c == '\n' || strPos>sizeof(string))
        {
            char *found = strstr(string, pattern);
            if (found != NULL)
            {
                gotResult = true;
                printf("\"%s\" found at pos %d line %d: %s", pattern, strPos, strCnt, string);
            }
            memset(string, 0, sizeof(string));
            strPos = 0;
            ++strCnt;
        }
    }
    fclose(f);
    
    if(!gotResult) printf("\"%s\" not found.\n", pattern);
    return 1;
}

static unsigned short Checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int ping(char *argStr)
{
    char inputHost[MAX_INPUT_LEN];
    char hostname[MAX_INPUT_LEN];
    char ip[NI_MAXHOST];
    int sockfd;
    struct sockaddr_in addrOut;
    struct sockaddr_in addrIn;
    int addrIn_len;
    struct sockaddr_in addrReverseLookup;
    struct hostent *hostEntity;    
    int ttlValue = PING_HOPS;
    struct timeval timeoutValue =
    {
        .tv_sec = PING_TIMEOUT,
        .tv_usec = 0
    };
    int pingCount = PING_COUNT;
    bool success;
    char buffer[128];
    struct ping_pkt {
        struct icmphdr hdr;
        char msg[PING_LEN - sizeof(struct icmphdr)];
    }pkt;
    struct timespec timeStart, timeEnd;
    long double rtt_msec = 0, total_msec = 0;
    int successCount = 0;

    //get the host
    getOneArg(inputHost, argStr);
    printf("host is %s\n", inputHost);
    if(!strlen(inputHost))
    {
        printf("Wrong usage ping.\n");
        return 1;
    }

    //fetch ip
    printf("\nResolving host %s...\n", inputHost);
    if ((hostEntity = gethostbyname(inputHost)) == NULL)
    {
        printf("Failed resolving host.\n");
        return 1;
    }
    strcpy(ip, inet_ntoa(*(struct in_addr *)hostEntity->h_addr));
    addrOut.sin_family = hostEntity->h_addrtype;
    addrOut.sin_port = htons(PING_PORT);
    addrOut.sin_addr.s_addr = *(long *)hostEntity->h_addr;
    printf("IP is %s\n", ip);

    //fetch name back
    addrReverseLookup.sin_family = AF_INET;
    addrReverseLookup.sin_addr.s_addr = inet_addr(ip);
    if (getnameinfo((struct sockaddr *)&addrReverseLookup, sizeof(struct sockaddr_in), hostname, sizeof(hostname), NULL, 0, NI_NAMEREQD))
    {
        printf("Failed reverse lookup of %s\n", ip);
        return 1;
    }
    else printf("Hostname is %s\n", hostname);

    //set up socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) 
    {
        printf("Failed acquiring socket, %d.\n", errno);
        return 1;
    } 
    if (setsockopt(sockfd, SOL_IP, IP_TTL, &ttlValue, sizeof ttlValue) != 0)
    {
        printf("Setting socket options to TTL failed!\n");
        return 1;
    } 
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeoutValue, sizeof timeoutValue);

    //ping loop
    for (int i=0; i<PING_COUNT; ++i)
    {
        success = true;

        //prep packet
        bzero(&pkt, sizeof(pkt));
        pkt.hdr.type = ICMP_ECHO;
        pkt.hdr.un.echo.id = getpid();
        
        for (int j = 0; j < sizeof(pkt.msg) - 1; j++) pkt.msg[j] = j + '0';
        pkt.msg[i] = 0;
        pkt.hdr.un.echo.sequence = i;        
        pkt.hdr.checksum = Checksum(&pkt, sizeof(pkt));

        usleep(PING_COOLDOWN);

        //send
        clock_gettime(CLOCK_MONOTONIC, &timeStart);
        if (sendto(sockfd, &pkt, sizeof(pkt), 0, &addrOut, sizeof addrOut) <= 0) 
        {
            printf("Failed sending packet.\n");
            success = false;
        }

        //receive
        if (recvfrom(sockfd, &buffer, sizeof(buffer), 0, &addrIn, &addrIn_len) <= 0)
        {
            printf("Failed receiving packet, %d.\n", errno);
            success = false;
        }
        else
        {
            clock_gettime(CLOCK_MONOTONIC, &timeEnd);
            double timeElapsed = ((double)(timeEnd.tv_nsec - timeStart.tv_nsec)) / 1000000.0;
            rtt_msec = (timeEnd.tv_sec - timeStart.tv_sec) * 1000.0 + timeElapsed;
            if(success)
            {
                //report result
                struct iphdr *ipHdr = (struct iphdr *)&buffer;
                struct icmphdr *icmpHdr = (struct icmphdr *)&buffer[20];
                if (!(icmpHdr->type == 0 && icmpHdr->code == 0)) {
                    printf("Error... Packet received with ICMP type %d code %d\n", icmpHdr->type, icmpHdr->code);
                } else {
                    printf("%d bytes from %s (h: %s) (ip: %s) msg_seq = %d ttl = %d rtt = %Lf ms.\n", PING_LEN, inputHost, hostname, ip, i, ipHdr->ttl, rtt_msec);
                    successCount++;
                }
            }
        }
    }

    //stats
    printf("Packets sent: %d, received: %d, lost %d\n", PING_COUNT, successCount, PING_COUNT-successCount);

    return 0;
}