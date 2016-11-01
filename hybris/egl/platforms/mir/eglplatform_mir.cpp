
#include <mir_toolkit/mir_client_library.h>
#include <mir_toolkit/client_types_nbs.h>
#include <mir_toolkit/mir_render_surface.h>
#include <android-config.h>
#include <ws.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <hybris/common/binding.h>
#include <stdlib.h>
#include "logging.h"

extern "C" {
#include <eglplatformcommon.h>
}

static gralloc_module_t *gralloc = 0;
static alloc_device_t *alloc = 0;

struct MirDisplay : _EGLDisplay
{
    MirDisplay(MirConnection* con)
    {
        dpy = EGL_NO_DISPLAY;
        connection = con;
    }
    MirConnection* connection;
};

extern "C" void mir_init_module(struct ws_egl_interface *egl_iface)
{
	int err;
	hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **) &gralloc);
	err = gralloc_open((const hw_module_t *) gralloc, &alloc);
	TRACE("++ %lu wayland: got gralloc %p err:%s", pthread_self(), gralloc, strerror(-err));
	eglplatformcommon_init(egl_iface, gralloc, alloc);
}

extern "C" _EGLDisplay *mir_GetDisplay(EGLNativeDisplayType display)
{
	return new MirDisplay((MirConnection*) display);
}

extern "C" void mir_Terminate(_EGLDisplay *dpy)
{
	delete dpy;
}

struct MirNativeWindowType : _EGLNativeWindowType
{
    MirBufferStream* stream;
    MirRenderSurface * rs;
    int width;
    int height;
};

extern "C" struct _EGLNativeWindowType *mir_CreateWindow(EGLNativeWindowType win, _EGLDisplay *disp, EGLConfig config)
{
    int width = -1;
    int height = -1;
    MirPixelFormat format = mir_pixel_format_abgr_8888;
    MirDisplay* display = (MirDisplay*) disp;
    MirRenderSurface* rs = (MirRenderSurface*) win;
    MirBufferStream* stream = NULL;

    MirNativeWindowType* t = new MirNativeWindowType;
    mir_render_surface_logical_size(rs, &t->width, &t->height);
    format = mir_connection_get_egl_pixel_format(display->connection, display->dpy, config);

    stream = mir_render_surface_create_buffer_stream_sync(
        rs, t->width, t->height, format, mir_buffer_usage_hardware);
  
    t->win = (EGLNativeWindowType) mir_buffer_stream_get_egl_native_window(stream);
    t->stream = stream;
    t->rs = rs;
    printf("REALLY TRULY\n");
    return t;
}

extern "C" void mir_DestroyWindow(struct _EGLNativeWindowType * t)
{
    delete t;
}

extern "C" void mir_prepareSwap(
    EGLDisplay dpy, _EGLNativeWindowType* win, EGLint *damage_rects, EGLint damage_n_rects)
{
    MirNativeWindowType* type = (MirNativeWindowType*)win;
    int width = -1;
    int height = -1;
    mir_render_surface_logical_size(type->rs, &width, &height);
    if (width != type->width || height != type->height)
    {
        mir_buffer_stream_set_size(type->stream, width, height);
        type->width = width;
        type->height = height;
    }
}

struct ws_module ws_module_info = {
	mir_init_module,
	mir_GetDisplay,
	mir_Terminate,
	mir_CreateWindow,
	mir_DestroyWindow,
	eglplatformcommon_eglGetProcAddress,
	eglplatformcommon_passthroughImageKHR,
	eglplatformcommon_eglQueryString,
    mir_prepareSwap,
    NULL,
    NULL
};
