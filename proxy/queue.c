#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>

#include "queue.h"

void mutex_unlock_for_wait(void *arg) {
	queue_t *q = arg;
	pthread_mutex_unlock(q->mutex);
}

queue_t* queue_init(int max_count, void* (*func)(void*) ) {

	queue_t *q = malloc(sizeof(queue_t));
	if (!q) {
		printf("Cannot allocate memory for a queue\n");
		abort();
	}
	
	q->mutex = malloc(sizeof(pthread_mutex_t));
	if (!q->mutex) {
		printf("Cannot allocate memory for a mutex\n");
		abort();
	}
	pthread_mutex_init(q->mutex, PTHREAD_MUTEX_DEFAULT);


	q->cond = malloc(sizeof(pthread_cond_t));
	if (!q->cond) {
		printf("Cannot allocate memory for a mutex\n");
		abort();
	}
	pthread_cond_init(q->cond, NULL);

	q->routin = func;
	q->first = NULL;
	q->last = NULL;

	return q;
}

void queue_destroy(queue_t *q) {
	pthread_mutex_lock(q->mutex);
	pthread_cleanup_push(mutex_unlock_for_wait, q);


	for (qnode_t *it = q->first; it != NULL;) {
		qnode_t *next = it->next;
		free(it);
		it = next;
	}


	pthread_mutex_unlock(q->mutex);
	pthread_mutex_destroy(q->mutex);
	pthread_cond_destroy(q->cond);
	free((void*)q->mutex);

	free(q);
	q = NULL;
	pthread_cleanup_pop(0);
}

int queue_add(queue_t *q, void* arg) {
	pthread_mutex_lock(q->mutex);
	pthread_cleanup_push(mutex_unlock_for_wait, q);

	//while (q->count == q->max_count) {
		//pthread_cond_wait(q->cond, q->mutex);
		//pthread_testcancel();
	//}

	

	//assert(q->count <= q->max_count);

	qnode_t *new = malloc(sizeof(qnode_t));
	if (!new) {
		printf("Cannot allocate memory for new node\n");
		pthread_mutex_unlock(q->mutex);
		abort();
	}

	printf("добавляю %p\n", arg);

	new->args = arg;
	new->next = NULL;

	if (!q->first)
		q->first = q->last = new;
	else {
		q->last->next = new;
		q->last = q->last->next;
	}

	pthread_mutex_unlock(q->mutex);
	pthread_cond_broadcast(q->cond);

	pthread_cleanup_pop(0);
	return 1;
}

void queue_get(queue_t *q, void** ret) {
	pthread_mutex_lock(q->mutex);
	pthread_cleanup_push(mutex_unlock_for_wait, q);
	while (!q->first) {
		pthread_cond_wait(q->cond, q->mutex);

	}


	//assert(q->count >= 0);



	qnode_t *tmp = q->first;

	*ret = tmp->args;

	printf("возвращаю %p\n", *ret);
	q->first = q->first->next;
	free(tmp);
	pthread_mutex_unlock(q->mutex);
	//pthread_cond_broadcast(q->cond);

	pthread_cleanup_pop(0);

	return;
}

