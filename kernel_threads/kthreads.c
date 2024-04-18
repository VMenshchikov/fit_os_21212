#include "kthreads.h"
#include <stdlib.h>
#include <linux/sched.h>    
#include <sched.h>          
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <linux/mman.h>

#include <stdio.h>
#include <linux/futex.h>


#define STACK_SIZE (1024 * 1024 * 8)

//а можно было просто вставать в блокировку на мутексе?(там жу футекс(+-))
// Обертка над системным вызовом futex
static int futex_wait(_Atomic volatile int *uaddr, int val) {
    return syscall(SYS_futex, uaddr, FUTEX_WAIT, val, NULL, NULL, 0);
}

static int futex_wake(_Atomic volatile int *uaddr, int val) {
    return syscall(SYS_futex, uaddr, FUTEX_WAKE, val, NULL, NULL, 0);
}


int clone_func(void* arg) {
    kthread* arg_t = (kthread*)arg;
    arg_t->result = arg_t->start_routine(arg_t->arg);
    arg_t->exited = 1;

    if (arg_t->joined == 1) {
        futex_wake(&arg_t->exited, 1);
        { // для проверки(вывести)
            perror("futex_wake");
        }
    }
    return 0;
}

int kthreads_create(kthread* thread, void *(start_routine), void *arg) {
    thread->start_routine = start_routine;
    thread->arg = arg;
    thread->joined = 0;
    thread->exited = 0;
    thread->stack_size = STACK_SIZE;


    thread->stack = (char *)mmap(NULL, thread->stack_size, PROT_READ | PROT_WRITE,  MAP_PRIVATE | MAP_GROWSDOWN | MAP_ANONYMOUS, -1, 0);    
    if (thread->stack == NULL) return -1;


    char* stack_top = thread->stack + thread->stack_size;
    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | CLONE_SYSVSEM ;// | CLONE_NEWPID ;//| CLONE_SETTLS;

    thread->pid = clone(clone_func, stack_top, flags, (void*)thread);
    return 0;
}

int kthreads_join(kthread* thread, void** retval) {

    if (thread->joined != 0) {
        return -1;
    } 

    thread->joined = 1;
    futex_wait(&thread->exited, 0);
    {
        perror("futex_wait");
    }


    *retval = thread->result;
    char* stack_top = thread->stack + (thread->stack_size / sizeof(int));
    munmap(thread->stack, thread->stack_size);
    return 0;
}


