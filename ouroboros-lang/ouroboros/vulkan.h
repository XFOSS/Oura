#ifndef VULKAN_H
#define VULKAN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Vulkan instance and device management
void vulkan_init();
int vulkan_create_instance(const char* app_name);
int vulkan_select_physical_device();
int vulkan_select_device(); // Alias for vulkan_select_physical_device
int vulkan_create_logical_device();
int vulkan_create_device(); // Alias for vulkan_create_logical_device
void vulkan_cleanup();

// Vulkan surface and swapchain
int vulkan_create_surface(void* window_handle, int window_system);
int vulkan_create_swapchain(int width, int height);
void vulkan_recreate_swapchain(int width, int height);

// Vulkan render passes and framebuffers
int vulkan_create_render_pass();
int vulkan_create_framebuffers();

// Vulkan graphics pipeline
int vulkan_create_descriptor_set_layout();
int vulkan_create_pipeline_layout();
int vulkan_create_graphics_pipeline(const char* vertex_shader, const char* fragment_shader);

// Vulkan buffers and memory
int vulkan_create_vertex_buffer(void* vertices, size_t size);
int vulkan_create_index_buffer(void* indices, size_t size);
int vulkan_create_uniform_buffer(size_t size);
void vulkan_update_uniform_buffer(unsigned int buffer_index, void* data, size_t size);

// Vulkan descriptor sets
int vulkan_create_descriptor_pool();
int vulkan_create_descriptor_sets();
void vulkan_update_descriptor_sets();

// Vulkan command buffers
int vulkan_create_command_pool();
int vulkan_create_command_buffers();
int vulkan_record_command_buffer(unsigned int image_index);

// Vulkan synchronization
int vulkan_create_sync_objects();

// Vulkan rendering
int vulkan_draw_frame();
void vulkan_wait_idle();

// Vulkan texture
int vulkan_create_texture(int width, int height, unsigned char* data);
int vulkan_create_texture_sampler();

// Vulkan compute
int vulkan_create_compute_pipeline(const char* compute_shader);
int vulkan_dispatch_compute(unsigned int x, unsigned int y, unsigned int z);

// Vulkan debug
void vulkan_enable_validation_layers();
void vulkan_disable_validation_layers();

// Additional functions needed for the graphics abstraction layer
int vulkan_begin_render_pass(float r, float g, float b, float a);
int vulkan_end_render_pass();
int vulkan_present();
int vulkan_draw(int vertex_count, int instance_count);
void* vulkan_create_window(int width, int height, const char* title);
int vulkan_is_window_closed();
void vulkan_poll_events();

#endif // VULKAN_H 
