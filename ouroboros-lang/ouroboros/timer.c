#include <stdio.h>
#include <unistd.h>
#include "timer.h"
void set_timeout(void (*callback)(void), int seconds) {
    printf("[TIMER] Waiting %d seconds...\n", seconds);
    sleep(seconds);
    callback();
}
