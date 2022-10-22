/********************************************************************************
*                                                                               *
*                            V i s u a l   C l a s s                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXElement.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXException.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXGLVisual.h"


/*
  Notes:

  - FXGLVisual builds a visual/pixelformat suitable for GL drawing.

  - Selection of visual/pixelformat is based on a best match to a
    given set of hints, according to some heuristics:

    1) Prefer color depth close to asked ones; really like to
       get a bit MORE color, rather than LESS, however.

    2) If we wanted Z-buffer, it is STRONGLY preferred; If there's
       a choice, we prefer more color depth over more Z-depth; if
       we already have more colors than requested, we prefer to meet
       requested Z depth.

    3) If we wanted double buffer, we strongly prefer it over color and
       Z depth, but HAVING a Z-buffer is still more important.

    4) If we wanted alpha buffer, it is preferred, but Z-buffering
       and double buffering are considered more important.
       If there's a choice, we prefer to receive a few MORE bits of
       alpha buffer than we asked for, rather than LESS.

    5) If we wanted stereo, we prefer it, but almost everything except
       the color-, alpha-, and Z-depths are more important.

  - Some further tuning may be desired, but I think this should satisfy
    most cases....

  - Note that as long as OpenGL is in any way supported, you should ALWAYS
    be able to get at least some visual/pixelformat you can draw on.

  - As far as hardware acceleration goes, H/W acceleration should be
    enabled, possibly at the expense of color-, alpha-, and Z-depth;
    but NEVER at the expense of presence or absence of a requested feature.
    We only drop FEATURES which are requested if there is neither hardware
    nor software support.

    For example, we may trade in some Z-depth, but not the entire Z-buffer,
    to get a hardware accelerated visual/pixelformat.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXGLVisual,FXVisual,nullptr,0)


// Deserialization
FXGLVisual::FXGLVisual(){
  redSize=0;
  greenSize=0;
  blueSize=0;
  alphaSize=0;
  depthSize=0;
  stencilSize=0;
  multiSamples=0;
  accumRedSize=0;
  accumGreenSize=0;
  accumBlueSize=0;
  accumAlphaSize=0;
  actualRedSize=0;
  actualGreenSize=0;
  actualBlueSize=0;
  actualAlphaSize=0;
  actualDepthSize=0;
  actualStencilSize=0;
  actualMultiSamples=0;
  actualAccumRedSize=0;
  actualAccumGreenSize=0;
  actualAccumBlueSize=0;
  actualAccumAlphaSize=0;
  doubleBuffer=false;
  stereoBuffer=false;
  accelerated=false;
  copying=false;
  }


// Construct
FXGLVisual::FXGLVisual(FXApp* a,FXuint flgs):FXVisual(a,flgs,0){
  FXTRACE((100,"FXGLVisual::FXGLVisual %p\n",this));
  redSize=8;
  greenSize=8;
  blueSize=8;
  alphaSize=0;
  depthSize=24;
  stencilSize=0;
  multiSamples=0;
  accumRedSize=0;
  accumGreenSize=0;
  accumBlueSize=0;
  accumAlphaSize=0;
  actualRedSize=0;
  actualGreenSize=0;
  actualBlueSize=0;
  actualAlphaSize=0;
  actualDepthSize=0;
  actualStencilSize=0;
  actualMultiSamples=0;
  actualAccumRedSize=0;
  actualAccumGreenSize=0;
  actualAccumBlueSize=0;
  actualAccumAlphaSize=0;
  doubleBuffer=false;
  stereoBuffer=false;
  accelerated=false;
  copying=false;
  }


// Test if OpenGL is possible
FXbool FXGLVisual::hasOpenGL(FXApp* application){
  if(application->isInitialized()){
#ifdef HAVE_GL_H
#ifdef WIN32            // WIN32
    return true;
#else                   // UNIX
    return glXQueryExtension((Display*)application->getDisplay(),nullptr,nullptr);
#endif
#endif
    }
  return false;
  }


/*******************************************************************************/

// Describes particular FB configuration
struct FXGLVisual::FXGLSpecs {
  int redsize;
  int greensize;
  int bluesize;
  int alphasize;
  int depthsize;
  int stencilsize;
  int accumredsize;
  int accumgreensize;
  int accumbluesize;
  int accumalphasize;
  int doublebuffer;
  int stereobuffer;
  int multisamples;
  int composition;
  int image;
  int accel;
  int copy;
  };


