/********************************************************************************
*                                                                               *
*                     G L  R e n d e r i n g   C o n t e x t                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXElement.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXException.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXFont.h"
#include "FXVisual.h"
#include "FXGLVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXGLContext.h"


/*
  Notes:
  - Creates FXGLContext based on frame buffer properties described in the desired FXGLVisual.
  - When realized, match actual hardware against desired frame buffer properties and create a
    FXGLContext conformant with the best matching hardware configuration.  Note that we don't
    have a window yet, necessarily!
  - There will be three different ways to make a FXGLCanvas:

      1 Each FXGLCanvas has its own FXGLContext; The FXGLContext is owned by the FXGLCanvas and
        destroyed when the FXGLCanvas is.

      2 Each FXGLCanvas has its own FXGLContext, but it may share the display list and other
        GL objects with those of another FXGLContext.  Thus the other FXGLContext with which
        it shares must be passed in.

      3 The FXGLCanvas shares the FXGLContext with another FXGLCanvas.  This is probably the
        most efficient way as all the GL state information is preserved between the FXGLCanvas
        windows.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// Object implementation
FXIMPLEMENT(FXGLContext,FXId,nullptr,0)


// Make GL context
FXGLContext::FXGLContext():surface(nullptr),visual(nullptr),shared(nullptr){
  FXTRACE((100,"FXGLContext::FXGLContext %p\n",this));
  }


// Make a GL context
FXGLContext::FXGLContext(FXApp *a,FXGLVisual *vis,FXGLContext* shr):FXId(a),surface(nullptr),visual(vis),shared(shr){
  FXTRACE((100,"FXGLContext::FXGLContext %p\n",this));
  }


// Create GL context
void FXGLContext::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE((100,"%s::create %p\n",getClassName(),this));

      // Got to have a visual
      if(!visual){ fxerror("%s::create: trying to create context without a visual.\n",getClassName()); }

      // If sharing contexts for display lists, shared context must be created already
      if(shared && !shared->id()){ fxerror("%s::create: trying to create context before shared context has been created.\n",getClassName()); }

      // Initialize visual
      visual->create();

#ifdef HAVE_GL_H
#if defined(WIN32)
      PIXELFORMATDESCRIPTOR pfd={sizeof(PIXELFORMATDESCRIPTOR),1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      HWND hwnd=CreateWindow(TEXT("GLTEMP"),TEXT(""),0,0,0,0,0,(HWND)nullptr,(HMENU)nullptr,(HINSTANCE)getApp()->getDisplay(),nullptr);
      HDC hdc=::GetDC(hwnd);
      SetPixelFormat(hdc,(FXint)(FXival)visual->id(),&pfd);
      xid=(FXID)wglCreateContext(hdc);
#if 0 // New code from Vadim ///
      if(desired_gl>2){
        int attribList[8]={
          WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
          WGL_CONTEXT_MINOR_VERSION_ARB, 3,
          WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
          0, 0
          };
        wglMakeCurrent(hdc,(HGLRC)xid);
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
        wglCreateContextAttribsARB=(PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
        if(wglCreateContextAttribsARB!=nullptr){                           // OpenGL 3.0 is supported
          HGLRC hrc=wglCreateContextAttribsARB(hdc,0,attribList);
          wglDeleteContext((HGLRC)xid);
          xid=hrc;
          }
        }
#endif
      // I hope I didn't get this backward; the new context obviously has no
      // display lists yet, but the old one may have, as it has already been around
      // for a while.  If you see this fail and can't explain why, then that might
      // be what's going on.  Report this to jeroen@fox-toolkit.net
      if(shared && !wglShareLists((HGLRC)shared->id(),(HGLRC)xid)){
        throw FXWindowException("unable to share GL context.");
        }
      ::ReleaseDC(hwnd,hdc);
      DestroyWindow(hwnd);
//#elif defined(GLX_VERSION_1_3)
//      xid=(FXID)glXCreateNewContext((Display*)getApp()->getDisplay(),(GLXFBConfig)visual->id(),GLX_RGBA_TYPE,shared?(GLXContext)shared->id():nullptr,true);
#else
      XVisualInfo vi;
      vi.visual=(Visual*)visual->visual;
      vi.visualid=vi.visual->visualid;
      vi.screen=DefaultScreen((Display*)getApp()->getDisplay());
      vi.depth=visual->getDepth();
      vi.c_class=vi.visual->c_class;
      vi.red_mask=vi.visual->red_mask;
      vi.green_mask=vi.visual->green_mask;
      vi.blue_mask=vi.visual->blue_mask;
      vi.colormap_size=vi.visual->map_entries;
      vi.bits_per_rgb=vi.visual->bits_per_rgb;
      xid=(FXID)glXCreateContext((Display*)getApp()->getDisplay(),&vi,shared?(GLXContext)shared->id():nullptr,true);
#endif
#endif
      if(!xid){
        throw FXWindowException("unable to create GL context.");
        }
      }
    }
  }

#if 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>

#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

// Helper to check for extension string presence
static bool isExtensionSupported(const char *extList,const char *extension){
  const char *start,*where,*terminator;

  // Extension names should not have spaces
  where=strchr(extension,' ');
  if(where || *extension=='\0') return false;

  // It takes a bit of care to be fool-proof about parsing the OpenGL
  // extensions string. Don't be fooled by sub-strings, etc.
  for(start=extList; ; ){
    where=strstr(start,extension);
    if(!where) break;
    terminator=where+strlen(extension);
    if(where==start || *(where-1)==' '){
      if(*terminator==' ' || *terminator=='\0') return true;
      }
    start=terminator;
    }
  return false;
  }


static bool ctxErrorOccurred = false;

static int ctxErrorHandler( Display *dpy, XErrorEvent *ev ){
  ctxErrorOccurred = true;
  return 0;
  }


int main (int argc, char ** argv){
  Display *display = XOpenDisplay(0);

  if ( !display ){
    printf( "Failed to open X display\n" );
    exit(1);
    }

  // Get a matching FB config
  static int visual_attribs[] = {
    GLX_X_RENDERABLE    , True,
    GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
    GLX_RENDER_TYPE     , GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
    GLX_RED_SIZE        , 8,
    GLX_GREEN_SIZE      , 8,
    GLX_BLUE_SIZE       , 8,
    GLX_ALPHA_SIZE      , 8,
    GLX_DEPTH_SIZE      , 24,
    GLX_STENCIL_SIZE    , 8,
    GLX_DOUBLEBUFFER    , True,
    //GLX_SAMPLE_BUFFERS  , 1,
    //GLX_SAMPLES         , 4,
    None
    };

  int glx_major, glx_minor;

  // FBConfigs were added in GLX version 1.3.
  if ( !glXQueryVersion( display, &glx_major, &glx_minor ) || ( ( glx_major == 1 ) && ( glx_minor < 3 ) ) || ( glx_major < 1 ) ){
    printf( "Invalid GLX version" );
    exit(1);
    }

  printf( "Getting matching framebuffer configs\n" );
  int fbcount;
  GLXFBConfig *fbc = glXChooseFBConfig( display, DefaultScreen( display ), visual_attribs, &fbcount );
  if ( !fbc ){
    printf( "Failed to retrieve a framebuffer config\n" );
    exit(1);
    }
  printf( "Found %d matching FB configs.\n", fbcount );

  // Pick the FB config/visual with the most samples per pixel
  printf( "Getting XVisualInfos\n" );
  int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;

  int i;
  for ( i = 0; i < fbcount; i++ ){
    XVisualInfo *vi = glXGetVisualFromFBConfig( display, fbc[i] );
    if ( vi ){
      int samp_buf, samples;
      glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
      glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLES       , &samples  );

      printf( "  Matching fbconfig %d, visual ID 0x%2x: SAMPLE_BUFFERS = %d,"" SAMPLES = %d\n", i, vi -> visualid, samp_buf, samples );

      if ( best_fbc < 0 || samp_buf && samples > best_num_samp ) best_fbc = i, best_num_samp = samples;
      if ( worst_fbc < 0 || !samp_buf || samples < worst_num_samp ) worst_fbc = i, worst_num_samp = samples;
      }
    XFree( vi );
    }

  GLXFBConfig bestFbc = fbc[ best_fbc ];

  // Be sure to free the FBConfig list allocated by glXChooseFBConfig()
  XFree( fbc );

  // Get a visual
  XVisualInfo *vi = glXGetVisualFromFBConfig( display, bestFbc );
  printf( "Chosen visual ID = 0x%x\n", vi->visualid );

  printf( "Creating colormap\n" );
  XSetWindowAttributes swa;
  Colormap cmap;
  swa.colormap = cmap = XCreateColormap( display,RootWindow( display, vi->screen ), vi->visual, AllocNone );
  swa.background_pixmap = None ;
  swa.border_pixel      = 0;
  swa.event_mask        = StructureNotifyMask;

  printf( "Creating window\n" );
  Window win = XCreateWindow( display, RootWindow( display, vi->screen ), 0, 0, 100, 100, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel|CWColormap|CWEventMask, &swa );
  if ( !win ){
    printf( "Failed to create window.\n" );
    exit(1);
    }

  // Done with the visual info data
  XFree( vi );

  XStoreName( display, win, "GL 3.0 Window" );

  printf( "Mapping window\n" );
  XMapWindow( display, win );

  // Get the default screen's GLX extension list
  const char *glxExts = glXQueryExtensionsString( display,DefaultScreen( display ) );

  // NOTE: It is not necessary to create or make current to a context before
  // calling glXGetProcAddressARB
  glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
  glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );

  GLXContext ctx = 0;

  // Install an X error handler so the application won't exit if GL 3.0
  // context allocation fails.
  //
  // Note this error handler is global.  All display connections in all threads
  // of a process use the same error handler, so be sure to guard against other
  // threads issuing X commands while this code is running.
  ctxErrorOccurred = false;
  int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);

  // Check for the GLX_ARB_create_context extension string and the function.
  // If either is not present, use GLX 1.3 context creation method.
  if ( !isExtensionSupported( glxExts, "GLX_ARB_create_context" ) || !glXCreateContextAttribsARB ){
    printf( "glXCreateContextAttribsARB() not found ... using old-style GLX context\n" );
    ctx = glXCreateNewContext( display, bestFbc, GLX_RGBA_TYPE, 0, True );
    }

  // If it does, try to get a GL 3.0 context!
  else {
    int context_attribs[] = {
      GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
      GLX_CONTEXT_MINOR_VERSION_ARB, 0,
      //GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
      None
      };

    printf( "Creating context\n" );
    ctx = glXCreateContextAttribsARB( display, bestFbc, 0,True, context_attribs );

    // Sync to ensure any errors generated are processed.
    XSync( display, False );
    if ( !ctxErrorOccurred && ctx ){
      printf( "Created GL 3.0 context\n" );
      }
    else {
      // Couldn't create GL 3.0 context.  Fall back to old-style 2.x context.
      // When a context version below 3.0 is requested, implementations will
      // return the newest context version compatible with OpenGL versions less
      // than version 3.0.
      // GLX_CONTEXT_MAJOR_VERSION_ARB = 1
      context_attribs[1] = 1;
      // GLX_CONTEXT_MINOR_VERSION_ARB = 0
      context_attribs[3] = 0;

      ctxErrorOccurred = false;

      printf( "Failed to create GL 3.0 context ... using old-style GLX context\n" );
      ctx = glXCreateContextAttribsARB( display, bestFbc, 0, True, context_attribs );
      }
    }

  // Sync to ensure any errors generated are processed.
  XSync( display, False );

  // Restore the original error handler
  XSetErrorHandler( oldHandler );

  if ( ctxErrorOccurred || !ctx ){
    printf( "Failed to create an OpenGL context\n" );
    exit(1);
    }

  // Verifying that context is a direct context
  if ( ! glXIsDirect ( display, ctx ) ){
    printf( "Indirect GLX rendering context obtained\n" );
    }
  else{
    printf( "Direct GLX rendering context obtained\n" );
    }

  printf( "Making context current\n" );
  glXMakeCurrent( display, win, ctx );

  glClearColor ( 0, 0.5, 1, 1 );
  glClear ( GL_COLOR_BUFFER_BIT );
  glXSwapBuffers ( display, win );

  sleep( 1 );

  glClearColor ( 1, 0.5, 0, 1 );
  glClear ( GL_COLOR_BUFFER_BIT );
  glXSwapBuffers ( display, win );

  sleep( 1 );

  glXMakeCurrent( display, 0, 0 );
  glXDestroyContext( display, ctx );

  XDestroyWindow( display, win );
  XFreeColormap( display, cmap );
  XCloseDisplay( display );
  }
#endif

// Detach the GL context
void FXGLContext::detach(){
  visual->detach();
#ifdef HAVE_GL_H
  if(xid){
    FXTRACE((100,"FXGLContext::detach %p\n",this));
    surface=nullptr;
    xid=0;
    }
#endif
  }


// Destroy the GL context
void FXGLContext::destroy(){
#ifdef HAVE_GL_H
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE((100,"FXGLContext::destroy %p\n",this));
#ifdef WIN32
      wglDeleteContext((HGLRC)xid);
#else
      glXDestroyContext((Display*)getApp()->getDisplay(),(GLXContext)xid);
#endif
      }
    surface=nullptr;
    xid=0;
    }
#endif
  }


// Change visual
void FXGLContext::setVisual(FXGLVisual* vis){
  if(!vis){ fxerror("%s::setVisual: NULL visual\n",getClassName()); }
  if(xid){ fxerror("%s::setVisual: visual should be set before calling create()\n",getClassName()); }
  visual=vis;
  }


// Change share context
void FXGLContext::setShared(FXGLContext *ctx){
  if(xid){ fxerror("%s::setShared: sharing should be set before calling create()\n",getClassName()); }
  shared=ctx;
  }


//  Make the rendering context of drawable current
FXbool FXGLContext::begin(FXDrawable *draw){
#ifdef HAVE_GL_H
  if(xid && !surface){
#ifdef WIN32
    HDC hdc=(HDC)draw->GetDC();
    if(draw->getVisual()->colormap){
      SelectPalette(hdc,(HPALETTE)draw->getVisual()->colormap,false);
      RealizePalette(hdc);
      }
    if(wglMakeCurrent(hdc,(HGLRC)xid)){
      surface=draw;
      return true;
      }
//#elif defined(GLX_VERSION_1_3)
//    if(glXMakeContextCurrent((Display*)getApp()->getDisplay(),draw->id(),draw->id(),(GLXContext)xid)){
//      surface=draw;
//      return true;
//      }
#else
    if(glXMakeCurrent((Display*)getApp()->getDisplay(),draw->id(),(GLXContext)xid)){
      surface=draw;
      return true;
      }
#endif
    }
#endif
  return false;
  }


// Make the rendering context of drawable non-current
FXbool FXGLContext::end(){
#ifdef HAVE_GL_H
  if(xid && surface){
#ifdef WIN32
    HDC hdc=wglGetCurrentDC();
    if(wglMakeCurrent(nullptr,nullptr)!=0){
      surface->ReleaseDC(hdc);
      surface=nullptr;
      return true;
      }
//#elif defined(GLX_VERSION_1_3)
//    if(glXMakeContextCurrent((Display*)getApp()->getDisplay(),None,None,nullptr)){
//      surface=nullptr;
//      return true;
//      }
#else
    if(glXMakeCurrent((Display*)getApp()->getDisplay(),None,(GLXContext)nullptr)){
      surface=nullptr;
      return true;
      }
#endif
   }
#endif
  return false;
  }


// Used by GL to swap the buffers in double buffer mode, or flush a single buffer
void FXGLContext::swapBuffers(){
#ifdef HAVE_GL_H
  if(xid){
#ifdef WIN32
    // SwapBuffers(wglGetCurrentDC());
    if(wglSwapLayerBuffers(wglGetCurrentDC(),WGL_SWAP_MAIN_PLANE)==false){
      SwapBuffers(wglGetCurrentDC());
      }
#else
    glXSwapBuffers((Display*)getApp()->getDisplay(),glXGetCurrentDrawable());
#endif
    }
#endif
  }


// Return true if THIS context is current
FXbool FXGLContext::isCurrent() const {
#ifdef HAVE_GL_H
  if(xid){
#ifdef WIN32
    return (FXID)wglGetCurrentContext()==xid;
#else
    return (FXID)glXGetCurrentContext()==xid;
#endif
    }
#endif
  return false;
  }


// Return true if thread has ANY current context
FXbool FXGLContext::hasCurrent(){
#ifdef HAVE_GL_H
#ifdef WIN32
  return wglGetCurrentContext()!=nullptr;
#else
  return glXGetCurrentContext()!=nullptr;
#endif
#else
  return false;
#endif
  }


// Has double buffering
FXbool FXGLContext::isDoubleBuffer() const {
  return visual->isDoubleBuffer();
  }


// Has stereo buffering
FXbool FXGLContext::isStereo() const {
  return visual->isStereo();
  }


// Save data
void FXGLContext::save(FXStream& store) const {
  FXId::save(store);
  store << visual;
  store << shared;
  }


// Load data
void FXGLContext::load(FXStream& store){
  FXId::load(store);
  store >> visual;
  store >> shared;
  }


// Close and release any resources
FXGLContext::~FXGLContext(){
  FXTRACE((100,"FXGLContext::~FXGLContext %p\n",this));
  destroy();
  surface=(FXDrawable*)-1L;
  visual=(FXGLVisual*)-1L;
  shared=(FXGLContext*)-1L;
  }


/*******************************************************************************/


