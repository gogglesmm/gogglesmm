/********************************************************************************
*                                                                               *
*                            I c o n   S o u r c e                              *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXStream.h"
#include "FXFile.h"
#include "FXFileStream.h"
#include "FXMemoryStream.h"
#include "FXString.h"
#include "FXPath.h"
#include "FXIcon.h"
#include "FXImage.h"
#include "FXIconSource.h"

// Built-in icon formats
#include "FXBMPIcon.h"
#include "FXGIFIcon.h"
#include "FXICOIcon.h"
#include "FXIFFIcon.h"
#include "FXPCXIcon.h"
#include "FXPPMIcon.h"
#include "FXRASIcon.h"
#include "FXRGBIcon.h"
#include "FXTGAIcon.h"
#include "FXXBMIcon.h"
#include "FXXPMIcon.h"
#include "FXDDSIcon.h"
#include "FXEXEIcon.h"

// Built-in image formats
#include "FXBMPImage.h"
#include "FXGIFImage.h"
#include "FXICOImage.h"
#include "FXIFFImage.h"
#include "FXPCXImage.h"
#include "FXPPMImage.h"
#include "FXRASImage.h"
#include "FXRGBImage.h"
#include "FXTGAImage.h"
#include "FXXBMImage.h"
#include "FXXPMImage.h"
#include "FXDDSImage.h"
#include "FXEXEImage.h"

// Formats requiring external libraries
#ifndef CORE_IMAGE_FORMATS
#ifdef HAVE_JPEG_H
#include "FXJPGIcon.h"
#include "FXJPGImage.h"
#endif
#ifdef HAVE_PNG_H
#include "FXPNGIcon.h"
#include "FXPNGImage.h"
#endif
#ifdef HAVE_TIFF_H
#include "FXTIFIcon.h"
#include "FXTIFImage.h"
#endif
#endif
#ifdef HAVE_JP2_H
#include "FXJP2Icon.h"
#include "FXJP2Image.h"
#endif
#ifdef HAVE_WEBP_H
#include "FXWEBPIcon.h"
#include "FXWEBPImage.h"
#endif

/*
  Notes:
  - Either load an icon from a file, or load from already open stream.
  - Recognition of some image/icon types based on contents may be less
    certain due to poorly defined signature information in the file.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// Default icon source used when none provided
FXIconSource FXIconSource::defaultIconSource;


// Object implementation
FXIMPLEMENT(FXIconSource,FXObject,nullptr,0)


// Scale image or icon to size
FXImage* FXIconSource::scaleToSize(FXImage *image,FXint size,FXint qual) const {
  if(image){
    if((image->getWidth()>size) || (image->getHeight()>size)){
      if(image->getWidth()>image->getHeight()){
        image->scale(size,(size*image->getHeight())/image->getWidth(),qual);
        }
      else{
        image->scale((size*image->getWidth())/image->getHeight(),size,qual);
        }
      }
    }
  return image;
  }


// Create icon from file type
FXIcon *FXIconSource::iconFromType(FXApp* app,const FXString& type) const {
  if(FXString::comparecase(FXBMPIcon::fileExt,type)==0){
    return new FXBMPIcon(app,nullptr,0,IMAGE_ALPHAGUESS);
    }
  if(FXString::comparecase(FXGIFIcon::fileExt,type)==0){
    return new FXGIFIcon(app);
    }
  if(FXString::comparecase(FXICOIcon::fileExt,type)==0 || FXString::comparecase("cur",type)==0){
    return new FXICOIcon(app);
    }
  if(FXString::comparecase(FXIFFIcon::fileExt,type)==0 || FXString::comparecase("lbm",type)==0){
    return new FXIFFIcon(app);
    }
  if(FXString::comparecase(FXPCXIcon::fileExt,type)==0){
    return new FXPCXIcon(app);
    }
  if(FXString::comparecase(FXPPMIcon::fileExt,type)==0 || FXString::comparecase("pbm",type)==0 || FXString::comparecase("pgm",type)==0 || FXString::comparecase("pnm",type)==0){
    return new FXPPMIcon(app);
    }
  if(FXString::comparecase(FXRASIcon::fileExt,type)==0){
    return new FXRASIcon(app);
    }
  if(FXString::comparecase(FXRGBIcon::fileExt,type)==0){
    return new FXRGBIcon(app);
    }
  if(FXString::comparecase(FXTGAIcon::fileExt,type)==0){
    return new FXTGAIcon(app);
    }
  if(FXString::comparecase(FXXBMIcon::fileExt,type)==0){
    return new FXXBMIcon(app);
    }
  if(FXString::comparecase(FXXPMIcon::fileExt,type)==0){
    return new FXXPMIcon(app);
    }
  if(FXString::comparecase(FXDDSIcon::fileExt,type)==0){
    return new FXDDSIcon(app);
    }
  if(FXString::comparecase(FXEXEIcon::fileExt,type)==0){
    return new FXEXEIcon(app);
    }
#ifndef CORE_IMAGE_FORMATS
#ifdef HAVE_JPEG_H
  if(FXString::comparecase(FXJPGIcon::fileExt,type)==0 || FXString::comparecase("jpeg",type)==0){
    return new FXJPGIcon(app);
    }
#endif
#ifdef HAVE_PNG_H
  if(FXString::comparecase(FXPNGIcon::fileExt,type)==0){
    return new FXPNGIcon(app);
    }
#endif
#ifdef HAVE_TIFF_H
  if(FXString::comparecase(FXTIFIcon::fileExt,type)==0 || FXString::comparecase("tiff",type)==0){
    return new FXTIFIcon(app);
    }
#endif
#ifdef HAVE_JP2_H
  if(FXString::comparecase(FXJP2Icon::fileExt,type)==0){
    return new FXJP2Icon(app);
    }
#endif
#ifdef HAVE_WEBP_H
  if(FXString::comparecase(FXWEBPIcon::fileExt,type)==0){
    return new FXWEBPIcon(app);
    }
#endif
#endif
  return nullptr;
  }


// Create image from file type
FXImage *FXIconSource::imageFromType(FXApp* app,const FXString& type) const {
  if(FXString::comparecase(FXBMPImage::fileExt,type)==0){
    return new FXBMPImage(app);
    }
  if(FXString::comparecase(FXGIFImage::fileExt,type)==0){
    return new FXGIFImage(app);
    }
  if(FXString::comparecase(FXICOImage::fileExt,type)==0 || FXString::comparecase("cur",type)==0){
    return new FXICOImage(app);
    }
  if(FXString::comparecase(FXIFFImage::fileExt,type)==0 || FXString::comparecase("lbm",type)==0){
    return new FXIFFImage(app);
    }
  if(FXString::comparecase(FXPCXImage::fileExt,type)==0){
    return new FXPCXImage(app);
    }
  if(FXString::comparecase(FXPPMImage::fileExt,type)==0 || FXString::comparecase("pbm",type)==0 || FXString::comparecase("pgm",type)==0 || FXString::comparecase("pnm",type)==0){
    return new FXPPMImage(app);
    }
  if(FXString::comparecase(FXRASImage::fileExt,type)==0){
    return new FXRASImage(app);
    }
  if(FXString::comparecase(FXRGBImage::fileExt,type)==0){
    return new FXRGBImage(app);
    }
  if(FXString::comparecase(FXTGAImage::fileExt,type)==0){
    return new FXTGAImage(app);
    }
  if(FXString::comparecase(FXXBMImage::fileExt,type)==0){
    return new FXXBMImage(app);
    }
  if(FXString::comparecase(FXXPMImage::fileExt,type)==0){
    return new FXXPMImage(app);
    }
  if(FXString::comparecase(FXDDSImage::fileExt,type)==0){
    return new FXDDSImage(app);
    }
  if(FXString::comparecase(FXEXEImage::fileExt,type)==0){
    return new FXEXEImage(app);
    }
#ifndef CORE_IMAGE_FORMATS
#ifdef HAVE_JPEG_H
  if(FXString::comparecase(FXJPGImage::fileExt,type)==0 || FXString::comparecase("jpeg",type)==0){
    return new FXJPGImage(app);
    }
#endif
#ifdef HAVE_PNG_H
  if(FXString::comparecase(FXPNGImage::fileExt,type)==0){
    return new FXPNGImage(app);
    }
#endif
#ifdef HAVE_TIFF_H
  if(FXString::comparecase(FXTIFImage::fileExt,type)==0 || FXString::comparecase("tiff",type)==0){
    return new FXTIFImage(app);
    }
#endif
#ifdef HAVE_JP2_H
  if(FXString::comparecase(FXJP2Image::fileExt,type)==0){
    return new FXJP2Image(app);
    }
#endif
#ifdef HAVE_WEBP_H
  if(FXString::comparecase(FXWEBPImage::fileExt,type)==0){
    return new FXWEBPImage(app);
    }
#endif
#endif
  return nullptr;
  }


// Determine icon type from first header bytes in stream
FXIcon *FXIconSource::iconFromStream(FXApp* app,FXStream& store) const {
  if(fxcheckBMP(store)){
    return new FXBMPIcon(app,nullptr,0,IMAGE_ALPHAGUESS);
    }
  if(fxcheckGIF(store)){
    return new FXGIFIcon(app);
    }
  if(fxcheckIFF(store)){
    return new FXIFFIcon(app);
    }
  if(fxcheckPPM(store)){
    return new FXPPMIcon(app);
    }
  if(fxcheckRAS(store)){
    return new FXRASIcon(app);
    }
  if(fxcheckXBM(store)){
    return new FXXBMIcon(app);
    }
  if(fxcheckXPM(store)){
    return new FXXPMIcon(app);
    }
  if(fxcheckDDS(store)){
    return new FXDDSIcon(app);
    }
  if(fxcheckEXE(store)){
    return new FXEXEIcon(app);
    }
#ifndef CORE_IMAGE_FORMATS
#ifdef HAVE_JPEG_H
  if(fxcheckJPG(store)){
    return new FXJPGIcon(app);
    }
#endif
#ifdef HAVE_PNG_H
  if(fxcheckPNG(store)){
    return new FXPNGIcon(app);
    }
#endif
#ifdef HAVE_TIFF_H
  if(fxcheckTIF(store)){
    return new FXTIFIcon(app);
    }
#endif
#ifdef HAVE_JP2_H
  if(fxcheckJP2(store)){
    return new FXJP2Icon(app);
    }
#endif
#ifdef HAVE_WEBP_H
  if(fxcheckWEBP(store)){
    return new FXWEBPIcon(app);
    }
#endif
#endif
  if(fxcheckPCX(store)){
    return new FXPCXIcon(app);
    }
  if(fxcheckICO(store)){
    return new FXICOIcon(app);
    }
  if(fxcheckRGB(store)){
    return new FXRGBIcon(app);
    }
  if(fxcheckTGA(store)){
    return new FXTGAIcon(app);
    }
  return nullptr;
  }


// Determine image type from first header bytes in stream
FXImage *FXIconSource::imageFromStream(FXApp* app,FXStream& store) const {
  if(fxcheckBMP(store)){
    return new FXBMPImage(app);
    }
  if(fxcheckGIF(store)){
    return new FXGIFImage(app);
    }
  if(fxcheckIFF(store)){
    return new FXIFFImage(app);
    }
  if(fxcheckPPM(store)){
    return new FXPPMImage(app);
    }
  if(fxcheckRAS(store)){
    return new FXRASImage(app);
    }
  if(fxcheckXBM(store)){
    return new FXXBMImage(app);
    }
  if(fxcheckXPM(store)){
    return new FXXPMImage(app);
    }
  if(fxcheckDDS(store)){
    return new FXDDSImage(app);
    }
  if(fxcheckEXE(store)){
    return new FXEXEImage(app);
    }
#ifndef CORE_IMAGE_FORMATS
#ifdef HAVE_JPEG_H
  if(fxcheckJPG(store)){
    return new FXJPGImage(app);
    }
#endif
#ifdef HAVE_PNG_H
  if(fxcheckPNG(store)){
    return new FXPNGImage(app);
    }
#endif
#ifdef HAVE_TIFF_H
  if(fxcheckTIF(store)){
    return new FXTIFImage(app);
    }
#endif
#ifdef HAVE_JP2_H
  if(fxcheckJP2(store)){
    return new FXJP2Image(app);
    }
#endif
#ifdef HAVE_WEBP_H
  if(fxcheckWEBP(store)){
    return new FXWEBPImage(app);
    }
#endif
#endif
  if(fxcheckPCX(store)){
    return new FXPCXImage(app);
    }
  if(fxcheckICO(store)){
    return new FXICOImage(app);
    }
  if(fxcheckRGB(store)){
    return new FXRGBImage(app);
    }
  if(fxcheckTGA(store)){
    return new FXTGAImage(app);
    }
  return nullptr;
  }


// Load from file
FXIcon *FXIconSource::loadIconFile(FXApp* app,const FXString& filename,const FXString& type) const {
  FXIcon *icon=nullptr;
  FXTRACE((150,"FXIconSource loadIcon(%s)\n",filename.text()));
  if(!filename.empty()){
    FXFileStream store;
    if(store.open(filename,FXStreamLoad,65536)){
      if(type.empty()){
        icon=loadIconStream(app,store,FXPath::extension(filename));
        }
      else{
        icon=loadIconStream(app,store,type);
        }
      store.close();
      }
    }
  return icon;
  }


// Load from data array
FXIcon *FXIconSource::loadIconData(FXApp* app,const FXuchar *pixels,const FXString& type) const {
  FXIcon *icon=nullptr;
  if(pixels){
    FXMemoryStream store;
    store.open(FXStreamLoad,const_cast<FXuchar*>(pixels));
    icon=loadIconStream(app,store,type);
    store.close();
    }
  return icon;
  }


// Load from already open stream
FXIcon *FXIconSource::loadIconStream(FXApp* app,FXStream& store,const FXString& type) const {
  FXIcon *icon=nullptr;
  if(!type.empty()){
    icon=iconFromType(app,type);
    }
  if(!icon){
    icon=iconFromStream(app,store);
    }
  if(icon){
    if(icon->loadPixels(store)) return icon;
    delete icon;
    }
  return nullptr;
  }


// Load from file
FXImage *FXIconSource::loadImageFile(FXApp* app,const FXString& filename,const FXString& type) const {
  FXImage *image=nullptr;
  FXTRACE((150,"FXIconSource loadImage(%s)\n",filename.text()));
  if(!filename.empty()){
    FXFileStream store;
    if(store.open(filename,FXStreamLoad,65536)){
      if(type.empty()){
        image=loadImageStream(app,store,FXPath::extension(filename));
        }
      else{
        image=loadImageStream(app,store,type);
        }
      store.close();
      }
    }
  return image;
  }


// Load from data array
FXImage *FXIconSource::loadImageData(FXApp* app,const FXuchar *pixels,const FXString& type) const {
  FXImage *image=nullptr;
  if(pixels){
    FXMemoryStream store;
    store.open(FXStreamLoad,const_cast<FXuchar*>(pixels));
    image=loadImageStream(app,store,type);
    store.close();
    }
  return image;
  }


// Load from already open stream
FXImage *FXIconSource::loadImageStream(FXApp* app,FXStream& store,const FXString& type) const {
  FXImage *image=nullptr;
  if(!type.empty()){
    image=imageFromType(app,type);
    }
  if(!image){
    image=imageFromStream(app,store);
    }
  if(image){
    if(image->loadPixels(store)) return image;
    delete image;
    }
  return nullptr;
  }


// Load icon and scale it such that its dimensions does not exceed given size
FXIcon *FXIconSource::loadScaledIconFile(FXApp* app,const FXString& filename,FXint size,FXint qual,const FXString& type) const {
  return (FXIcon*)scaleToSize(loadIconFile(app,filename,type),size,qual);
  }


// Load from data array
FXIcon *FXIconSource::loadScaledIconData(FXApp* app,const FXuchar *pixels,FXint size,FXint qual,const FXString& type) const {
  return (FXIcon*)scaleToSize(loadIconData(app,pixels,type),size,qual);
  }


// Load icon and scale it such that its dimensions does not exceed given size
FXIcon *FXIconSource::loadScaledIconStream(FXApp* app,FXStream& store,FXint size,FXint qual,const FXString& type) const {
  return (FXIcon*)scaleToSize(loadIconStream(app,store,type),size,qual);
  }


// Load image and scale it such that its dimensions does not exceed given size
FXImage *FXIconSource::loadScaledImageFile(FXApp* app,const FXString& filename,FXint size,FXint qual,const FXString& type) const {
  return scaleToSize(loadImageFile(app,filename,type),size,qual);
  }


// Load from data array
FXImage *FXIconSource::loadScaledImageData(FXApp* app,const FXuchar *pixels,FXint size,FXint qual,const FXString& type) const {
  return (FXImage*)scaleToSize(loadImageData(app,pixels,type),size,qual);
  }


// Load image and scale it such that its dimensions does not exceed given size
FXImage *FXIconSource::loadScaledImageStream(FXApp* app,FXStream& store,FXint size,FXint qual,const FXString& type) const {
  return scaleToSize(loadImageStream(app,store,type),size,qual);
  }

}