// Match actual GL depth to desired one
FXint FXGLVisual::matchSpecs(const FXGLSpecs& s){
  FXint match,dred,dgreen,dblue,dalpha,ddepth,dstencil,dsamples,daccred,daccgreen,daccblue,daccalpha;

  // We prefer to get a few MORE bits in RGBA than we asked for
  dred   = s.redsize-redSize;     if(dred<0)   dred   *= -100;
  dgreen = s.greensize-greenSize; if(dgreen<0) dgreen *= -100;
  dblue  = s.bluesize-blueSize;   if(dblue<0)  dblue  *= -100;
  dalpha = s.alphasize-alphaSize; if(dalpha<0) dalpha *= -100;

  // Prefer better Z than asked, but colors more important
  ddepth = s.depthsize-depthSize; if(ddepth<0) ddepth *= -10;

  // We care about colors and Z depth more than stencil depth
  dstencil = s.stencilsize-stencilSize; if(dstencil<0) dstencil *= -1;

  // Accumulation buffers
  daccred=s.accumredsize-accumRedSize;       if(daccred<0)   daccred   *= -1;
  daccgreen=s.accumgreensize-accumGreenSize; if(daccgreen<0) daccgreen *= -1;
  daccblue=s.accumbluesize-accumBlueSize;    if(daccblue<0)  daccblue  *= -1;
  daccalpha=s.accumalphasize-accumAlphaSize; if(daccalpha<0) daccalpha *= -1;

  // Want the best colors, of course
  match=dred+dgreen+dblue+dalpha;

  // Accumulation buffers
  match+=daccred+daccgreen+daccblue+daccalpha;

  // Hardware accelerated is normally a plus
  if(!s.accel && !(flags&VISUAL_NO_ACCEL)){
    match+=10000;
    }

  // Extra penalty for no alpha if we asked for alpha, but no
  // penalty at all if there is alpha and we didn't ask for it.
  if(alphaSize>0){
    if(s.alphasize<1) match+=100000;
    }

  // Wanted Z-buffer
  if(depthSize>0){
    if(s.depthsize<1) match+=10000000;
    else match+=ddepth;
    }
  else{
    if(s.depthsize>0) match+=10000000;
    }

  // Stencil buffers desired
  if(stencilSize>0){
    if(s.stencilsize<1) match+=10000;
    else match+=dstencil;
    }
  else{
    if(s.stencilsize>0) match+=1;
    }

  // Multisamples
  if(multiSamples>0){
    dsamples=s.multisamples-multiSamples;
    if(dsamples<0) dsamples*=-10;
    match+=dsamples;
    }
  else{
    if(s.multisamples>0) match+=100;
    }

  // Double buffering also quite strongly preferred
  if(flags&VISUAL_DOUBLE_BUFFER){
    if(!s.doublebuffer) match+=1000000;
    }
  else{
    if(s.doublebuffer) match+=1000000;
    }

  // Stereo not so important
  if(flags&VISUAL_STEREO){
    if(!s.stereobuffer) match+=10000;
    }
  else{
    if(s.stereobuffer) match+=10000;
    }

  // Swap copies also important
  if(flags&VISUAL_SWAP_COPY){
    if(!s.copy) match+=10000000;
    }

  // Composition support would be nice to have
  if(!s.composition) match+=100;

  // Off-screen drawing would be nice
  if(!s.image) match+=1;

  return match;
  }


#if defined(WIN32) ///////////////// WIN32 //////////////////////////////////////

#ifndef PFD_SUPPORT_COMPOSITION
#define PFD_SUPPORT_COMPOSITION         0x00008000
#endif

#ifdef HAVE_GL_H

// Palette struct
struct LOGPALETTE256 {
  WORD         palVersion;
  WORD         palNumEntries;
  PALETTEENTRY palPalEntry[257];
  };


// System colors to match against
static FXuchar defSysClr[20][3] = {
    {  0,  0,  0},
    {128,  0,  0},
    {  0,128,  0},
    {128,128,  0},
    {  0,  0,128},
    {128,  0,128},
    {  0,128,128},
    {192,192,192},

    {192,220,192},
    {166,202,240},
    {255,251,240},
    {160,160,164},

    {128,128,128},
    {255,  0,  0},
    {  0,255,  0},
    {255,255,  0},
    {  0,  0,255},
    {255,  0,255},
    {  0,255,255},
    {255,255,255}
    };

static int defaultOverride[13] = {
  0, 3, 24, 27, 64, 67, 88, 173, 181, 236, 247, 164, 91
  };


