// shims/legacy/Opengl32Shim.cpp
// Stub for opengl32. wglCreateContext returns a fake HGLRC; wglMakeCurrent
// returns TRUE. The GL function entry points (glClear, glBegin, glEnd, ...)
// are all no-ops. Games that try to use OpenGL will not render anything
// (they should be using the D3D11 bridge instead).

#include "UwpSdkIncludes.h"
#include <atomic>

#include "../ShimRegistry.h"

#ifndef GL_NO_ERROR
#define GL_NO_ERROR 0
#endif
#ifndef GL_TRUE
#define GL_TRUE 1
#endif
#ifndef GL_FALSE
#define GL_FALSE 0
#endif
#ifndef GL_POINTS
#define GL_POINTS 0x0000
#endif
#ifndef GL_LINES
#define GL_LINES 0x0001
#endif
#ifndef GL_LINE_LOOP
#define GL_LINE_LOOP 0x0002
#endif
#ifndef GL_LINE_STRIP
#define GL_LINE_STRIP 0x0003
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif
#ifndef GL_TRIANGLE_STRIP
#define GL_TRIANGLE_STRIP 0x0005
#endif
#ifndef GL_TRIANGLE_FAN
#define GL_TRIANGLE_FAN 0x0006
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
#ifndef GL_QUAD_STRIP
#define GL_QUAD_STRIP 0x0008
#endif
#ifndef GL_POLYGON
#define GL_POLYGON 0x0009
#endif
#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif
#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT 0x00000100
#endif
#ifndef GL_STENCIL_BUFFER_BIT
#define GL_STENCIL_BUFFER_BIT 0x00000400
#endif
#ifndef GL_MODELVIEW
#define GL_MODELVIEW 0x1700
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION 0x1701
#endif
#ifndef GL_TEXTURE
#define GL_TEXTURE 0x1702
#endif
#ifndef GL_FLOAT
#define GL_FLOAT 0x1406
#endif
#ifndef GL_DOUBLE
#define GL_DOUBLE 0x140A
#endif
#ifndef GL_INT
#define GL_INT 0x1404
#endif
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT 0x1405
#endif
#ifndef GL_BYTE
#define GL_BYTE 0x1400
#endif
#ifndef GL_UNSIGNED_BYTE
#define GL_UNSIGNED_BYTE 0x1401
#endif
#ifndef GL_SHORT
#define GL_SHORT 0x1402
#endif
#ifndef GL_UNSIGNED_SHORT
#define GL_UNSIGNED_SHORT 0x1403
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif
#ifndef WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#endif

#ifndef HGLRC
typedef DWORD HGLRC;
#endif
#ifndef GLubyte
typedef unsigned char GLubyte;
#endif

