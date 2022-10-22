/********************************************************************************
*                                                                               *
*                         C u r s o r - O b j e c t                             *
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
#include "FXColors.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXVisual.h"
#include "FXCursor.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXException.h"


/*
  Notes:
  - Cursor size should be less than or equal to 32x32; limitation in Windows!
  - Need standard glyph for "invisible" cursor.
  - Keep hotx and hoty INSIDE the cursor glyph!!
  - Thanks Niall Douglas <s_sourceforge@nedprod.com> for the changes for
    alpha-blended cursors.
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001

#define DISPLAY(app)     ((Display*)((app)->display))
#define DARKCOLOR(r,g,b) (((r)+(g)+(b))<382)
#define CURSOR_STOCK     255
#define CURSOR_MASK      (CURSOR_KEEP)

using namespace FX;

/*******************************************************************************/

namespace FX {

extern FXbool fxloadXBM(FXColor*& data,const FXuchar *pixels,const FXuchar *mask,FXint width,FXint height);


// Object implementation
FXIMPLEMENT(FXCursor,FXId,nullptr,0)


// Deserialization
FXCursor::FXCursor(){
  data=nullptr;
  width=0;
  height=0;
  hotx=0;
  hoty=0;
  options=CURSOR_ARROW;
  }


// Make stock cursor
FXCursor::FXCursor(FXApp* a,FXStockCursor curid):FXId(a){
  FXTRACE((TOPIC_CONSTRUCT,"FXCursor::FXCursor %p\n",this));
  data=nullptr;
  width=0;
  height=0;
  hotx=0;
  hoty=0;
  options=curid;
  }


// Make cursor from source and mask
FXCursor::FXCursor(FXApp* a,const FXuchar* src,const FXuchar* msk,FXint w,FXint h,FXint hx,FXint hy):FXId(a){
  FXTRACE((TOPIC_CONSTRUCT,"FXCursor::FXCursor %p\n",this));
  fxloadXBM(data,src,msk,w,h);
  width=w;
  height=h;
  hotx=FXCLAMP(0,hx,width-1);
  hoty=FXCLAMP(0,hy,height-1);
  options=CURSOR_OWNED;
  }


// Make cursor from FXColor pixels
FXCursor::FXCursor(FXApp* a,const FXColor *pix,FXint w,FXint h,FXint hx,FXint hy):FXId(a){
  FXTRACE((TOPIC_CONSTRUCT,"FXCursor::FXCursor %p\n",this));
  data=const_cast<FXColor*>(pix);
  width=w;
  height=h;
  hotx=FXCLAMP(0,hx,width-1);
  hoty=FXCLAMP(0,hy,height-1);
  options=0;
  }


// Return true if color cursor
FXbool FXCursor::isColor() const {
  if(data){
    for(FXint i=width*height-1; 0<=i; i--){
      if(data[i]!=FXColors::Black && data[i]!=FXColors::White && FXALPHAVAL(data[i])!=0) return true;
      }
    }
  return false;
  }


// Create cursor
void FXCursor::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE((TOPIC_CREATION,"%s::create %p\n",getClassName(),this));
#if defined(WIN32)      // WIN32

      // Mapping to standard WIN32 cursors
      const LPCTSTR stock[]={IDC_ARROW,IDC_ARROW,IDC_ARROW,IDC_IBEAM,IDC_WAIT,IDC_CROSS,IDC_SIZENS,IDC_SIZEWE,IDC_SIZEALL};

      FXASSERT_STATIC(sizeof(FXID)>=sizeof(HCURSOR));

      // Building stock cursor
      if(options&CURSOR_STOCK){
        FXTRACE((TOPIC_CREATION,"%s::create: stock cursor\n",getClassName()));
        xid=LoadCursor(nullptr,stock[options&CURSOR_STOCK]);
        }

