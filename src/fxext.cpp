/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2010 by Sander Jansen. All Rights Reserved      *
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
#include "gmdefs.h"
#include "fxext.h"

// Map
FXDEFMAP(GMThreadDialog) GMThreadDialogMap[]={
  FXMAPFUNC(SEL_KEYPRESS,0,GMThreadDialog::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,GMThreadDialog::onKeyRelease),
  FXMAPFUNC(SEL_CLOSE,0,GMThreadDialog::onCmdCancel),
  FXMAPFUNC(SEL_COMMAND,FXDialogBox::ID_ACCEPT,GMThreadDialog::onCmdAccept),
  FXMAPFUNC(SEL_CHORE,FXDialogBox::ID_CANCEL,GMThreadDialog::onCmdCancel),
  FXMAPFUNC(SEL_TIMEOUT,FXDialogBox::ID_CANCEL,GMThreadDialog::onCmdCancel),
  FXMAPFUNC(SEL_COMMAND,FXDialogBox::ID_CANCEL,GMThreadDialog::onCmdCancel),
  FXMAPFUNC(SEL_COMMAND,GMThreadDialog::ID_THREAD_EXEC,GMThreadDialog::onThreadExec),
  };


// Object implementation
FXIMPLEMENT(GMThreadDialog,FXTopWindow,GMThreadDialogMap,ARRAYNUMBER(GMThreadDialogMap))

GMThreadDialog::GMThreadDialog(FXApp* a,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXTopWindow(a,name,NULL,NULL,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
  code=0;
  }

GMThreadDialog::GMThreadDialog(FXWindow* own,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXTopWindow(own,name,NULL,NULL,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
  code=0;
  }

long GMThreadDialog::onThreadExec(FXObject*,FXSelector,void*){
  create();
  show();
  return 1;
  }

FXuint GMThreadDialog::execute(FXMessageChannel* channel) {
  mutex.lock();
  channel->message(this,FXSEL(SEL_COMMAND,ID_THREAD_EXEC),NULL,0);
  condition.wait(mutex);
  mutex.unlock();
  return code;
  }

// Close dialog with an accept
long GMThreadDialog::onCmdAccept(FXObject*,FXSelector,void*){
  mutex.lock();
  hide();
  destroy();
  code=1;
  condition.signal();
  mutex.unlock();
  return 1;
  }

// Close dialog with a cancel
long GMThreadDialog::onCmdCancel(FXObject*,FXSelector,void*){
  mutex.lock();
  hide();
  destroy();
  code=0;
  condition.signal();
  mutex.unlock();
  return 1;
  }

// Keyboard press; handle escape to close the dialog
long GMThreadDialog::onKeyPress(FXObject* sender,FXSelector sel,void* ptr){
  if(FXTopWindow::onKeyPress(sender,sel,ptr)) return 1;
  if(((FXEvent*)ptr)->code==KEY_Escape){
    handle(this,FXSEL(SEL_COMMAND,ID_CANCEL),NULL);
    return 1;
    }
  return 0;
  }

// Keyboard release; handle escape to close the dialog
long GMThreadDialog::onKeyRelease(FXObject* sender,FXSelector sel,void* ptr){
  if(FXTopWindow::onKeyRelease(sender,sel,ptr)) return 1;
  if(((FXEvent*)ptr)->code==KEY_Escape){
    return 1;
    }
  return 0;
  }











// Get highlight color
static FXColor gm_make_hilite_color(FXColor clr){
  FXuint r,g,b;
  r=FXREDVAL(clr);
  g=FXGREENVAL(clr);
  b=FXBLUEVAL(clr);
  r=FXMAX(31,r);
  g=FXMAX(31,g);
  b=FXMAX(31,b);
  r=(133*r)/100;
  g=(133*g)/100;
  b=(133*b)/100;
  r=FXMIN(255,r);
  g=FXMIN(255,g);
  b=FXMIN(255,b);
  return FXRGB(r,g,b);
  }

// Get shadow color
static FXColor gm_make_shadow_color(FXColor clr){
  FXuint r,g,b;
  r=FXREDVAL(clr);
  g=FXGREENVAL(clr);
  b=FXBLUEVAL(clr);
  r=(66*r)/100;
  g=(66*g)/100;
  b=(66*b)/100;
  return FXRGB(r,g,b);
  }





// Fill vertical gradient rectangle
void fillVerticalGradient(FXDCWindow & dc,FXint x,FXint y,FXint w,FXint h,FXColor top,FXColor bottom){
  register FXint rr,gg,bb,dr,dg,db,r1,g1,b1,r2,g2,b2,yl,yh,yy,dy,n,t;
  if(0<w && 0<h){
    r1=FXREDVAL(top);
    r2=FXREDVAL(bottom);
    g1=FXGREENVAL(top);
    g2=FXGREENVAL(bottom);
    b1=FXBLUEVAL(top);
    b2=FXBLUEVAL(bottom);
    dr=r2-r1;
    dg=g2-g1;
    db=b2-b1;
    n=FXABS(dr);
    if((t=FXABS(dg))>n) n=t;
    if((t=FXABS(db))>n) n=t;
    n++;
    if(n>h) n=h;
    if(n>128) n=128;
    rr=(r1<<16)+32767;
    gg=(g1<<16)+32767;
    bb=(b1<<16)+32767;
    yy=32767;
    dr=(dr<<16)/n;
    dg=(dg<<16)/n;
    db=(db<<16)/n;
    dy=(h<<16)/n;
    do{
      yl=yy>>16;
      yy+=dy;
      yh=yy>>16;
      dc.setForeground(FXRGB(rr>>16,gg>>16,bb>>16));
      dc.fillRectangle(x,y+yl,w,yh-yl);
      rr+=dr;
      gg+=dg;
      bb+=db;
      }
    while(yh<h);
    }
  }

// Fill horizontal gradient rectangle
void fillHorizontalGradient(FXDCWindow & dc,FXint x,FXint y,FXint w,FXint h,FXColor left,FXColor right){
  register FXint rr,gg,bb,dr,dg,db,r1,g1,b1,r2,g2,b2,xl,xh,xx,dx,n,t;
  if(0<w && 0<h){
    r1=FXREDVAL(left);
    r2=FXREDVAL(right);
    g1=FXGREENVAL(left);
    g2=FXGREENVAL(right);
    b1=FXBLUEVAL(left);
    b2=FXBLUEVAL(right);
    dr=r2-r1;
    dg=g2-g1;
    db=b2-b1;
    n=FXABS(dr);
    if((t=FXABS(dg))>n) n=t;
    if((t=FXABS(db))>n) n=t;
    n++;
    if(n>w) n=w;
    if(n>128) n=128;
    rr=(r1<<16)+32767;
    gg=(g1<<16)+32767;
    bb=(b1<<16)+32767;
    xx=32767;
    dr=(dr<<16)/n;
    dg=(dg<<16)/n;
    db=(db<<16)/n;
    dx=(w<<16)/n;
    do{
      xl=xx>>16;
      xx+=dx;
      xh=xx>>16;
      dc.setForeground(FXRGB(rr>>16,gg>>16,bb>>16));
      dc.fillRectangle(x+xl,y,xh-xl,h);
      rr+=dr;
      gg+=dg;
      bb+=db;
      }
    while(xh<w);
    }
  }

FXIMPLEMENT(GMScrollFrame,FXVerticalFrame,NULL,0)

GMScrollFrame::GMScrollFrame(){
  }

GMScrollFrame::GMScrollFrame(FXComposite*p):FXVerticalFrame(p,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_LINE,0,0,0,0,0,0,0,0,0,0){
  borderColor=getApp()->getShadowColor();
  }


FXIMPLEMENT(GMScrollHFrame,FXHorizontalFrame,NULL,0)

GMScrollHFrame::GMScrollHFrame(){
  }

GMScrollHFrame::GMScrollHFrame(FXComposite*p):FXHorizontalFrame(p,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_LINE,0,0,0,0,0,0,0,0,0,0){
  borderColor=getApp()->getShadowColor();
  }


FXIMPLEMENT(GMTabFrame,FXVerticalFrame,NULL,0)

GMTabFrame::GMTabFrame(){
  }

GMTabFrame::GMTabFrame(FXComposite*p):FXVerticalFrame(p,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_LINE,0,0,0,0){
  borderColor=getApp()->getShadowColor();
  }


FXIMPLEMENT(GMHeaderItem,FXHeaderItem,NULL,0)


// Map
FXDEFMAP(GMHeader) GMHeaderMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMHeader::onPaint),
  };


// Object implementation
FXIMPLEMENT(GMHeader,FXHeader,GMHeaderMap,ARRAYNUMBER(GMHeaderMap))


// Make a Header
GMHeader::GMHeader(){
  }

// Make a Header
GMHeader::GMHeader(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):
  FXHeader(p,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb){
  }

