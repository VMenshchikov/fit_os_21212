#include <stdio.h>
#include <stdlib.h>
#include "kthreads.h"
#include <pthread.h>


void* function(void* arg){
    sleep(1); // что бы принты не перекрывались
    printf("Тред запущен, %d\n", *(int*)arg);
    //sleep(5);
    printf("Пока\n");
    int* a = (int*)malloc(sizeof(int));
    (*a) =  *((int*)arg)+1;

    return (void*)a;
}

int main()
{
    kthread t;
    int a = 5;
    int stat = kthreads_create(&t, function, (void*)&a);
    printf("Создание: %d\n", stat);


    void*ret;
    
    kthreads_join(&t, &ret);

    printf("ПОлучил:%d\n", *((int*)ret));
    
    //free(ret);


    return 0;
}
