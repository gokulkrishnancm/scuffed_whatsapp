/*
   Includes common headerfiles 
*/

#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>

#define SERVER_PORT 18000 
#define MAXLINE 4096
#define MAXWORD 951
#define LOCALHOST "127.0.0.1"
#define IPV4_STRLEN 16
#define SERVER_BACKLOG 10

#define ERROR_HANDLE() {\
    fprintf(stderr, "[!] ERROR ("__FILE__" :%d): %s\n", __LINE__, strerror(errno));\
    exit(1);\
}

#define PERROR() (fprintf(stderr, "[!] ERROR ("__FILE__" :%d): %s\n", __LINE__, strerror(errno)))

// fflush doesnt work on wsl2 D: 
#define CLEAR_STDIN() {\
    while (getchar() != '\n');\
}

#define THREAD_INIT(pclient,pfunc) \
{\
    pthread_t thread;\
    pthread_create(&thread, NULL, pfunc, pclient);\
}

typedef struct client {
    char name[MAXWORD+1];
    int socket;
    char straddr[IPV4_STRLEN+1];
    bool available;
    struct client *next;
} Client;

#endif