// Handle repaint
long GMHeader::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);
  register FXint x,y,w,h,i,ilo,ihi;

  // Set font
  dc.setFont(font);

  // Paint background

  fillVerticalGradient(dc,ev->rect.x,0,ev->rect.w,height-1,gm_make_hilite_color(backColor),gm_make_hilite_color(gm_make_shadow_color(backColor)));
  dc.setForeground(shadowColor);
  dc.fillRectangle(ev->rect.x,height-1,ev->rect.w,1);

 // dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);

  // Vertical
  if(options&HEADER_VERTICAL){

    // Determine affected items
    ilo=getItemAt(ev->rect.y);
    ihi=getItemAt(ev->rect.y+ev->rect.h);

    // Fragment below first item
    if(ilo<0){
      y=pos;
      if(0<items.no()){
        y=pos+items[0]->getPos();
        }
      if(0<y){
        if(options&FRAME_THICK)
          drawDoubleRaisedRectangle(dc,0,0,width,y);
        else if(options&FRAME_RAISED)
          drawRaisedRectangle(dc,0,0,width,y);
        }
      ilo=0;
      }

    // Fragment above last item
    if(ihi>=items.no()){
      y=pos;
      if(0<items.no()){
        y=pos+items[items.no()-1]->getPos()+items[items.no()-1]->getSize();
        }
      if(y<height){
        if(options&FRAME_THICK)
          drawDoubleRaisedRectangle(dc,0,y,width,height-y);
        else if(options&FRAME_RAISED)
          drawRaisedRectangle(dc,0,y,width,height-y);
        }
      ihi=items.no()-1;
      }

    // Draw only affected items
    for(i=ilo; i<=ihi; i++){
      y=pos+items[i]->getPos();
      h=items[i]->getSize();
      if(items[i]->isPressed()){
        if(options&FRAME_THICK)
          drawDoubleSunkenRectangle(dc,0,y,width,h);
        else if(options&FRAME_RAISED)
          drawSunkenRectangle(dc,0,y,width,h);
        }
      else{
        if(options&FRAME_THICK)
          drawDoubleRaisedRectangle(dc,0,y,width,h);
        else if(options&FRAME_RAISED)
          drawRaisedRectangle(dc,0,y,width,h);
        }
      ((GMHeaderItem*)items[i])->draw(this,dc,0,y,width,h);
      }
    }

  // Horizontal
  else{

    // Determine affected items
    ilo=getItemAt(ev->rect.x);
    ihi=getItemAt(ev->rect.x+ev->rect.w);

    // Fragment below first item
    if(ilo<0){
      x=pos;
      if(0<items.no()){
        x=pos+items[0]->getPos();
        }
/*
      if(0<x){
        if(options&FRAME_THICK)
          drawDoubleRaisedRectangle(dc,0,0,x,height);
        else if(options&FRAME_RAISED)
          drawRaisedRectangle(dc,0,0,x,height);
        }
*/
      ilo=0;
      }

    // Fragment above last item
    if(ihi>=items.no()){
      x=pos;
      if(0<items.no()){
        x=pos+items[items.no()-1]->getPos()+items[items.no()-1]->getSize();
        }
      if(x<width){

        dc.setForeground(hiliteColor);
        dc.fillRectangle(x,0,1,height);
        }

 /*

        if(options&FRAME_THICK)
          drawDoubleRaisedRectangle(dc,x,0,width-x,height);
        else if(options&FRAME_RAISED)
          drawRaisedRectangle(dc,x,0,width-x,height);
        }
*/
      ihi=items.no()-1;
      }

    // Draw only the affected items
    for(i=ilo; i<=ihi; i++){
      x=pos+items[i]->getPos();
      w=items[i]->getSize();
      if(items[i]->isPressed()){

        if (x>0) {
        dc.setForeground(hiliteColor);
        dc.fillRectangle(x,0,1,height);
         }

        dc.setForeground(shadowColor);
        dc.fillRectangle(x+w-1,0,1,height);


      /*
        if(options&FRAME_THICK)
          drawDoubleSunkenRectangle(dc,x,0,w,height);
        else if(options&FRAME_RAISED)
          drawSunkenRectangle(dc,x,0,w,height);

          */
        }
      else{
        if (x>0) {

        dc.setForeground(hiliteColor);
        dc.fillRectangle(x,0,1,height);
          }

        dc.setForeground(shadowColor);
        dc.fillRectangle(x+w-1,0,1,height);
      /*
        if(options&FRAME_THICK)
          drawDoubleRaisedRectangle(dc,x,0,w,height);
        else if(options&FRAME_RAISED)
          drawRaisedRectangle(dc,x,0,w,height);

         */
        }
      ((GMHeaderItem*)items[i])->draw(this,dc,x,0,w,height);
      }
    }
  return 1;
  }



FXIMPLEMENT(GMMenuCommand,FXMenuCommand,NULL,0)

GMMenuCommand::GMMenuCommand(FXComposite* p,const FXString& text,FXIcon* ic,FXObject* tgt,FXSelector sel,FXuint opts) : FXMenuCommand(p,text,ic,tgt,sel,opts){
  backColor=getApp()->getBackColor();
  }


FXIMPLEMENT(GMMenuCheck,FXMenuCheck,NULL,0)

GMMenuCheck::GMMenuCheck(FXComposite* p,const FXString& text,FXObject* tgt,FXSelector sel,FXuint opts) : FXMenuCheck(p,text,tgt,sel,opts){
  backColor=getApp()->getBackColor();
  }


FXIMPLEMENT(GMMenuRadio,FXMenuRadio,NULL,0)

GMMenuRadio::GMMenuRadio(FXComposite* p,const FXString& text,FXObject* tgt,FXSelector sel,FXuint opts) : FXMenuRadio(p,text,tgt,sel,opts){
  backColor=getApp()->getBackColor();
  }


FXIMPLEMENT(GMMenuCascade,FXMenuCascade,NULL,0)

GMMenuCascade::GMMenuCascade(FXComposite* p,const FXString& text,FXIcon* ic,FXPopup* pup,FXuint opts) : FXMenuCascade(p,text,ic,pup,opts){
  backColor=getApp()->getBackColor();
  }

FXIMPLEMENT(GMMenuPane,FXMenuPane,NULL,0)

GMMenuPane::GMMenuPane(FXWindow* owner,FXuint opts) : FXMenuPane(owner,opts) {
  borderColor=getApp()->getShadowColor();
  setFrameStyle(FRAME_LINE);
  }


FXDEFMAP(GMTextField) GMTextFieldMap[]={
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,GMTextField::onLeftBtnPress)
  };

FXIMPLEMENT(GMTextField,FXTextField,GMTextFieldMap,ARRAYNUMBER(GMTextFieldMap))

GMTextField::GMTextField(FXComposite* p,FXint ncols,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb) : FXTextField(p,ncols,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb){
  borderColor=getApp()->getShadowColor();
  setFrameStyle(FRAME_LINE);
  }

FXbool GMTextField::extendWordSelection(FXint pos,FXbool /*notify*/) {
  register FXint sp,ep;
  pos=contents.validate(FXCLAMP(0,pos,contents.length()));
  if(pos<=anchor){
    sp=wordStart(pos);
    ep=wordEnd(anchor);
    }
  else{
    sp=wordStart(anchor);
    ep=wordEnd(pos);
    }
  return setSelection(sp,ep-sp);
  }







// Pressed left button
long GMTextField::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* ev=(FXEvent*)ptr;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    flags&=~FLAG_UPDATE;
    if(ev->click_count==1){
      setCursorPos(index(ev->win_x));
      if(ev->state&SHIFTMASK){
        extendSelection(cursor);
        }
      else{
        killSelection();
        setAnchorPos(cursor);
        }
      makePositionVisible(cursor);
      flags|=FLAG_PRESSED;
      }
    else if (ev->click_count==2) {
      setAnchorPos(cursor);
      extendWordSelection(cursor,true);
      }
    else{
      setAnchorPos(0);
      setCursorPos(contents.length());
      extendSelection(contents.length());
      makePositionVisible(cursor);
      }
    return 1;
    }
  return 0;
  }


FXIMPLEMENT(GMSpinner,FXSpinner,NULL,0)

GMSpinner::GMSpinner(FXComposite* p,FXint ncols,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb) : FXSpinner(p,ncols,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb){
  borderColor=getApp()->getShadowColor();
  setFrameStyle(FRAME_LINE);
  upButton->setFrameStyle(FRAME_NONE);
  downButton->setFrameStyle(FRAME_NONE);

  upButton->setPadLeft(1);
  upButton->setPadRight(1);
  upButton->setPadTop(2);
  upButton->setPadBottom(2);
  downButton->setPadLeft(1);
  downButton->setPadRight(1);
  downButton->setPadTop(2);
  downButton->setPadBottom(2);
  }


FXDEFMAP(GMMenuTitle) GMMenuTitleMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMMenuTitle::onPaint),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_POST,GMMenuTitle::onCmdPost),
  };

FXIMPLEMENT(GMMenuTitle,FXMenuTitle,GMMenuTitleMap,ARRAYNUMBER(GMMenuTitleMap))

GMMenuTitle::GMMenuTitle(FXComposite* p,const FXString& text,FXIcon* ic,FXPopup* pup,FXuint opts) : FXMenuTitle(p,text,ic,pup,opts){
  selbackColor=getApp()->getSelbackColor();
  seltextColor=getApp()->getSelforeColor();
  }


