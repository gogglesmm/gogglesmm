/********************************************************************************
*                                                                               *
*                      C l i p p i n g   R e g i o n                            *
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
#include "FXStream.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXRegion.h"


/*
  Notes:
  - Add some more ways to create regions
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

// Construct new empty region
FXRegion::FXRegion(){
#ifdef WIN32
  region=(void*)CreateRectRgn(0,0,0,0);
#else
  region=XCreateRegion();
#endif
  }


// Construct rectangle region
FXRegion::FXRegion(FXint x,FXint y,FXint w,FXint h){
#ifdef WIN32
  region=(void*)CreateRectRgn(x,y,x+w,y+h);
#else
  XRectangle r;
  r.x=x; r.y=y; r.width=w; r.height=h;
  region=XCreateRegion();
  XUnionRectWithRegion(&r,(Region)region,(Region)region);
#endif
  }


// Construct new region from rectangle rect
FXRegion::FXRegion(const FXRectangle& rect){
#ifdef WIN32
  region=(void*)CreateRectRgn(rect.x,rect.y,rect.x+rect.w,rect.y+rect.h);
#else
  region=XCreateRegion();
  XUnionRectWithRegion(const_cast<XRectangle*>((const XRectangle*)&rect),(Region)region,(Region)region);
#endif
  }


// Construct polygon region
FXRegion::FXRegion(const FXPoint* points,FXuint npoints,FXbool winding){
#ifdef WIN32
  FXuint i;
  POINT pts[1024];
  for(i=0; i<npoints; i++){
    pts[i].x=points[i].x;
    pts[i].y=points[i].y;
    }
  region=(void*)CreatePolygonRgn(pts,npoints,winding?WINDING:ALTERNATE);
#else
  region=XPolygonRegion(const_cast<XPoint*>((const XPoint*)points),npoints,winding?WindingRule:EvenOddRule);
#endif
  }


// Construct new region copied from region r
FXRegion::FXRegion(const FXRegion& r){
#ifdef WIN32
  region=(void*)CreateRectRgn(0,0,0,0);
  CombineRgn((HRGN)region,(HRGN)r.region,(HRGN)region,RGN_COPY);
#else
  region=XCreateRegion();
  XUnionRegion((Region)r.region,(Region)region,(Region)region);
#endif
  }


// Assign region r to this one
FXRegion& FXRegion::operator=(const FXRegion& r){
#ifdef WIN32
  CombineRgn((HRGN)region,(HRGN)r.region,(HRGN)r.region,RGN_COPY);
#else
  if(region!=r.region){
    XDestroyRegion((Region)region);
    region=XCreateRegion();
    XUnionRegion((Region)r.region,(Region)region,(Region)region);
    }
#endif
  return *this;
  }


// Return true if region is empty
FXbool FXRegion::empty() const {
#ifdef WIN32
  return OffsetRgn((HRGN)region,0,0)==NULLREGION;
#else
  return XEmptyRegion((Region)region);
#endif
  }


// Return true if region contains point
FXbool FXRegion::contains(FXint x,FXint y) const {
#ifdef WIN32
  return region && PtInRegion((HRGN)region,x,y);
#else
  return XPointInRegion((Region)region,x,y);
#endif
  }

// Return true if region contains rectangle
// Contributed by Daniel Gehriger <gehriger@linkcad.com>.
FXbool FXRegion::contains(FXint x,FXint y,FXint w,FXint h) const {
#ifdef WIN32
  RECT rect;
  rect.left   = x;
  rect.top    = y;
  rect.right  = x + w;
  rect.bottom = y + h;
  return region && RectInRegion((HRGN)region,&rect);
#else
  return XRectInRegion((Region)region,x,y,w,h);
#endif
  }


// Return bounding box
FXRectangle FXRegion::bounds() const {
#ifdef WIN32
  RECT rect;
  GetRgnBox((HRGN)region,&rect);
  return FXRectangle((FXshort)rect.left,(FXshort)rect.top,(FXshort)(rect.right-rect.left),(FXshort)(rect.bottom-rect.top));
#else
  XRectangle rect;
  XClipBox((Region)region,&rect);
  return FXRectangle(rect.x,rect.y,rect.width,rect.height);
#endif
  }


// Offset region by dx,dy
FXRegion& FXRegion::offset(FXint dx,FXint dy){
#ifdef WIN32
  OffsetRgn((HRGN)region,dx,dy);
#else
  XOffsetRegion((Region)region,dx,dy);
#endif
  return *this;
  }


// Return true if region equal to this one
FXbool FXRegion::operator==(const FXRegion& r) const {
#ifdef WIN32
  return EqualRgn((HRGN)region,(HRGN)r.region)!=0;
#else
  return XEqualRegion((Region)region,(Region)r.region);
#endif
  }


// Return true if region not equal to this one
FXbool FXRegion::operator!=(const FXRegion& r) const {
#ifdef WIN32
  return EqualRgn((HRGN)region,(HRGN)r.region)==0;
#else
  return !XEqualRegion((Region)region,(Region)r.region);
#endif
  }


// Union region r with this one
FXRegion& FXRegion::operator+=(const FXRegion& r){
#ifdef WIN32
  CombineRgn((HRGN)region,(HRGN)region,(HRGN)r.region,RGN_OR);
#else
  Region res=XCreateRegion();
  XUnionRegion((Region)region,(Region)r.region,res);
  XDestroyRegion((Region)region);
  region=res;
#endif
  return *this;
  }


// Intersect region r with this one
FXRegion& FXRegion::operator*=(const FXRegion& r){
#ifdef WIN32
  CombineRgn((HRGN)region,(HRGN)region,(HRGN)r.region,RGN_AND);
#else
  Region res=XCreateRegion();
  XIntersectRegion((Region)region,(Region)r.region,res);
  XDestroyRegion((Region)region);
  region=res;
#endif
  return *this;
  }


// Subtract region r from this one
FXRegion& FXRegion::operator-=(const FXRegion& r){
#ifdef WIN32
  CombineRgn((HRGN)region,(HRGN)region,(HRGN)r.region,RGN_DIFF);
#else
  Region res=XCreateRegion();
  XSubtractRegion((Region)region,(Region)r.region,res);
  XDestroyRegion((Region)region);
  region=res;
#endif
  return *this;
  }


// Xor region r with this one
FXRegion& FXRegion::operator^=(const FXRegion& r){
#ifdef WIN32
  CombineRgn((HRGN)region,(HRGN)region,(HRGN)r.region,RGN_XOR);
#else
  Region res=XCreateRegion();
  XXorRegion((Region)region,(Region)r.region,res);
  XDestroyRegion((Region)region);
  region=res;
#endif
  return *this;
  }


// Union region r with this one
FXRegion FXRegion::operator+(const FXRegion& r) const {
  FXRegion res;
#ifdef WIN32
  CombineRgn((HRGN)res.region,(HRGN)region,(HRGN)r.region,RGN_OR);
#else
  XUnionRegion((Region)region,(Region)r.region,(Region)res.region);
#endif
  return res;
  }


// Intersect region r with this one
FXRegion FXRegion::operator*(const FXRegion& r) const {
  FXRegion res;
#ifdef WIN32
  CombineRgn((HRGN)res.region,(HRGN)region,(HRGN)r.region,RGN_AND);
#else
  XIntersectRegion((Region)region,(Region)r.region,(Region)res.region);
#endif
  return res;
  }


// Subtract region r from this one
FXRegion FXRegion::operator-(const FXRegion& r) const {
  FXRegion res;
#ifdef WIN32
  CombineRgn((HRGN)res.region,(HRGN)region,(HRGN)r.region,RGN_DIFF);
#else
  XSubtractRegion((Region)region,(Region)r.region,(Region)res.region);
#endif
  return res;
  }


// Xor region r with this one
FXRegion FXRegion::operator^(const FXRegion& r) const {
  FXRegion res;
#ifdef WIN32
  CombineRgn((HRGN)res.region,(HRGN)region,(HRGN)r.region,RGN_XOR);
#else
  XXorRegion((Region)region,(Region)r.region,(Region)res.region);
#endif
  return res;
  }


// Reset region to empty
void FXRegion::reset(){
#ifdef WIN32
  DeleteObject((HRGN)region);
  region=(void*)CreateRectRgn(0,0,0,0);
#else
  XDestroyRegion((Region)region);
  region=XCreateRegion();
#endif
  }


// Destroy region
FXRegion::~FXRegion(){
#ifdef WIN32
  DeleteObject((HRGN)region);
#else
  XDestroyRegion((Region)region);
#endif
  }

}

