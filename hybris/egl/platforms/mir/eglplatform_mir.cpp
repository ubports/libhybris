/*
 * Copyright (C) 2017 Canonical Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

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
static unsigned int const usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER;

struct MirNativeWindowBase : _EGLNativeWindowType
{
    virtual ~MirNativeWindowBase() {};
    virtual void prepareSwap() = 0;
    virtual void setSwapInterval(EGLint interval) = 0;
protected:
    MirNativeWindowBase() {};
private:
    MirNativeWindowBase(MirNativeWindowBase const&);
    MirNativeWindowBase& operator=(MirNativeWindowBase const&);
};

struct MirBaseDisplay : _EGLDisplay
{
    MirBaseDisplay(EGLDisplay display)
    {
        dpy = display;
    }

    virtual struct _EGLNativeWindowType* create_window(
        EGLNativeWindowType win, EGLConfig config) = 0;
    virtual void destroy_window(ANativeWindow* window) = 0;
    virtual void passthrough_img(EGLContext *ctx, EGLenum *target, EGLClientBuffer *b, const EGLint **attrib_list) = 0;

protected:
    MirBaseDisplay() {}
private:
    MirBaseDisplay(MirBaseDisplay const&);
    MirBaseDisplay& operator=(MirBaseDisplay const&);
};


struct MirNativeWindowType : MirNativeWindowBase
{
    MirNativeWindowType(
        MirExtensionAndroidEGLV1 const* ext,
        MirRenderSurface* surface, EGLint format)
    {
        int width = -1;
        int height = -1;
        mir_render_surface_get_size(surface, &width, &height);
        win = ext->create_window(surface, width, height, format, usage);
        rs = surface;
    }

    ~MirNativeWindowType()
    {
        ext->destroy_window(win);
    }

    void prepareSwap()
    {
        int w = -1;
        int h = -1;
        mir_render_surface_get_size(rs, &w, &h);
        if (w != width || h != height)
        {
            native_window_set_buffers_dimensions(win, w, h);
            width = w;
            height = h;
        }
    }

    void setSwapInterval(EGLint interval)
    {
        ANativeWindow* anw = win;
        anw->setSwapInterval(anw, interval);
    }

    MirExtensionAndroidEGLV1 const* ext;
    MirRenderSurface * rs;
    int width;
    int height;
};

struct NullNativeWindowType : MirNativeWindowBase
{
    NullNativeWindowType(EGLNativeWindowType t)
    {
        win = t;
    }

    void prepareSwap()
    {
    }

    void setSwapInterval(EGLint interval)
    {
    }
};

struct MirDisplay : MirBaseDisplay
{
    MirDisplay(MirConnection* con, MirExtensionAndroidEGLV1 const* ext) :
        MirBaseDisplay(ext->to_display(connection)),
        connection(con),
        ext(ext)
    {
    }

    struct _EGLNativeWindowType* create_window(
        EGLNativeWindowType win, EGLConfig config)
    {
        EGLint format = 0;
        eglGetConfigAttrib(dpy, config, EGL_NATIVE_VISUAL_ID, &format);
        return new MirNativeWindowType(ext, (MirRenderSurface*) win, format);
    }

    void destroy_window(ANativeWindow* window)
    {
        ext->destroy_window(window);
    }

    void passthrough_img(EGLContext *ctx, EGLenum *target, EGLClientBuffer *b, const EGLint **attrib_list)
    {
        MirBuffer* buffer = (MirBuffer*) *b;
        *b = create_buffer(buffer);
        *target = EGL_NATIVE_BUFFER_ANDROID;
    }

    ANativeWindowBuffer* create_buffer(MirBuffer* buffer)
    {
        return ext->create_buffer(buffer);
    }

    void destroy_buffer(ANativeWindowBuffer* buffer)
    {
        return ext->destroy_buffer(buffer);
    }

private:
    MirConnection* connection;
    MirExtensionAndroidEGLV1 const* ext;
};

struct ErrorDisplay : MirBaseDisplay
{
    ErrorDisplay() :
        MirBaseDisplay(EGL_NO_DISPLAY)
    {
    }

    struct _EGLNativeWindowType* create_window(
        EGLNativeWindowType win, EGLConfig config)
    {
        return NULL;
    }

    void destroy_window(ANativeWindow* window)
    {
    }

    void passthrough_img(EGLContext *ctx, EGLenum *target, EGLClientBuffer *b, const EGLint **attrib_list)
    {
    }
};

static int const legacy_dpy = 12345;
struct NullDisplay : MirBaseDisplay
{
    NullDisplay() :
        MirBaseDisplay((EGLDisplay)&legacy_dpy)
    {
    }

    struct _EGLNativeWindowType* create_window(EGLNativeWindowType win, EGLConfig config)
    {
        return new NullNativeWindowType(win);
    }

    void destroy_window(ANativeWindow* window)
    {
    }

    void passthrough_img(EGLContext *ctx, EGLenum *target, EGLClientBuffer *b, const EGLint **attrib_list)
    {
    }
};

extern "C" void mir_init_module(struct ws_egl_interface *egl_iface)
{
    int err;
    hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **) &gralloc);
    err = gralloc_open((const hw_module_t *) gralloc, &alloc);
    TRACE("++ %lu mir: got gralloc %p err:%s", pthread_self(), gralloc, strerror(-err));
    eglplatformcommon_init(egl_iface, gralloc, alloc);
}

extern "C" _EGLDisplay *mir_GetDisplay(EGLNativeDisplayType display)
{
    if (display == EGL_DEFAULT_DISPLAY)
    {
        TRACE("++ %lu mir: using null mir platform", pthread_self());
        return new NullDisplay;
    }

    MirConnection* connection = static_cast<MirConnection*>(display);
    MirExtensionAndroidEGLV1 const* ext =  mir_extension_android_egl_v1(connection);
    if (!ext)
    {
        HYBRIS_ERROR("could not access android extensions from mir library");
        return new ErrorDisplay;
    }
    TRACE("++ %lu mir: using mir platform", pthread_self());
    return new MirDisplay(connection, ext);
}

extern "C" void mir_Terminate(_EGLDisplay *dpy)
{
    MirBaseDisplay* d = (MirBaseDisplay*) dpy;
    delete d;
}

extern "C" struct _EGLNativeWindowType *mir_CreateWindow(EGLNativeWindowType win, _EGLDisplay *dis, EGLConfig config)
{
    MirBaseDisplay* d = (MirBaseDisplay*) dis;
    return d->create_window(win, config);
}

extern "C" void mir_DestroyWindow(struct _EGLNativeWindowType * t)
{
    MirNativeWindowType* native_win = (MirNativeWindowType*) t;
    delete native_win;
}

extern "C" void mir_prepareSwap(
    EGLDisplay dpy, _EGLNativeWindowType* win, EGLint *damage_rects, EGLint damage_n_rects)
{
    MirNativeWindowType* type = (MirNativeWindowType*)win;
    type->prepareSwap();
}

extern "C" void mir_setSwapInterval(
    EGLDisplay dpy, struct _EGLNativeWindowType* win, EGLint interval)
{
    MirNativeWindowType* type = (MirNativeWindowType*)win;
    type->setSwapInterval(interval);
}

extern "C" void
mir_passthroughImageKHR(_EGLDisplay* disp, EGLContext *ctx,
    EGLenum *target, EGLClientBuffer *b, const EGLint **attrib_list)
{
    MirBaseDisplay* d = (MirBaseDisplay*) disp;
    d->passthrough_img(ctx, target, b, attrib_list);
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