// Handle repaint
long GMMenuTitle::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);
  FXint xx,yy;
  dc.setFont(font);
  xx=6;
  yy=0;
  if(isEnabled()){
    if(isActive()){
      dc.setForeground(gm_make_shadow_color(selbackColor));

//      dc.fillRectangle(0,height-1,width,1);
      dc.fillRectangle(width-1,0,1,height);
      dc.fillRectangle(0,0,width,1);
      dc.fillRectangle(0,0,1,height);

      dc.setForeground(selbackColor);
      dc.fillRectangle(1,1,width-2,height-1);
      xx++;
      yy++;
      }
    else if(underCursor()){
      dc.setForeground(backColor);
      dc.fillRectangle(0,0,width,height);
     }
    else{
      dc.setForeground(backColor);
      dc.fillRectangle(0,0,width,height);
      }
    if(icon){
      dc.drawIcon(icon,xx,yy+(height-icon->getHeight())/2);
      xx+=5+icon->getWidth();
      }
    if(!label.empty()){
      yy+=font->getFontAscent()+(height-font->getFontHeight())/2;
      dc.setForeground(isActive() ? seltextColor : textColor);
      dc.drawText(xx,yy,label);
      if(0<=hotoff){
        dc.fillRectangle(xx+font->getTextWidth(&label[0],hotoff),yy+1,font->getTextWidth(&label[hotoff],wclen(&label[hotoff])),1);
        }
      }
    }
  else{
    dc.setForeground(backColor);
    dc.fillRectangle(0,0,width,height);
    if(icon){
      dc.drawIconSunken(icon,xx,yy+(height-icon->getHeight())/2);
      xx+=5+icon->getWidth();
      }
    if(!label.empty()){
      yy+=font->getFontAscent()+(height-font->getFontHeight())/2;
      dc.setForeground(hiliteColor);
      dc.drawText(xx+1,yy+1,label);
      if(0<=hotoff){
        dc.fillRectangle(xx+font->getTextWidth(&label[0],hotoff),yy+1,font->getTextWidth(&label[hotoff],wclen(&label[hotoff])),1);
        }
      dc.setForeground(shadowColor);
      dc.drawText(xx,yy,label);
      if(0<=hotoff){
        dc.fillRectangle(xx+font->getTextWidth(&label[0],hotoff),yy+1,font->getTextWidth(&label[hotoff],wclen(&label[hotoff])),1);
        }
      }
    }
  return 1;
  }

// Post the menu
long GMMenuTitle::onCmdPost(FXObject*,FXSelector,void*){
  FXint x,y,side;
  if(pane && !pane->shown()){
    translateCoordinatesTo(x,y,getRoot(),0,0);
    side=getParent()->getLayoutHints();
    if(side&LAYOUT_SIDE_LEFT){  // Vertical
      //y-=1;
      if(side&LAYOUT_SIDE_BOTTOM){      // On right
        x-=pane->getDefaultWidth();
        }
      else{                             // On left
        x+=width;
        }
      }
    else{                       // Horizontal
      //x-=1;
      if(side&LAYOUT_SIDE_BOTTOM){      // On bottom
        y-=pane->getDefaultHeight();
        }
      else{                             // On top
        y+=height;
        }
      }
    pane->popup(getParent(),x,y);
    if(!getParent()->grabbed()) getParent()->grab();
    }
  flags&=~FLAG_UPDATE;
  flags|=FLAG_ACTIVE;
  update();
  return 1;
  }


FXDEFMAP(GMButton) GMButtonMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMButton::onPaint)
  };

FXIMPLEMENT(GMButton,FXButton,GMButtonMap,ARRAYNUMBER(GMButtonMap))


GMButton::GMButton(){
  }

GMButton::GMButton(FXComposite* p,const FXString& text,FXIcon* ic,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb) : FXButton(p,text,ic,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb) {
  }

// Handle repaint
long GMButton::onPaint(FXObject*,FXSelector,void* ptr){
 FXint tw=0,th=0,iw=0,ih=0,tx,ty,ix,iy;
  FXEvent*ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);

//  shadowColor         = gm_make_shadow_color(baseColor);
  FXColor top         = gm_make_hilite_color(backColor);
  FXColor bottom      = gm_make_hilite_color(shadowColor);
  FXColor shade       = gm_make_hilite_color(shadowColor);
  FXColor bordercolor = shadowColor;

  FXPoint basebackground[4]={FXPoint(0,0),FXPoint(width-1,0),FXPoint(0,height-1),FXPoint(width-1,height-1)};

  FXPoint bordershade[16]={FXPoint(0,1),FXPoint(1,0),FXPoint(1,2),FXPoint(2,1),
                          FXPoint(width-2,0),FXPoint(width-1,1),FXPoint(width-3,1),FXPoint(width-2,2),
                          FXPoint(0,height-2),FXPoint(1,height-1),FXPoint(1,height-3),FXPoint(2,height-2),
                          FXPoint(width-1,height-2),FXPoint(width-2,height-1),FXPoint(width-2,height-3),FXPoint(width-3,height-2)
                          };
  FXPoint bordercorners[4]={FXPoint(1,1),FXPoint(1,height-2),FXPoint(width-2,1),FXPoint(width-2,height-2)};

  if (options&BUTTON_TOOLBAR && (!underCursor() || !isEnabled())) {
    dc.setForeground(baseColor);
    dc.fillRectangle(0,0,width,height);
    }
  else if (state==STATE_UP && ((options&BUTTON_TOOLBAR)==0 || (options&BUTTON_TOOLBAR && underCursor()))) {

    /// Outside Background
    dc.setForeground(baseColor);
    dc.drawPoints(basebackground,4);

    /// Border
    dc.setForeground(bordercolor);
    dc.drawRectangle(2,0,width-5,0);
    dc.drawRectangle(2,height-1,width-5,height-1);
    dc.drawRectangle(0,2,0,height-5);
    dc.drawRectangle(width-1,2,0,height-5);
    dc.drawPoints(bordercorners,4);
    dc.setForeground(shade);
    dc.drawPoints(bordershade,16);

    fillVerticalGradient(dc,2,1,width-4,height-2,top,bottom);
    dc.setForeground(top);
    dc.drawRectangle(1,3,0,height-7);
    dc.setForeground(bottom);
    dc.drawRectangle(width-2,3,0,height-7);
    }
  else {
    /// Outside Background
    dc.setForeground(baseColor);
    dc.drawPoints(basebackground,4);

    /// Border
    dc.setForeground(bordercolor);
    dc.drawRectangle(2,0,width-5,0);
    dc.drawRectangle(2,height-1,width-5,height-1);
    dc.drawRectangle(0,2,0,height-5);
    dc.drawRectangle(width-1,2,0,height-5);
    dc.drawPoints(bordercorners,4);
    dc.setForeground(shade);
    dc.drawPoints(bordershade,16);

    dc.setForeground(baseColor);
    dc.fillRectangle(2,1,width-4,height-2);


    //dc.setForeground(FXRGB(0xdc,0xd4,0xc9));
    //dc.fillRectangle(2,1,width-4,height-2);
    }

  // Place text & icon
  if(!label.empty()){
    tw=labelWidth(label);
    th=labelHeight(label);
    }
  if(icon){
    iw=icon->getWidth();
    ih=icon->getHeight();
    }

  just_x(tx,ix,tw,iw);
  just_y(ty,iy,th,ih);

  // Shift a bit when pressed
  if(state && (options&(FRAME_RAISED|FRAME_SUNKEN))){ ++tx; ++ty; ++ix; ++iy; }

  // Draw enabled state
  if(isEnabled()){
    if(icon){
      dc.drawIcon(icon,ix,iy);
      }
    if(!label.empty()){
      dc.setFont(font);
      dc.setForeground(textColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      }
    if(hasFocus()){
      dc.drawFocusRectangle(border+1,border+1,width-2*border-2,height-2*border-2);
      }
    }

  // Draw grayed-out state
  else{
    if(icon){
      dc.drawIconSunken(icon,ix,iy);
      }
    if(!label.empty()){
      dc.setFont(font);
      dc.setForeground(hiliteColor);
      drawLabel(dc,label,hotoff,tx+1,ty+1,tw,th);
      dc.setForeground(shadowColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      }
    }
  return 1;
  }















FXDEFMAP(GMToggleButton) GMToggleButtonMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMToggleButton::onPaint)
  };

FXIMPLEMENT(GMToggleButton,FXToggleButton,GMToggleButtonMap,ARRAYNUMBER(GMToggleButtonMap))


GMToggleButton::GMToggleButton(){
  }

GMToggleButton::GMToggleButton(FXComposite* p,const FXString& text1,const FXString& text2,FXIcon* ic1,FXIcon* ic2,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb) : FXToggleButton(p,text1,text2,ic1,ic2,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb) {
  }

