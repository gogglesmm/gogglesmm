/********************************************************************************
*                                                                               *
*              H o r i z o n t a l   C o n t a i n e r   O b j e c t            *
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
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXHorizontalFrame.h"


/*
  Notes:
  - Filled items shrink as well as stretch.
  - Stretch is proportional to default size; this way, at default size,
    it is exactly correct.
  - Tabbing order takes widget layout into account
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXHorizontalFrame) FXHorizontalFrameMap[]={
  FXMAPFUNC(SEL_FOCUS_PREV,0,FXHorizontalFrame::onFocusLeft),
  FXMAPFUNC(SEL_FOCUS_NEXT,0,FXHorizontalFrame::onFocusRight),
  };


// Object implementation
FXIMPLEMENT(FXHorizontalFrame,FXPacker,FXHorizontalFrameMap,ARRAYNUMBER(FXHorizontalFrameMap))


// Make a horizontal one
FXHorizontalFrame::FXHorizontalFrame(FXComposite* p,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
  }


// Compute minimum width based on child layout hints
FXint FXHorizontalFrame::getDefaultWidth(){
  FXint w,wcum,wmax,mw;
  FXWindow* child;
  FXuint hints;
  wcum=wmax=mw=0;
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else if(options&PACK_UNIFORM_WIDTH) w=mw;
      else w=child->getDefaultWidth();
      if((hints&LAYOUT_RIGHT)&&(hints&LAYOUT_CENTER_X)){
        w=child->getX()+w;
        if(w>wmax) wmax=w;
        }
      else{
        if(wcum) wcum+=hspacing;
        wcum+=w;
        }
      }
    }
  wcum+=padleft+padright+(border<<1);
  return FXMAX(wcum,wmax);
  }


// Compute minimum height based on child layout hints
FXint FXHorizontalFrame::getDefaultHeight(){
  FXint h,hmax,hcum,mh;
  FXWindow* child;
  FXuint hints;
  hmax=hcum=mh=0;
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
      else if(options&PACK_UNIFORM_HEIGHT) h=mh;
      else h=child->getDefaultHeight();
      if((hints&LAYOUT_BOTTOM)&&(hints&LAYOUT_CENTER_Y)){
        h=child->getY()+h;
        if(h>hmax) hmax=h;
        }
      else{
        if(h>hcum) hcum=h;
        }
      }
    }
  hcum+=padtop+padbottom+(border<<1);
  return FXMAX(hcum,hmax);
  }


// Recalculate layout
void FXHorizontalFrame::layout(){
  FXint left,right,top,bottom,remain,extra_space,total_space,t,x,y,w,h;
  FXWindow* child;
  FXint sumexpand=0;
  FXint numexpand=0;
  FXint mw=0;
  FXint mh=0;
  FXint e=0;
  FXuint hints;

  // Placement rectangle; right/bottom non-inclusive
  left=border+padleft;
  right=width-border-padright;
  top=border+padtop;
  bottom=height-border-padbottom;
  remain=right-left;

  // Get maximum child size
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();

  // Find number of paddable children and total width
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(!((hints&LAYOUT_RIGHT)&&(hints&LAYOUT_CENTER_X))){
        if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else w=child->getDefaultWidth();
        FXASSERT(w>=0);
        if((hints&LAYOUT_CENTER_X) || ((hints&LAYOUT_FILL_X) && !(hints&LAYOUT_FIX_WIDTH))){
          sumexpand+=w;
          numexpand+=1;
          }
        else{
          remain-=w;
          }
        remain-=hspacing;
        }
      }
    }

  // Child spacing correction
  remain+=hspacing;

  // Do the layout
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();

      // Determine child height
      if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
      else if(options&PACK_UNIFORM_HEIGHT) h=mh;
      else if(hints&LAYOUT_FILL_Y) h=bottom-top;
      else h=child->getDefaultHeight();

      // Determine child y-position
      if((hints&LAYOUT_BOTTOM)&&(hints&LAYOUT_CENTER_Y)) y=child->getY();
      else if(hints&LAYOUT_CENTER_Y) y=top+(bottom-top-h)/2;
      else if(hints&LAYOUT_BOTTOM) y=bottom-h;
      else y=top;

      // Layout child in X
      x=child->getX();
      if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else if(options&PACK_UNIFORM_WIDTH) w=mw;
      else w=child->getDefaultWidth();
      if(!((hints&LAYOUT_RIGHT)&&(hints&LAYOUT_CENTER_X))){
        extra_space=0;
        total_space=0;
        if((hints&LAYOUT_FILL_X) && !(hints&LAYOUT_FIX_WIDTH)){
          if(sumexpand>0){                            // Divide space proportionally to width
            t=w*remain;
            FXASSERT(sumexpand>0);
            w=t/sumexpand;
            e+=t%sumexpand;
            if(e>=sumexpand){w++;e-=sumexpand;}
            }
          else{                                       // Divide the space equally
            FXASSERT(numexpand>0);
            w=remain/numexpand;
            e+=remain%numexpand;
            if(e>=numexpand){w++;e-=numexpand;}
            }
          }
        else if(hints&LAYOUT_CENTER_X){
          if(sumexpand>0){                            // Divide space proportionally to width
            t=w*remain;
            FXASSERT(sumexpand>0);
            total_space=t/sumexpand-w;
            e+=t%sumexpand;
            if(e>=sumexpand){total_space++;e-=sumexpand;}
            }
          else{                                       // Divide the space equally
            FXASSERT(numexpand>0);
            total_space=remain/numexpand-w;
            e+=remain%numexpand;
            if(e>=numexpand){total_space++;e-=numexpand;}
            }
          extra_space=total_space/2;
          }
        if(hints&LAYOUT_RIGHT){
          x=right-w-extra_space;
          right=right-w-hspacing-total_space;
          }
        else{
          x=left+extra_space;
          left=left+w+hspacing+total_space;
          }
        }
      child->position(x,y,w,h);
      }
    }
  flags&=~FLAG_DIRTY;
  }

}

