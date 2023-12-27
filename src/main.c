#include "foundation/foundation.h"
#include "ui/ui.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define GLFW_INCLUDE_ES3
#include <GLFW/glfw3.h>
#include <GLES3/gl3.h>

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#include <uv.h>

static ui_renderer_t renderer;
static ui_state_t state;
static ui_input_t source_input;
static ui_label_t copyright;
static ui_label_t fps_label;
static ui_label_t status_label;

static ustring_view fps_str;
#define MAX_FPX_BITS 16
static ustring_view status_str;
#define MAX_STATUS_BITS 256

static GLFWcursor *default_cursor;
static GLFWcursor *text_input_cursor;
static GLFWcursor *resize_hori_cursor;
static GLFWcursor *resize_vert_cursor;

static bool empty_launch = true;

static void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    //printf("key: %d, scancode: %d, action: %d, mods: %d\n", key, scancode, action, mods);
    if (action == GLFW_PRESS) {
        ui_state_key_press(&state, key);
    } else if (action == GLFW_RELEASE) {
        ui_state_key_release(&state, key);
    }
    
    const bool control_pressed = ui_state_is_key_pressed(&state, KEY_LEFT_CONTROL) ||
        ui_state_is_key_pressed(&state, KEY_RIGHT_CONTROL) ||
        ui_state_is_key_pressed(&state, KEY_LEFT_SUPER) ||
        ui_state_is_key_pressed(&state, KEY_RIGHT_SUPER);
    if (control_pressed && ui_state_is_key_pressed(&state, KEY_Q)) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

static void mouse_button(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            state.left_mouse_press = true;
            state.left_mouse_is_pressed = true;
            script_window_mouse_down(0);
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            state.right_mouse_press = true;
            state.right_mouse_is_pressed = true;
            script_window_mouse_down(1);
        } else {
            state.middle_mouse_press = true;
            state.middle_mouse_is_pressed = true;
            script_window_mouse_down(2);
        }
    } else if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            state.left_mouse_release = true;
            state.left_mouse_is_pressed = false;
            script_window_mouse_up(0);
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            state.right_mouse_release = true;
            state.right_mouse_is_pressed = false;
            script_window_mouse_up(1);
        } else {
            state.middle_mouse_release = true;
            state.middle_mouse_is_pressed = false;
            script_window_mouse_up(2);
        }
    }
}

static void resize_callback(GLFWwindow *window, int width, int height) {
    script_context_t *ctx = script_context_share();
    glfwGetFramebufferSize(window, &ctx->framebuffer_width, &ctx->framebuffer_height);
    ctx->display_ratio = (f64)ctx->framebuffer_height / (f64)ctx->height;
    f32 ui_width = (f32)width / ctx->ui_scale * ctx->display_ratio;
    f32 ui_height = (f32)height / ctx->ui_scale * ctx->display_ratio;
    renderer.window_size.x = ui_width;
    renderer.window_size.y = ui_height;
    state.window_rect = (ui_rect){
        .x = 0.f,
        .y = 0.f,
        .w = ui_width,
        .h = ui_height
    };
    script_window_resize(width, height);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (state.active == -1 && state.hover == -1) script_window_mouse_scroll(xoffset, yoffset);
}

static void script_init(GLFWwindow *window, ustring_view uri) {
    script_context_cleanup();
    script_context_t *ctx = script_context_share();
    ctx->window = window;
    script_module_browser_register();
    script_module_webgl2_register();
    ustring content;
    url_t url = url_parse(uri);
    if (url.valid) {
        printf("protocol: %s\nhost: %s\nport: %d\npath: %s\n", url.protocol.data, url.host.data, url.port, url.path.data);
        content = io_http_get(url);
    } else {
        printf("load file: %s\n", uri.base.data);
        content = io_read_file(uri);
    }
    if (content.length > 0) {
        script_eval(content, uri);
        ustring_free(&content);
        empty_launch = false;
    }

    glfwGetWindowSize(window, &ctx->width, &ctx->height);
    resize_callback(window, ctx->width, ctx->height);
}

static void renderer_init(GLFWwindow* window, ustring_view uri) {
    ui_renderer_init(&renderer);
    ui_state_init(&state, &renderer);
    ui_input_init(&source_input, uri);
    ui_label_init(&copyright, ustring_view_STR("@2023 union native"));
    copyright.element.constraint.alignment = CENTER;
    copyright.scale = 0.5;

    fps_str.base.data = malloc(MAX_FPX_BITS);
    memset((void*)fps_str.base.data, 0, MAX_FPX_BITS);
    fps_str.length = MAX_FPX_BITS;
    ui_label_init(&fps_label, fps_str);
    fps_label.element.constraint.alignment = BOTTOM | RIGHT;
    fps_label.element.constraint.margin.right = 32.f;
    fps_label.element.constraint.margin.bottom = 10.f;
    fps_label.scale = 0.7f;

    status_str.base.data = malloc(MAX_STATUS_BITS);
    memset((void*)status_str.base.data, 0, MAX_STATUS_BITS);
    status_str.length = MAX_STATUS_BITS;
    ui_label_init(&status_label, status_str);
    status_label.element.constraint.alignment = BOTTOM | LEFT;
    status_label.element.constraint.margin.left = 10.f;
    status_label.element.constraint.margin.bottom = 10.f;
    status_label.scale = 0.7f;

    default_cursor = glfwCreateStandardCursor(CURSOR_Default);
    text_input_cursor = glfwCreateStandardCursor(CURSOR_Text);
    resize_hori_cursor = glfwCreateStandardCursor(CURSOR_ResizeHori);
    resize_vert_cursor = glfwCreateStandardCursor(CURSOR_ResizeVert);
}

