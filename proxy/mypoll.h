#pragma once
#define _GNU_SOURCE

#include <poll.h>
#include <pthread.h>
//#include <sys/poll.h>
#include "sockets.h"

typedef struct pollfd pollfd;

typedef struct spoll{
    
    pollfd *fds;
    Sockets **socs;


    int size;
    int capasity;
    
    pthread_spinlock_t* lock;

} sPoll;

int sPollInit(sPoll *s, unsigned cap);
int sPollExtension(sPoll *s);
int sPollPush(sPoll *s, Sockets *soc);
Sockets* sPollPop(sPoll *s, int ind);