// Handle repaint
long GMToggleButton::onPaint(FXObject*,FXSelector,void* ptr){
  FXint tw=0,th=0,iw=0,ih=0,tx,ty,ix,iy;
  FXEvent *ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);


  FXColor top         = gm_make_hilite_color(backColor);
  FXColor bottom      = gm_make_hilite_color(shadowColor);
  FXColor shade       = gm_make_hilite_color(shadowColor);
  FXColor bordercolor = shadowColor;

  FXPoint basebackground[4]={FXPoint(0,0),FXPoint(width-1,0),FXPoint(0,height-1),FXPoint(width-1,height-1)};

  FXPoint bordershade[16]={FXPoint(0,1),FXPoint(1,0),FXPoint(1,2),FXPoint(2,1),
                          FXPoint(width-2,0),FXPoint(width-1,1),FXPoint(width-3,1),FXPoint(width-2,2),
                          FXPoint(0,height-2),FXPoint(1,height-1),FXPoint(1,height-3),FXPoint(2,height-2),
                          FXPoint(width-1,height-2),FXPoint(width-2,height-1),FXPoint(width-2,height-3),FXPoint(width-3,height-2)
                          };
  FXPoint bordercorners[4]={FXPoint(1,1),FXPoint(1,height-2),FXPoint(width-2,1),FXPoint(width-2,height-2)};


  // Got a border at all?
  if(options&(FRAME_RAISED|FRAME_SUNKEN)){

    // Toolbar style
    if(options&TOGGLEBUTTON_TOOLBAR){

      // Enabled and cursor inside and down
      if(down || ((options&TOGGLEBUTTON_KEEPSTATE) && state)){


        /// Outside Background
        dc.setForeground(baseColor);
        dc.drawPoints(basebackground,4);

        /// Border
        dc.setForeground(bordercolor);
        dc.drawRectangle(2,0,width-5,0);
        dc.drawRectangle(2,height-1,width-5,height-1);
        dc.drawRectangle(0,2,0,height-5);
        dc.drawRectangle(width-1,2,0,height-5);
        dc.drawPoints(bordercorners,4);
        dc.setForeground(shade);
        dc.drawPoints(bordershade,16);

        dc.setForeground(baseColor);
        dc.fillRectangle(2,1,width-4,height-2);
        }

      // Enabled and cursor inside, and up
      else if(isEnabled() && underCursor()){
        /// Outside Background
        dc.setForeground(baseColor);
        dc.drawPoints(basebackground,4);

        /// Border
        dc.setForeground(bordercolor);
        dc.drawRectangle(2,0,width-5,0);
        dc.drawRectangle(2,height-1,width-5,height-1);
        dc.drawRectangle(0,2,0,height-5);
        dc.drawRectangle(width-1,2,0,height-5);
        dc.drawPoints(bordercorners,4);
        dc.setForeground(shade);
        dc.drawPoints(bordershade,16);

        fillVerticalGradient(dc,2,1,width-4,height-2,top,bottom);
        dc.setForeground(top);
        dc.drawRectangle(1,3,0,height-7);
        dc.setForeground(bottom);
        dc.drawRectangle(width-2,3,0,height-7);
        }

      // Disabled or unchecked or not under cursor
      else{
        dc.setForeground(backColor);
        dc.fillRectangle(0,0,width,height);
        }
      }

    // Normal style
    else{

      // Draw sunken if pressed
      if(down || ((options&TOGGLEBUTTON_KEEPSTATE) && state)){
        dc.setForeground(hiliteColor);
        dc.fillRectangle(border,border,width-border*2,height-border*2);
        if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,0,0,width,height);
        else drawSunkenRectangle(dc,0,0,width,height);
        }

      // Draw raised if not currently pressed down
      else{
        dc.setForeground(backColor);
        dc.fillRectangle(border,border,width-border*2,height-border*2);
        if(options&FRAME_THICK) drawDoubleRaisedRectangle(dc,0,0,width,height);
        else drawRaisedRectangle(dc,0,0,width,height);
        }

      }
    }

  // No borders
  else{
    dc.setForeground(backColor);
    dc.fillRectangle(0,0,width,height);
    }

  // Place text & icon
  if(state && !altlabel.empty()){
    tw=labelWidth(altlabel);
    th=labelHeight(altlabel);
    }
  else if(!label.empty()){
    tw=labelWidth(label);
    th=labelHeight(label);
    }
  if(state && alticon){
    iw=alticon->getWidth();
    ih=alticon->getHeight();
    }
  else if(icon){
    iw=icon->getWidth();
    ih=icon->getHeight();
    }

  just_x(tx,ix,tw,iw);
  just_y(ty,iy,th,ih);

  // Shift a bit when pressed
  if((down || ((options&TOGGLEBUTTON_KEEPSTATE) && state)) && (options&(FRAME_RAISED|FRAME_SUNKEN))){ ++tx; ++ty; ++ix; ++iy; }

  // Draw enabled state
  if(isEnabled()){
    if(state && alticon){
      dc.drawIcon(alticon,ix,iy);
      }
    else if(icon){
      dc.drawIcon(icon,ix,iy);
      }
    if(state && !altlabel.empty()){
      dc.setFont(font);
      dc.setForeground(textColor);
      drawLabel(dc,altlabel,althotoff,tx,ty,tw,th);
      }
    else if(!label.empty()){
      dc.setFont(font);
      dc.setForeground(textColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      }
    if(hasFocus()){
      dc.drawFocusRectangle(border+1,border+1,width-2*border-2,height-2*border-2);
      }
    }

  // Draw grayed-out state
  else{
    if(state && alticon){
      dc.drawIconSunken(alticon,ix,iy);
      }
    else if(icon){
      dc.drawIconSunken(icon,ix,iy);
      }
    if(state && !altlabel.empty()){
      dc.setFont(font);
      dc.setForeground(hiliteColor);
      drawLabel(dc,altlabel,althotoff,tx+1,ty+1,tw,th);
      dc.setForeground(shadowColor);
      drawLabel(dc,altlabel,althotoff,tx,ty,tw,th);
      }
    else if(!label.empty()){
      dc.setFont(font);
      dc.setForeground(hiliteColor);
      drawLabel(dc,label,hotoff,tx+1,ty+1,tw,th);
      dc.setForeground(shadowColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      }
    }
  return 1;
  }



FXIMPLEMENT(GMRadioButton,FXRadioButton,NULL,0)


GMRadioButton::GMRadioButton(){
  }

GMRadioButton::GMRadioButton(FXComposite* p,const FXString& text,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb) : FXRadioButton(p,text,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb) {
 // borderColor=getApp()->getShadowColor();

  borderColor=getApp()->getBackColor();
  hiliteColor=getApp()->getShadowColor();
  baseColor=getApp()->getBackColor();


  }

FXDEFMAP(GMCheckButton) GMCheckButtonMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMCheckButton::onPaint)
  };

FXIMPLEMENT(GMCheckButton,FXCheckButton,GMCheckButtonMap,ARRAYNUMBER(GMCheckButtonMap))


GMCheckButton::GMCheckButton(){
  }

GMCheckButton::GMCheckButton(FXComposite* p,const FXString& text,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb) : FXCheckButton(p,text,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb) {
  }

long GMCheckButton::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXint tw=0,th=0,tx,ty,ix,iy;
  FXDCWindow dc(this,ev);

  // Figure text size
  if(!label.empty()){
    tw=labelWidth(label);
    th=labelHeight(label);
    }

  // Placement
  just_x(tx,ix,tw,13);
  just_y(ty,iy,th,13);

  // Widget background
  dc.setForeground(backColor);
  dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);

  // Check background
  if(check==MAYBE || !isEnabled())
    dc.setForeground(baseColor);
  else
    dc.setForeground(boxColor);
  dc.fillRectangle(ix+2,iy+2,9,9);

  // Check border
  if(options&CHECKBUTTON_PLUS){
    dc.setForeground(textColor);
    dc.drawRectangle(ix+2,iy+2,8,8);
    }
  else{
    dc.setForeground(shadowColor);
    dc.drawRectangle(ix+1,iy+1,10,10);
    }

  // Check color
  if(check==MAYBE || !isEnabled())
    dc.setForeground(shadowColor);
  else
    dc.setForeground(checkColor);

  // Show as +
  if(options&CHECKBUTTON_PLUS){
    if(check!=TRUE){
      dc.fillRectangle(ix+6,iy+4,1,5);
      }
    dc.fillRectangle(ix+4,iy+6,5,1);
    }

  // Show as v
  else{
    if(check!=FALSE){
      FXSegment seg[6];
#ifndef WIN32
      seg[0].x1=3+ix; seg[0].y1=5+iy; seg[0].x2=5+ix; seg[0].y2=7+iy;
      seg[1].x1=3+ix; seg[1].y1=6+iy; seg[1].x2=5+ix; seg[1].y2=8+iy;
      seg[2].x1=3+ix; seg[2].y1=7+iy; seg[2].x2=5+ix; seg[2].y2=9+iy;
      seg[3].x1=5+ix; seg[3].y1=7+iy; seg[3].x2=9+ix; seg[3].y2=3+iy;
      seg[4].x1=5+ix; seg[4].y1=8+iy; seg[4].x2=9+ix; seg[4].y2=4+iy;
      seg[5].x1=5+ix; seg[5].y1=9+iy; seg[5].x2=9+ix; seg[5].y2=5+iy;
#else
      seg[0].x1=3+ix; seg[0].y1=5+iy; seg[0].x2=5+ix; seg[0].y2=7+iy;
      seg[1].x1=3+ix; seg[1].y1=6+iy; seg[1].x2=5+ix; seg[1].y2=8+iy;
      seg[2].x1=3+ix; seg[2].y1=7+iy; seg[2].x2=5+ix; seg[2].y2=9+iy;
      seg[3].x1=5+ix; seg[3].y1=7+iy; seg[3].x2=10+ix; seg[3].y2=2+iy;
      seg[4].x1=5+ix; seg[4].y1=8+iy; seg[4].x2=10+ix; seg[4].y2=3+iy;
      seg[5].x1=5+ix; seg[5].y1=9+iy; seg[5].x2=10+ix; seg[5].y2=4+iy;
#endif
      dc.drawLineSegments(seg,6);
      }
    }

  // Text
  if(!label.empty()){
    dc.setFont(font);
    if(isEnabled()){
      dc.setForeground(textColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      if(hasFocus()){
        dc.drawFocusRectangle(tx-1,ty-1,tw+2,th+2);
        }
      }
    else{
      dc.setForeground(hiliteColor);
      drawLabel(dc,label,hotoff,tx+1,ty+1,tw,th);
      dc.setForeground(shadowColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      }
    }

  // Frame
  drawFrame(dc,0,0,width,height);

  return 1;
  }














#define MENUBUTTONARROW_WIDTH   11
#define MENUBUTTONARROW_HEIGHT  5

#define MENUBUTTON_MASK         (MENUBUTTON_AUTOGRAY|MENUBUTTON_AUTOHIDE|MENUBUTTON_TOOLBAR|MENUBUTTON_NOARROWS)
#define POPUP_MASK              (MENUBUTTON_UP|MENUBUTTON_LEFT)
#define ATTACH_MASK             (MENUBUTTON_ATTACH_RIGHT|MENUBUTTON_ATTACH_CENTER)



FXDEFMAP(GMMenuButton) GMMenuButtonMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMMenuButton::onPaint)
  };

