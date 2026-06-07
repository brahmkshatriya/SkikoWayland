#include <algorithm>
#include <dlfcn.h>

#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>

#include "jni_helpers.h"

using GLFuncPtr = void (*)();

static GLFuncPtr getEGLGLProc(void*, const char* name) {
    void* proc = reinterpret_cast<void*>(eglGetProcAddress(name));
    if (proc == nullptr) {
        proc = dlsym(RTLD_DEFAULT, name);
    }
    return reinterpret_cast<GLFuncPtr>(proc);
}

class WaylandEGLContext {
public:
    struct wl_display* wlDisplay = nullptr;

    struct wl_surface* surface = nullptr;
    struct wl_egl_window* eglWindow = nullptr;

    EGLDisplay eglDisplay = EGL_NO_DISPLAY;
    EGLConfig eglConfig = nullptr;
    EGLContext eglContext = EGL_NO_CONTEXT;
    EGLSurface eglSurface = EGL_NO_SURFACE;

    int width = 1;
    int height = 1;
    int bufferWidth = 1;
    int bufferHeight = 1;
    ~WaylandEGLContext() {
        if (eglDisplay != EGL_NO_DISPLAY) {
            eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (eglSurface != EGL_NO_SURFACE) {
                eglDestroySurface(eglDisplay, eglSurface);
            }
            if (eglContext != EGL_NO_CONTEXT) {
                eglDestroyContext(eglDisplay, eglContext);
            }
            eglTerminate(eglDisplay);
        }
        if (eglWindow != nullptr) {
            wl_egl_window_destroy(eglWindow);
        }
    }
};

static bool initWayland(WaylandEGLContext* context, struct wl_display* display) {
    context->wlDisplay = display;
    return context->wlDisplay != nullptr;
}

static bool initEGL(WaylandEGLContext* context, bool transparency) {
    context->eglDisplay = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(context->wlDisplay));
    if (context->eglDisplay == EGL_NO_DISPLAY) {
        return false;
    }

    EGLint major = 0;
    EGLint minor = 0;
    if (!eglInitialize(context->eglDisplay, &major, &minor)) {
        return false;
    }

    if (!eglBindAPI(EGL_OPENGL_API)) {
        return false;
    }

    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, transparency ? 8 : 0,
        EGL_DEPTH_SIZE, 0,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };

    EGLint numConfigs = 0;
    if (!eglChooseConfig(context->eglDisplay, configAttribs, &context->eglConfig, 1, &numConfigs) ||
        numConfigs == 0) {
        return false;
    }

    const EGLint contextAttribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 0,
        EGL_NONE
    };

    context->eglContext = eglCreateContext(context->eglDisplay, context->eglConfig,
                                           EGL_NO_CONTEXT, contextAttribs);
    return context->eglContext != EGL_NO_CONTEXT;
}

static bool ensureSurface(WaylandEGLContext* context,
                          struct wl_surface* surface,
                          int width,
                          int height,
                          int bufferWidth,
                          int bufferHeight) {
    width = std::max(width, 1);
    height = std::max(height, 1);
    bufferWidth = std::max(bufferWidth, 1);
    bufferHeight = std::max(bufferHeight, 1);

    if (context->surface != surface) {
        if (context->eglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(context->eglDisplay, context->eglSurface);
            context->eglSurface = EGL_NO_SURFACE;
        }
        if (context->eglWindow != nullptr) {
            wl_egl_window_destroy(context->eglWindow);
            context->eglWindow = nullptr;
        }

        context->surface = surface;
        wl_surface_set_buffer_scale(context->surface, 1);
        context->eglWindow = wl_egl_window_create(context->surface, bufferWidth, bufferHeight);
        if (context->eglWindow == nullptr) {
            return false;
        }
        context->eglSurface = eglCreateWindowSurface(context->eglDisplay,
                                                     context->eglConfig,
                                                     reinterpret_cast<EGLNativeWindowType>(context->eglWindow),
                                                     nullptr);
        if (context->eglSurface == EGL_NO_SURFACE) {
            return false;
        }
    } else if (context->bufferWidth != bufferWidth || context->bufferHeight != bufferHeight) {
        wl_egl_window_resize(context->eglWindow, bufferWidth, bufferHeight, 0, 0);
    }

    context->width = width;
    context->height = height;
    context->bufferWidth = bufferWidth;
    context->bufferHeight = bufferHeight;

    wl_display_flush(context->wlDisplay);
    return true;
}