// Make palette
static HPALETTE makeOpenGLPalette(PIXELFORMATDESCRIPTOR* info){
  LOGPALETTE256 palette;
  int num,i,j,rr,gg,bb;
  int rmax,gmax,bmax;
  HPALETTE hPalette;

  // Size of palette array
  num=1<<((PIXELFORMATDESCRIPTOR*)info)->cColorBits;

  FXASSERT(num<=256);

  // Maximum values each color
  rmax=(1 << info->cRedBits)-1;
  gmax=(1 << info->cGreenBits)-1;
  bmax=(1 << info->cBlueBits)-1;

  // Build palette
  for(rr=0; rr<=rmax; rr++){
    for(gg=0; gg<=gmax; gg++){
      for(bb=0; bb<=bmax; bb++){
        i = (rr << info->cRedShift) | (gg << info->cGreenShift) | (bb << info->cBlueShift);
        palette.palPalEntry[i].peRed = (255*rr)/rmax;
        palette.palPalEntry[i].peGreen = (255*gg)/gmax;
        palette.palPalEntry[i].peBlue = (255*bb)/bmax;
        palette.palPalEntry[i].peFlags = PC_NOCOLLAPSE;
        }
      }
    }

  // For 8-bit palette
  if((info->cColorBits==8) && (info->cRedBits==3) && (info->cRedShift==0) && (info->cGreenBits==3) && (info->cGreenShift==3) && (info->cBlueBits==2) && (info->cBlueShift==6)){
    for(j=1; j<=12; j++){
      palette.palPalEntry[defaultOverride[j]].peRed=defSysClr[j][0];
      palette.palPalEntry[defaultOverride[j]].peGreen=defSysClr[j][1];
      palette.palPalEntry[defaultOverride[j]].peBlue=defSysClr[j][1];
      palette.palPalEntry[defaultOverride[j]].peFlags=0;
      }
    }

  // Fill in the rest
  palette.palVersion=0x300;
  palette.palNumEntries=num;

  // Make palette
  hPalette=CreatePalette((const LOGPALETTE*)&palette);

  return hPalette;
  }

#endif


