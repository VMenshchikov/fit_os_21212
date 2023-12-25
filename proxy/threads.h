typedef struct {
    queue_t* queue;
    sPoll* poll;
} Data;

void* connectionThread(void* arg);
void* pollThread(void* arg);
void* poolThread(void* arg);