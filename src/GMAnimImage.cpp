/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2010 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#include <fx.h>
#include "gmdefs.h"

#include "GMAnimImage.h"

FXDEFMAP(GMAnimImage) GMAnimImageMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMAnimImage::onPaint),
  FXMAPFUNC(SEL_TIMEOUT,GMAnimImage::ID_TIMER,GMAnimImage::onTimer)
  };

FXIMPLEMENT(GMAnimImage,FXImageFrame,GMAnimImageMap,ARRAYNUMBER(GMAnimImageMap))


GMAnimImage::GMAnimImage(){
  index=1;
  imgw=imgh=32;
  nrow=8;
  ncol=4;
  }

// Construct it
GMAnimImage::GMAnimImage(FXComposite* p,FXImage *img,FXint base,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXImageFrame(p,img,opts,x,y,w,h,pl,pr,pt,pb){
  index=1;
  imgw=imgh=base;
  nrow=img->getWidth()/imgw;
  ncol=img->getHeight()/imgh;
  }

GMAnimImage::~GMAnimImage(){
  getApp()->removeTimeout(this,ID_TIMER);
  }

void GMAnimImage::show() {
  FXImageFrame::show();
  getApp()->addTimeout(this,ID_TIMER,TIME_MSEC(100));
  }

void GMAnimImage::hide() {
  FXImageFrame::hide();
  getApp()->removeTimeout(this,ID_TIMER);
  }


void GMAnimImage::create() {
  FXImageFrame::create();
  getApp()->addTimeout(this,ID_TIMER,TIME_MSEC(50));
  }

// Get default width
FXint GMAnimImage::getDefaultWidth(){
  register FXint w=0;
  if(image) w=imgw;
  return w+padleft+padright+(border<<1);
  }

// Get default height
FXint GMAnimImage::getDefaultHeight(){
  register FXint h=0;
  if(image) h=imgh;
  return h+padtop+padbottom+(border<<1);
  }

long GMAnimImage::onTimer(FXObject*,FXSelector,void*){
  if (index==((nrow*ncol)-1))
    index=1;
  else
    index++;
  update();
  getApp()->addTimeout(this,ID_TIMER,TIME_MSEC(100));
  return 0;
  }


// Draw the image
long GMAnimImage::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);
  FXint imgx,imgy;
  dc.setForeground(backColor);
  if(image){
    if(options&JUSTIFY_LEFT) imgx=padleft+border;
    else if(options&JUSTIFY_RIGHT) imgx=width-padright-border-imgw;
    else imgx=border+padleft+(width-padleft-padright-(border<<1)-imgw)/2;
    if(options&JUSTIFY_TOP) imgy=padtop+border;
    else if(options&JUSTIFY_BOTTOM) imgy=height-padbottom-border-imgh;
    else imgy=border+padtop+(height-padbottom-padtop-(border<<1)-imgh)/2;
    dc.fillRectangle(border,border,imgx-border,height-(border<<1));
    dc.fillRectangle(imgx+imgw,border,width-border-imgx-imgw,height-(border<<1));
    dc.fillRectangle(imgx,border,imgw,imgy-border);
    dc.fillRectangle(imgx,imgy+imgh,imgw,height-border-imgy-imgh);
    dc.drawArea(image,(index%nrow)*imgw,(index/nrow)*imgh,imgw,imgh,imgx,imgy);
    }
  else{
    dc.fillRectangle(border,border,width-(border<<1),height-(border<<1));
    }
  drawFrame(dc,0,0,width,height);
  return 1;
  }