// Initialize
void FXGLVisual::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE((100,"%s::create %p\n",getClassName(),this));
#ifdef HAVE_GL_H
      PIXELFORMATDESCRIPTOR pfd;
      int bestmatch=1000000000;
      int best=-1;
      int match;
      int npf;
      HDC hdc;

      // Get some window handle
      hdc=GetDC(GetDesktopWindow());

      // Get number of supported pixel formats
      pfd.nSize=sizeof(PIXELFORMATDESCRIPTOR);
      pfd.nVersion=1;

      // Get number of supported pixel formats
      if(0<(npf=DescribePixelFormat(hdc,1,sizeof(PIXELFORMATDESCRIPTOR),&pfd))){

        FXTRACE((140,"Found %d OpenGL configs\n",npf));

        // Try to find the best
        for(int v=1; v<=npf; v++){
          FXGLSpecs specs;

          // Get info about this visual
          DescribePixelFormat(hdc,v,sizeof(PIXELFORMATDESCRIPTOR),&pfd);

          // Make sure this visual is valid
//          if(ChoosePixelFormat(hdc,&pfd)!=v) continue;

          // Get supported render type; we don't want index mode
          if(pfd.iPixelType==PFD_TYPE_COLORINDEX) continue;

          // OpenGL support is required
          if(!(pfd.dwFlags&PFD_SUPPORT_OPENGL)) continue;

          // Draw to window is required
          if(!(pfd.dwFlags&PFD_DRAW_TO_WINDOW)) continue;

          // Get planes
          specs.redsize=pfd.cRedBits;
          specs.greensize=pfd.cGreenBits;
          specs.bluesize=pfd.cBlueBits;
          specs.alphasize=pfd.cAlphaBits;
          specs.depthsize=pfd.cDepthBits;
          specs.stencilsize=pfd.cStencilBits;
          specs.accumredsize=pfd.cAccumRedBits;
          specs.accumgreensize=pfd.cAccumGreenBits;
          specs.accumbluesize=pfd.cAccumBlueBits;
          specs.accumalphasize=pfd.cAccumAlphaBits;

          // Double buffer capable
          specs.doublebuffer=(pfd.dwFlags&PFD_DOUBLEBUFFER)!=0;

          // Stereo capable
          specs.stereobuffer=(pfd.dwFlags&PFD_STEREO)!=0;

          // Multisample
          specs.multisamples=0;

          // Windows Vista and Windows 7 composition support
          specs.composition=(pfd.dwFlags&PFD_SUPPORT_COMPOSITION)!=0;

          // Bitmap drawing would be nice
          specs.image=(pfd.dwFlags&PFD_DRAW_TO_BITMAP)!=0;

          // Hardware accelerated format
          specs.accel=(pfd.dwFlags&PFD_GENERIC_FORMAT)==0;

          // Swap buffers by copying
          specs.copy=(pfd.dwFlags&PFD_SWAP_COPY)!=0;

          // Match specs
          match=matchSpecs(specs);

          // Trace
          FXTRACE((150,"Config: #%d: match=%d\n",v,match));
          FXTRACE((150,"  drawables  = %s%s\n",(pfd.dwFlags&PFD_DRAW_TO_WINDOW)?"PFD_DRAW_TO_WINDOW ":"",(pfd.dwFlags&PFD_DRAW_TO_BITMAP)?"PFD_DRAW_TO_BITMAP ":""));
          FXTRACE((150,"  render     = %s\n",(pfd.iPixelType==PFD_TYPE_COLORINDEX)?"PFD_TYPE_COLORINDEX":(pfd.iPixelType==PFD_TYPE_RGBA)?"PFD_TYPE_RGBA":""));
          FXTRACE((150,"  red size   = %d\n",specs.redsize));
          FXTRACE((150,"  green size = %d\n",specs.greensize));
          FXTRACE((150,"  blue size  = %d\n",specs.bluesize));
          FXTRACE((150,"  alpha size = %d\n",specs.alphasize));
          FXTRACE((150,"  depth size = %d\n",specs.depthsize));
          FXTRACE((150,"  stencil    = %d\n",specs.stencilsize));
          FXTRACE((150,"  acc red    = %d\n",specs.accumredsize));
          FXTRACE((150,"  acc green  = %d\n",specs.accumgreensize));
          FXTRACE((150,"  acc blue   = %d\n",specs.accumbluesize));
          FXTRACE((150,"  acc alpha  = %d\n",specs.accumalphasize));
          FXTRACE((150,"  double buf = %d\n",specs.doublebuffer));
          FXTRACE((150,"  stereo buf = %d\n",specs.stereobuffer));
          FXTRACE((150,"  samples    = %d\n",specs.multisamples));
          FXTRACE((150,"  accel      = %d\n",specs.accel));

          // May the best visual win
          if(match<=bestmatch){
            actualRedSize=specs.redsize;
            actualGreenSize=specs.greensize;
            actualBlueSize=specs.bluesize;
            actualAlphaSize=specs.alphasize;
            actualDepthSize=specs.depthsize;
            actualStencilSize=specs.stencilsize;
            actualMultiSamples=specs.multisamples;
            actualAccumRedSize=specs.accumredsize;
            actualAccumGreenSize=specs.accumgreensize;
            actualAccumBlueSize=specs.accumbluesize;
            actualAccumAlphaSize=specs.accumalphasize;
            doubleBuffer=specs.doublebuffer;
            stereoBuffer=specs.stereobuffer;
            accelerated=specs.accel;
            copying=specs.copy;
            bestmatch=match;
            best=v;
            }
          }
        }

      // Hopefully, we got one
      if(0<=best){
        FXTRACE((140,"Best Config: #%d: match=%d\n",best,bestmatch));

        // Fill in visual data
        depth=actualRedSize+actualGreenSize+actualBlueSize;
        numred=(1<<actualRedSize);
        numgreen=(1<<actualGreenSize);
        numblue=(1<<actualBlueSize);
        numcolors=numred*numgreen*numblue;

        // Make a palette for it if needed
        if(pfd.dwFlags&PFD_NEED_PALETTE){
          colormap=makeOpenGLPalette(&pfd);
          freemap=true;
          }

        // Remember best config
        xid=(void*)(FXival)best;
        }

      // Done with that window
      ::ReleaseDC(GetDesktopWindow(),hdc);

      // Make non-current
      wglMakeCurrent(nullptr,nullptr);
#endif
      }

    // Test if successful
    if(!xid){
      throw FXWindowException("unable to create GL context.");
      }
    }
  }


#else //////////////////////////////// X11 OLD //////////////////////////////////


#ifndef GLX_ARB_multisample
#define GLX_SAMPLE_BUFFERS_ARB             100000
#define GLX_SAMPLES_ARB                    100001
#endif
#ifndef GLX_EXT_visual_rating
#define GLX_VISUAL_CAVEAT_EXT              0x20
#define GLX_SLOW_VISUAL_EXT                0x8001
#define GLX_NON_CONFORMANT_VISUAL_EXT      0x800D
#endif


