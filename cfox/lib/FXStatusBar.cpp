/********************************************************************************
*                                                                               *
*                        S t a t u s   B a r   W i d g e t                      *
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
#include "FXAccelTable.h"
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXIcon.h"
#include "FXFrame.h"
#include "FXDragCorner.h"
#include "FXStatusLine.h"
#include "FXStatusBar.h"



/*
  Notes:
  - Drag corner shown whenever option says it is.
*/


using namespace FX;


/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXStatusBar,FXHorizontalFrame,nullptr,0)


// Make a status bar
FXStatusBar::FXStatusBar(FXComposite* p,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXHorizontalFrame(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
  corner=new FXDragCorner(this);
  status=new FXStatusLine(this);
  }


// Compute minimum width based on child layout hints
FXint FXStatusBar::getDefaultWidth(){
  FXint w,wcum,numc;
  FXWindow* child;
  FXuint hints;
  wcum=numc=0;
  for(child=corner->getNext(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else w=child->getDefaultWidth();
      wcum+=w;
      numc++;
      }
    }
  if(numc>1) wcum+=(numc-1)*hspacing;
  if((options&STATUSBAR_WITH_DRAGCORNER) && (numc>1)) wcum+=corner->getDefaultWidth();
  return padleft+padright+wcum+(border<<1);
  }


// Compute minimum height based on child layout hints
FXint FXStatusBar::getDefaultHeight(){
  FXint h,hmax;
  FXWindow* child;
  FXuint hints;
  hmax=0;
  for(child=corner->getNext(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
      else h=child->getDefaultHeight();
      if(hmax<h) hmax=h;
      }
    }
  h=padtop+padbottom+hmax;
  if((options&STATUSBAR_WITH_DRAGCORNER) && (h<corner->getDefaultHeight())) h=corner->getDefaultHeight();
  return h+(border<<1);
  }


// Recalculate layout
void FXStatusBar::layout(){
  FXint left,right,top,bottom;
  FXint remain,extra_space,total_space,t;
  FXint x,y,w,h;
  FXint numc=0;
  FXint sumexpand=0;
  FXint numexpand=0;
  FXint e=0;
  FXuint hints;
  FXWindow* child;

  // Placement rectangle; right/bottom non-inclusive
  left=border+padleft;
  right=width-border-padright;
  top=border+padtop;
  bottom=height-border-padbottom;
  remain=right-left;

  // Find number of paddable children and total width
  for(child=corner->getNext(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else w=child->getDefaultWidth();
      FXASSERT(w>=0);
      if((hints&LAYOUT_CENTER_X) || ((hints&LAYOUT_FILL_X) && !(hints&LAYOUT_FIX_WIDTH))){
        sumexpand+=w;
        numexpand+=1;
        }
      else{
        remain-=w;
        }
      numc++;
      }
    }

  // Child spacing
  if(numc>1) remain-=hspacing*(numc-1);

  // Subtract corner width
  if((options&STATUSBAR_WITH_DRAGCORNER) && (numc>1)){
    right-=corner->getDefaultWidth();
    remain-=corner->getDefaultWidth();
    }

  // Do the layout
  for(child=corner->getNext(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();

      // Layout child in Y
      if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
      else h=child->getDefaultHeight();
      extra_space=0;
      if((hints&LAYOUT_FILL_Y) && !(hints&LAYOUT_FIX_HEIGHT)){
        h=bottom-top;
        if(h<0) h=0;
        }
      else if(hints&LAYOUT_CENTER_Y){
        if(h<(bottom-top)) extra_space=(bottom-top-h)/2;
        }
      if(hints&LAYOUT_BOTTOM)
        y=bottom-extra_space-h;
      else /*hints&LAYOUT_TOP*/
        y=top+extra_space;

      // Layout child in X
      if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else w=child->getDefaultWidth();
      extra_space=0;
      total_space=0;
      if((hints&LAYOUT_FILL_X) && !(hints&LAYOUT_FIX_WIDTH)){
        if(sumexpand>0){
          t=w*remain;
          FXASSERT(sumexpand>0);
          w=t/sumexpand;
          e+=t%sumexpand;
          if(e>=sumexpand){w++;e-=sumexpand;}
          }
        else{
          FXASSERT(numexpand>0);
          w=remain/numexpand;
          e+=remain%numexpand;
          if(e>=numexpand){w++;e-=numexpand;}
          }
        }
      else if(hints&LAYOUT_CENTER_X){
        if(sumexpand>0){
          t=w*remain;
          FXASSERT(sumexpand>0);
          total_space=t/sumexpand-w;
          e+=t%sumexpand;
          if(e>=sumexpand){total_space++;e-=sumexpand;}
          }
        else{
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
      else{/*hints&LAYOUT_LEFT*/
        x=left+extra_space;
        left=left+w+hspacing+total_space;
        }
      child->position(x,y,w,h);
      }
    }

  // Just make sure corner grip's on top
  if((options&STATUSBAR_WITH_DRAGCORNER)){
    if(numc>1)
      corner->position(width-border-corner->getDefaultWidth(),height-border-corner->getDefaultHeight(),corner->getDefaultWidth(),corner->getDefaultHeight());
    else
      corner->position(width-padright-border-corner->getDefaultWidth(),height-border-padbottom-corner->getDefaultHeight(),corner->getDefaultWidth(),corner->getDefaultHeight());
    corner->show();
    corner->raise();
    }
  else{
    corner->hide();
    }
  flags&=~FLAG_DIRTY;
  }


// Show or hide the drag corner
void FXStatusBar::setCornerStyle(FXbool withcorner){
  FXuint opts=(((0-withcorner)^options)&STATUSBAR_WITH_DRAGCORNER)^options;
  if(options!=opts){
    options=opts;
    recalc();
    update();
    }
  }


// Return true if drag corner shown
FXbool FXStatusBar::getCornerStyle() const {
  return (options&STATUSBAR_WITH_DRAGCORNER)!=0;
  }


// Save object to stream
void FXStatusBar::save(FXStream& store) const {
  FXHorizontalFrame::save(store);
  store << corner;
  store << status;
  }


// Load object from stream
void FXStatusBar::load(FXStream& store){
  FXHorizontalFrame::load(store);
  store >> corner;
  store >> status;
  }


// Destruct
FXStatusBar::~FXStatusBar(){
  corner=(FXDragCorner*)-1L;
  status=(FXStatusLine*)-1L;
  }

}

