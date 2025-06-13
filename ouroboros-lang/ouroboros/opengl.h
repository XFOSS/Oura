#ifndef OPENGL_H
#define OPENGL_H

#include <stdio.h>
#include <stdlib.h>

// OpenGL constants (to avoid including GL/glew.h)
#define GL_ARRAY_BUFFER              0x8892
#define GL_STATIC_DRAW               0x88E4
#define GL_FLOAT                     0x1406
#define GL_TRIANGLES                 0x0004
#define GL_UNSIGNED_INT              0x1405
#define GL_COLOR_BUFFER_BIT          0x4000
#define GL_DEPTH_BUFFER_BIT          0x0100
#define GL_TEXTURE0                  0x84C0
#define GL_TEXTURE_2D                0x0DE1
#define GL_TEXTURE_WRAP_S            0x2802
#define GL_TEXTURE_WRAP_T            0x2803
#define GL_TEXTURE_MIN_FILTER        0x2801
#define GL_TEXTURE_MAG_FILTER        0x2800
#define GL_REPEAT                    0x2901
#define GL_LINEAR                    0x2601
#define GL_LINEAR_MIPMAP_LINEAR      0x2703
#define GL_RGB                       0x1907
#define GL_RGBA                      0x1908
#define GL_VERTEX_SHADER             0x8B31
#define GL_FRAGMENT_SHADER           0x8B30
#define GL_COMPILE_STATUS            0x8B81
#define GL_LINK_STATUS               0x8B82
#define GL_FALSE                     0
#define GL_TRUE                      1

// OpenGL initialization and context management
void opengl_init();
void opengl_create_context(int width, int height, const char* title);
void opengl_destroy_context();
int opengl_is_context_valid();

// Shader management
unsigned int opengl_create_shader(const char* vertex_src, const char* fragment_src);
void opengl_use_shader(unsigned int shader_program);
void opengl_set_uniform_int(unsigned int shader, const char* name, int value);
void opengl_set_uniform_float(unsigned int shader, const char* name, float value);
void opengl_set_uniform_vec3(unsigned int shader, const char* name, float x, float y, float z);
void opengl_set_uniform_vec4(unsigned int shader, const char* name, float x, float y, float z, float w);
void opengl_set_uniform_mat4(unsigned int shader, const char* name, float* matrix);

// Buffer and vertex array management
unsigned int opengl_create_vertex_array();
unsigned int opengl_create_buffer();
void opengl_bind_vertex_array(unsigned int vao);
void opengl_bind_buffer(unsigned int buffer, int target);
void opengl_buffer_data(int target, size_t size, void* data, int usage);
void opengl_vertex_attrib_pointer(unsigned int index, int size, int type, int normalized, int stride, size_t offset);
void opengl_enable_vertex_attrib_array(unsigned int index);

// Texture management
unsigned int opengl_create_texture(int width, int height, unsigned char* data, int format);
void opengl_bind_texture(unsigned int texture, int slot);

// Rendering functions
void opengl_clear(float r, float g, float b, float a);
void opengl_draw_arrays(int mode, int first, int count);
void opengl_draw_elements(int mode, int count, int type, void* indices);
void opengl_swap_buffers();

// Advanced features
void opengl_enable(int feature);
void opengl_disable(int feature);
void opengl_blend_func(int src_factor, int dst_factor);
void opengl_depth_func(int func);

#endif // OPENGL_H 