FXIMPLEMENT(GMMenuButton,FXMenuButton,GMMenuButtonMap,ARRAYNUMBER(GMMenuButtonMap))

GMMenuButton::GMMenuButton(){
  }

GMMenuButton::GMMenuButton(FXComposite* p,const FXString& text,FXIcon* ic,FXPopup* pup,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb) : FXMenuButton(p,text,ic,pup,opts,x,y,w,h,pl,pr,pt,pb) {
  }


// Handle repaint
long GMMenuButton::onPaint(FXObject*,FXSelector,void* ptr){
  FXint tw=0,th=0,iw=0,ih=0,tx,ty,ix,iy;
  FXEvent *ev=(FXEvent*)ptr;
  FXPoint points[3];
  FXDCWindow dc(this,ev);

  // Got a border at all?
  if(options&(FRAME_RAISED|FRAME_SUNKEN)){

    // Toolbar style
    if(options&MENUBUTTON_TOOLBAR){

      // Enabled and cursor inside, and not popped up
      if(isEnabled() && underCursor() && !state){
        dc.setForeground(backColor);
        dc.fillRectangle(border,border,width-border*2,height-border*2);
        if(options&FRAME_THICK) drawDoubleRaisedRectangle(dc,0,0,width,height);
        else drawRaisedRectangle(dc,0,0,width,height);
        }

      // Enabled and popped up
      else if(isEnabled() && state){
        dc.setForeground(hiliteColor);
        dc.fillRectangle(border,border,width-border*2,height-border*2);
        if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,0,0,width,height);
        else drawSunkenRectangle(dc,0,0,width,height);
        }

      // Disabled or unchecked or not under cursor
      else{
        dc.setForeground(backColor);
        dc.fillRectangle(0,0,width,height);
        }
      }

    // Normal style
    else{

      // Draw in up state if disabled or up
      if(!isEnabled() || !state){
        dc.setForeground(backColor);
        dc.fillRectangle(border,border,width-border*2,height-border*2);
        if(options&FRAME_THICK) drawDoubleRaisedRectangle(dc,0,0,width,height);
        else drawRaisedRectangle(dc,0,0,width,height);
        }

      // Draw sunken if enabled and either checked or pressed
      else{
        dc.setForeground(hiliteColor);
        dc.fillRectangle(border,border,width-border*2,height-border*2);
        if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,0,0,width,height);
        else drawSunkenRectangle(dc,0,0,width,height);
        }
      }
    }

  // No borders
  else{
    if(isEnabled() && state){
      dc.setForeground(hiliteColor);
      dc.fillRectangle(0,0,width,height);
      }
    else{
      dc.setForeground(backColor);
      dc.fillRectangle(0,0,width,height);
      }
    }

  // Position text & icon
//  if(!label.empty()){
//    tw=labelWidth(label);
//    th=labelHeight(label);
//    }

  // Icon?
  if(icon){
    tw=icon->getWidth();
    th=icon->getHeight();
    }

  // Arrows?
  if(!(options&MENUBUTTON_NOARROWS)){
    iw=MENUBUTTONARROW_WIDTH;
    ih=MENUBUTTONARROW_HEIGHT;
    }

  // Keep some room for the arrow!
  just_x(tx,ix,tw,iw);
  just_y(ty,iy,th,ih);

  // Move a bit when pressed
  if(state){ ++tx; ++ty; ++ix; ++iy; }

  // Draw icon
  if(icon){
    if(isEnabled())
      dc.drawIcon(icon,tx,ty);
    else
      dc.drawIconSunken(icon,tx,ty);
    }

  // Draw arrows
  if(!(options&MENUBUTTON_NOARROWS)){

    // Right arrow
    if((options&MENUBUTTON_RIGHT)==MENUBUTTON_RIGHT){
      if(isEnabled())
        dc.setForeground(textColor);
      else
        dc.setForeground(shadowColor);
      points[0].x=ix;
      points[0].y=iy;
      points[1].x=ix;
      points[1].y=iy+MENUBUTTONARROW_WIDTH-1;
      points[2].x=ix+MENUBUTTONARROW_HEIGHT;
      points[2].y=(FXshort)(iy+(MENUBUTTONARROW_WIDTH>>1));
      dc.fillPolygon(points,3);
      }

    // Left arrow
    else if(options&MENUBUTTON_LEFT){
      if(isEnabled())
        dc.setForeground(textColor);
      else
        dc.setForeground(shadowColor);
      points[0].x=ix+MENUBUTTONARROW_HEIGHT;
      points[0].y=iy;
      points[1].x=ix+MENUBUTTONARROW_HEIGHT;
      points[1].y=iy+MENUBUTTONARROW_WIDTH-1;
      points[2].x=ix;
      points[2].y=(FXshort)(iy+(MENUBUTTONARROW_WIDTH>>1));
      dc.fillPolygon(points,3);
      }

    // Up arrow
    else if(options&MENUBUTTON_UP){
      if(isEnabled())
        dc.setForeground(textColor);
      else
        dc.setForeground(shadowColor);
      points[0].x=(FXshort)(ix+(MENUBUTTONARROW_WIDTH>>1));
      points[0].y=iy-1;
      points[1].x=ix;
      points[1].y=iy+MENUBUTTONARROW_HEIGHT;
      points[2].x=ix+MENUBUTTONARROW_WIDTH;
      points[2].y=iy+MENUBUTTONARROW_HEIGHT;
      dc.fillPolygon(points,3);
      }

    // Down arrow
    else{
      if(isEnabled())
        dc.setForeground(textColor);
      else
        dc.setForeground(shadowColor);
      points[0].x=ix+1;
      points[0].y=iy;
      points[2].x=ix+MENUBUTTONARROW_WIDTH-1;
      points[2].y=iy;
      points[1].x=(FXshort)(ix+(MENUBUTTONARROW_WIDTH>>1));
      points[1].y=iy+MENUBUTTONARROW_HEIGHT;
      dc.fillPolygon(points,3);
      }
    }

  // Draw text
  if(!label.empty()){
    dc.setFont(font);
    if(isEnabled()){
      dc.setForeground(textColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      }
    else{
      dc.setForeground(hiliteColor);
      drawLabel(dc,label,hotoff,tx+1,ty+1,tw,th);
      dc.setForeground(shadowColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      }
    }

  // Draw focus
  if(hasFocus()){
    if(isEnabled()){
      dc.drawFocusRectangle(border+1,border+1,width-2*border-2,height-2*border-2);
      }
    }
  return 1;
  }



// Get default width
FXint GMMenuButton::getDefaultWidth(){
  FXint tw=0,iw=0,s=4,w,pw;
//  if(!label.empty()){ tw=labelWidth(label); s=4; }
  if(!(options&MENUBUTTON_NOARROWS)){
    if(options&MENUBUTTON_LEFT) iw=MENUBUTTONARROW_HEIGHT; else iw=MENUBUTTONARROW_WIDTH;
    }
  if(icon) tw=icon->getWidth();
 if(!(options&(ICON_AFTER_TEXT|ICON_BEFORE_TEXT))) w=FXMAX(tw,iw); else w=tw+iw+s;
  w=padleft+padright+(border<<1)+w;
  if(!(options&MENUBUTTON_LEFT) && (options&MENUBUTTON_ATTACH_RIGHT) && (options&MENUBUTTON_ATTACH_CENTER)){
    if(pane){ pw=pane->getDefaultWidth(); if(pw>w) w=pw; }
    }
  return w;
  }


// Get default height
FXint GMMenuButton::getDefaultHeight(){
  FXint th=0,ih=0,h,ph;
//  if(!label.empty()){ th=labelHeight(label); }
  if(!(options&MENUBUTTON_NOARROWS)){
    if(options&MENUBUTTON_LEFT) ih=MENUBUTTONARROW_WIDTH; else ih=MENUBUTTONARROW_HEIGHT;
    }
  if(icon) th=icon->getHeight();
  if(!(options&(ICON_ABOVE_TEXT|ICON_BELOW_TEXT))) h=FXMAX(th,ih); else h=th+ih;
  h=padtop+padbottom+(border<<1)+h;
  if((options&MENUBUTTON_LEFT) && (options&MENUBUTTON_ATTACH_BOTTOM) && (options&MENUBUTTON_ATTACH_CENTER)){
    if(pane){ ph=pane->getDefaultHeight(); if(ph>h) h=ph; }
    }
  return h;
  }

























FXDEFMAP(GMHeaderButton) GMHeaderButtonMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMHeaderButton::onPaint)
  };

