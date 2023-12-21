

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <poll.h>
#include <sys/poll.h>
#include <fcntl.h>

#include "queue.h"
#include "mypoll.h"
#include "sockets.h"
#include "threads.h"

#define POOL_THREADS 10
#define START_CAP_POLL 100

int main(){
    

    queue_t* newConnections;
    //закинул в ноду аргументы и указатель на функцию, 
    //хотел сюда пихать функцию и запускать ее в треде,
    //но в итоге эта функция лежит в самом треде
    //(второй аргумент init)
    newConnections = queue_init(500, NULL);


    sPoll *spoll;
    spoll = sPollInit(START_CAP_POLL);


    queue_t* pool;
    pool = queue_init(500, NULL);


    pthread_t conn_th;
    pthread_t poll_th;
    pthread_t pool_th[POOL_THREADS];


    void* ptrsConn[2];
    ptrsConn[0] = newConnections;
    ptrsConn[1] = spoll;

    Data d1;
    d1.queue = newConnections;
    d1.poll = spoll;

    pthread_create(&conn_th, NULL, connectionThread, &d1);

   /*  void* ptrsPoll[2];
    ptrsPoll[0] = spoll;
    ptrsPoll[1] = pool; */

    Data d2;
    d2.queue = pool;
    d2.poll = spoll;

    pthread_create(&poll_th, NULL, pollThread, &d2);

    for (int i = 0; i < POOL_THREADS; i++) {
        pthread_create(&pool_th[i], NULL, poolThread, pool);
    }

    // Создаем сокет
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Ошибка при создании сокета");
        exit(EXIT_FAILURE);
    }

    // Устанавливаем блокирующий режим для чтения
    int flags = fcntl(server_socket, F_GETFL, 0);
    if (flags == -1) {
        perror("Ошибка при получении флагов сокета");
        close(server_socket);
        return 1;
    }

    flags &= ~O_NONBLOCK;  // Сбрасываем флаг O_NONBLOCK

    if (fcntl(server_socket, F_SETFL, flags) == -1) {
        perror("Ошибка при установке блокирующего режима для сокета");
        close(server_socket);
        return 1;
    }


    // Задаем адрес и порт для сервера
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(80);

    // Привязываем сокет к адресу и порту
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Ошибка при привязке сокета");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Начинаем прослушивание на порту
    if (listen(server_socket, 5) == -1) {
        perror("Ошибка при прослушивании порта");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Сервер слушает на порту 80...\n");

    while (1) {
        // Ждем подключения клиента
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == -1) {
            perror("Ошибка при принятии подключения");
            continue;
        }

        printf("Принято соединение от клиента\n");
        Sockets* tmp = (Sockets*)malloc(sizeof(Sockets));
        tmp->client = client_socket;
        tmp->server = 0;
        queue_add(newConnections, tmp);
    }
    return EXIT_SUCCESS;
}
