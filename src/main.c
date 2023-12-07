#include "foundation/io.h"
#include "foundation/network.h"
#include "foundation/script.h"
#include "foundation/logger.h"
#include "foundation/webgl2.h"

#include <string.h>

#define GLFW_INCLUDE_ES3
#include <GLFW/glfw3.h>
#include <GLES3/gl3.h>
#include <stdlib.h>
#include <stdio.h>

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}
 
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void size_callback(GLFWwindow* window, int width, int height)
{
    script_window_resize(script_context_share(), width, height);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    printf("scroll_callback: %f, %f\n", xoffset, yoffset);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: un /path/to/sample.js\n");
        return 0;
    }

    GLFWwindow* window;
    glfwSetErrorCallback(error_callback);
 
    if (!glfwInit())
        exit(EXIT_FAILURE);

    printf("GLFW platform: %d\n", glfwGetPlatform());

#if defined(OS_MACOS)
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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(1080, 720, "union_native", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetScrollCallback(window, scroll_callback);
    glfwSetWindowSizeCallback(window, size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);

    printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
    printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));

    logger_init();

    script_context_t *script_context = script_context_share();
    int width, height, left, top, right, bottom;
    glfwGetFramebufferSize(window, &width, &height);
    script_module_browser_register(script_context);
    script_module_webgl2_register(script_context);
    glfwGetWindowFrameSize(window, &left, &top, &right, &bottom);
    script_window_resize(script_context, width - left - right, height - top - bottom);

    ustring_t content;
    ustring_t source = ustring_str(argv[1]);
    url_t url = url_parse(source);
    if (url.valid) {
        printf("protocol: %s\n host: %s port: %d, path: %s", url.protocol.data, url.host.data, url.port, url.path.data);
        content = io_http_get(url);
    } else {
        content = io_read_file(source);
    }
    script_eval(script_context, content, source);

    glfwSwapInterval(1);
    while (!glfwWindowShouldClose(window))
    {
        script_frame_tick(script_context);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    script_context_destroy(script_context);
 
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}