// Initialize
void FXGLVisual::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE((100,"%s::create %p\n",getClassName(),this));
#ifdef HAVE_GL_H
      int majoropcode,errorbase,eventbase;

      // Assume the default
      visual=DefaultVisual((Display*)getApp()->getDisplay(),DefaultScreen((Display*)getApp()->getDisplay()));
      depth=DefaultDepth((Display*)getApp()->getDisplay(),DefaultScreen((Display*)getApp()->getDisplay()));

      // OpenGL is available if we're talking to an OpenGL-capable X-Server
      if(XQueryExtension((Display*)getApp()->getDisplay(),"GLX",&majoropcode,&errorbase,&eventbase)){
        if(glXQueryExtension((Display*)getApp()->getDisplay(),&errorbase,&eventbase)){
          int major,minor;

          // Try get OpenGL version info
          if(glXQueryVersion((Display*)getApp()->getDisplay(),&major,&minor)){
            XVisualInfo vitemplate,*vi; int nvi;

            // Report version found
            FXTRACE((100,"Found GLX version: %d.%d (Major: %d, Error: %d, Event: %d)\n",major,minor,majoropcode,errorbase,eventbase));

            // Scan for all visuals of given screen
            vitemplate.screen=DefaultScreen((Display*)getApp()->getDisplay());
            vi=XGetVisualInfo((Display*)getApp()->getDisplay(),VisualScreenMask,&vitemplate,&nvi);
            if(vi){
              int defvisualid=XVisualIDFromVisual(DefaultVisual((Display*)getApp()->getDisplay(),DefaultScreen((Display*)getApp()->getDisplay())));
              int samplebuffers=0;
              int bestmatch=1000000000;
              int best=-1;
              int value;
              int match;

              FXTRACE((150,"Found %d configs\n",nvi));
              FXTRACE((150,"Default visualid=0x%02x\n",defvisualid));

              // Find the best one
              for(int v=0; v<nvi; v++){
                FXGLSpecs specs;

                // GL support is requested
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_USE_GL,&value)!=Success || (value==0)) continue;

                // Get supported drawable targets
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_RGBA,&value)!=Success || (value==0)) continue;

                // Get overlay level
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_LEVEL,&value)!=Success || (value!=0)) continue;

                // Get plane depths
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_RED_SIZE,&specs.redsize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_GREEN_SIZE,&specs.greensize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_BLUE_SIZE,&specs.bluesize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_ALPHA_SIZE,&specs.alphasize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_DEPTH_SIZE,&specs.depthsize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_STENCIL_SIZE,&specs.stencilsize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_ACCUM_RED_SIZE,&specs.accumredsize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_ACCUM_GREEN_SIZE,&specs.accumgreensize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_ACCUM_BLUE_SIZE,&specs.accumbluesize)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_ACCUM_ALPHA_SIZE,&specs.accumalphasize)!=Success) continue;

                // Get stereo and double buffer support
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_DOUBLEBUFFER,&specs.doublebuffer)!=Success) continue;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_STEREO,&specs.stereobuffer)!=Success) continue;

                // Get multisample support (if we don't succeed, set it to zero)
                specs.multisamples=0;
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_SAMPLE_BUFFERS_ARB,&samplebuffers)==Success && samplebuffers){
                  if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_SAMPLES_ARB,&specs.multisamples)!=Success) continue;
                  }

                // Check if accelerated or not (assume yes)
                if(glXGetConfig((Display*)getApp()->getDisplay(),&vi[v],GLX_VISUAL_CAVEAT_EXT,&specs.accel)==Success){
                  specs.accel=(specs.accel!=GLX_SLOW_VISUAL_EXT);        // Penalize if any caveats present
                  }
                else{
                  specs.accel=1;
                  }

                // Composition and swap copy
                specs.composition=false;
                specs.image=false;
                specs.copy=false;

                // Match specs
                match=matchSpecs(specs);

                // Trace
                FXTRACE((150,"Config: #%d: match=%d\n",v,match));
                FXTRACE((150,"  visualid   = 0x%02lx\n",vi[v].visualid));
                FXTRACE((150,"  red size   = %d\n",specs.redsize));
                FXTRACE((150,"  green size = %d\n",specs.greensize));
                FXTRACE((150,"  blue size  = %d\n",specs.bluesize));
                FXTRACE((150,"  alpha size = %d\n",specs.alphasize));
                FXTRACE((150,"  depth size = %d\n",specs.depthsize));
                FXTRACE((150,"  stencil    = %d\n",specs.stencilsize));
                FXTRACE((150,"  acc red    = %d\n",specs.accumredsize));
                FXTRACE((150,"  acc green  = %d\n",specs.accumgreensize));
                FXTRACE((150,"  acc blue   = %d\n",specs.accumbluesize));
                FXTRACE((150,"  acc alpha  = %d\n",specs.accumalphasize));
                FXTRACE((150,"  double buf = %d\n",specs.doublebuffer));
                FXTRACE((150,"  stereo buf = %d\n",specs.stereobuffer));
                FXTRACE((150,"  samples    = %d\n",specs.multisamples));
                FXTRACE((150,"  accel      = %d\n",specs.accel));

                // May the best config win
                if(match<=bestmatch){

                  // All other things being equal, we prefer default visual!
                  if((match<bestmatch) || (vi[v].visualid==defvisualid)){
                    actualRedSize=specs.redsize;
                    actualGreenSize=specs.greensize;
                    actualBlueSize=specs.bluesize;
                    actualAlphaSize=specs.alphasize;
                    actualDepthSize=specs.depthsize;
                    actualStencilSize=specs.stencilsize;
                    actualMultiSamples=specs.multisamples;
                    actualAccumRedSize=specs.accumredsize;
                    actualAccumGreenSize=specs.accumgreensize;
                    actualAccumBlueSize=specs.accumbluesize;
                    actualAccumAlphaSize=specs.accumalphasize;
                    doubleBuffer=specs.doublebuffer;
                    stereoBuffer=specs.stereobuffer;
                    accelerated=specs.accel;
                    copying=specs.copy;
                    bestmatch=match;
                    best=v;
                    }
                  }
                }

              // We should have one now
              if(0<=best){
                FXTRACE((140,"Best Config: #%d: match=%d\n",best,bestmatch));

                // Remember visual, depth, visualid
                visual=vi[best].visual;
                depth=vi[best].depth;

                // Initialize colormap
                setupcolormap();

                // Make GC's for this visual
                scrollgc=setupgc(true);
                gc=setupgc(false);

                // Remember best config
                xid=((Visual*)visual)->visualid;
                }

              // Free visual info
              XFree((char*)vi);
              }
            }
          }
        }
