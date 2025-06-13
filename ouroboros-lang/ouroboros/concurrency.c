#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "concurrency.h"

void* thread_entry(void *arg) {
    void (**fp)(void *) = arg;
    (*fp)(NULL);
    return NULL;
}

void start_thread(void (*fn)(void *), void *arg) {
    (void)arg; // Mark parameter as unused
    pthread_t thread;
    void **data = malloc(sizeof(void *));
    *data = fn;
    pthread_create(&thread, NULL, thread_entry, data);
    pthread_detach(thread);
    printf("[THREAD] Started new thread\n");
}