FXIMPLEMENT(GMHeaderButton,FXButton,GMHeaderButtonMap,ARRAYNUMBER(GMHeaderButtonMap))


GMHeaderButton::GMHeaderButton(){
  arrowstate=ARROW_UP;
  }

GMHeaderButton::GMHeaderButton(FXComposite* p,const FXString& text,FXIcon* ic,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb) : FXButton(p,text,ic,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb) {
  arrowstate=ARROW_UP;
  }

void GMHeaderButton::setArrowState(FXuint s) {
  arrowstate=s;
  }

FXuint GMHeaderButton::getArrowState() const {
  return arrowstate;
  }



long GMHeaderButton::onPaint(FXObject*,FXSelector,void*ptr){
  FXint tw=0,th=0,iw=0,ih=0,tx,ty,ix,iy;
  FXEvent *ev=(FXEvent*)ptr;

  // Start drawing
  FXDCWindow dc(this,ev);

  // Got a border at all?
  if(options&(FRAME_RAISED|FRAME_SUNKEN)){

    // Toolbar style
    if(options&BUTTON_TOOLBAR){

      // Enabled and cursor inside, and up
      if(isEnabled() && underCursor() && (state==STATE_UP)){
        dc.setForeground(backColor);
        dc.fillRectangle(border,border,width-border*2,height-border*2);
        if(options&FRAME_THICK) drawDoubleRaisedRectangle(dc,0,0,width,height);
        else drawRaisedRectangle(dc,0,0,width,height);
        }

      // Enabled and cursor inside and down
      else if(isEnabled() && underCursor() && (state==STATE_DOWN)){
        dc.setForeground(backColor);
        dc.fillRectangle(border,border,width-border*2,height-border*2);
        if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,0,0,width,height);
        else drawSunkenRectangle(dc,0,0,width,height);
        }

      // Enabled and checked
      else if(isEnabled() && (state==STATE_ENGAGED)){
        dc.setForeground(hiliteColor);
        dc.fillRectangle(border,border,width-border*2,height-border*2);
        if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,0,0,width,height);
        else drawSunkenRectangle(dc,0,0,width,height);
        }

      // Disabled or unchecked or not under cursor
      else{
        dc.setForeground(backColor);
        dc.fillRectangle(0,0,width,height);
        }
      }

    // Normal style
    else{

      // Default
      if(isDefault()){

        // Draw in up state if disabled or up
        if(!isEnabled() || (state==STATE_UP)){
          dc.setForeground(backColor);
          dc.fillRectangle(border+1,border+1,width-border*2-1,height-border*2-1);
          dc.setForeground(FXRGB(255,255,255));
          dc.fillRectangle(width-2,1,1,height-1);

         // if(options&FRAME_THICK) drawDoubleRaisedRectangle(dc,1,1,width-1,height-1);
         // else drawRaisedRectangle(dc,1,1,width-3,height-1);
          }

        // Draw sunken if enabled and either checked or pressed
        else{
          if(state==STATE_ENGAGED) dc.setForeground(hiliteColor); else dc.setForeground(backColor);
          dc.fillRectangle(border,border,width-border*2-1,height-border*2-1);
          if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,0,0,width-1,height-1);
          else drawSunkenRectangle(dc,0,0,width-1,height-1);
          }

        // Black default border
        drawBorderRectangle(dc,0,0,width,height);
        }

      // Non-Default
      else{

        // Draw in up state if disabled or up
        if(!isEnabled() || (state==STATE_UP)){
          dc.setForeground(backColor);
          //dc.fillRectangle(border,border,width-border*2,height-border*2);

          //fillVerticalGradient(dc,border,border,width-border*2,height-border*2,makeHiliteColor(makeHiliteColor(makeShadowColor(backColor))),makeHiliteColor(makeShadowColor(backColor)));
//          fillVerticalGradient(dc,0,0,width,height-1,makeHiliteColor(makeHiliteColor(makeShadowColor(backColor))),makeHiliteColor(makeShadowColor(backColor)));

          fillVerticalGradient(dc,ev->rect.x,0,ev->rect.w,height-1,gm_make_hilite_color(backColor),gm_make_hilite_color(gm_make_shadow_color(backColor)));


          dc.setForeground(shadowColor);
          dc.fillRectangle(0,height-1,width,1);





//          if(options&FRAME_THICK) drawDoubleRaisedRectangle(dc,0,0,width,height);
///          else drawRaisedRectangle(dc,0,0,width-1,height);

//      dc.setForeground(backColor);

//          dc.fillRectangle(width-2,0,2,height-1);

          }

        // Draw sunken if enabled and either checked or pressed
        else{
          if(state==STATE_ENGAGED) dc.setForeground(hiliteColor); else dc.setForeground(backColor);
          dc.fillRectangle(border,border,width-border*2,height-border*2);
          if(options&FRAME_THICK) drawDoubleSunkenRectangle(dc,0,0,width,height);
          else drawSunkenRectangle(dc,0,0,width,height);
          }
        }
      }
    }

  // No borders
  else{
    if(isEnabled() && (state==STATE_ENGAGED)){
      dc.setForeground(hiliteColor);
      dc.fillRectangle(0,0,width,height);
      }
    else{
      dc.setForeground(backColor);
      dc.fillRectangle(0,0,width,height);
      }
    }

  // Place text & icon
  if(!label.empty()){
    tw=labelWidth(label);
    th=labelHeight(label);
    }
  if(icon){
    iw=icon->getWidth();
    ih=icon->getHeight();
    }

  just_x(tx,ix,tw,iw);
  just_y(ty,iy,th,ih);

  // Shift a bit when pressed
  if(state && (options&(FRAME_RAISED|FRAME_SUNKEN))){ ++tx; ++ty; ++ix; ++iy; }

  // Draw enabled state
  if(isEnabled()){
    if(icon){
      dc.drawIcon(icon,ix,iy);
      }
    if(!label.empty()){
      dc.setFont(font);
      dc.setForeground(textColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      }
//    if(hasFocus()){
//      dc.drawFocusRectangle(border+1,border+1,width-2*border-2,height-2*border-2);
//      }
    }

  // Draw grayed-out state
  else{
    if(icon){
      dc.drawIconSunken(icon,ix,iy);
      }
    if(!label.empty()){
      dc.setFont(font);
      dc.setForeground(hiliteColor);
      drawLabel(dc,label,hotoff,tx+1,ty+1,tw,th);
      dc.setForeground(shadowColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      }
    }

  // Draw arrows
  if(arrowstate&(ARROW_UP|ARROW_DOWN)){
    FXint aa=(font->getFontHeight()-5)|1;
    FXint ay=(height-aa)/2;
    FXint ax=width-aa-border-border-2;
    if(arrowstate&ARROW_UP){
      dc.setForeground(hiliteColor);
      dc.drawLine(ax+aa/2,ay,ax+aa-1,ay+aa);
      dc.drawLine(ax,ay+aa,ax+aa,ay+aa);
      dc.setForeground(shadowColor);
      dc.drawLine(ax+aa/2,ay,ax,ay+aa);
      }
    else{
      dc.setForeground(hiliteColor);
      dc.drawLine(ax+aa/2,ay+aa,ax+aa-1,ay);
      dc.setForeground(shadowColor);
      dc.drawLine(ax+aa/2,ay+aa,ax,ay);
      dc.drawLine(ax,ay,ax+aa,ay);
      }
    }
  return 1;
  }


FXIMPLEMENT(GMScrollArea,FXScrollArea,NULL,0)

void GMScrollArea::replaceScrollbars(FXScrollArea *fs) {
  GMScrollArea * s = (GMScrollArea*)(fs);
  delete s->vertical;
  delete s->horizontal;
  delete s->corner;
  s->vertical=new GMScrollBar(fs,fs,GMScrollArea::ID_VSCROLLED,SCROLLBAR_VERTICAL);
  s->horizontal=new GMScrollBar(fs,fs,GMScrollArea::ID_HSCROLLED,SCROLLBAR_HORIZONTAL);
  s->corner=new GMScrollCorner(fs);
  }



FXIMPLEMENT(GMTreeListBox,FXTreeListBox,NULL,0)

void GMTreeListBox::replace(FXTreeListBox *fs) {
  GMTreeListBox * s = (GMTreeListBox*)(fs);
  GMScrollArea::replaceScrollbars(s->tree);

  s->borderColor=s->getApp()->getShadowColor();
  s->setFrameStyle(FRAME_LINE);
  s->pane->setBorderColor(s->borderColor);

  s->button->setFrameStyle(FRAME_NONE);
  s->button->setPadLeft(2);
  s->button->setPadRight(2);
  s->button->setYOffset(0);
  s->button->setXOffset(1);
  }



// Map
FXDEFMAP(GMScrollBar) GMScrollBarMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMScrollBar::onPaint),
  };


// Object implementation
FXIMPLEMENT(GMScrollBar,FXScrollBar,GMScrollBarMap,ARRAYNUMBER(GMScrollBarMap))


