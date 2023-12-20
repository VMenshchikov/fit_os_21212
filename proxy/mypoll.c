#define _GNU_SOURCE

#include <stdlib.h>
#include <poll.h>
#include <pthread.h>
#include <memory.h>
//#include <sys/poll.h>
#include <stdio.h>

#include "sockets.h"
#include "mypoll.h"


sPoll* sPollInit(unsigned cap) {
    sPoll* s = (sPoll*)malloc(sizeof(sPoll));

    s->capasity = cap;
    s->size = 0;

    s->lock = malloc(sizeof(pthread_spinlock_t));
    pthread_spin_init(s->lock, PTHREAD_PROCESS_SHARED);

    s->socs = (Sockets**)malloc(sizeof(Sockets*) * cap);
    s->fds = (pollfd*)malloc(sizeof(pollfd) * cap);
    
    printf("socs: %d %p\n", s->size, s->socs);

    return s;
}

//вызывается при захваченом spinlock
int sPollExtension(sPoll *s) {
    Sockets** newSocs = (Sockets**)malloc(sizeof(Sockets*) * (s->capasity * 2));
    pollfd* newFds = (pollfd*)malloc(sizeof(pollfd) * (s->capasity * 2));
    
    memcpy(newSocs, s->socs, sizeof(Sockets*) * (s->capasity));
    memcpy(newFds, s->fds, sizeof(pollfd) * (s->capasity));

    free(s->socs);
    free(s->fds);

    s->socs = newSocs;
    s->fds = newFds;

    s->capasity *= 2;

}

int sPollPush(sPoll *s, Sockets *soc) {
    pthread_spin_lock(s->lock);

    if (s->size == s->capasity) {
        printf("ext\n");
        sPollExtension(s);
    }

    //Sockets* tmp =  s->socs + sizeof(Sockets*) * s->size;
    //tmp = soc;

    printf("socs: %d %p\n", s->size, s->socs);

    s->socs[s->size] = soc;

    s->fds[s->size].fd = soc->server;
    s->fds[s->size].events = POLLIN;

    s->size++;

    pthread_spin_unlock(s->lock);
}

Sockets* sPollPop(sPoll *s, int ind) {
    //pthread_spin_lock(s->lock);

    Sockets* ret = s->socs[ind];

    memmove(s->socs + ind*sizeof(Sockets*), s->socs + ((ind + 1) * sizeof(Sockets*)), sizeof(Sockets*) * (s->size - ind -1));
    memmove(s->fds + ind*sizeof(pollfd*), s->fds + ((ind + 1) * sizeof(Sockets*)), sizeof(Sockets*) * (s->size - ind -1));

    // 0 1 2 3 4 5 // size 6
    // 0 1 _ 3 4 5 // ind 2
    // cipy size-ind-1

    s->size--;
    //pthread_spin_unlock(s->lock);

    return ret;
}


