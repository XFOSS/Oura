#ifndef GRAPHICS_H
#define GRAPHICS_H

// Graphics system abstraction layer
// This file provides a unified interface to both OpenGL and Vulkan backends

// Initialize the graphics system with the specified API
void graphics_init(const char* api_name);

// Create a window with the specified dimensions and title
void graphics_create_window(int width, int height, const char* title);

// Clear the screen with the specified color
void graphics_clear(float r, float g, float b, float a);

// Swap buffers to display the rendered content
void graphics_swap_buffers();

// Shutdown the graphics system
void graphics_shutdown();

// Create a shader program from vertex and fragment shader sources
unsigned int graphics_create_shader(const char* vertex_src, const char* fragment_src);

// Use the specified shader program
void graphics_use_shader(unsigned int shader);

// Create a buffer for vertex data
unsigned int graphics_create_buffer();

// Draw using the specified primitive mode, starting index, and count
void graphics_draw_arrays(int mode, int first, int count);

#endif // GRAPHICS_H 
