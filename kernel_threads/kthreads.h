#define _GNU_SOURCE
#include <unistd.h>
typedef struct {
    void *(*start_routine)(void *);
    void *arg;
    void *result;

    _Atomic volatile int joined;
    _Atomic volatile int exited;
    //int status;

    char* stack;
    int stack_size;
    pid_t pid;
} kthread;


int kthreads_join(kthread* thread, void** retval);
int kthreads_create(kthread* kthread, void *(start_routine), void *arg);
