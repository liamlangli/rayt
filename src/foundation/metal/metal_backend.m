#include "metal_backend.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>


typedef struct metal_device_t {
    id<MTLDevice> gpu;
    id<MTLCommandQueue> queue;
    CAMetalLayer *swapchain;
} metal_device_t;
static metal_device_t *g_metal_device;

typedef struct metal_library_t {
    id<MTLLibrary> library;
} metal_library_t;

metal_device_t* metal_create_default_device(void) {
    const id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    const id<MTLCommandQueue> queue = [device newCommandQueue];
    CAMetalLayer *swapchain = [CAMetalLayer layer];
    swapchain.device = device;
    swapchain.opaque = YES;

    metal_device_t *metal_device = (metal_device_t*)malloc(sizeof(metal_device_t));
    metal_device->gpu = device;
    metal_device->queue = queue;
    metal_device->swapchain = swapchain;
    g_metal_device = metal_device;
    return metal_device;
}

GLFWwindow* metal_create_window(const char* title, rect_t rect) {
    GLFWwindow *native_window = glfwCreateWindow((i32)rect.w, (i32)rect.h, title, NULL, NULL);
    NSWindow *nswindow = glfwGetCocoaWindow(native_window);
    nswindow.contentView.layer = g_metal_device->swapchain;
    nswindow.contentView.wantsLayer = YES;
    return native_window;
}

metal_library_t* metal_create_library_from_source(metal_device_t* device, const char* source)
{
    id<MTLDevice> gpu = device->gpu;
    NSError *error = NULL;
    id<MTLLibrary> library = [gpu newLibraryWithSource:[NSString stringWithUTF8String:source] options:nil error:&error];
    if (error) {
        printf("Error creating library: %s\n", [[error localizedDescription] UTF8String]);
        return NULL;
    }

    metal_library_t *metal_library = (metal_library_t*)malloc(sizeof(metal_library_t));
    metal_library->library = library;
    return metal_library;
}

void metal_present(void) {
    @autoreleasepool {
        id<CAMetalDrawable> surface = [g_metal_device->swapchain nextDrawable];

        MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].clearColor = MTLClearColorMake(1, 0, 0, 1);
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass.colorAttachments[0].texture = surface.texture;

        id<MTLCommandBuffer> buffer = [g_metal_device->queue commandBuffer];
        id<MTLRenderCommandEncoder> encoder = [buffer renderCommandEncoderWithDescriptor:pass];
        [encoder endEncoding];
        [buffer presentDrawable:surface];
        [buffer commit];
    }
}
