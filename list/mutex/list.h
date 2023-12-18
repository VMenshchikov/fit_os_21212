#include <pthread.h>

typedef struct _Node {
    char value[100];
    struct _Node* next;
    pthread_mutex_t sync;
} Node;

typedef struct _Storage {
    Node *first;
} Storage;

Storage* initList(int);
void destroyList(Storage*);