#if defined(HAVE_XFT_H) && defined(HAVE_GL_H)

// Xft version
static FXbool glXUseXftFont(XftFont* font,int first,int count,int listBase){
  GLint swapbytes,lsbfirst,rowlength,skiprows,skippixels,alignment,list;
  GLfloat x0,y0,dx,dy;
  FT_Face face;
  FT_Error err;
  FXint i,size,x,y;
  FXuchar *glyph;
  FXbool result=false;

  // Save the current packing mode for bitmaps
  glGetIntegerv(GL_UNPACK_SWAP_BYTES,&swapbytes);
  glGetIntegerv(GL_UNPACK_LSB_FIRST,&lsbfirst);
  glGetIntegerv(GL_UNPACK_ROW_LENGTH,&rowlength);
  glGetIntegerv(GL_UNPACK_SKIP_ROWS,&skiprows);
  glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&skippixels);
  glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);

  // Set desired packing modes
  glPixelStorei(GL_UNPACK_SWAP_BYTES,GL_FALSE);
  glPixelStorei(GL_UNPACK_LSB_FIRST,GL_FALSE);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  // Get face info
  face=XftLockFace(font);

  // Render font glyphs; use FreeType to render to bitmap
  for(i=first; i<count; i++){
    list=listBase+i;

    // Load glyph
    err=FT_Load_Glyph(face,FT_Get_Char_Index(face,i),FT_LOAD_DEFAULT);
    if(err) goto x;

    // Render glyph
    err=FT_Render_Glyph(face->glyph,FT_RENDER_MODE_MONO);
    if(err) goto x;

    // Pitch may be negative, its the stride between rows
    size=FXABS(face->glyph->bitmap.pitch) * face->glyph->bitmap.rows;

    // Glyph coordinates; note info in freetype is 6-bit fixed point
    x0=-(face->glyph->metrics.horiBearingX>>6);
    y0=(face->glyph->metrics.height-face->glyph->metrics.horiBearingY)>>6;
    dx=face->glyph->metrics.horiAdvance>>6;
    dy=0;

    // Allocate glyph data
    if(!allocElms(glyph,size)) goto x;

    // Copy into OpenGL bitmap format; note OpenGL upside down
    for(y=0; y<(FXint)face->glyph->bitmap.rows; y++){
      for(x=0; x<face->glyph->bitmap.pitch; x++){
        glyph[y*face->glyph->bitmap.pitch+x]=face->glyph->bitmap.buffer[(face->glyph->bitmap.rows-y-1)*face->glyph->bitmap.pitch+x];
        }
      }

    // Put bitmap into display list
    glNewList(list,GL_COMPILE);
    glBitmap(FXABS(face->glyph->bitmap.pitch)<<3,face->glyph->bitmap.rows,x0,y0,dx,dy,glyph);
    glEndList();

    // Free glyph data
    freeElms(glyph);
    }

  // Success
  result=true;

  // Restore packing modes
