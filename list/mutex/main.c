#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "list.h"

_Atomic int swap = 0;
int s1 = 0;
int s2 = 0;
int s3 = 0;


void* swaper(void* arg) {
    Storage* storage = (Storage*) arg;
    while(1){


    Node* nodes[3];

    pthread_mutex_lock(&storage->sync);
    if (rand() % 100 > 33) {
        if (storage->first != NULL) {
            pthread_mutex_lock(&storage->first->sync);
            nodes[0] = storage->first;
            if (nodes[0]->next != NULL) {
                pthread_mutex_lock(&nodes[0]->next->sync);
                nodes[1] = nodes[0]->next;
                if (nodes[0]->next != NULL) {
                    nodes[2] = nodes[1]->next;

                    storage->first = nodes[1];
                    nodes[1]->next = nodes[0];
                    nodes[0]->next=nodes[2];

                    swap++;
                }
                pthread_mutex_unlock(&storage->first->next->sync);
            }
            //pthread_mutex_unlock(&nodes[0]->sync);
        }
        pthread_mutex_unlock(&storage->sync);
    } else {
        pthread_mutex_lock(&storage->first->sync);
        pthread_mutex_unlock(&storage->sync);

    }

    
    for (nodes[0] = storage->first; nodes[0]->next != NULL; nodes[0] = nodes[1]) {
        pthread_mutex_lock(&nodes[0]->next->sync);
        nodes[1] = nodes[0]->next;
        if (nodes[1]->next != NULL) {
            pthread_mutex_lock(&nodes[1]->next->sync);
            nodes[2] = nodes[1]->next;
            if (nodes[2]->next && rand() % 100 > 33) {
                swap++;
                nodes[0]->next = nodes[2];
                nodes[1]->next = nodes[2]->next;
                nodes[2]->next = nodes[1];
            }
            pthread_mutex_unlock(&nodes[2]->sync);
        }
        pthread_mutex_unlock(&nodes[0]->sync);
    }
        
    pthread_mutex_unlock(&nodes[0]->sync);
    } 
}

void* seeker1(void* arg) {
    Storage* storage = (Storage*) arg;
    for(;;) {
        for (Node* node = storage->first; node->next != NULL; node = node->next) {
            pthread_testcancel();
            pthread_mutex_lock(&node->sync);
            pthread_mutex_lock(&node->next->sync);

            if (strlen(node->value) < strlen(node->next->value)) s1++;

            pthread_mutex_unlock(&node->next->sync);
            pthread_mutex_unlock(&node->sync);
        } 
    }
}

void* seeker2(void* arg) {
    Storage* storage = (Storage*) arg;
    for(;;) {
        for (Node* node = storage->first; node->next != NULL; node = node->next) {
            pthread_testcancel();
            pthread_mutex_lock(&node->sync);
            pthread_mutex_lock(&node->next->sync);

            if (strlen(node->value) > strlen(node->next->value)) s2++;

           
            pthread_mutex_unlock(&node->next->sync);
            pthread_mutex_unlock(&node->sync);
        } 
    }
}

void* seeker3(void* arg) {
    Storage* storage = (Storage*) arg;
    for(;;) {
        for (Node* node = storage->first; node->next != NULL; node = node->next) {
            pthread_testcancel();
            pthread_mutex_lock(&node->sync);
            pthread_mutex_lock(&node->next->sync);

            if (strlen(node->value) == strlen(node->next->value)) s3++;

            pthread_mutex_unlock(&node->next->sync);
            pthread_mutex_unlock(&node->sync);

        } 
    }
}

int main(int argc, char const *argv[])
{   
    if (argc != 2) return -1;
    const int lenList = atoi(argv[1]);
    Storage* storage = initList(lenList);
    pthread_t threads[6];

    pthread_create(&threads[0], NULL, swaper, storage);
    pthread_create(&threads[1], NULL, swaper, storage);
    pthread_create(&threads[2], NULL, swaper, storage);
    pthread_create(&threads[3], NULL, seeker1, storage);
    pthread_create(&threads[4], NULL, seeker2, storage);
    pthread_create(&threads[5], NULL, seeker3, storage);

    for (int i = 0; i < 15; i++) {
        printf("Перестановки:%d. Возрастает:%d. Убывает:%d. Равно:%d\n", swap, s1, s2, s3);
        //printf("first: %p %s", storage->first, storage->first->value);
        sleep(1);
    }

    for (int i = 0; i < 6; ++i) {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
    }

    destroyList(storage);

    return 0;
}
