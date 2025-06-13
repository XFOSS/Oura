#ifndef CONCURRENCY_H
#define CONCURRENCY_H

void start_thread(void (*fn)(void *), void *arg);

#endif // CONCURRENCY_H