extern "C" {

JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_context_WaylandOpenGLContextHandlerKt_getEGLGetProcAddressFunc(
    JNIEnv* env,
    jobject redrawer) {
    return toJavaPointer(getEGLGLProc);
}

JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_redrawer_LinuxWaylandEGLRedrawerKt_createContext(
    JNIEnv* env,
    jobject redrawer,
    jlong displayPtr,
    jlong parentSurfacePtr,
    jint width,
    jint height,
    jint bufferWidth,
    jint bufferHeight,
    jboolean transparency) {
    wl_display* display = fromJavaPointer<wl_display*>(displayPtr);
    wl_surface* parentSurface = fromJavaPointer<wl_surface*>(parentSurfacePtr);
    if (display == nullptr || parentSurface == nullptr) {
        return 0;
    }

    WaylandEGLContext* context = new WaylandEGLContext();
    if (!initWayland(context, display) ||
        !initEGL(context, transparency == JNI_TRUE) ||
        !ensureSurface(context, parentSurface, width, height, bufferWidth, bufferHeight)) {
        delete context;
        return 0;
    }

    return toJavaPointer(context);
}

JNIEXPORT void JNICALL Java_org_jetbrains_skiko_redrawer_LinuxWaylandEGLRedrawerKt_updateSurface(
    JNIEnv* env,
    jobject redrawer,
    jlong contextPtr,
    jlong parentSurfacePtr,
    jint width,
    jint height,
    jint bufferWidth,
    jint bufferHeight) {
    WaylandEGLContext* context = fromJavaPointer<WaylandEGLContext*>(contextPtr);
    wl_surface* parentSurface = fromJavaPointer<wl_surface*>(parentSurfacePtr);
    if (context == nullptr || parentSurface == nullptr) {
        return;
    }
    ensureSurface(context, parentSurface, width, height, bufferWidth, bufferHeight);
}

JNIEXPORT void JNICALL Java_org_jetbrains_skiko_redrawer_LinuxWaylandEGLRedrawerKt_makeCurrent(
    JNIEnv* env,
    jobject redrawer,
    jlong contextPtr) {
    WaylandEGLContext* context = fromJavaPointer<WaylandEGLContext*>(contextPtr);
    if (context == nullptr) {
        return;
    }
    eglMakeCurrent(context->eglDisplay, context->eglSurface, context->eglSurface, context->eglContext);
}

JNIEXPORT void JNICALL Java_org_jetbrains_skiko_redrawer_LinuxWaylandEGLRedrawerKt_destroyContext(
    JNIEnv* env,
    jobject redrawer,
    jlong contextPtr) {
    WaylandEGLContext* context = fromJavaPointer<WaylandEGLContext*>(contextPtr);
    delete context;
}

JNIEXPORT void JNICALL Java_org_jetbrains_skiko_redrawer_LinuxWaylandEGLRedrawerKt_setSwapInterval(
    JNIEnv* env,
    jobject redrawer,
    jlong contextPtr,
    jint interval) {
    WaylandEGLContext* context = fromJavaPointer<WaylandEGLContext*>(contextPtr);
    if (context == nullptr) {
        return;
    }
    eglSwapInterval(context->eglDisplay, interval);
}

JNIEXPORT void JNICALL Java_org_jetbrains_skiko_redrawer_LinuxWaylandEGLRedrawerKt_swapBuffers(
    JNIEnv* env,
    jobject redrawer,
    jlong contextPtr) {
    WaylandEGLContext* context = fromJavaPointer<WaylandEGLContext*>(contextPtr);
    if (context == nullptr) {
        return;
    }
    eglSwapBuffers(context->eglDisplay, context->eglSurface);
    wl_display_flush(context->wlDisplay);
}

}