      // Building custom cursor
      else{

        // Should have data
        if(!data){ fxerror("%s::create: cursor needs pixel data.\n",getClassName()); }

        // Let's hope it's the correct size!
        if(width>32 || height>32){ fxerror("%s::create: cursor exceeds maximum size of 32x32 pixels\n",getClassName()); }

        // We have support for color cursors (WindowXP and up) and its a color cursor
        OSVERSIONINFO osvi={sizeof(OSVERSIONINFO)};
        if(isColor() && GetVersionEx((OSVERSIONINFO*)&osvi) && (osvi.dwPlatformId==VER_PLATFORM_WIN32_NT) && (osvi.dwMajorVersion==5) && (osvi.dwMinorVersion>=0 || osvi.dwMajorVersion>5)){
          const BITMAPV4HEADER bi={sizeof(BITMAPV4HEADER),32,-32,1,32,BI_BITFIELDS,0,0,0,0,0,0x00FF0000,0x0000FF00,0x000000FF,0xFF000000,0,{{0,0,0},{0,0,0},{0,0,0}},0,0,0};
          FXTRACE((TOPIC_CREATION,"%s::create: custom color %dx%d cursor\n",getClassName(),width,height));

          // Make a DIB
          void *imgdata=0;
          HDC hdc=GetDC(nullptr);
          HBITMAP himage=CreateDIBSection(hdc,(BITMAPINFO*)&bi,DIB_RGB_COLORS,&imgdata,nullptr,0);
          ReleaseDC(nullptr,hdc);
          if(!himage){ throw FXImageException("unable to create cursor"); }

          // Fill in data
          FXColor *imgptr=(FXColor*)imgdata;
          FXColor *srcimgptr=data;
          memset(imgdata,0,32*32*sizeof(FXColor));
          for(FXint j=0; j<height; j++){
            for(FXint i=0; i<width; i++){
              *imgptr++=*srcimgptr++;
              }
            imgptr+=(32-width);
            }

          // Strawman mask bitmap
          HBITMAP hmask=CreateBitmap(32,32,1,1,nullptr);
          if(!hmask){ throw FXImageException("unable to create cursor"); }

          // Create cursor
          ICONINFO iconinfo;
          iconinfo.fIcon=false;
          iconinfo.xHotspot=hotx;
          iconinfo.yHotspot=hoty;
          iconinfo.hbmMask=hmask;
          iconinfo.hbmColor=himage;
          xid=CreateIconIndirect(&iconinfo);

          // No longer needed
          DeleteObject(hmask);
          DeleteObject(himage);
          }

        // No support for color cursor or simple black/white cursor
        else{
          FXuchar tmpxor[128];
          FXuchar tmpand[128];
          FXint   srcoffset=0;
          FXint   dstoffset=0;
          FXTRACE((TOPIC_CREATION,"%s::create: custom b/w %dx%d cursor\n",getClassName(),width,height));
          memset(tmpand,0xff,sizeof(tmpand));
          memset(tmpxor,0x00,sizeof(tmpxor));
          for(FXint j=0; j<height; j++){
            for(FXint i=0; i<width; i++){
              if(((FXuchar*)(data+srcoffset+i))[3]>=128){
                tmpand[dstoffset+(i>>3)]&=~(128>>(i&7));
                if(!DARKCOLOR(((FXuchar*)(data+srcoffset+i))[2],((FXuchar*)(data+srcoffset+i))[1],((FXuchar*)(data+srcoffset+i))[0])){
                  tmpxor[dstoffset+(i>>3)]|=(128>>(i&7));
                  }
                }
              }
            srcoffset+=width;
            dstoffset+=4;
            }
          xid=CreateCursor((HINSTANCE)(getApp()->display),hotx,hoty,32,32,tmpand,tmpxor);
          }
        }

#else                   // X11

      // Mapping to standard X11 cursors
      const FXuint stock[]={XC_left_ptr,XC_left_ptr,XC_right_ptr,XC_xterm,XC_watch,XC_crosshair,XC_sb_h_double_arrow,XC_sb_v_double_arrow,XC_fleur};

      FXASSERT_STATIC(sizeof(FXID)>=sizeof(Cursor));

      // Building stock cursor
      if(options&CURSOR_STOCK){
        FXTRACE((TOPIC_CREATION,"%s::create: stock cursor\n",getClassName()));
        xid=XCreateFontCursor(DISPLAY(getApp()),stock[options&CURSOR_STOCK]);
        }

