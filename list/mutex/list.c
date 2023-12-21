#define _GNU_SOURCE

#include "list.h"

#include <pthread.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>

void destroyList(Storage* storage) {
    if (storage == NULL) return;

    Node* node = storage->first;
    while (node != NULL) {
        pthread_mutex_destroy(&node->sync);
        Node* tmp = node->next;
        free(node);
        node = tmp;
    }

    free(storage);
}

Node* createNode(int lenText) {
    Node* newNode = (Node*)calloc(1, sizeof(Node));
    if (newNode == NULL) {
        return NULL;
    }
    
    pthread_mutex_init(&newNode->sync, PTHREAD_MUTEX_DEFAULT);
    memset(&newNode->value, '1', lenText);
    newNode->next = NULL;

    return newNode;
}

Storage* initList(int cap) {
    Storage* ret = (Storage*)malloc(sizeof(Storage)); 
    if (ret == NULL) return NULL;
    
    srand(time(NULL));

    Node* node = createNode(rand()%100+1);
    if (node == NULL) {
        free(ret);
        return NULL;
    }


    ret->first = node;
    for (int i = 1; i < cap; ++i) {
        Node* newNode = createNode(rand()%100+1);
        if (node == NULL) {
            destroyList(ret);
        return NULL;
        }

    node->next = newNode;
    node = newNode;   
    }

    return ret;
}