// For deserialization
GMScrollBar::GMScrollBar(){
  }

// Make a scrollbar
GMScrollBar::GMScrollBar(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h): FXScrollBar(p,tgt,sel,opts,x,y,w,h) {
//shadowColor=gm_make_shadow_color(getApp()->getBaseColor());
  }

void GMScrollBar::drawThumb(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h){
  if(options&SCROLLBAR_HORIZONTAL){
    fillVerticalGradient(dc,x+2,y,w-4,h-1,gm_make_hilite_color(FXApp::instance()->getSelbackColor()),gm_make_hilite_color(gm_make_shadow_color(FXApp::instance()->getSelbackColor())));
    dc.setForeground(shadowColor);
    dc.fillRectangle(x+w-1,y,1,h);
    dc.fillRectangle(x,y,1,h);
    dc.setForeground(gm_make_hilite_color(getApp()->getSelbackColor()));
    dc.fillRectangle(x+1,y+h-1,w-2,1);
    dc.fillRectangle(x+w-2,y,1,h-1);
    dc.fillRectangle(x+1,y,1,h-1);
    if (w>20) {
      dc.setForeground(gm_make_shadow_color(getApp()->getSelbackColor()));
      dc.fillRectangle(thumbpos+thumbsize / 2,(height/2) - 2,1,6);
      dc.fillRectangle((thumbpos+thumbsize / 2) - 3,(height/2) - 2,1,6);
      dc.fillRectangle((thumbpos+thumbsize / 2) + 3,(height/2) - 2,1,6);
      }
    }
  else {
    fillHorizontalGradient(dc,x,y+2,w-1,h-4,gm_make_hilite_color(getApp()->getSelbackColor()),gm_make_hilite_color(gm_make_shadow_color(getApp()->getSelbackColor())));
    dc.setForeground(shadowColor);
    dc.fillRectangle(x,y+h-1,w,1);
    dc.fillRectangle(x,y,w,1);

    dc.setForeground(gm_make_hilite_color(getApp()->getSelbackColor()));
    dc.fillRectangle(x+w-1,y+1,1,h-2);
    dc.fillRectangle(x,y+1,w-1,1);
    dc.fillRectangle(x,y+h-2,w-1,1);

    if (h>20) {
      dc.setForeground(gm_make_shadow_color(getApp()->getSelbackColor()));
      dc.fillRectangle((width/2) - 2,thumbpos+thumbsize / 2,6,1);
      dc.fillRectangle((width/2) - 2,(thumbpos+thumbsize / 2) - 3,6,1);
      dc.fillRectangle((width/2) - 2,(thumbpos+thumbsize / 2) + 3,6,1);
      }
    }
  }

// Handle repaint
long GMScrollBar::onPaint(FXObject*,FXSelector,void* ptr){
  register FXEvent *ev=(FXEvent*)ptr;
  register int total;
  FXDCWindow dc(this,ev);
  if(options&SCROLLBAR_HORIZONTAL){
    total=width-height-height;
    dc.setForeground(shadowColor);
    dc.fillRectangle(0,0,width,1);
    if(thumbsize<total){
      drawThumb(dc,thumbpos,1,thumbsize,height-1);
      if (thumbpos-height>0) {
        dc.setForeground(shadowColor);
        dc.fillRectangle(height,1,1,height-1);
        dc.setForeground(gm_make_hilite_color(shadowColor));
        dc.fillRectangle(height+1,1,thumbpos-height-1,height-1);
        }

      if (width-height-thumbpos-thumbsize>0) {
        dc.setForeground(shadowColor);
        dc.fillRectangle(width-height-1,1,1,height-1);
        dc.setForeground(gm_make_hilite_color(shadowColor));
        dc.fillRectangle(thumbpos+thumbsize,1,width-height-thumbpos-thumbsize-1,height);
        }
      }
    else{                                                   // Non-scrollable
      dc.fillRectangle(height,1,1,height-1);
      dc.fillRectangle(width-height-1,1,1,height-1);
      dc.setForeground(gm_make_hilite_color(shadowColor));
      dc.fillRectangle(height+1,1,total-2,height-1);
      }
    dc.setFillStyle(FILL_SOLID);
    dc.setForeground(backColor);
    dc.fillRectangle(width-height,1,height,height-1);
    drawRightArrow(dc,width-height,0,height,height,(mode==MODE_INC));
    dc.setForeground(backColor);
    dc.fillRectangle(0,1,height,height-1);
    drawLeftArrow(dc,0,0,height,height,(mode==MODE_DEC));
    }
  else{
    total=height-width-width;
    dc.setForeground(shadowColor);
    dc.fillRectangle(0,0,1,height);
    if(thumbsize<total){
      drawThumb(dc,1,thumbpos,width-1,thumbsize);
      if (thumbpos-width>0) {
        dc.setForeground(shadowColor);
        dc.fillRectangle(1,width,width-1,1);
        dc.setForeground(gm_make_hilite_color(shadowColor));
        dc.fillRectangle(1,width+1,width-1,thumbpos-width-1);
        }
      if (height-width-thumbpos-thumbsize>0){
        dc.setForeground(shadowColor);
        dc.fillRectangle(1,height-width-1,width-1,1);
        dc.setForeground(gm_make_hilite_color(shadowColor));
        dc.fillRectangle(1,thumbpos+thumbsize,width-1,height-width-thumbpos-thumbsize-1);
        }
      }
    else{                                                   // Non-scrollable
      dc.fillRectangle(1,width,width-1,1);
      dc.fillRectangle(1,height-width-1,width-1,1);
      dc.setForeground(gm_make_hilite_color(shadowColor));
      dc.fillRectangle(1,width+1,width-1,total-2);
      }
    dc.setFillStyle(FILL_SOLID);
    dc.setForeground(backColor);
    dc.fillRectangle(1,height-width,width-1,width);
    drawDownArrow(dc,0,height-width,width,width,(mode==MODE_INC));
    dc.setForeground(backColor);
    dc.fillRectangle(1,0,width-1,width);
    drawUpArrow(dc,0,0,width,width,(mode==MODE_DEC));
    }
  return 1;
  }

// Map
FXDEFMAP(GMScrollCorner) GMScrollCornerMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMScrollCorner::onPaint),
  };

FXIMPLEMENT(GMScrollCorner,FXScrollCorner,GMScrollCornerMap,ARRAYNUMBER(GMScrollCornerMap))

GMScrollCorner::GMScrollCorner(FXComposite*p):FXScrollCorner(p){
  shadowColor=getApp()->getShadowColor();
  }

// Slightly different from Frame border
long GMScrollCorner::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);
  dc.setForeground(backColor);
  dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);
  dc.setForeground(shadowColor);
  dc.fillRectangle(ev->rect.x,0,ev->rect.w,1);
  dc.fillRectangle(0,ev->rect.y,1,ev->rect.h);
  return 1;
  }



FXIMPLEMENT(GMTabBook,FXTabBook,NULL,0)

GMTabBook::GMTabBook(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb) :
	FXTabBook(p,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb) {
	}


/// Make sure old tab gets repainted
void GMTabBook::setCurrent(FXint panel,FXbool notify) {
	if (panel!=current) {
		FXint old = current;
		FXTabBook::setCurrent(panel,notify);
		FXWindow * window = childAtIndex(old<<1);
		if (window) { window->update(); }
		}
	}



// Map
FXDEFMAP(GMTabItem) GMTabItemMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMTabItem::onPaint),
  };

FXIMPLEMENT(GMTabItem,FXTabItem,GMTabItemMap,ARRAYNUMBER(GMTabItemMap))

GMTabItem::GMTabItem(FXTabBar* p,const FXString& text,FXIcon* ic,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint /*pt*/,FXint /*pb*/) : FXTabItem(p,text,ic,opts,x,y,w,h,pl,pr,5,5) {
  }

//void GMTabItem::raise() {
//  FXTabItem::raise();
//  update();
//  recalc();
//  }


