#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui.h"

// Simplified GUI implementation for demonstration purposes

void init_gui() {
    printf("[GUI] GUI initialized (simulated)\n");
}

void draw_window(const char *title, int width, int height) {
    printf("[GUI] Window '%s' drawn [%d x %d] (simulated)\n", title, width, height);
}

void draw_label(const char *text) {
    printf("[GUI] Label: \"%s\" (simulated)\n", text);
}

void draw_button(const char *label) {
    printf("[GUI] Button: [%s] (simulated)\n", label);
}

void gui_message_loop() {
    printf("[GUI] Message loop (simulated)\n");
}
