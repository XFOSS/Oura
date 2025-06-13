#ifndef EVENT_H
#define EVENT_H
typedef void (*EventHandler)(void);
void register_event(const char *name, EventHandler handler);
void trigger_event(const char *name);
#endif // EVENT_H