namespace xwr {

static std::atomic<uint32_t> g_hglrc{0x7000};

// ---------------------------------------------------------------------------
// wgl
// ---------------------------------------------------------------------------
extern "C" HGLRC __stdcall Shim_wglCreateContext(HDC) {
    return (HGLRC)g_hglrc.fetch_add(1);
}
extern "C" BOOL __stdcall Shim_wglMakeCurrent(HDC, HGLRC) { return TRUE; }
extern "C" BOOL __stdcall Shim_wglDeleteContext(HGLRC) { return TRUE; }
extern "C" HGLRC __stdcall Shim_wglGetCurrentContext() { return (HGLRC)0; }
extern "C" HDC __stdcall Shim_wglGetCurrentDC() { return (HDC)0; }
extern "C" PROC __stdcall Shim_wglGetProcAddress(LPCSTR) { return (PROC)nullptr; }
extern "C" BOOL __stdcall Shim_wglShareLists(HGLRC, HGLRC) { return TRUE; }
extern "C" BOOL __stdcall Shim_wglCopyContext(HGLRC, HGLRC, UINT) { return TRUE; }
extern "C" BOOL __stdcall Shim_wglChoosePixelFormat(HDC, const int*, const float*, UINT, int*) { return FALSE; }
extern "C" int  __stdcall Shim_wglDescribePixelFormat(HDC, int, UINT, void*) { return 0; }
extern "C" int  __stdcall Shim_wglGetPixelFormat(HDC) { return 0; }
extern "C" BOOL __stdcall Shim_wglSetPixelFormat(HDC, int, const void*) { return FALSE; }
extern "C" BOOL __stdcall Shim_wglSwapBuffers(HDC) { return TRUE; }
extern "C" BOOL __stdcall Shim_wglUseFontBitmapsW(HDC, DWORD, DWORD, DWORD) { return FALSE; }
extern "C" BOOL __stdcall Shim_wglUseFontOutlinesW(HDC, DWORD, DWORD, DWORD, float, float, int, void*) { return FALSE; }
extern "C" BOOL __stdcall Shim_wglMakeContextCurrentARB(HDC, HDC, HGLRC) { return TRUE; }
extern "C" HGLRC __stdcall Shim_wglCreateContextAttribsARB(HDC, HGLRC, const int*) {
    return (HGLRC)g_hglrc.fetch_add(1);
}

// ---------------------------------------------------------------------------
// GL functions — no-ops
// ---------------------------------------------------------------------------
extern "C" void __stdcall Shim_glClear(unsigned int) { }
extern "C" void __stdcall Shim_glClearColor(float, float, float, float) { }
extern "C" void __stdcall Shim_glClearDepth(double) { }
extern "C" void __stdcall Shim_glClearStencil(int) { }
extern "C" void __stdcall Shim_glEnable(unsigned int) { }
extern "C" void __stdcall Shim_glDisable(unsigned int) { }
extern "C" void __stdcall Shim_glEnableClientState(unsigned int) { }
extern "C" void __stdcall Shim_glDisableClientState(unsigned int) { }
extern "C" unsigned char __stdcall Shim_glIsEnabled(unsigned int) { return GL_FALSE; }
extern "C" void __stdcall Shim_glBegin(unsigned int) { }
extern "C" void __stdcall Shim_glEnd() { }
extern "C" void __stdcall Shim_glEndList() { }
extern "C" void __stdcall Shim_glNewList(unsigned int, unsigned int) { }
extern "C" void __stdcall Shim_glCallList(unsigned int) { }
extern "C" void __stdcall Shim_glDeleteLists(unsigned int, int) { }
extern "C" unsigned int __stdcall Shim_glGenLists(int) { return 0; }
extern "C" void __stdcall Shim_glVertex2f(float, float) { }
extern "C" void __stdcall Shim_glVertex3f(float, float, float) { }
extern "C" void __stdcall Shim_glVertex4f(float, float, float, float) { }
extern "C" void __stdcall Shim_glVertex2d(double, double) { }
extern "C" void __stdcall Shim_glVertex3d(double, double, double) { }
extern "C" void __stdcall Shim_glVertex2i(int, int) { }
extern "C" void __stdcall Shim_glVertex3i(int, int, int) { }
extern "C" void __stdcall Shim_glVertex2sv(const short*) { }
extern "C" void __stdcall Shim_glVertex3fv(const float*) { }
extern "C" void __stdcall Shim_glVertex3dv(const double*) { }
extern "C" void __stdcall Shim_glColor3f(float, float, float) { }
extern "C" void __stdcall Shim_glColor4f(float, float, float, float) { }
extern "C" void __stdcall Shim_glColor3ub(unsigned char, unsigned char, unsigned char) { }
extern "C" void __stdcall Shim_glColor4ub(unsigned char, unsigned char, unsigned char, unsigned char) { }
extern "C" void __stdcall Shim_glColor3fv(const float*) { }
extern "C" void __stdcall Shim_glColor4fv(const float*) { }
extern "C" void __stdcall Shim_glTexCoord2f(float, float) { }
extern "C" void __stdcall Shim_glTexCoord2fv(const float*) { }
extern "C" void __stdcall Shim_glNormal3f(float, float, float) { }
extern "C" void __stdcall Shim_glNormal3fv(const float*) { }
extern "C" void __stdcall Shim_glRectf(float, float, float, float) { }
extern "C" void __stdcall Shim_glRecti(int, int, int, int) { }
extern "C" void __stdcall Shim_glMatrixMode(unsigned int) { }
extern "C" void __stdcall Shim_glLoadIdentity() { }
extern "C" void __stdcall Shim_glLoadMatrixf(const float*) { }
extern "C" void __stdcall Shim_glLoadMatrixd(const double*) { }
extern "C" void __stdcall Shim_glMultMatrixf(const float*) { }
extern "C" void __stdcall Shim_glPushMatrix() { }
extern "C" void __stdcall Shim_glPopMatrix() { }
extern "C" void __stdcall Shim_glOrtho(double, double, double, double, double, double) { }
extern "C" void __stdcall Shim_glFrustum(double, double, double, double, double, double) { }
extern "C" void __stdcall Shim_glViewport(int, int, int, int) { }
extern "C" void __stdcall Shim_glScissor(int, int, int, int) { }
extern "C" void __stdcall Shim_glClipPlane(unsigned int, const double*) { }
extern "C" void __stdcall Shim_glGetClipPlane(unsigned int, double*) { }
extern "C" void __stdcall Shim_glTranslated(double, double, double) { }
extern "C" void __stdcall Shim_glTranslatef(float, float, float) { }
extern "C" void __stdcall Shim_glRotated(double, double, double, double) { }
extern "C" void __stdcall Shim_glRotatef(float, float, float, float) { }
extern "C" void __stdcall Shim_glScaled(double, double, double) { }
extern "C" void __stdcall Shim_glScalef(float, float, float) { }
extern "C" void __stdcall Shim_glGetIntegerv(unsigned int, int*) { }
extern "C" void __stdcall Shim_glGetFloatv(unsigned int, float*) { }
extern "C" void __stdcall Shim_glGetDoublev(unsigned int, double*) { }
extern "C" void __stdcall Shim_glGetBooleanv(unsigned int, unsigned char*) { }
extern "C" const GLubyte* __stdcall Shim_glGetString(unsigned int) { return (const GLubyte*)""; }
extern "C" int __stdcall Shim_glGetError() { return GL_NO_ERROR; }
extern "C" void __stdcall Shim_glHint(unsigned int, unsigned int) { }
extern "C" void __stdcall Shim_glFlush() { }
extern "C" void __stdcall Shim_glFinish() { }
extern "C" void __stdcall Shim_glBlendFunc(unsigned int, unsigned int) { }
extern "C" void __stdcall Shim_glDepthFunc(unsigned int) { }
extern "C" void __stdcall Shim_glDepthMask(unsigned char) { }
extern "C" void __stdcall Shim_glDepthRange(double, double) { }
extern "C" void __stdcall Shim_glStencilFunc(unsigned int, int, unsigned int) { }
extern "C" void __stdcall Shim_glStencilMask(unsigned int) { }
extern "C" void __stdcall Shim_glStencilOp(unsigned int, unsigned int, unsigned int) { }
extern "C" void __stdcall Shim_glAlphaFunc(unsigned int, float) { }
extern "C" void __stdcall Shim_glLogicOp(unsigned int) { }
extern "C" void __stdcall Shim_glPixelStorei(unsigned int, int) { }
extern "C" void __stdcall Shim_glPixelStoref(unsigned int, float) { }
extern "C" void __stdcall Shim_glPixelTransferi(unsigned int, int) { }
extern "C" void __stdcall Shim_glPixelTransferf(unsigned int, float) { }
extern "C" void __stdcall Shim_glPixelMapfv(unsigned int, int, const float*) { }
extern "C" void __stdcall Shim_glPixelMapuiv(unsigned int, int, const unsigned int*) { }
extern "C" void __stdcall Shim_glPixelZoom(float, float) { }
extern "C" void __stdcall Shim_glDrawPixels(int, int, unsigned int, unsigned int, const void*) { }
extern "C" void __stdcall Shim_glReadPixels(int, int, int, int, unsigned int, unsigned int, void*) { }
extern "C" void __stdcall Shim_glCopyPixels(int, int, int, int, unsigned int) { }
extern "C" void __stdcall Shim_glDrawBuffer(unsigned int) { }
extern "C" void __stdcall Shim_glReadBuffer(unsigned int) { }
extern "C" void __stdcall Shim_glIndexMask(unsigned int) { }
extern "C" void __stdcall Shim_glColorMask(unsigned char, unsigned char, unsigned char, unsigned char) { }
extern "C" void __stdcall Shim_glShadeModel(unsigned int) { }
extern "C" void __stdcall Shim_glFrontFace(unsigned int) { }
extern "C" void __stdcall Shim_glCullFace(unsigned int) { }
extern "C" void __stdcall Shim_glPolygonMode(unsigned int, unsigned int) { }
extern "C" void __stdcall Shim_glPolygonOffset(float, float) { }
extern "C" void __stdcall Shim_glPolygonStipple(const unsigned char*) { }
extern "C" void __stdcall Shim_glLineStipple(int, unsigned short) { }
extern "C" void __stdcall Shim_glLineWidth(float) { }
extern "C" void __stdcall Shim_glPointSize(float) { }
extern "C" void __stdcall Shim_glMaterialfv(unsigned int, unsigned int, const float*) { }
extern "C" void __stdcall Shim_glMaterialf(unsigned int, unsigned int, float) { }
extern "C" void __stdcall Shim_glLightfv(unsigned int, unsigned int, const float*) { }
extern "C" void __stdcall Shim_glLightf(unsigned int, unsigned int, float) { }
extern "C" void __stdcall Shim_glLightModeli(unsigned int, int) { }
extern "C" void __stdcall Shim_glLightModelfv(unsigned int, const float*) { }
extern "C" void __stdcall Shim_glGenTextures(int, unsigned int*) { }
extern "C" void __stdcall Shim_glDeleteTextures(int, const unsigned int*) { }
extern "C" void __stdcall Shim_glBindTexture(unsigned int, unsigned int) { }
extern "C" void __stdcall Shim_glTexImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void*) { }
extern "C" void __stdcall Shim_glTexImage1D(unsigned int, int, int, int, int, unsigned int, unsigned int, const void*) { }
extern "C" void __stdcall Shim_glTexSubImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void*) { }
extern "C" void __stdcall Shim_glTexParameteri(unsigned int, unsigned int, int) { }
extern "C" void __stdcall Shim_glTexParameterf(unsigned int, unsigned int, float) { }
extern "C" void __stdcall Shim_glTexParameterfv(unsigned int, unsigned int, const float*) { }
extern "C" void __stdcall Shim_glTexEnvf(unsigned int, unsigned int, float) { }
extern "C" void __stdcall Shim_glTexEnvi(unsigned int, unsigned int, int) { }
extern "C" void __stdcall Shim_glTexGend(unsigned int, unsigned int, double) { }
extern "C" void __stdcall Shim_glTexGeni(unsigned int, unsigned int, int) { }
extern "C" void __stdcall Shim_glGenLists_listbase(unsigned int) { }
extern "C" void __stdcall Shim_glListBase(unsigned int) { }
extern "C" void __stdcall Shim_glPushAttrib(unsigned int) { }
extern "C" void __stdcall Shim_glPopAttrib() { }
extern "C" void __stdcall Shim_glPushClientAttrib(unsigned int) { }
extern "C" void __stdcall Shim_glPopClientAttrib() { }
extern "C" void __stdcall Shim_glVertexPointer(int, unsigned int, int, const void*) { }
extern "C" void __stdcall Shim_glColorPointer(int, unsigned int, int, const void*) { }
extern "C" void __stdcall Shim_glTexCoordPointer(int, unsigned int, int, const void*) { }
extern "C" void __stdcall Shim_glNormalPointer(unsigned int, int, const void*) { }
extern "C" void __stdcall Shim_glInterleavedArrays(unsigned int, int, const void*) { }
extern "C" void __stdcall Shim_glDrawArrays(unsigned int, int, int) { }
extern "C" void __stdcall Shim_glDrawElements(unsigned int, int, unsigned int, const void*) { }
extern "C" void __stdcall Shim_glArrayElement(int) { }
extern "C" void __stdcall Shim_glEdgeFlagPointer(int, const void*) { }
extern "C" void __stdcall Shim_glIndexPointer(unsigned int, int, const void*) { }
extern "C" void __stdcall Shim_glFogf(unsigned int, float) { }
extern "C" void __stdcall Shim_glFogi(unsigned int, int) { }
extern "C" void __stdcall Shim_glFogfv(unsigned int, const float*) { }
extern "C" void __stdcall Shim_glFogiv(unsigned int, const int*) { }

}  // namespace xwr