/// FIXME determine last tab based visibility...
long GMTabItem::onPaint(FXObject*,FXSelector,void*){
  FXTabBar * bar = (FXTabBar*)getParent();
  FXint tab = bar->indexOfChild(this)/2;
  FXint ntabs = (bar->numChildren()/2);
  FXint ctab = bar->getCurrent();


  FXbool is_last = (tab==(ntabs-1));

  if (!is_last) {
    is_last=true;
    for (FXint t=tab+1;t<ntabs;t++){
      if (bar->childAtIndex(t<<1)->shown()) {
        is_last=false;
        break;
        }
      }
    }

  FXDCWindow dc(this);

  FXint tw=0,th=0,iw=0,ih=0,tx,ty,ix,iy;

      dc.setForeground(shadowColor);
      dc.fillRectangle(0,0,width,1);
      if (tab==ctab && tab==0)
        dc.fillRectangle(0,0,1,height);
      else
        dc.fillRectangle(0,0,1,height-1);

    /// last one or active one
    if (is_last || tab==ctab) {
      dc.fillRectangle(width-1,0,1,height-1);
      if (tab!=ctab)
        fillVerticalGradient(dc,1,1,width-2,height-2,gm_make_hilite_color(shadowColor),gm_make_hilite_color(shadowColor));
      else {
/*
        dc.setForeground(makeShadowColor(getApp()->getSelbackColor()));
        dc.fillRectangle(0,0,width,1);
        dc.fillRectangle(width-1,0,1,4);
        dc.fillRectangle(0,0,1,4);
        dc.fillRectangle(1,3,width-2,1);
*/
/*

        dc.setForeground(getApp()->getSelbackColor());
        dc.fillRectangle(1,1,width-2,2);
*/
//        dc.setForeground(backColor);
//        dc.fillRectangle(1,4,width-2,height-4);

        dc.setForeground(getApp()->getSelbackColor());
//        dc.fillRectangle(1,1,width-2,height-2);
        fillVerticalGradient(dc,1,1,width-2,height-2,gm_make_hilite_color(backColor),backColor);

        dc.setForeground(backColor);
        if (tab==0)
          dc.fillRectangle(1,height-1,width-1,height-1);
        else
          dc.fillRectangle(0,height-1,width,height-1);
        }
      }
    else {
      fillVerticalGradient(dc,1,1,width-1,height-2,gm_make_hilite_color(shadowColor),gm_make_hilite_color(shadowColor));
      }

  if(!label.empty()){
    tw=labelWidth(label);
    th=labelHeight(label);
    }
  if(icon){
    iw=icon->getWidth();
    ih=icon->getHeight();
    }
  just_x(tx,ix,tw,iw);
  just_y(ty,iy,th,ih);
  if(icon){
    if(isEnabled())
      dc.drawIcon(icon,ix,iy);
    else
      dc.drawIconSunken(icon,ix,iy);
    }
  if(!label.empty()){
    dc.setFont(font);
    if(isEnabled()){
      dc.setForeground(textColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      if(hasFocus()){
        dc.drawFocusRectangle(border+1,border+1,width-2*border-2,height-2*border-2);
        }
      }
    else{
      dc.setForeground(hiliteColor);
      drawLabel(dc,label,hotoff,tx+1,ty+1,tw,th);
      dc.setForeground(shadowColor);
      drawLabel(dc,label,hotoff,tx,ty,tw,th);
      }
    }
  return 1;
  }



FXIMPLEMENT(GMListBox,FXListBox,NULL,0);

GMListBox::GMListBox(){
  }

GMListBox::GMListBox(FXComposite*p,FXObject*tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb)
  : FXListBox(p,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb) {
  borderColor=getApp()->getShadowColor();
  setFrameStyle(FRAME_LINE);

  GMScrollArea::replaceScrollbars(list);

  pane->setBorderColor(borderColor);
  button->setFrameStyle(FRAME_NONE);
  button->setPadLeft(2);
  button->setPadRight(2);
  button->setYOffset(0);
  button->setXOffset(1);
  }


void GMListBox::create(){
  FXListBox::create();
  ewmh_change_window_type(pane,WINDOWTYPE_COMBO);
  }



FXIMPLEMENT(GMComboBox,FXComboBox,NULL,0);

GMComboBox::GMComboBox(){
  }

GMComboBox::GMComboBox(FXComposite *p,FXint cols,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb)
  :FXComboBox(p,cols,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb){
  borderColor=getApp()->getShadowColor();
  setFrameStyle(FRAME_LINE);
  pane->setBorderColor(borderColor);

  GMScrollArea::replaceScrollbars(list);
  button->setFrameStyle(FRAME_NONE);
  button->setPadLeft(2);
  button->setPadRight(2);
  button->setYOffset(0);
  button->setXOffset(1);
  }

void GMComboBox::create(){
  FXComboBox::create();
  ewmh_change_window_type(pane,WINDOWTYPE_COMBO);
  }


FXIMPLEMENT(GMImageFrame,FXImageFrame,NULL,0);

GMImageFrame::GMImageFrame(){
  }

GMImageFrame::GMImageFrame(FXComposite *p,FXImage * img,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb)
  :FXImageFrame(p,img,opts,x,y,w,h,pl,pr,pt,pb){
  borderColor=getApp()->getShadowColor();
  backColor=getApp()->getBackColor();
  }



FXIMPLEMENT(GMCoverFrame,FXVerticalFrame,NULL,0);

GMCoverFrame::GMCoverFrame(){
  }

GMCoverFrame::GMCoverFrame(FXComposite *p)
  :FXVerticalFrame(p,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_LINE,0,0,0,0,5,5,5,5){
  borderColor=getApp()->getShadowColor();
  backColor=getApp()->getBackColor();
  }


FXIMPLEMENT(GMProgressBar,FXProgressBar,NULL,0);

GMProgressBar::GMProgressBar(){
  }

GMProgressBar::GMProgressBar(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb)
  :FXProgressBar(p,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb){
  borderColor=getApp()->getShadowColor();
  barColor=getApp()->getSelbackColor();
  textAltColor=getApp()->getSelforeColor();//FXRGB(255,255,255);
  setFrameStyle(FRAME_LINE);
  }


FXDEFMAP(GMTrackProgressBar) GMTrackProgressBarMap[]={
  FXMAPFUNC(SEL_MOTION,0,GMTrackProgressBar::onMotion),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,GMTrackProgressBar::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,GMTrackProgressBar::onLeftBtnRelease)
  };

FXIMPLEMENT(GMTrackProgressBar,FXProgressBar,GMTrackProgressBarMap,ARRAYNUMBER(GMTrackProgressBarMap));

GMTrackProgressBar::GMTrackProgressBar(){
  flags|=FLAG_ENABLED;
  }

GMTrackProgressBar::GMTrackProgressBar(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb)
  :FXProgressBar(p,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb){
  flags|=FLAG_ENABLED;
  borderColor=getApp()->getShadowColor();
  barColor=getApp()->getSelbackColor();
  textAltColor=getApp()->getSelforeColor();
  setFrameStyle(FRAME_LINE);
  barsize=7;
  }



// Moving
long GMTrackProgressBar::onMotion(FXObject*,FXSelector,void*){
//  register FXEvent *event=(FXEvent*)ptr;
  register FXuint /*xx,yy,ww,hh,lo,hi,*/p=0/*,h,travel*/;
  if(!isEnabled()) return 0;
  if(flags&FLAG_PRESSED){
/*

    yy=border+padtop;
    xx=border+padleft;
    hh=height-(border<<1)-padtop-padbottom;
    ww=width-(border<<1)-padleft-padright;
    if(options&PROGRESSBAR_VERTICAL){
      h=event->win_y-dragpoint;
      travel=hh-headsize;
      if(h<yy) h=yy;
      if(h>yy+travel) h=yy+travel;
      if(h!=headpos){
        FXMINMAX(lo,hi,headpos,h);
        headpos=h;
        update(border,lo-1,width-(border<<1),hi+headsize+2-lo);
        }
      if(travel>0)
        p=range[0]+((range[1]-range[0])*(yy+travel-h)+travel/2)/travel;    // Use rounding!!
      else
        p=range[0];
      }
    else{
      h=event->win_x-dragpoint;
      travel=ww-headsize;
      if(h<xx) h=xx;
      if(h>xx+travel) h=xx+travel;
      if(h!=headpos){
        FXMINMAX(lo,hi,headpos,h);
        headpos=h;
        update(lo-1,border,hi+headsize+2-lo,height-(border<<1));
        }
      if(travel>0)
        p=range[0]+((range[1]-range[0])*(h-xx)+travel/2)/travel;    // Use rounding!!
      else
        p=range[0];
      }
    if(p<range[0]) p=range[0];
    if(p>range[1]) p=range[1];
    if(pos!=p){
      pos=p;
      flags|=FLAG_CHANGED;
      if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)pos);
      }
*/
    if(progress!=p){
      progress=p;
      flags|=FLAG_CHANGED;
      if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)progress);
      }
    return 1;
    }
  return 0;
  }

// Pressed LEFT button
long GMTrackProgressBar::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
//  register FXEvent *event=(FXEvent*)ptr;
//  register FXint p=progress;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    flags&=~FLAG_UPDATE;
/*
    if(options&PROGRESSBAR_VERTICAL){
      if(event->win_y<headpos){
        p=progress+incr;
        }
      else if(event->win_y>(headpos+headsize)){
        p=progress-incr;
        }
      else{
        dragpoint=event->win_y-headpos;
        flags|=FLAG_PRESSED;
        }
      }
    else{
      if(event->win_x<headpos){
        p=progress-incr;
        }
      else if(event->win_x>(headpos+headsize)){
        p=progress+incr;
        }
      else{
        dragpoint=event->win_x-headpos;
        flags|=FLAG_PRESSED;
        }

      }
*/

//    if(p<range[0]) p=range[0];
//    if(p>range[1]) p=range[1];
/*
    if(p!=progress){
      setProgress(p);
      flags|=FLAG_CHANGED;
      if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)progress);
      }
*/
    return 1;
    }
  return 0;
  }


// Released Left button
long GMTrackProgressBar::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  register FXEvent *event=(FXEvent*)ptr;
//  register FXuint flgs=flags;
  if(isEnabled()){
    ungrab();

    FXint xx=border+padleft;
    FXint ww=width-(border<<1)-padleft-padright;


//    setProgress(progress);                                         // Hop to exact position
    flags&=~FLAG_PRESSED;
    flags&=~FLAG_CHANGED;
    flags|=FLAG_UPDATE;
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;

    if (event->click_x > xx && event->click_x < xx+ww) {
      FXdouble pos = (event->click_x - xx) / (FXdouble)ww;
      if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),&pos);
      }
    return 1;
    }
  return 0;
  }

