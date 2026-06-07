#include <jawt_md.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <cstdlib>
#include <unistd.h>
#include <stdio.h>
#include "jni_helpers.h"

struct wl_display;
struct wl_surface;

typedef struct jawt_WaylandDrawingSurfaceInfo {
    struct wl_display *display;
    struct wl_surface *parentSurface;
    int x;
    int y;
    int width;
    int height;
    int scale;
    double effectiveScale;
    int javaX;
    int javaY;
    int javaWidth;
    int javaHeight;
} JAWT_WaylandDrawingSurfaceInfo;

extern "C"
{
    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_AWTLinuxDrawingSurfaceKt_getDisplay(JNIEnv *env, jobject redrawer, jlong platformInfoPtr)
    {
        JAWT_X11DrawingSurfaceInfo *dsi_x11 = fromJavaPointer<JAWT_X11DrawingSurfaceInfo *>(platformInfoPtr);
        Display *display = dsi_x11->display;
        return toJavaPointer(display);
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_AWTLinuxDrawingSurfaceKt_getWindow(JNIEnv *env, jobject redrawer, jlong platformInfoPtr)
    {
        JAWT_X11DrawingSurfaceInfo *dsi_x11 = fromJavaPointer<JAWT_X11DrawingSurfaceInfo *>(platformInfoPtr);
        Window window = dsi_x11->drawable;
        return toJavaPointer(window);
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_AWTLinuxDrawingSurfaceKt_getWaylandDisplay(JNIEnv *env, jobject redrawer, jlong platformInfoPtr)
    {
        JAWT_WaylandDrawingSurfaceInfo *dsi_wl = fromJavaPointer<JAWT_WaylandDrawingSurfaceInfo *>(platformInfoPtr);
        return toJavaPointer(dsi_wl->display);
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_AWTLinuxDrawingSurfaceKt_getWaylandParentSurface(JNIEnv *env, jobject redrawer, jlong platformInfoPtr)
    {
        JAWT_WaylandDrawingSurfaceInfo *dsi_wl = fromJavaPointer<JAWT_WaylandDrawingSurfaceInfo *>(platformInfoPtr);
        return toJavaPointer(dsi_wl->parentSurface);
    }

    JNIEXPORT jint JNICALL Java_org_jetbrains_skiko_AWTLinuxDrawingSurfaceKt_getWaylandX(JNIEnv *env, jobject redrawer, jlong platformInfoPtr)
    {
        JAWT_WaylandDrawingSurfaceInfo *dsi_wl = fromJavaPointer<JAWT_WaylandDrawingSurfaceInfo *>(platformInfoPtr);
        return dsi_wl->x;
    }

    JNIEXPORT jint JNICALL Java_org_jetbrains_skiko_AWTLinuxDrawingSurfaceKt_getWaylandY(JNIEnv *env, jobject redrawer, jlong platformInfoPtr)
    {
        JAWT_WaylandDrawingSurfaceInfo *dsi_wl = fromJavaPointer<JAWT_WaylandDrawingSurfaceInfo *>(platformInfoPtr);
        return dsi_wl->y;
    }

    JNIEXPORT jint JNICALL Java_org_jetbrains_skiko_AWTLinuxDrawingSurfaceKt_getWaylandWidth(JNIEnv *env, jobject redrawer, jlong platformInfoPtr)
    {
        JAWT_WaylandDrawingSurfaceInfo *dsi_wl = fromJavaPointer<JAWT_WaylandDrawingSurfaceInfo *>(platformInfoPtr);
        return dsi_wl->width;
    }

    JNIEXPORT jint JNICALL Java_org_jetbrains_skiko_AWTLinuxDrawingSurfaceKt_getWaylandHeight(JNIEnv *env, jobject redrawer, jlong platformInfoPtr)
    {
        JAWT_WaylandDrawingSurfaceInfo *dsi_wl = fromJavaPointer<JAWT_WaylandDrawingSurfaceInfo *>(platformInfoPtr);
        return dsi_wl->height;
    }

    JNIEXPORT jint JNICALL Java_org_jetbrains_skiko_AWTLinuxDrawingSurfaceKt_getWaylandScale(JNIEnv *env, jobject redrawer, jlong platformInfoPtr)
    {
        JAWT_WaylandDrawingSurfaceInfo *dsi_wl = fromJavaPointer<JAWT_WaylandDrawingSurfaceInfo *>(platformInfoPtr);
        return dsi_wl->scale;
    }

    JNIEXPORT jdouble JNICALL Java_org_jetbrains_skiko_AWTLinuxDrawingSurfaceKt_getWaylandEffectiveScale(JNIEnv *env, jobject redrawer, jlong platformInfoPtr)
    {
        JAWT_WaylandDrawingSurfaceInfo *dsi_wl = fromJavaPointer<JAWT_WaylandDrawingSurfaceInfo *>(platformInfoPtr);
        return dsi_wl->effectiveScale;
    }

    JNIEXPORT jint JNICALL Java_org_jetbrains_skiko_AWTLinuxDrawingSurfaceKt_getWaylandJavaX(JNIEnv *env, jobject redrawer, jlong platformInfoPtr)
    {
        JAWT_WaylandDrawingSurfaceInfo *dsi_wl = fromJavaPointer<JAWT_WaylandDrawingSurfaceInfo *>(platformInfoPtr);
        return dsi_wl->javaX;
    }

    JNIEXPORT jint JNICALL Java_org_jetbrains_skiko_AWTLinuxDrawingSurfaceKt_getWaylandJavaY(JNIEnv *env, jobject redrawer, jlong platformInfoPtr)
    {
        JAWT_WaylandDrawingSurfaceInfo *dsi_wl = fromJavaPointer<JAWT_WaylandDrawingSurfaceInfo *>(platformInfoPtr);
        return dsi_wl->javaY;
    }

    JNIEXPORT jint JNICALL Java_org_jetbrains_skiko_AWTLinuxDrawingSurfaceKt_getWaylandJavaWidth(JNIEnv *env, jobject redrawer, jlong platformInfoPtr)
    {
        JAWT_WaylandDrawingSurfaceInfo *dsi_wl = fromJavaPointer<JAWT_WaylandDrawingSurfaceInfo *>(platformInfoPtr);
        return dsi_wl->javaWidth;
    }

    JNIEXPORT jint JNICALL Java_org_jetbrains_skiko_AWTLinuxDrawingSurfaceKt_getWaylandJavaHeight(JNIEnv *env, jobject redrawer, jlong platformInfoPtr)
    {
        JAWT_WaylandDrawingSurfaceInfo *dsi_wl = fromJavaPointer<JAWT_WaylandDrawingSurfaceInfo *>(platformInfoPtr);
        return dsi_wl->javaHeight;
    }

}
