#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opengl.h"
#include "vulkan.h"

// Graphics system abstraction layer
// This file provides a unified interface to both OpenGL and Vulkan backends

typedef enum {
    GRAPHICS_API_NONE,
    GRAPHICS_API_OPENGL,
    GRAPHICS_API_VULKAN
} GraphicsAPI;

static GraphicsAPI current_api = GRAPHICS_API_NONE;

void graphics_init(const char* api_name) {
    printf("[GRAPHICS] Initializing graphics system with API: %s\n", api_name);
    
    if (strcmp(api_name, "opengl") == 0 || strcmp(api_name, "OpenGL") == 0) {
        opengl_init();
        current_api = GRAPHICS_API_OPENGL;
    } else if (strcmp(api_name, "vulkan") == 0 || strcmp(api_name, "Vulkan") == 0) {
        vulkan_init();
        current_api = GRAPHICS_API_VULKAN;
    } else {
        printf("[GRAPHICS] Warning: Unknown graphics API '%s', defaulting to OpenGL\n", api_name);
        opengl_init();
        current_api = GRAPHICS_API_OPENGL;
    }
}

void graphics_create_window(int width, int height, const char* title) {
    printf("[GRAPHICS] Creating window: %dx%d - %s\n", width, height, title);
    
    if (current_api == GRAPHICS_API_OPENGL) {
        opengl_create_context(width, height, title);
    } else if (current_api == GRAPHICS_API_VULKAN) {
        vulkan_create_instance(title);
        vulkan_select_device();
        vulkan_create_device();
        vulkan_create_window(width, height, title);
    } else {
        printf("[GRAPHICS] Error: Graphics API not initialized\n");
    }
}

void graphics_clear(float r, float g, float b, float a) {
    if (current_api == GRAPHICS_API_OPENGL) {
        opengl_clear(r, g, b, a);
    } else if (current_api == GRAPHICS_API_VULKAN) {
        vulkan_begin_render_pass(r, g, b, a);
    }
}

void graphics_swap_buffers() {
    if (current_api == GRAPHICS_API_OPENGL) {
        opengl_swap_buffers();
    } else if (current_api == GRAPHICS_API_VULKAN) {
        vulkan_end_render_pass();
        vulkan_present();
    }
}

void graphics_shutdown() {
    printf("[GRAPHICS] Shutting down graphics system\n");
    
    if (current_api == GRAPHICS_API_OPENGL) {
        opengl_destroy_context();
    } else if (current_api == GRAPHICS_API_VULKAN) {
        vulkan_cleanup();
    }
    
    current_api = GRAPHICS_API_NONE;
}

// Create shader program
unsigned int graphics_create_shader(const char* vertex_src, const char* fragment_src) {
    if (current_api == GRAPHICS_API_OPENGL) {
        return opengl_create_shader(vertex_src, fragment_src);
    } else if (current_api == GRAPHICS_API_VULKAN) {
        return vulkan_create_graphics_pipeline(vertex_src, fragment_src);
    }
    return 0;
}

// Use shader program
void graphics_use_shader(unsigned int shader) {
    if (current_api == GRAPHICS_API_OPENGL) {
        opengl_use_shader(shader);
    }
    // No equivalent in Vulkan as pipeline is bound during command buffer recording
}

// Create buffer
unsigned int graphics_create_buffer() {
    if (current_api == GRAPHICS_API_OPENGL) {
        return opengl_create_buffer();
    }
    // For Vulkan we'd need more information, returning a dummy value
    return 1;
}

// Draw arrays
void graphics_draw_arrays(int mode, int first, int count) {
    if (current_api == GRAPHICS_API_OPENGL) {
        opengl_draw_arrays(mode, first, count);
    } else if (current_api == GRAPHICS_API_VULKAN) {
        vulkan_draw(count, 1);
    }
} 
