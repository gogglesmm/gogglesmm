/********************************************************************************
*                                                                               *
*                               I c o n - O b j e c t                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXIcon.h"


/*
  Notes:
  - Debug the render function between different hosts.
  - Option to guess alpha color from corners.
  - The shape pixmap [X11] actually contains TWO shapes:- the
    shape mask (transparency) as well as the etch mask (gray out&transparency).
    We do this because most icons are 16x16 or so, and we can fit two of them
    side-by-side in the space of one:- because the bitmaps are typically
    padded to a width of 32 anyway :-)  Either way, the overhead for the
    header will be smaller.
  - When we make FXImage always to have an alpha channel, add method to set
    alpha channel to transparent when color is equal to alpha-color.
    This simplifies the shape/etch mask procedure.
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001


#define DARKCOLOR(r,g,b) (((r)+(g)+(b))<thresh)

using namespace FX;

/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXIcon,FXImage,nullptr,0)


// For deserialization
FXIcon::FXIcon():shape(0),etch(0),transp(0),thresh(382){
  }


// Initialize nicely
FXIcon::FXIcon(FXApp* a,const FXColor *pix,FXColor clr,FXuint opts,FXint w,FXint h):FXImage(a,pix,opts,w,h),shape(0),etch(0),transp(clr),thresh(382){
  FXTRACE((TOPIC_CONSTRUCT,"FXIcon::FXIcon %p\n",this));
  }


// Guess alpha color based on corners; the initial guess is standard GUI color
FXColor FXIcon::guesstransp() const {
  FXColor guess=FXRGB(192,192,192);
  FXint best,t;
  FXColor color[4];
  if(data && 0<width && 0<height){
    best=-1;
    color[0]=getPixel(0,0);
    color[1]=getPixel(width-1,0);
    color[2]=getPixel(0,height-1);
    color[3]=getPixel(width-1,height-1);
    if((t=((color[0]==color[1])+(color[0]==color[2])+(color[0]==color[3])))>best){ guess=color[0]; best=t; }
    if((t=((color[1]==color[2])+(color[1]==color[3])+(color[1]==color[0])))>best){ guess=color[1]; best=t; }
    if((t=((color[2]==color[3])+(color[2]==color[0])+(color[2]==color[1])))>best){ guess=color[2]; best=t; }
    if((t=((color[3]==color[0])+(color[3]==color[1])+(color[3]==color[2])))>best){ guess=color[3]; }
    }
  return guess;
  }


// Determine threshold for etch mask
FXshort FXIcon::guessthresh() const {
  FXshort guess=382;
  if(data && 0<width && 0<height){
    FXint med=(width*height)>>1;
    FXint i,j,cum;
    FXint frequency[766];
    memset(frequency,0,sizeof(frequency));
    for(i=0; i<width*height; ++i){
      frequency[((const FXuchar*)(data+i))[2]+((const FXuchar*)(data+i))[1]+((const FXuchar*)(data+i))[0]]++;
      }
    for(i=0,cum=0; i<766; ++i){
      if((cum+=frequency[i])>=med) break;
      }
    for(j=765,cum=0; j>0; --j){
      if((cum+=frequency[j])>=med) break;
      }
    guess=((i+j+1)>>1)+1;               // Fanglin Zhu: raise threshold by one in case of single-color image
    }
  return guess;
  }


// Create icon
void FXIcon::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE((TOPIC_CREATION,"%s::create %p\n",getClassName(),this));

      // Initialize visual
      visual->create();

#ifdef WIN32
      // Create a memory DC compatible with current display
      HDC hdc=::GetDC(GetDesktopWindow());
      xid=::CreateCompatibleBitmap(hdc,FXMAX(width,1),FXMAX(height,1));
      ::ReleaseDC(GetDesktopWindow(),hdc);
      if(!xid){ fxerror("%s::create: unable to create image.\n",getClassName()); }

      // Make shape bitmap
      shape=::CreateBitmap(FXMAX(width,1),FXMAX(height,1),1,1,nullptr);
      if(!shape){ fxerror("%s::create: unable to create icon.\n",getClassName()); }

      // Make etch bitmap
      etch=::CreateBitmap(FXMAX(width,1),FXMAX(height,1),1,1,nullptr);
      if(!etch){ fxerror("%s::create: unable to create icon.\n",getClassName()); }
#else

      // Make image pixmap
      xid=XCreatePixmap((Display*)getApp()->getDisplay(),XDefaultRootWindow((Display*)getApp()->getDisplay()),FXMAX(width,1),FXMAX(height,1),visual->depth);
      if(!xid){ fxerror("%s::create: unable to create icon.\n",getClassName()); }

      // Make shape pixmap
      shape=XCreatePixmap((Display*)getApp()->getDisplay(),XDefaultRootWindow((Display*)getApp()->getDisplay()),FXMAX(width,1),FXMAX(height,1),1);
      if(!shape){ fxerror("%s::create: unable to create icon.\n",getClassName()); }

      // Make etch pixmap
      etch=XCreatePixmap((Display*)getApp()->getDisplay(),XDefaultRootWindow((Display*)getApp()->getDisplay()),FXMAX(width,1),FXMAX(height,1),1);
      if(!etch){ fxerror("%s::create: unable to create icon.\n",getClassName()); }
#endif

      // Render pixels
      render();

      // If we're not keeping the pixel buffer, release it
      if(!(options&IMAGE_KEEP)) release();
      }
    }
  }


// Detach icon
void FXIcon::detach(){
  visual->detach();
  if(xid){
    FXTRACE((TOPIC_CREATION,"%s::detach %p\n",getClassName(),this));
    shape=0;
    etch=0;
    xid=0;
    }
  }


// Destroy icon
void FXIcon::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE((TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this));
#ifdef WIN32
      ::DeleteObject(shape);
      ::DeleteObject(etch);
      ::DeleteObject(xid);
#else
      XFreePixmap((Display*)getApp()->getDisplay(),shape);
      XFreePixmap((Display*)getApp()->getDisplay(),etch);
      XFreePixmap((Display*)getApp()->getDisplay(),xid);
#endif
      }
    shape=0;
    etch=0;
    xid=0;
    }
  }


#ifdef WIN32            // WINDOWS


struct BITMAPINFO2 {
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD          bmiColors[2];
  };


// Render Icon MS-Windows
void FXIcon::render(){
  if(xid){
    FXint bytes_per_line,x,y;
    FXuchar *msk,*ets;
    FXColor *img;
    FXuchar *maskdata;
    FXuchar *etchdata;
    BITMAPINFO2 bmi;
    HDC hdcmsk;

    FXTRACE((TOPIC_CREATION,"%s::render %p\n",getClassName(),this));

    // Render the image (color) pixels as usual
    FXImage::render();

    // Fill with pixels if there is data
    if(data && 0<width && 0<height){

      // Set up the bitmap info
      bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
      bmi.bmiHeader.biWidth=width;
      bmi.bmiHeader.biHeight=height;
      bmi.bmiHeader.biPlanes=1;
      bmi.bmiHeader.biBitCount=1;
      bmi.bmiHeader.biCompression=BI_RGB;
      bmi.bmiHeader.biSizeImage=0;
      bmi.bmiHeader.biXPelsPerMeter=0;
      bmi.bmiHeader.biYPelsPerMeter=0;
      bmi.bmiHeader.biClrUsed=0;
      bmi.bmiHeader.biClrImportant=0;
      bmi.bmiColors[0].rgbBlue=0;
      bmi.bmiColors[0].rgbGreen=0;
      bmi.bmiColors[0].rgbRed=0;
      bmi.bmiColors[0].rgbReserved=0;
      bmi.bmiColors[1].rgbBlue=255;
      bmi.bmiColors[1].rgbGreen=255;
      bmi.bmiColors[1].rgbRed=255;
      bmi.bmiColors[1].rgbReserved=0;

      // Allocate temp bit buffer
      bytes_per_line=((width+31)&~31)>>3;
      callocElms(maskdata,height*bytes_per_line);
      callocElms(etchdata,height*bytes_per_line);

      msk=maskdata+height*bytes_per_line;
      ets=etchdata+height*bytes_per_line;
      if(options&IMAGE_OPAQUE){                 // Opaque image
        img=data;
        for(y=0; y<height; y++){
          ets-=bytes_per_line;
          for(x=0; x<width; x++){
            if(!DARKCOLOR(((FXuchar*)(img+x))[2],((FXuchar*)(img+x))[1],((FXuchar*)(img+x))[0])){ ets[x>>3]|=0x80>>(x&7); }
            }
          img+=width;
          }
        }
      else if(options&(IMAGE_ALPHACOLOR|IMAGE_ALPHAGUESS)){     // Transparent color
        img=data;
        for(y=0; y<height; y++){
          msk-=bytes_per_line;
          ets-=bytes_per_line;
          for(x=0; x<width; x++){
            if(!DARKCOLOR(((FXuchar*)(img+x))[2],((FXuchar*)(img+x))[1],((FXuchar*)(img+x))[0])){ ets[x>>3]|=0x80>>(x&7); }
            if(img[x]==transp){ msk[x>>3]|=0x80>>(x&7); ets[x>>3]|=0x80>>(x&7); }
            }
          img+=width;
          }
        }
      else{                                     // Transparency channel
        img=data;
        for(y=0; y<height; y++){
          msk-=bytes_per_line;
          ets-=bytes_per_line;
          for(x=0; x<width; x++){
            if(!DARKCOLOR(((FXuchar*)(img+x))[2],((FXuchar*)(img+x))[1],((FXuchar*)(img+x))[0])){ ets[x>>3]|=0x80>>(x&7); }
            if(((FXuchar*)(img+x))[3]==0){ msk[x>>3]|=0x80>>(x&7); ets[x>>3]|=0x80>>(x&7); }
            }
          img+=width;
          }
        }

      // The MSDN documentation for SetDIBits() states that "the device context
      // identified by the (first) parameter is used only if the DIB_PAL_COLORS
      // constant is set for the (last) parameter". This may be true, but under
      // Win95 you must pass in a non-NULL hdc for the first parameter; otherwise
      // this call to SetDIBits() will fail (in contrast, it works fine under
      // Windows NT if you pass in a NULL hdc).
      hdcmsk=::CreateCompatibleDC(nullptr);

      // Set mask data
      if(!::SetDIBits(hdcmsk,(HBITMAP)shape,0,height,maskdata,(BITMAPINFO*)&bmi,DIB_RGB_COLORS)){
        fxerror("%s::render: unable to render pixels\n",getClassName());
        }

      // Set etch data
      if(!::SetDIBits(hdcmsk,(HBITMAP)etch,0,height,etchdata,(BITMAPINFO*)&bmi,DIB_RGB_COLORS)){
        fxerror("%s::render: unable to render pixels\n",getClassName());
        }
      freeElms(maskdata);
      freeElms(etchdata);
      ::GdiFlush();

      // We AND the image with the mask, then we can do faster and more
      // flicker-free icon painting later using the `black source' method
      HBITMAP hmsk=(HBITMAP)::SelectObject(hdcmsk,(HBITMAP)shape);
      HDC hdcmem=::CreateCompatibleDC(nullptr);
      HBITMAP hbmp=(HBITMAP)::SelectObject(hdcmem,(HBITMAP)xid);
      ::SetBkColor(hdcmem,RGB(0,0,0));                // 1 -> black
      ::SetTextColor(hdcmem,RGB(255,255,255));        // 0 -> white
      ::BitBlt(hdcmem,0,0,width,height,hdcmsk,0,0,SRCAND);
      ::SelectObject(hdcmem,hbmp);
      ::SelectObject(hdcmsk,hmsk);
      ::DeleteDC(hdcmem);
      ::DeleteDC(hdcmsk);
      }
    }
  }


#else                   // X11


// Render icon X Windows
void FXIcon::render(){
  if(xid){
    XImage *xim=nullptr;
    FXbool shmi=false;
    FXColor *img;
    FXint x,y;
    XGCValues values;
    GC gc;
#ifdef HAVE_XSHM_H
    XShmSegmentInfo shminfo;
#endif

    FXTRACE((TOPIC_CREATION,"%s::render shape %p\n",getClassName(),this));

    // Render the image pixels
    FXImage::render();

    // Fill with pixels if there is data
    if(data && 0<width && 0<height){

      // Turn it on iff both supported and desired
#ifdef HAVE_XSHM_H
      if(options&IMAGE_SHMI) shmi=getApp()->shmi;
#endif

      // First try XShm
#ifdef HAVE_XSHM_H
      if(shmi){
        xim=XShmCreateImage((Display*)getApp()->getDisplay(),(Visual*)visual->visual,1,ZPixmap,nullptr,&shminfo,width,height);
        if(!xim){ shmi=0; }
        if(shmi){
          shminfo.shmid=shmget(IPC_PRIVATE,xim->bytes_per_line*xim->height,IPC_CREAT|0777);
          if(shminfo.shmid==-1){ xim->data=nullptr; XDestroyImage(xim); xim=nullptr; shmi=0; }
          if(shmi){
            shminfo.shmaddr=xim->data=(char*)shmat(shminfo.shmid,0,0);
            shminfo.readOnly=false;
            XShmAttach((Display*)getApp()->getDisplay(),&shminfo);
            FXTRACE((150,"Bitmap XSHM attached at memory=%p (%d bytes)\n",xim->data,xim->bytes_per_line*xim->height));
            }
          }
        }
#endif

      // Try the old fashioned way
      if(!shmi){

        // Try create image
        xim=XCreateImage((Display*)getApp()->getDisplay(),(Visual*)visual->visual,1,ZPixmap,0,nullptr,width,height,32,0);
        if(!xim){ fxerror("%s::render: unable to render icon.\n",getClassName()); }

        // Try create temp pixel store
        if(!callocElms(xim->data,xim->bytes_per_line*height)){ fxerror("%s::render: unable to allocate memory.\n",getClassName()); }
        }

      // Make GC
      values.foreground=0xffffffff;
      values.background=0xffffffff;
      gc=XCreateGC((Display*)getApp()->getDisplay(),shape,GCForeground|GCBackground,&values);

      // Should have succeeded
      FXASSERT(xim);

      FXTRACE((150,"bm width = %d\n",xim->width));
      FXTRACE((150,"bm height = %d\n",xim->height));
      FXTRACE((150,"bm format = %s\n",xim->format==XYBitmap?"XYBitmap":xim->format==XYPixmap?"XYPixmap":"ZPixmap"));
      FXTRACE((150,"bm byte_order = %s\n",(xim->byte_order==MSBFirst)?"MSBFirst":"LSBFirst"));
      FXTRACE((150,"bm bitmap_unit = %d\n",xim->bitmap_unit));
      FXTRACE((150,"bm bitmap_bit_order = %s\n",(xim->bitmap_bit_order==MSBFirst)?"MSBFirst":"LSBFirst"));
      FXTRACE((150,"bm bitmap_pad = %d\n",xim->bitmap_pad));
      FXTRACE((150,"bm bitmap_unit = %d\n",xim->bitmap_unit));
      FXTRACE((150,"bm depth = %d\n",xim->depth));
      FXTRACE((150,"bm bytes_per_line = %d\n",xim->bytes_per_line));
      FXTRACE((150,"bm bits_per_pixel = %d\n",xim->bits_per_pixel));

      // Fill shape mask
      if(options&IMAGE_OPAQUE){                                 // Opaque image
        FXTRACE((150,"Shape rectangle\n"));
        memset(xim->data,0xff,xim->bytes_per_line*height);
        }
      else if(options&(IMAGE_ALPHACOLOR|IMAGE_ALPHAGUESS)){     // Transparent color
        FXTRACE((150,"Shape from alpha-color\n"));
        img=data;
        for(y=0; y<height; y++){
          for(x=0; x<width; x++){
            XPutPixel(xim,x,y,(img[x]!=transp));
            }
          img+=width;
          }
        }
      else{                                                     // Transparency channel
        FXTRACE((150,"Shape from alpha-channel\n"));
        img=data;
        for(y=0; y<height; y++){
          for(x=0; x<width; x++){
            XPutPixel(xim,x,y,(((FXuchar*)(img+x))[3]!=0));
            }
          img+=width;
          }
        }

      // Transfer image
#ifdef HAVE_XSHM_H
      if(shmi){
        XShmPutImage((Display*)getApp()->getDisplay(),shape,gc,xim,0,0,0,0,width,height,False);
        XSync((Display*)getApp()->getDisplay(),False);
        }
#endif
      if(!shmi){
        XPutImage((Display*)getApp()->getDisplay(),shape,gc,xim,0,0,0,0,width,height);
        }

      // Fill etch image
      if(options&IMAGE_OPAQUE){                                 // Opaque image
        img=data;
        for(y=0; y<height; y++){
          for(x=0; x<width; x++){
            XPutPixel(xim,x,y,DARKCOLOR(((FXuchar*)(img+x))[2],((FXuchar*)(img+x))[1],((FXuchar*)(img+x))[0]));
            }
          img+=width;
          }
        }
      else if(options&(IMAGE_ALPHACOLOR|IMAGE_ALPHAGUESS)){     // Transparent color
        img=data;
        for(y=0; y<height; y++){
          for(x=0; x<width; x++){
            XPutPixel(xim,x,y,(img[x]!=transp) && DARKCOLOR(((FXuchar*)(img+x))[2],((FXuchar*)(img+x))[1],((FXuchar*)(img+x))[0]));
            }
          img+=width;
          }
        }
      else{                                                     // Transparency channel
        img=data;
        for(y=0; y<height; y++){
          for(x=0; x<width; x++){
            XPutPixel(xim,x,y,(((FXuchar*)(img+x))[3]!=0) && DARKCOLOR(((FXuchar*)(img+x))[2],((FXuchar*)(img+x))[1],((FXuchar*)(img+x))[0]));
            }
          img+=width;
          }
        }

      // Transfer image
#ifdef HAVE_XSHM_H
      if(shmi){
        XShmPutImage((Display*)getApp()->getDisplay(),etch,gc,xim,0,0,0,0,width,height,False);
        XSync((Display*)getApp()->getDisplay(),False);
        }
#endif
      if(!shmi){
        XPutImage((Display*)getApp()->getDisplay(),etch,gc,xim,0,0,0,0,width,height);
        }

      // Clean up
#ifdef HAVE_XSHM_H
      if(shmi){
        FXTRACE((150,"Bitmap XSHM detached at memory=%p (%d bytes)\n",xim->data,xim->bytes_per_line*xim->height));
        XShmDetach((Display*)getApp()->getDisplay(),&shminfo);
        xim->data=nullptr;
        XDestroyImage(xim);
        shmdt(shminfo.shmaddr);
        shmctl(shminfo.shmid,IPC_RMID,0);
        }
#endif
      if(!shmi){
        freeElms(xim->data);
        XDestroyImage(xim);
        }
      XFreeGC((Display*)getApp()->getDisplay(),gc);
      }
    }
  }

#endif


// Resize pixmap to the specified width and height; the contents become undefined
void FXIcon::resize(FXint w,FXint h){
  if(w<1) w=1;
  if(h<1) h=1;
  FXTRACE((TOPIC_CREATION,"%s::resize(%d,%d) %p\n",getClassName(),w,h,this));
  if(width!=w || height!=h){

    // Resize device dependent pixmap
    if(xid){
#ifdef WIN32
      // Delete old bitmaps
      ::DeleteObject(xid);
      ::DeleteObject(shape);
      ::DeleteObject(etch);

      // Create a bitmap compatible with current display
      HDC hdc=::GetDC(GetDesktopWindow());
      xid=::CreateCompatibleBitmap(hdc,w,h);
      ::ReleaseDC(GetDesktopWindow(),hdc);
      if(!xid){ fxerror("%s::resize: unable to resize image.\n",getClassName()); }

      // Make shape bitmap
      shape=::CreateBitmap(w,h,1,1,nullptr);
      if(!shape){ fxerror("%s::create: unable to create icon.\n",getClassName()); }

      // Make etch bitmap
      etch=::CreateBitmap(w,h,1,1,nullptr);
      if(!etch){ fxerror("%s::create: unable to create icon.\n",getClassName()); }
#else
      // Get depth (should use visual!!)
      int dd=visual->getDepth();

      // Free old pixmaps
      XFreePixmap((Display*)getApp()->getDisplay(),xid);
      XFreePixmap((Display*)getApp()->getDisplay(),etch);
      XFreePixmap((Display*)getApp()->getDisplay(),shape);

      // Make new pixmap
      xid=XCreatePixmap((Display*)getApp()->getDisplay(),XDefaultRootWindow((Display*)getApp()->getDisplay()),w,h,dd);
      if(!xid){ fxerror("%s::resize: unable to resize image.\n",getClassName()); }

      // Make shape pixmap
      shape=XCreatePixmap((Display*)getApp()->getDisplay(),XDefaultRootWindow((Display*)getApp()->getDisplay()),w,h,1);
      if(!shape){ fxerror("%s::create: unable to create icon.\n",getClassName()); }

      // Make etch pixmap
      etch=XCreatePixmap((Display*)getApp()->getDisplay(),XDefaultRootWindow((Display*)getApp()->getDisplay()),w,h,1);
      if(!etch){ fxerror("%s::create: unable to create icon.\n",getClassName()); }
#endif
      }
    }

  // Resize data array
  if(data){
    if(!(options&IMAGE_OWNED)){       // Need to own array
      allocElms(data,w*h);
      options|=IMAGE_OWNED;
      }
    else if(w*h!=width*height){
      resizeElms(data,w*h);
      }
    }

  // Remember new size
  width=w;
  height=h;
  }



// Clean up
FXIcon::~FXIcon(){
  FXTRACE((TOPIC_CONSTRUCT,"FXIcon::~FXIcon %p\n",this));
  destroy();
  }

}