#endif
      }

    // Test if successful
    if(!xid){
      throw FXWindowException("no matching GL configuration.");
      }
    }
  }


#endif //////////////////////////////////////////////////////////////////////////


// Detach visual
void FXGLVisual::detach(){
#ifdef HAVE_GL_H
  if(xid){
    FXTRACE((100,"%s::detach %p\n",getClassName(),this));
    colormap=0;
    freemap=false;
    xid=0;
    }
#endif
  }


// Destroy visual
void FXGLVisual::destroy(){
#ifdef HAVE_GL_H
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE((100,"%s::destroy %p\n",getClassName(),this));
#ifdef WIN32
      if(colormap){ DeleteObject(colormap); }
#else
      if(freemap){ XFreeColormap((Display*)getApp()->getDisplay(),colormap); }
      if(scrollgc){ XFreeGC((Display*)getApp()->getDisplay(),(GC)scrollgc); }
      if(gc){ XFreeGC((Display*)getApp()->getDisplay(),(GC)gc); }
#endif
      }
#ifndef WIN32
    scrollgc=0;
    gc=0;
#endif
    colormap=0;
    freemap=false;
    xid=0;
    }
#endif
  }


// Save to stream
void FXGLVisual::save(FXStream& store) const {
  FXVisual::save(store);
  store << redSize;
  store << greenSize;
  store << blueSize;
  store << alphaSize;
  store << depthSize;
  store << stencilSize;
  store << multiSamples;
  store << accumRedSize;
  store << accumGreenSize;
  store << accumBlueSize;
  store << accumAlphaSize;
  }


// Load from stream
void FXGLVisual::load(FXStream& store){
  FXVisual::load(store);
  store >> redSize;
  store >> greenSize;
  store >> blueSize;
  store >> alphaSize;
  store >> depthSize;
  store >> stencilSize;
  store >> multiSamples;
  store >> accumRedSize;
  store >> accumGreenSize;
  store >> accumBlueSize;
  store >> accumAlphaSize;
  }


// Destroy
FXGLVisual::~FXGLVisual(){
  FXTRACE((100,"FXGLVisual::~FXGLVisual %p\n",this));
  destroy();
  }


}
