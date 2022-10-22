/********************************************************************************
*                                                                               *
*                          R e c t a n g l e    C l a s s                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1994,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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

using namespace FX;

/*******************************************************************************/

namespace FX {

// Fast inlines
static inline FXshort _max(FXshort a,FXshort b){ return a>b?a:b; }
static inline FXshort _min(FXshort a,FXshort b){ return a<b?a:b; }


// Grow by amount
FXRectangle& FXRectangle::grow(FXshort margin){
  x-=margin;
  y-=margin;
  w+=(margin+margin);
  h+=(margin+margin);
  return *this;
  }


// Grow by different amounts horizontally and vertically
FXRectangle& FXRectangle::grow(FXshort hormargin,FXshort vermargin){
  x-=hormargin;
  y-=vermargin;
  w+=(hormargin+hormargin);
  h+=(vermargin+vermargin);
  return *this;
  }


// Grow by different amounts on all sides
FXRectangle& FXRectangle::grow(FXshort leftmargin,FXshort rightmargin,FXshort topmargin,FXshort bottommargin){
  x-=leftmargin;
  y-=topmargin;
  w+=(leftmargin+rightmargin);
  h+=(topmargin+bottommargin);
  return *this;
  }


// Shrink by amount
FXRectangle& FXRectangle::shrink(FXshort margin){
  x+=margin;
  y+=margin;
  w-=(margin+margin);
  h-=(margin+margin);
  return *this;
  }


// Shrink by different amounts horizontally and vertically
FXRectangle& FXRectangle::shrink(FXshort hormargin,FXshort vermargin){
  x+=hormargin;
  y+=vermargin;
  w-=(hormargin+hormargin);
  h-=(vermargin+vermargin);
  return *this;
  }


// Shrink by different amounts on all sides
FXRectangle& FXRectangle::shrink(FXshort leftmargin,FXshort rightmargin,FXshort topmargin,FXshort bottommargin){
  x+=leftmargin;
  y+=topmargin;
  w-=(leftmargin+rightmargin);
  h-=(topmargin+bottommargin);
  return *this;
  }


// Union with rectangle
FXRectangle& FXRectangle::operator+=(const FXRectangle &r){
  w=_max(x+w,r.x+r.w); x=_min(x,r.x); w-=x;
  h=_max(y+h,r.y+r.h); y=_min(y,r.y); h-=y;
  return *this;
  }


// Intersection with rectangle
FXRectangle& FXRectangle::operator*=(const FXRectangle &r){
  w=_min(x+w,r.x+r.w); x=_max(x,r.x); w-=x;
  h=_min(y+h,r.y+r.h); y=_max(y,r.y); h-=y;
  return *this;
  }


// Union between rectangles
FXRectangle FXRectangle::operator+(const FXRectangle& r) const {
  FXshort xx=_min(x,r.x);
  FXshort ww=_max(x+w,r.x+r.w)-xx;
  FXshort yy=_min(y,r.y);
  FXshort hh=_max(y+h,r.y+r.h)-yy;
  return FXRectangle(xx,yy,ww,hh);
  }


// Intersection between rectangles
FXRectangle FXRectangle::operator*(const FXRectangle& r) const {
  FXshort xx=_max(x,r.x);
  FXshort ww=_min(x+w,r.x+r.w)-xx;
  FXshort yy=_max(y,r.y);
  FXshort hh=_min(y+h,r.y+r.h)-yy;
  return FXRectangle(xx,yy,ww,hh);
  }


// Pieces of this rectangle after taking a bite out of it
void FXRectangle::bite(FXRectangle pieces[],const FXRectangle& b) const {
  pieces[0].x=pieces[1].x=x;
  pieces[0].y=pieces[3].y=y;
  pieces[2].w=pieces[3].w=x+w;
  pieces[1].h=pieces[2].h=y+h;
  pieces[1].w=pieces[2].x=b.x;
  pieces[0].h=pieces[1].y=b.y;
  pieces[0].w=pieces[3].x=b.x+b.w;
  pieces[2].y=pieces[3].h=b.y+b.h;
  if(pieces[1].w<pieces[1].x) pieces[1].w=pieces[2].x=pieces[1].x;
  if(pieces[0].h<pieces[0].y) pieces[0].h=pieces[1].y=pieces[0].y;
  if(pieces[3].x>pieces[3].w) pieces[3].x=pieces[0].w=pieces[3].w;
  if(pieces[2].y>pieces[2].h) pieces[2].y=pieces[3].h=pieces[2].h;
  if(pieces[1].w>pieces[3].x) pieces[1].w=pieces[2].x=pieces[3].x;
  if(pieces[0].h>pieces[2].y) pieces[0].h=pieces[1].y=pieces[2].y;
  if(pieces[3].x<pieces[1].w) pieces[3].x=pieces[0].w=pieces[1].w;
  if(pieces[2].y<pieces[0].h) pieces[2].y=pieces[3].h=pieces[0].h;
  pieces[0].w-=pieces[0].x;
  pieces[0].h-=pieces[0].y;
  pieces[1].w-=pieces[1].x;
  pieces[1].h-=pieces[1].y;
  pieces[2].w-=pieces[2].x;
  pieces[2].h-=pieces[2].y;
  pieces[3].w-=pieces[3].x;
  pieces[3].h-=pieces[3].y;
  }


// Save object to a stream
FXStream& operator<<(FXStream& store,const FXRectangle& r){
  store << r.x << r.y << r.w << r.h;
  return store;
  }


// Load object from a stream
FXStream& operator>>(FXStream& store,FXRectangle& r){
  store >> r.x >> r.y >> r.w >> r.h;
  return store;
  }

}
