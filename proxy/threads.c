#define _GNU_SOURCE

#include <arpa/inet.h>
#include <netdb.h>
#include <memory.h>
#include <pthread.h>


#include "queue.h"
#include "sockets.h"
#include "mypoll.h"

#define BUFFER_SIZE 1024

void* connectionThread(void* arg) {
    queue_t* newConnectQueue = (queue_t*) arg;
    sPoll* spoll = (sPoll*) (arg + sizeof(queue_t*));

    //ssize_t bytes_read;

    for (;;) {
        Sockets* newSockets = (Sockets*)malloc(sizeof(Sockets));

        queue_get(newConnectQueue, newSockets);

        char buffer[BUFFER_SIZE];
        // Чтение запроса от клиента
        while (( read(newSockets->client, buffer, BUFFER_SIZE)) > 0) {}

        int dnslen;
        char* start, end;

        start = strstr(buffer, "//");
        if (!start) {
            closeSockets(*newSockets);
            free(newSockets);
            continue;
        }
        start+=2;

        end = strchr(buffer, '/');
        if (!end) {
            closeSockets(*newSockets);
            free(newSockets);
            continue;     
        }
        dnslen = start-end;

        char* dns = (char*)malloc(dnslen+1);
        strncpy(dns, start, dnslen);

        
        struct hostent *host_entry;
        host_entry = gethostbyname(dns);
        if (host_entry == NULL) {
            closeSockets(*newSockets);
            free(newSockets);
            continue; 
        }


        if (host_entry->h_addrtype != AF_INET) {
            closeSockets(*newSockets);
            free(newSockets);
            continue; 
        }

        struct in_addr addr;
        memcpy(&addr, host_entry->h_addr_list[0], sizeof(struct in_addr));
        
        newSockets->server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (newSockets->server == -1) {
            closeSockets(*newSockets);
            free(newSockets);
            continue;
        }

        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(80);
        server.sin_addr = addr;

        if (connect(newSockets->server, (struct sockaddr*)&server, sizeof(server)) == -1) {
            closeSockets(*newSockets);
            free(newSockets);
            continue;
        }

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
        
        int recived; 
        while (!(recived = recv(newSockets->client, buffer, BUFFER_SIZE, 0))) {
            if (recived == -1) {
                closeSockets(*newSockets);
                free(newSockets);
                break;
            }
            send(newSockets->server, buffer, recived, 0);
        }

        if (recived == -1) continue;
        
        sPollPush(spoll, newSockets);

    }
    return NULL;
}

void* pollThread(void* arg){
    sPoll* spoll = (sPoll*) arg;
    queue_t* poolQueue = (queue_t*) (arg + sizeof(sPoll*));

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
    } 
}

void* poolThread(void* arg) {
    queue_t* pool = (queue_t*) arg;

    for (;;) {
        Sockets* soc;
        queue_get(pool, soc);
        

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