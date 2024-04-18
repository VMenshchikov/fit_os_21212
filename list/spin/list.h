#include <pthread.h>

typedef struct _Node {
    char value[100];
    struct _Node* next;
    pthread_spinlock_t sync;
} Node;

typedef struct _Storage {
    pthread_spinlock_t sync;
    Node *first;
} Storage;

Storage* initList(int);
void destroyList(Storage*);
