#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "event.h"
typedef struct Event {
    char name[64];
    EventHandler handler;
    struct Event *next;
} Event;
static Event *events = NULL;
void register_event(const char *name, EventHandler handler) {
    Event *e = malloc(sizeof(Event));
    strncpy(e->name, name, sizeof(e->name));
    e->handler = handler;
    e->next = events;
    events = e;
    printf("[EVENT] Registered: %s\n", name);
}
void trigger_event(const char *name) {
    Event *e = events;
    while (e) {
        if (strcmp(e->name, name) == 0) {
            printf("[EVENT] Triggered: %s\n", name);
            e->handler();
            return;
        }
        e = e->next;
    }
    printf("[EVENT] Not found: %s\n", name);
}
