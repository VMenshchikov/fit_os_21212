#define _GNU_SOURCE

#include <arpa/inet.h>
#include <netdb.h>
#include <memory.h>
#include <pthread.h>


#include "queue.h"
#include "sockets.h"
#include "mypoll.h"
#include "threads.h"

#define BUFFER_SIZE 1024



void* connectionThread(void* arg) {
    Data d = *(Data*)arg;

    queue_t* newConnectQueue = d.queue;
    sPoll* spoll = d.poll;
    printf("socs перед пушем: %d %p\n", spoll->size, spoll->socs);
    //ssize_t bytes_read;

    for (;;) {
        Sockets* newSockets;
        queue_get(newConnectQueue, (void**)&newSockets);
        printf("получил %p\n", newSockets);
        char buffer[BUFFER_SIZE];
        // Чтение запроса от клиента
        int readed;
        //printf("начал читать %d\n", newSockets->client);
        if (( readed = read(newSockets->client, buffer, BUFFER_SIZE)) > 0) {
            //printf("прочитал %d: %s\n", readed, buffer);
        }

        if (readed == -1) {
            printf("соединение разорвано\n");
            closeSockets(*newSockets);
            free(newSockets);
            continue;
        }

        int dnslen;
        char *start, *end;

        start = strstr(buffer, "//");
        if (!start) {
            closeSockets(*newSockets);
            free(newSockets);
            continue;
        }
        start+=2;

        end = strchr(start, '/');
        if (!end) {
            closeSockets(*newSockets);
            free(newSockets);
            continue;     
        }
        dnslen = end-start;

        

        char* dns = (char*)malloc(dnslen+1);
        strncpy(dns, start, dnslen);
        
        printf("dns %s\n", dns);


        const char *port = "80";
        const char *path = "/";
        // Получаем IP-адрес по доменному имени
        struct addrinfo hints, *result, *rp;
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
    
        int status = getaddrinfo(dns, port, &hints, &result);
        if (status != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
            exit(EXIT_FAILURE);
        }

        // Перебираем результаты, используем первый подходящий IP-адрес
        int soc = -1;
        for (rp = result; rp != NULL; rp = rp->ai_next) {
            soc = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (soc == -1) {
                continue; // Ошибка при создании сокета, пробуем следующий адрес
            }

            if (connect(soc, rp->ai_addr, rp->ai_addrlen) != -1) {
                break; // Успешное соединение
            }
    
            close(soc); // Ошибка соединения, закрываем сокет и пробуем следующий адрес
        }

        freeaddrinfo(result); // Освобождаем структуры addrinfo

        newSockets->server = soc;

        printf("создал сокет %d\n", newSockets->server);

        int sended = 0;
        while ( sended != strlen(buffer)) {
            int n = send(newSockets->server, buffer, strlen(buffer), 0);
            if (n == -1) {
                sended = -1;
                break;
            }
            sended+=n;
        }
        if (sended == -1) {
            closeSockets(*newSockets);
            free(newSockets);
            continue;
        }
        memset(buffer, 0, BUFFER_SIZE);

        printf("переслал\n");
        
        /* int recived; 
        while ((recived = recv(newSockets->client, buffer, BUFFER_SIZE, 0))) {
            if (recived == -1) {
                closeSockets(*newSockets);
                free(newSockets);
                break;
            }
            send(newSockets->server, buffer, recived, 0);
        }

        if (recived == -1) continue; */

        printf("кидаю в poll\n");

        
        sPollPush(spoll, newSockets);

    }
    return NULL;
}

void* pollThread(void* arg){

    Data d = *(Data*)arg;

    queue_t* poolQueue = d.queue;
    sPoll* spoll = d.poll;

    //sPoll* spoll = (sPoll*) arg;
    //queue_t* poolQueue = (queue_t*) (arg + sizeof(sPoll*));

    for (;;) {
        pthread_spin_lock(spoll->lock);
        int count = spoll->size;
        pthread_spin_unlock(spoll->lock);

        int done = poll(spoll->fds, count, 100);

        pthread_spin_lock(spoll->lock);
        for (int i = 0; done; i++) {
            if(spoll->fds[i].revents & POLLIN) {
                Sockets* tmp = sPollPop(spoll, i);
                
                done--;
                i--;

                queue_add(poolQueue, tmp);
            }
        }
        pthread_spin_unlock(spoll->lock);
    } 
}

void* poolThread(void* arg) {
    queue_t* pool = (queue_t*) arg;
    for (;;) {
        
        Sockets* soc;
        queue_get(pool, (void**)&soc);

        char buffer[BUFFER_SIZE];
        int recv;
        while (recv = read(soc->server, &buffer, BUFFER_SIZE)) {
            if (recv == -1) break;
            //не уверен, что нужно(вообще нужно, но нам?) проверять, что все записалось
            write(soc->client, &buffer, recv);
        }
        //надеюсь, что соединение оборвется (по протоколу)
        if (recv == -1) {
            closeSockets(*soc);
            free(soc);
        }
    }
}