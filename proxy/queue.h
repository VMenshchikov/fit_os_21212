#pragma once
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>


typedef struct _QueueNode {
	void* args;
	struct _QueueNode *next;
} qnode_t;

typedef struct _Queue {
	qnode_t *first;
	qnode_t *last;

	void* (*routin)(void*);

	pthread_mutex_t *mutex;      
	pthread_cond_t *cond;
	
} queue_t;

queue_t* queue_init(int max_count, void* (*func)(void*));
void queue_destroy(queue_t *q);
int queue_add(queue_t *q, void* args);
void queue_get(queue_t *q, void **ret);

