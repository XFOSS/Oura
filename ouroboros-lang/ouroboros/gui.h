#ifndef GUI_H
#define GUI_H

void init_gui();
void draw_window(const char *title, int width, int height);
void draw_label(const char *text);
void draw_button(const char *label);
void gui_message_loop();

#endif // GUI_H
