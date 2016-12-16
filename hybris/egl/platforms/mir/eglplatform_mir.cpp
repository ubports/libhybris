
#define ANDROID
#include <EGL/eglplatform.h>
#include <mir_toolkit/mir_extension_core.h>
#include <mir_toolkit/extensions/android_egl.h>
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
    MirDisplay(MirConnection* con, MirExtensionAndroidEGL* ext) :
        connection(con),
        ext(ext)
    {
        dpy = to_native_display_type(connection);
    }

    EGLNativeDisplayType to_native_display_type(MirConnection* connection)
    {
        return ext->to_display(connection); 
    }

    ANativeWindow* create_window(MirBufferStream* bs)
    {
        return ext->create_window(bs);
    } 

    void destroy_window(ANativeWindow* window)
    {
        ext->destroy_window(window);
    }

    ANativeWindowBuffer* create_buffer(MirBuffer* buffer)
    {
        return ext->create_buffer(buffer);
    }

    void destroy_buffer(ANativeWindowBuffer* buffer)
    {
        return ext->destroy_buffer(buffer);
    }

    MirConnection* connection;
private:
    MirExtensionAndroidEGL* ext;
};

struct ErrorDisplay : _EGLDisplay
{
    ErrorDisplay()
    {
        dpy = EGL_NO_DISPLAY;
    }
};
ErrorDisplay const error_display;

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
    MirConnection* connection = static_cast<MirConnection*>(display);
    MirExtensionAndroidEGL* ext = static_cast<MirExtensionAndroidEGL*>(
        mir_connection_request_interface(
            connection,
            MIR_EXTENSION_ANDROID_EGL,
            MIR_EXTENSION_ANDROID_EGL_VERSION_0_1));

    if (!ext)
    {
        HYBRIS_ERROR("could not access android extensions from mir library");
        return const_cast<ErrorDisplay*>(&error_display);
    }
    return new MirDisplay(connection, ext);
}

extern "C" void mir_Terminate(_EGLDisplay *dpy)
{
	delete dpy;
}

struct MirNativeWindowType : _EGLNativeWindowType
{
    MirDisplay* display;
    MirBufferStream* stream;
    MirRenderSurface * rs;
    int width;
    int height;
};

extern "C" struct _EGLNativeWindowType *mir_CreateWindow(EGLNativeWindowType win, _EGLDisplay *disp, EGLConfig config)
{
    MirDisplay* display = (MirDisplay*) disp;

    MirNativeWindowType* t = new MirNativeWindowType;
    t->display = display;
    t->rs = (MirRenderSurface*) win;
    mir_render_surface_get_size(t->rs, &t->width, &t->height);
    MirPixelFormat format = mir_connection_get_egl_pixel_format(
        t->display->connection, display->dpy, config);
    t->stream = mir_render_surface_create_buffer_stream_sync(
        t->rs, t->width, t->height, format, mir_buffer_usage_hardware);
    t->win = display->create_window(t->stream);
    return t;
}

extern "C" void mir_DestroyWindow(struct _EGLNativeWindowType * t)
{
    MirNativeWindowType* native_win = (MirNativeWindowType*) t;
    native_win->display->destroy_window(native_win->win);
    delete native_win;
}

extern "C" void mir_prepareSwap(
    EGLDisplay dpy, _EGLNativeWindowType* win, EGLint *damage_rects, EGLint damage_n_rects)
{
    MirNativeWindowType* type = (MirNativeWindowType*)win;
    int width = -1;
    int height = -1;
    mir_render_surface_get_size(type->rs, &width, &height);
    if (width != type->width || height != type->height)
    {
        mir_buffer_stream_set_size(type->stream, width, height);
        type->width = width;
        type->height = height;
    }
}

extern "C" void mir_setSwapInterval(
    EGLDisplay dpy, struct _EGLNativeWindowType* win, EGLint interval)
{
    MirNativeWindowType* type = (MirNativeWindowType*)win;
    mir_buffer_stream_set_swapinterval(type->stream, interval); 
}

extern "C" void
mir_passthroughImageKHR(_EGLDisplay* disp, EGLContext *ctx,
    EGLenum *target, EGLClientBuffer *b, const EGLint **attrib_list)
{
    MirDisplay* display = (MirDisplay*) disp;
    MirBuffer* buffer = (MirBuffer*) *b;
    *b = display->create_buffer(buffer);
    *target = EGL_NATIVE_BUFFER_ANDROID;
}

struct ws_module ws_module_info = {
	mir_init_module,
	mir_GetDisplay,
	mir_Terminate,
	mir_CreateWindow,
	mir_DestroyWindow,
	eglplatformcommon_eglGetProcAddress,
	mir_passthroughImageKHR,
	eglplatformcommon_eglQueryString,
    mir_prepareSwap,
    NULL,
    mir_setSwapInterval
};
