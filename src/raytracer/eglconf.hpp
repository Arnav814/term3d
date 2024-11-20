#ifndef EGLCONF_HPP
#define EGLCONF_HPP
#include <EGL/egl.h>

static const EGLint configAttribs[] = {
          EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
          EGL_BLUE_SIZE, 8,
          EGL_GREEN_SIZE, 8,
          EGL_RED_SIZE, 8,
          EGL_DEPTH_SIZE, 8,
          EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
          EGL_NONE
};    

static const int pbufferWidth = 10;
static const int pbufferHeight = 10;

static const EGLint pbattr[] = {
        EGL_WIDTH, pbufferWidth,
        EGL_HEIGHT, pbufferHeight,
        EGL_NONE,
};

#endif /* EGLCONF_HPP */