      // Building custom cursor
      else{

        // Should have data
        if(!data){ fxerror("%s::create: cursor needs pixel data.\n",getClassName()); }

        // Let's hope it's the correct size!
        if(width>32 || height>32){ fxerror("%s::create: cursor exceeds maximum size of 32x32 pixels\n",getClassName()); }

        // We have support for color cursors and its a color cursor
#ifdef HAVE_XCURSOR_H
        if(isColor() && XcursorSupportsARGB(DISPLAY(getApp()))){
          FXTRACE((TOPIC_CREATION,"%s::create: custom color %dx%d cursor\n",getClassName(),width,height));
          XcursorImage *image=XcursorImageCreate(width,height);
          image->xhot=hotx;
          image->yhot=hoty;
          for(FXint s=0; s<width*height; s++){
#ifdef __APPLE__
            // A bug in Apple's X11 implementation has components reversed
            ((FXuchar(*)[4])image->pixels)[s][0]=((FXuchar(*)[4])data)[s][3];   // A
            ((FXuchar(*)[4])image->pixels)[s][1]=((FXuchar(*)[4])data)[s][2];   // R
            ((FXuchar(*)[4])image->pixels)[s][2]=((FXuchar(*)[4])data)[s][1];   // G
            ((FXuchar(*)[4])image->pixels)[s][3]=((FXuchar(*)[4])data)[s][0];   // B
#else
            image->pixels[s]=data[s];
#endif
            }
          xid=XcursorImageLoadCursor(DISPLAY(getApp()),image);
          XcursorImageDestroy(image);
          }

        // No support for color cursor or simple black/white cursor
        else{
#endif
          FXuchar shapebits[128];
          FXuchar maskbits[128];
          XColor  color[2];
          FXint   dstbytes=(width+7)/8;
          FXint   srcoffset=0;
          FXint   dstoffset=0;
          FXTRACE((TOPIC_CREATION,"%s::create: custom b/w %dx%d cursor\n",getClassName(),width,height));
          color[0].pixel=BlackPixel(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));
          color[1].pixel=WhitePixel(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp())));
          color[0].flags=DoRed|DoGreen|DoBlue;
          color[1].flags=DoRed|DoGreen|DoBlue;
          XQueryColors(DISPLAY(getApp()),DefaultColormap(DISPLAY(getApp()),DefaultScreen(DISPLAY(getApp()))),color,2);
          memset(shapebits,0,sizeof(shapebits));
          memset(maskbits,0,sizeof(maskbits));
          for(FXint j=0; j<height; j++){
            for(FXint i=0; i<width; i++){
              if(((FXuchar*)(data+srcoffset+i))[3]>=128){
                maskbits[dstoffset+(i>>3)]|=(1<<(i&7));
                if(DARKCOLOR(((FXuchar*)(data+srcoffset+i))[2],((FXuchar*)(data+srcoffset+i))[1],((FXuchar*)(data+srcoffset+i))[0])) shapebits[dstoffset+(i>>3)]|=(1<<(i&7));
                }
              }
            srcoffset+=width;
            dstoffset+=dstbytes;
            }
          Pixmap srcpix=XCreateBitmapFromData(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),(char*)shapebits,width,height);
          if(!srcpix){ throw FXImageException("unable to create cursor"); }
          Pixmap mskpix=XCreateBitmapFromData(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),(char*)maskbits,width,height);
          if(!mskpix){ throw FXImageException("unable to create cursor"); }
          xid=XCreatePixmapCursor(DISPLAY(getApp()),srcpix,mskpix,&color[0],&color[1],hotx,hoty);
          XFreePixmap(DISPLAY(getApp()),srcpix);
          XFreePixmap(DISPLAY(getApp()),mskpix);
#ifdef HAVE_XCURSOR_H
          }
#endif
        }
#endif

      // Were we successful?
      if(!xid){ throw FXImageException("unable to create cursor"); }

      // Release pixel buffer
      if(!(options&CURSOR_KEEP)) release();
      }
    }
  }


// Detach cursor
void FXCursor::detach(){
  if(xid){
    FXTRACE((TOPIC_CREATION,"%s::detach %p\n",getClassName(),this));
    xid=0;
    }
  }


// Release pixels buffer if it was owned
void FXCursor::release(){
  if(options&CURSOR_OWNED){
    options&=~CURSOR_OWNED;
    freeElms(data);
    }
  data=nullptr;
  }


// Destroy cursor
void FXCursor::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE((TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this));
#if defined(WIN32)
      DestroyCursor((HCURSOR)xid);
#else
      XFreeCursor(DISPLAY(getApp()),xid);
#endif
      }
    xid=0;
    }
  }


// Change options
void FXCursor::setOptions(FXuint opts){
  options=(options&~CURSOR_MASK) | (opts&CURSOR_MASK);
  }


// Set pixel data ownership flag
void FXCursor::setOwned(FXbool owned){
  options^=((0-owned)^options)&CURSOR_OWNED;
  }


// Get pixel ownership flag
FXbool FXCursor::isOwned() const {
  return (options&CURSOR_OWNED)!=0;
  }


// Save pixel data only
FXbool FXCursor::savePixels(FXStream& store) const {
  FXuint size=width*height;
  store.save(data,size);
  return true;
  }


// Load pixel data only
FXbool FXCursor::loadPixels(FXStream& store){
  FXuint size=width*height;
  if(options&CURSOR_OWNED){freeElms(data);}
  if(!allocElms(data,size)) return false;
  store.load(data,size);
  options|=CURSOR_OWNED;
  return true;
  }


// Save cursor to stream
void FXCursor::save(FXStream& store) const {
  FXuchar haspixels=(data!=nullptr);
  FXId::save(store);
  store << width << height << hotx << hoty;
  store << options;
  store << haspixels;
  if(haspixels) savePixels(store);
  }


// Load cursor from stream
void FXCursor::load(FXStream& store){
  FXuchar haspixels;
  FXId::load(store);
  store >> width >> height >> hotx >> hoty;
  store >> options;
  store >> haspixels;
  if(haspixels) loadPixels(store);
  }


// Clean up
FXCursor::~FXCursor(){
  FXTRACE((TOPIC_CONSTRUCT,"FXCursor::~FXCursor %p\n",this));
  destroy();
  if(options&CURSOR_OWNED){freeElms(data);}
  data=(FXColor *)-1L;
  }

}
