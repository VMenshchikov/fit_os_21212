#include <pthread.h>

typedef struct _Node {
    char value[100];
    struct _Node* next;
    pthread_rwlock_t sync;
} Node;

typedef struct _Storage {
    pthread_rwlock_t sync;
    Node *first;
} Storage;

Storage* initList(int);
void destroyList(Storage*);
