#pragma once
typedef struct _sockets{
    int client;
    int server;
} Sockets;


int closeSockets(Sockets s);