#include "sockets.h"

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