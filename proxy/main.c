#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#include <poll.h>
#include <sys/poll.h>

#include "queue.h"

#define BUFFER_SIZE 1024


typedef struct {
    int client;
    int server;
} Sockets;


int closeSockets(Sockets s) {
    int i = 0;
    if (s.client) {
        i++;
        close(s.client);
    }
    if (s.server) {
        i++;
        close(s.server);
    }
    return i;
}


void* connectionThread(void* arg) {
    queue_t* newConnectQueue = (queue_t*) arg;

    //ssize_t bytes_read;

    

    for (;;) {
        Sockets newSockets;

        queue_get(newConnectQueue, &newSockets);

        char buffer[BUFFER_SIZE];
        // Чтение запроса от клиента
        while (( read(newSockets.client, buffer, BUFFER_SIZE)) > 0) {
            // Отправка запроса на серверs
            //write(server_socket, buffer, bytes_read);
        }

        int dnslen;
        char* start, end;

        start = strstr(buffer, "//");
        if (!start) {
            closeSockets(newSockets);
            continue;
        }
        start+=2;

        end = strchr(buffer, '/');
        if (!end) {
            closeSockets(newSockets);
            continue;     
        }
        dnslen = start-end;

        char* dns = (char*)malloc(dnslen+1);
        strncpy(dns, start, dnslen);

        
        struct hostent *host_entry;
        host_entry = gethostbyname(dns);
        if (host_entry == NULL) {
            closeSockets(newSockets);
            continue; 
        }


        if (host_entry->h_addrtype != AF_INET) {
            closeSockets(newSockets);
            continue; 
        }

        struct in_addr addr;
        memcpy(&addr, host_entry->h_addr_list[0], sizeof(struct in_addr));
        
        newSockets.server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (newSockets.server == -1) {
            closeSockets(newSockets);
            continue;
        }

        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(80);
        server.sin_addr = addr;

        if (connect(newSockets.server, (struct sockaddr*)&server, sizeof(server)) == -1) {
            closeSockets(newSockets);
            continue;
        }

        int sended = 0;
        while ( sended != strlen(buffer)) {
            int n = send(newSockets.server, buffer, strlen(buffer), 0);
            if (n == -1) {
                sended = -1;
                break;
            }
            sended+=n;
        }
        if (sended == -1) {
            closeSockets(newSockets);
            continue;
        }
        memset(buffer, 0, BUFFER_SIZE);
        
        int recived; 
        while (!(recived = recv(newSockets.client, buffer, BUFFER_SIZE, 0))) {
            if (recived == -1) {
                closeSockets;
                break;
            }
            send(newSockets.server, buffer, recived, 0);
        }

        
        

    }

    return NULL;
}

int main(){

    queue_t newConnections;
    queue_init(&newConnections, NULL);

    pthread_t connThread;
    pthread_create(&connThread, NULL, connectionThread, &newConnections);


    pthread_mutex_t pollMutex = PTHREAD_MUTEX_INITIALIZER;



    // Создаем сокет
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Ошибка при создании сокета");
        exit(EXIT_FAILURE);
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
        queue_add(&newConnections, tmp);
    }
    return EXIT_SUCCESS;
}