static void ui_render(GLFWwindow *window) {
    state.cursor_type = CURSOR_Default;
    ui_rect rect = ui_rect_shrink((ui_rect){.x = 0, .y = 0, .w = state.window_rect.w, .h = 46.f}, 8.0f, 8.0f);
    if (ui_input(&state, &source_input, ui_theme_share()->panel_0, rect, 0, 0)) {
        printf("source_input: %s\n", source_input.label.text.base.data);
        script_init(window, source_input.label.text);
    }

    rect = ui_rect_shrink((ui_rect){.x = 0, .y = state.window_rect.h - 22.f, .w = state.window_rect.w, .h = 22.f}, 8.0f, 8.0f);
    ui_label(&state, &copyright, ui_theme_share()->text, rect, 0, 0);
    ui_renderer_render(&renderer);

    ui_label(&state, &fps_label, ui_theme_share()->transform_y, state.window_rect, 0, 0);
    ui_label(&state, &status_label, ui_theme_share()->text, state.window_rect, 0, 0);
}

#define FPS_MA 10
static double last_time[FPS_MA];
static int nb_frames = 0;
static void state_update(GLFWwindow *window) {
    int width, height, framebuffer_width, framebuffer_height;
    double mouse_x, mouse_y;

    script_context_t *ctx = script_context_share();
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    mouse_x = mouse_x * ctx->display_ratio;
    mouse_y = mouse_y * ctx->display_ratio;

    if (state.mouse_location.x != mouse_x || state.mouse_location.y != mouse_y) {
        if (state.active == -1 && state.hover == -1) script_window_mouse_move(mouse_x, mouse_y);
        state.mouse_location = (float2){.x = (f32)mouse_x / ctx->ui_scale, .y = (f32)mouse_y / ctx->ui_scale};
    }
 
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    if (ctx->framebuffer_width != framebuffer_width || ctx->framebuffer_height != framebuffer_height) {
        glfwGetWindowSize(window, &width, &height);
        resize_callback(window, width, height);
    }

    ui_state_update(&state);

    // set cursor
    switch (state.cursor_type)
    {
    case CURSOR_Default:
        glfwSetCursor(window, default_cursor);
        break;
        case CURSOR_Text:
        glfwSetCursor(window, text_input_cursor);
        break;
            case CURSOR_ResizeHori:
        glfwSetCursor(window, resize_hori_cursor);
        break;
            case CURSOR_ResizeVert:
        glfwSetCursor(window, resize_vert_cursor);
        break;
    default:
        break;
    }

    // compute fps
    double current_time = glfwGetTime();
    double delta_time = current_time - last_time[nb_frames % FPS_MA];
    last_time[nb_frames % FPS_MA] = current_time;
    nb_frames++;

    if (nb_frames > FPS_MA) {
        double fps = FPS_MA / (current_time - last_time[(nb_frames - FPS_MA) % FPS_MA]);
        sprintf((void*)fps_str.base.data, "%16.f", fps);
        ui_label_update_text(&fps_label, fps_str);
    }

    // update status label
    sprintf((void*)status_str.base.data, "h: %d, a: %d, f: %d, l: %d", state.hover, state.active, state.focus, state.left_mouse_is_pressed);
    ui_label_update_text(&status_label, status_str);
}

int main(int argc, char** argv) {
    GLFWwindow* window;
    glfwSetErrorCallback(error_callback);

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    if (!glfwInit())
        exit(EXIT_FAILURE);

#if defined(OS_MACOS) || defined(OS_WINDOWS)
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#elif defined(OS_LINUX)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    int window_width = 1080;
    int window_height = 720;

    window = glfwCreateWindow(window_width, window_height, "union_native", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetScrollCallback(window, scroll_callback);
    glfwSetWindowSizeCallback(window, resize_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button);
    glfwMakeContextCurrent(window);

    printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
    printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));

    logger_init();
    ustring_view uri = argc >= 2 ? ustring_view_STR(argv[1]) : ustring_view_STR("public/index.js");
    renderer_init(window, uri);
    script_init(window, uri);

    glfwSwapInterval(1);
    glFrontFace(GL_CCW);
    glDepthRangef(0.0f, 1.0f);

    while (!glfwWindowShouldClose(window))
    {
        if (empty_launch) {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        } else {
            script_frame_tick();
        }

        ui_render(window);
        state_update(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    script_context_destroy();
    ui_renderer_free(&renderer);

    glfwDestroyWindow(window);
    glfwTerminate();
    uv_loop_close(uv_default_loop());
    return 0;
}