// ===========================================================================
// Registration
// ===========================================================================
REGISTER_SHIM("opengl32", "wglCreateContext", (FARPROC)&xwr::Shim_wglCreateContext);
REGISTER_SHIM("opengl32", "wglMakeCurrent", (FARPROC)&xwr::Shim_wglMakeCurrent);
REGISTER_SHIM("opengl32", "wglDeleteContext", (FARPROC)&xwr::Shim_wglDeleteContext);
REGISTER_SHIM("opengl32", "wglGetCurrentContext", (FARPROC)&xwr::Shim_wglGetCurrentContext);
REGISTER_SHIM("opengl32", "wglGetCurrentDC", (FARPROC)&xwr::Shim_wglGetCurrentDC);
REGISTER_SHIM("opengl32", "wglGetProcAddress", (FARPROC)&xwr::Shim_wglGetProcAddress);
REGISTER_SHIM("opengl32", "wglShareLists", (FARPROC)&xwr::Shim_wglShareLists);
REGISTER_SHIM("opengl32", "wglCopyContext", (FARPROC)&xwr::Shim_wglCopyContext);
REGISTER_SHIM("opengl32", "wglChoosePixelFormat", (FARPROC)&xwr::Shim_wglChoosePixelFormat);
REGISTER_SHIM("opengl32", "wglDescribePixelFormat", (FARPROC)&xwr::Shim_wglDescribePixelFormat);
REGISTER_SHIM("opengl32", "wglGetPixelFormat", (FARPROC)&xwr::Shim_wglGetPixelFormat);
REGISTER_SHIM("opengl32", "wglSetPixelFormat", (FARPROC)&xwr::Shim_wglSetPixelFormat);
REGISTER_SHIM("opengl32", "wglSwapBuffers", (FARPROC)&xwr::Shim_wglSwapBuffers);
REGISTER_SHIM("opengl32", "wglUseFontBitmapsW", (FARPROC)&xwr::Shim_wglUseFontBitmapsW);
REGISTER_SHIM("opengl32", "wglUseFontOutlinesW", (FARPROC)&xwr::Shim_wglUseFontOutlinesW);
REGISTER_SHIM("opengl32", "wglMakeContextCurrentARB", (FARPROC)&xwr::Shim_wglMakeContextCurrentARB);
REGISTER_SHIM("opengl32", "wglCreateContextAttribsARB", (FARPROC)&xwr::Shim_wglCreateContextAttribsARB);
REGISTER_SHIM("opengl32", "glClear", (FARPROC)&xwr::Shim_glClear);
REGISTER_SHIM("opengl32", "glClearColor", (FARPROC)&xwr::Shim_glClearColor);
REGISTER_SHIM("opengl32", "glClearDepth", (FARPROC)&xwr::Shim_glClearDepth);
REGISTER_SHIM("opengl32", "glClearStencil", (FARPROC)&xwr::Shim_glClearStencil);
REGISTER_SHIM("opengl32", "glEnable", (FARPROC)&xwr::Shim_glEnable);
REGISTER_SHIM("opengl32", "glDisable", (FARPROC)&xwr::Shim_glDisable);
REGISTER_SHIM("opengl32", "glEnableClientState", (FARPROC)&xwr::Shim_glEnableClientState);
REGISTER_SHIM("opengl32", "glDisableClientState", (FARPROC)&xwr::Shim_glDisableClientState);
REGISTER_SHIM("opengl32", "glIsEnabled", (FARPROC)&xwr::Shim_glIsEnabled);
REGISTER_SHIM("opengl32", "glBegin", (FARPROC)&xwr::Shim_glBegin);
REGISTER_SHIM("opengl32", "glEnd", (FARPROC)&xwr::Shim_glEnd);
REGISTER_SHIM("opengl32", "glEndList", (FARPROC)&xwr::Shim_glEndList);
REGISTER_SHIM("opengl32", "glNewList", (FARPROC)&xwr::Shim_glNewList);
REGISTER_SHIM("opengl32", "glCallList", (FARPROC)&xwr::Shim_glCallList);
REGISTER_SHIM("opengl32", "glDeleteLists", (FARPROC)&xwr::Shim_glDeleteLists);
REGISTER_SHIM("opengl32", "glGenLists", (FARPROC)&xwr::Shim_glGenLists);
REGISTER_SHIM("opengl32", "glVertex2f", (FARPROC)&xwr::Shim_glVertex2f);
REGISTER_SHIM("opengl32", "glVertex3f", (FARPROC)&xwr::Shim_glVertex3f);
REGISTER_SHIM("opengl32", "glVertex4f", (FARPROC)&xwr::Shim_glVertex4f);
REGISTER_SHIM("opengl32", "glVertex2d", (FARPROC)&xwr::Shim_glVertex2d);
REGISTER_SHIM("opengl32", "glVertex3d", (FARPROC)&xwr::Shim_glVertex3d);
REGISTER_SHIM("opengl32", "glVertex2i", (FARPROC)&xwr::Shim_glVertex2i);
REGISTER_SHIM("opengl32", "glVertex3i", (FARPROC)&xwr::Shim_glVertex3i);
REGISTER_SHIM("opengl32", "glVertex2sv", (FARPROC)&xwr::Shim_glVertex2sv);
REGISTER_SHIM("opengl32", "glVertex3fv", (FARPROC)&xwr::Shim_glVertex3fv);
REGISTER_SHIM("opengl32", "glVertex3dv", (FARPROC)&xwr::Shim_glVertex3dv);
REGISTER_SHIM("opengl32", "glColor3f", (FARPROC)&xwr::Shim_glColor3f);
REGISTER_SHIM("opengl32", "glColor4f", (FARPROC)&xwr::Shim_glColor4f);
REGISTER_SHIM("opengl32", "glColor3ub", (FARPROC)&xwr::Shim_glColor3ub);
REGISTER_SHIM("opengl32", "glColor4ub", (FARPROC)&xwr::Shim_glColor4ub);
REGISTER_SHIM("opengl32", "glColor3fv", (FARPROC)&xwr::Shim_glColor3fv);
REGISTER_SHIM("opengl32", "glColor4fv", (FARPROC)&xwr::Shim_glColor4fv);
REGISTER_SHIM("opengl32", "glTexCoord2f", (FARPROC)&xwr::Shim_glTexCoord2f);
REGISTER_SHIM("opengl32", "glTexCoord2fv", (FARPROC)&xwr::Shim_glTexCoord2fv);
REGISTER_SHIM("opengl32", "glNormal3f", (FARPROC)&xwr::Shim_glNormal3f);
REGISTER_SHIM("opengl32", "glNormal3fv", (FARPROC)&xwr::Shim_glNormal3fv);
REGISTER_SHIM("opengl32", "glRectf", (FARPROC)&xwr::Shim_glRectf);
REGISTER_SHIM("opengl32", "glRecti", (FARPROC)&xwr::Shim_glRecti);
REGISTER_SHIM("opengl32", "glMatrixMode", (FARPROC)&xwr::Shim_glMatrixMode);
REGISTER_SHIM("opengl32", "glLoadIdentity", (FARPROC)&xwr::Shim_glLoadIdentity);
REGISTER_SHIM("opengl32", "glLoadMatrixf", (FARPROC)&xwr::Shim_glLoadMatrixf);
REGISTER_SHIM("opengl32", "glLoadMatrixd", (FARPROC)&xwr::Shim_glLoadMatrixd);
REGISTER_SHIM("opengl32", "glMultMatrixf", (FARPROC)&xwr::Shim_glMultMatrixf);
REGISTER_SHIM("opengl32", "glPushMatrix", (FARPROC)&xwr::Shim_glPushMatrix);
REGISTER_SHIM("opengl32", "glPopMatrix", (FARPROC)&xwr::Shim_glPopMatrix);
REGISTER_SHIM("opengl32", "glOrtho", (FARPROC)&xwr::Shim_glOrtho);
REGISTER_SHIM("opengl32", "glFrustum", (FARPROC)&xwr::Shim_glFrustum);
REGISTER_SHIM("opengl32", "glViewport", (FARPROC)&xwr::Shim_glViewport);
REGISTER_SHIM("opengl32", "glScissor", (FARPROC)&xwr::Shim_glScissor);
REGISTER_SHIM("opengl32", "glClipPlane", (FARPROC)&xwr::Shim_glClipPlane);
REGISTER_SHIM("opengl32", "glGetClipPlane", (FARPROC)&xwr::Shim_glGetClipPlane);
REGISTER_SHIM("opengl32", "glTranslated", (FARPROC)&xwr::Shim_glTranslated);
REGISTER_SHIM("opengl32", "glTranslatef", (FARPROC)&xwr::Shim_glTranslatef);
REGISTER_SHIM("opengl32", "glRotated", (FARPROC)&xwr::Shim_glRotated);
REGISTER_SHIM("opengl32", "glRotatef", (FARPROC)&xwr::Shim_glRotatef);
REGISTER_SHIM("opengl32", "glScaled", (FARPROC)&xwr::Shim_glScaled);
REGISTER_SHIM("opengl32", "glScalef", (FARPROC)&xwr::Shim_glScalef);
REGISTER_SHIM("opengl32", "glGetIntegerv", (FARPROC)&xwr::Shim_glGetIntegerv);
REGISTER_SHIM("opengl32", "glGetFloatv", (FARPROC)&xwr::Shim_glGetFloatv);
REGISTER_SHIM("opengl32", "glGetDoublev", (FARPROC)&xwr::Shim_glGetDoublev);
REGISTER_SHIM("opengl32", "glGetBooleanv", (FARPROC)&xwr::Shim_glGetBooleanv);
REGISTER_SHIM("opengl32", "glGetString", (FARPROC)&xwr::Shim_glGetString);
REGISTER_SHIM("opengl32", "glGetError", (FARPROC)&xwr::Shim_glGetError);
REGISTER_SHIM("opengl32", "glHint", (FARPROC)&xwr::Shim_glHint);
REGISTER_SHIM("opengl32", "glFlush", (FARPROC)&xwr::Shim_glFlush);
REGISTER_SHIM("opengl32", "glFinish", (FARPROC)&xwr::Shim_glFinish);
REGISTER_SHIM("opengl32", "glBlendFunc", (FARPROC)&xwr::Shim_glBlendFunc);
REGISTER_SHIM("opengl32", "glDepthFunc", (FARPROC)&xwr::Shim_glDepthFunc);
REGISTER_SHIM("opengl32", "glDepthMask", (FARPROC)&xwr::Shim_glDepthMask);
REGISTER_SHIM("opengl32", "glDepthRange", (FARPROC)&xwr::Shim_glDepthRange);
REGISTER_SHIM("opengl32", "glStencilFunc", (FARPROC)&xwr::Shim_glStencilFunc);
REGISTER_SHIM("opengl32", "glStencilMask", (FARPROC)&xwr::Shim_glStencilMask);
REGISTER_SHIM("opengl32", "glStencilOp", (FARPROC)&xwr::Shim_glStencilOp);
REGISTER_SHIM("opengl32", "glAlphaFunc", (FARPROC)&xwr::Shim_glAlphaFunc);
REGISTER_SHIM("opengl32", "glLogicOp", (FARPROC)&xwr::Shim_glLogicOp);
REGISTER_SHIM("opengl32", "glPixelStorei", (FARPROC)&xwr::Shim_glPixelStorei);
REGISTER_SHIM("opengl32", "glPixelStoref", (FARPROC)&xwr::Shim_glPixelStoref);
REGISTER_SHIM("opengl32", "glPixelTransferi", (FARPROC)&xwr::Shim_glPixelTransferi);
REGISTER_SHIM("opengl32", "glPixelTransferf", (FARPROC)&xwr::Shim_glPixelTransferf);
REGISTER_SHIM("opengl32", "glPixelMapfv", (FARPROC)&xwr::Shim_glPixelMapfv);
REGISTER_SHIM("opengl32", "glPixelMapuiv", (FARPROC)&xwr::Shim_glPixelMapuiv);
REGISTER_SHIM("opengl32", "glPixelZoom", (FARPROC)&xwr::Shim_glPixelZoom);
REGISTER_SHIM("opengl32", "glDrawPixels", (FARPROC)&xwr::Shim_glDrawPixels);
REGISTER_SHIM("opengl32", "glReadPixels", (FARPROC)&xwr::Shim_glReadPixels);
REGISTER_SHIM("opengl32", "glCopyPixels", (FARPROC)&xwr::Shim_glCopyPixels);
REGISTER_SHIM("opengl32", "glDrawBuffer", (FARPROC)&xwr::Shim_glDrawBuffer);
REGISTER_SHIM("opengl32", "glReadBuffer", (FARPROC)&xwr::Shim_glReadBuffer);
REGISTER_SHIM("opengl32", "glIndexMask", (FARPROC)&xwr::Shim_glIndexMask);
REGISTER_SHIM("opengl32", "glColorMask", (FARPROC)&xwr::Shim_glColorMask);
REGISTER_SHIM("opengl32", "glShadeModel", (FARPROC)&xwr::Shim_glShadeModel);
REGISTER_SHIM("opengl32", "glFrontFace", (FARPROC)&xwr::Shim_glFrontFace);
REGISTER_SHIM("opengl32", "glCullFace", (FARPROC)&xwr::Shim_glCullFace);
REGISTER_SHIM("opengl32", "glPolygonMode", (FARPROC)&xwr::Shim_glPolygonMode);
REGISTER_SHIM("opengl32", "glPolygonOffset", (FARPROC)&xwr::Shim_glPolygonOffset);
REGISTER_SHIM("opengl32", "glPolygonStipple", (FARPROC)&xwr::Shim_glPolygonStipple);
REGISTER_SHIM("opengl32", "glLineStipple", (FARPROC)&xwr::Shim_glLineStipple);
REGISTER_SHIM("opengl32", "glLineWidth", (FARPROC)&xwr::Shim_glLineWidth);
REGISTER_SHIM("opengl32", "glPointSize", (FARPROC)&xwr::Shim_glPointSize);
REGISTER_SHIM("opengl32", "glMaterialfv", (FARPROC)&xwr::Shim_glMaterialfv);
REGISTER_SHIM("opengl32", "glMaterialf", (FARPROC)&xwr::Shim_glMaterialf);
REGISTER_SHIM("opengl32", "glLightfv", (FARPROC)&xwr::Shim_glLightfv);
REGISTER_SHIM("opengl32", "glLightf", (FARPROC)&xwr::Shim_glLightf);
REGISTER_SHIM("opengl32", "glLightModeli", (FARPROC)&xwr::Shim_glLightModeli);
REGISTER_SHIM("opengl32", "glLightModelfv", (FARPROC)&xwr::Shim_glLightModelfv);
REGISTER_SHIM("opengl32", "glGenTextures", (FARPROC)&xwr::Shim_glGenTextures);
REGISTER_SHIM("opengl32", "glDeleteTextures", (FARPROC)&xwr::Shim_glDeleteTextures);
REGISTER_SHIM("opengl32", "glBindTexture", (FARPROC)&xwr::Shim_glBindTexture);
REGISTER_SHIM("opengl32", "glTexImage2D", (FARPROC)&xwr::Shim_glTexImage2D);
REGISTER_SHIM("opengl32", "glTexImage1D", (FARPROC)&xwr::Shim_glTexImage1D);
REGISTER_SHIM("opengl32", "glTexSubImage2D", (FARPROC)&xwr::Shim_glTexSubImage2D);
REGISTER_SHIM("opengl32", "glTexParameteri", (FARPROC)&xwr::Shim_glTexParameteri);
REGISTER_SHIM("opengl32", "glTexParameterf", (FARPROC)&xwr::Shim_glTexParameterf);
REGISTER_SHIM("opengl32", "glTexParameterfv", (FARPROC)&xwr::Shim_glTexParameterfv);
REGISTER_SHIM("opengl32", "glTexEnvf", (FARPROC)&xwr::Shim_glTexEnvf);
REGISTER_SHIM("opengl32", "glTexEnvi", (FARPROC)&xwr::Shim_glTexEnvi);
REGISTER_SHIM("opengl32", "glTexGend", (FARPROC)&xwr::Shim_glTexGend);
REGISTER_SHIM("opengl32", "glTexGeni", (FARPROC)&xwr::Shim_glTexGeni);
REGISTER_SHIM("opengl32", "glListBase", (FARPROC)&xwr::Shim_glListBase);
REGISTER_SHIM("opengl32", "glPushAttrib", (FARPROC)&xwr::Shim_glPushAttrib);
REGISTER_SHIM("opengl32", "glPopAttrib", (FARPROC)&xwr::Shim_glPopAttrib);
REGISTER_SHIM("opengl32", "glPushClientAttrib", (FARPROC)&xwr::Shim_glPushClientAttrib);
REGISTER_SHIM("opengl32", "glPopClientAttrib", (FARPROC)&xwr::Shim_glPopClientAttrib);
REGISTER_SHIM("opengl32", "glVertexPointer", (FARPROC)&xwr::Shim_glVertexPointer);
REGISTER_SHIM("opengl32", "glColorPointer", (FARPROC)&xwr::Shim_glColorPointer);
REGISTER_SHIM("opengl32", "glTexCoordPointer", (FARPROC)&xwr::Shim_glTexCoordPointer);
REGISTER_SHIM("opengl32", "glNormalPointer", (FARPROC)&xwr::Shim_glNormalPointer);
REGISTER_SHIM("opengl32", "glInterleavedArrays", (FARPROC)&xwr::Shim_glInterleavedArrays);
REGISTER_SHIM("opengl32", "glDrawArrays", (FARPROC)&xwr::Shim_glDrawArrays);
REGISTER_SHIM("opengl32", "glDrawElements", (FARPROC)&xwr::Shim_glDrawElements);
REGISTER_SHIM("opengl32", "glArrayElement", (FARPROC)&xwr::Shim_glArrayElement);
REGISTER_SHIM("opengl32", "glEdgeFlagPointer", (FARPROC)&xwr::Shim_glEdgeFlagPointer);
REGISTER_SHIM("opengl32", "glIndexPointer", (FARPROC)&xwr::Shim_glIndexPointer);
REGISTER_SHIM("opengl32", "glFogf", (FARPROC)&xwr::Shim_glFogf);
REGISTER_SHIM("opengl32", "glFogi", (FARPROC)&xwr::Shim_glFogi);
REGISTER_SHIM("opengl32", "glFogfv", (FARPROC)&xwr::Shim_glFogfv);
REGISTER_SHIM("opengl32", "glFogiv", (FARPROC)&xwr::Shim_glFogiv);