x:glPixelStorei(GL_UNPACK_SWAP_BYTES,swapbytes);
  glPixelStorei(GL_UNPACK_LSB_FIRST,lsbfirst);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,rowlength);
  glPixelStorei(GL_UNPACK_SKIP_ROWS,skiprows);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,skippixels);
  glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);

  // Unlock face
  XftUnlockFace(font);
  return result;
  }


#endif


// Create a display list of bitmaps from font glyphs in a font
FXbool glUseFXFont(FXFont* font,int first,int count,int list){
  FXbool result=false;
  if(!font || !font->id()){ fxerror("glUseFXFont: invalid font.\n"); }
  FXTRACE((100,"glUseFXFont: first=%d count=%d list=%d\n",first,count,list));
#ifdef HAVE_GL_H
#ifdef WIN32
  if(wglGetCurrentContext()){
    HDC hdc=wglGetCurrentDC();
    HFONT oldfont=(HFONT)SelectObject(hdc,(HFONT)font->id());
    // Replace wglUseFontBitmaps() with wglUseFontBitmapsW()
    // Change glCallLists() parameter:
    //   glCallLists(len,GL_UNSIGNED_SHORT,(GLushort*)unicodebuffer);
    // Figure out better values for "first" and "count".
    result=wglUseFontBitmaps(hdc,first,count,list);
    SelectObject(hdc,oldfont);
    }
#else
  if(glXGetCurrentContext()){
#ifdef HAVE_XFT_H                       // Using XFT
    result=glXUseXftFont((XftFont*)font->id(),first,count,list);
#else                                   // Using XLFD
    glXUseXFont((Font)font->id(),first,count,list);
    result=true;
#endif
    }
#endif
#endif
  return result;
  }

}
