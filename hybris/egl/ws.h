#ifndef __LIBHYBRIS_WS_H
#define __LIBHYBRIS_WS_H
#include <EGL/egl.h>
#include <EGL/eglext.h>

struct ws_egl_interface {
	void * (*android_egl_dlsym)(const char *symbol);

	int (*has_mapping)(EGLSurface surface);
	struct _EGLNativeWindowType* (*get_mapping)(EGLSurface surface);
};

struct egl_image
{
    EGLImageKHR egl_image;
    EGLClientBuffer egl_buffer;
    EGLenum target;
};

/* Defined in egl.c */
extern struct ws_egl_interface hybris_egl_interface;

struct _EGLDisplay {
	EGLDisplay dpy;
};

struct _EGLNativeWindowType {
	EGLNativeWindowType win;
};

struct _EGLClientBuffer {
    EGLClientBuffer buf;
};

struct ws_module {
	void (*init_module)(struct ws_egl_interface *egl_interface);

	struct _EGLDisplay *(*GetDisplay)(EGLNativeDisplayType native);
	void (*Terminate)(struct _EGLDisplay *display);
	struct _EGLNativeWindowType *(*CreateWindow)(EGLNativeWindowType win, struct _EGLDisplay *display, EGLConfig config);
	void (*DestroyWindow)(struct _EGLNativeWindowType* win);
	__eglMustCastToProperFunctionPointerType (*eglGetProcAddress)(const char *procname);
	void (*passthroughImageKHR)(struct _EGLDisplay* dpy, EGLContext *ctx, EGLenum *target, EGLClientBuffer *buffer, const EGLint **attrib_list);
	const char *(*eglQueryString)(EGLDisplay dpy, EGLint name, const char *(*real_eglQueryString)(EGLDisplay dpy, EGLint name));
	void (*prepareSwap)(EGLDisplay dpy, struct _EGLNativeWindowType* win, EGLint *damage_rects, EGLint damage_n_rects);
	void (*finishSwap)(EGLDisplay dpy, struct _EGLNativeWindowType* win);
	void (*setSwapInterval)(EGLDisplay dpy, struct _EGLNativeWindowType* win, EGLint interval);
};

struct _EGLDisplay *ws_GetDisplay(EGLNativeDisplayType native);
void ws_Terminate(struct _EGLDisplay *dpy);
struct _EGLNativeWindowType *ws_CreateWindow(EGLNativeWindowType win, struct _EGLDisplay *display, EGLConfig);
void ws_DestroyWindow(struct _EGLNativeWindowType* win);
__eglMustCastToProperFunctionPointerType ws_eglGetProcAddress(const char *procname);
void ws_passthroughImageKHR(struct _EGLDisplay* dpy, EGLContext *ctx, EGLenum *target, EGLClientBuffer *buffer, const EGLint **attrib_list);
const char *ws_eglQueryString(EGLDisplay dpy, EGLint name, const char *(*real_eglQueryString)(EGLDisplay dpy, EGLint name));
void ws_prepareSwap(EGLDisplay dpy, struct _EGLNativeWindowType* win, EGLint *damage_rects, EGLint damage_n_rects);
void ws_finishSwap(EGLDisplay dpy, struct _EGLNativeWindowType* win);
void ws_setSwapInterval(EGLDisplay dpy, struct _EGLNativeWindowType* win, EGLint interval);

#endif
