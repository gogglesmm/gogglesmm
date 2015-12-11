/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2008-2016 by Sander Jansen. All Rights Reserved      *
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
#include "gmutils.h"

#include "xincs.h"

#include "icons.h"
#include "FXPNGIcon.h"

#include "GMTrack.h"
#include "GMPlayerManager.h"
#include "GMWindow.h"
#include "GMRemote.h"


#include "GMApp.h"
#include "GMTrayIcon.h"
#include "GMIconTheme.h"


enum {
  SYSTEM_TRAY_REQUEST_DOCK  =0,
  SYSTEM_TRAY_BEGIN_MESSAGE =1,
  SYSTEM_TRAY_CANCEL_MESSAGE=2,
  };

enum {
  SYSTEM_TRAY_HORIZONTAL = 0,
  SYSTEM_TRAY_VERTICAL   = 1,
  SYSTEM_TRAY_UNKNOWN    = 2
  };


FXDEFMAP(GMTrayIcon) GMTrayIconMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMTrayIcon::onPaint),
  FXMAPFUNC(SEL_CONFIGURE,0,GMTrayIcon::onConfigure),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,GMTrayIcon::onLeftBtnPress),
  FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,0,GMTrayIcon::onMiddleBtnPress),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,0,GMTrayIcon::onRightBtnRelease),
  FXMAPFUNC(SEL_MOUSEWHEEL,0,GMTrayIcon::onMouseWheel),
  FXMAPFUNC(SEL_QUERY_TIP,0,GMTrayIcon::onQueryTip),
  };

FXIMPLEMENT(GMTrayIcon,GMPlug,GMTrayIconMap,ARRAYNUMBER(GMTrayIconMap));


GMTrayIcon::GMTrayIcon(FXApp * a) : GMPlug(a) {
  flags|=FLAG_ENABLED;
  }

GMTrayIcon::~GMTrayIcon(){
  if (icon) delete icon;
  }

void GMTrayIcon::updateIcon() {
  if (icon) {
    FXint size = icon->getWidth();

    /// Delete the old
    delete icon;
    icon=nullptr;

    /// Update
    if (size<=16) {
      icon = new FXPNGIcon(getApp(),gogglesmm_16_png,0,opaque ? IMAGE_OPAQUE : 0);
      icon->setVisual(getVisual());
      if (size!=16) icon->scale(size,size,FOX_SCALE_BEST);
      }
    else {
      icon = new FXPNGIcon(getApp(),gogglesmm_32_png,0,opaque ? IMAGE_OPAQUE : 0);
      icon->setVisual(getVisual());
      if (size!=32) icon->scale(size,size,FOX_SCALE_BEST);
      }

    icon->blend(GMPlayerManager::instance()->getPreferences().gui_tray_color);
    icon->create();

    // Mark Dirty
    update();
    }
  }

void GMTrayIcon::display(const GMTrack &track){
  setToolTip(FXString::value("%s\n%s\n%s (%d)",track.title.text(),track.artist.text(),track.album.text(),track.year));
  }

void GMTrayIcon::reset() {
  setToolTip(FXString::null);
  }

FXbool GMTrayIcon::findSystemTray() {
  FXString systemtray = FXString::value("_NET_SYSTEM_TRAY_S%d",DefaultScreen((Display*)getApp()->getDisplay()));
  Atom xtrayselection = XInternAtom((Display*)getApp()->getDisplay(),systemtray.text(),0);
  if (xtrayselection!=None) {
    xtraywindow = (FXID)XGetSelectionOwner((Display*)getApp()->getDisplay(),xtrayselection);
    }
  return (xtraywindow!=0);
  }


void GMTrayIcon::requestDock() {
  if (xid && xtraywindow) {
    XEvent ev;
    memset(&ev,0,sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = xtraywindow;
    ev.xclient.message_type = xtrayopcode;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = SYSTEM_TRAY_REQUEST_DOCK;
    ev.xclient.data.l[2] = xid;
    XSendEvent((Display*)getApp()->getDisplay(),xtraywindow,False,NoEventMask,&ev);
    XSync((Display*)getApp()->getDisplay(),0);
    }
  }

FXuint GMTrayIcon::getTrayOrientation(){
  if (xtrayorientation || xtrayxfceorientation) {
    FXuint orientation;
    Atom returntype;
    int returnformat;
    unsigned long nitems;
    unsigned long nbytes;
    long * bytes=nullptr;

    if (xtrayorientation && (XGetWindowProperty((Display*)getApp()->getDisplay(),xtraywindow,xtrayorientation,0,2,False,XA_CARDINAL,&returntype,&returnformat,&nitems,&nbytes,(unsigned char**)&bytes)==Success) && returntype==XA_CARDINAL){
      orientation=*(long*)bytes;
      XFree(bytes);
      return orientation;
      }

    if (bytes!=nullptr) {
      XFree(bytes);
      bytes=nullptr;
      }

    if (xtrayxfceorientation && (XGetWindowProperty((Display*)getApp()->getDisplay(),xtraywindow,xtrayxfceorientation,0,2,False,XA_CARDINAL,&returntype,&returnformat,&nitems,&nbytes,(unsigned char**)&bytes)==Success) && returntype==XA_CARDINAL){
      orientation=*(long*)bytes;
      XFree(bytes);
      return orientation;
      }

    if (bytes!=nullptr)  {
      XFree(bytes);
      bytes=nullptr;
      }
    }
  return SYSTEM_TRAY_UNKNOWN;
  }


FXuint GMTrayIcon::getTrayVisual(){
  if (xtrayvisual) {
    FXuint visualid;
    Atom returntype;
    int returnformat;
    unsigned long nitems;
    unsigned long nbytes;
    long * bytes=nullptr;

    if (xtrayvisual && (XGetWindowProperty((Display*)getApp()->getDisplay(),xtraywindow,xtrayvisual,0,2,False,XA_VISUALID,&returntype,&returnformat,&nitems,&nbytes,(unsigned char**)&bytes)==Success) && returntype==XA_VISUALID){
      visualid=*(long*)bytes;
      XFree(bytes);
      return visualid;
      }

    if (bytes!=nullptr) {
      XFree(bytes);
      bytes=nullptr;
      }
    }
  return 0;
  }

void GMTrayIcon::create(){
  xtrayopcode           = (FXID)XInternAtom((Display*)getApp()->getDisplay(),"_NET_SYSTEM_TRAY_OPCODE",0);
  xtrayorientation      = (FXID)XInternAtom((Display*)getApp()->getDisplay(),"_NET_SYSTEM_TRAY_ORIENTATION",0);
  xtrayxfceorientation  = (FXID)XInternAtom((Display*)getApp()->getDisplay(),"_NET_XFCE_TRAY_MANAGER_ORIENTATION",0);
  xtrayvisual           = (FXID)XInternAtom((Display*)getApp()->getDisplay(),"_NET_SYSTEM_TRAY_VISUAL",0);

  GMPlug::create();
  if (xid) {

    /// Set the size hints...
    XSizeHints size;
    size.flags=PMinSize|PMaxSize|PBaseSize|PAspect;
    size.x=0;
    size.y=0;
    size.width=0;
    size.height=0;
    size.width_inc=0;
    size.height_inc=0;
    size.min_aspect.x=1;
    size.min_aspect.y=1;
    size.max_aspect.x=1;
    size.max_aspect.y=1;
    size.win_gravity=0;
    size.win_gravity=0;

    size.min_width=8;
    size.min_height=8;
    size.max_width=64;
    size.max_height=64;
    size.base_width=32;
    size.base_height=32;
    XSetWMNormalHints((Display*)getApp()->getDisplay(),xid,&size);

    dock();
    }
  }

void GMTrayIcon::dock() {
  if (findSystemTray()){
    FXuint trayid = getTrayVisual();
    if (trayid) {
      if (trayid!=XVisualIDFromVisual((Visual*)getVisual()->getVisual()))
        opaque=true;
      else
        opaque=false;
      }

    if (!opaque) {
      /// Don't draw the background
      XSetWindowAttributes sattr;
      sattr.background_pixmap = ParentRelative;
      XChangeWindowAttributes((Display*)getApp()->getDisplay(),xid,CWBackPixmap,&sattr);
      }

    requestDock();
    }
  }

long GMTrayIcon::onConfigure(FXObject*,FXSelector,void*ptr){
  FXEvent * event = (FXEvent*)ptr;

  FXuint orientation = getTrayOrientation();
  FXint size=0;
  switch(orientation){
    case SYSTEM_TRAY_HORIZONTAL: size=event->rect.h; break;
    case SYSTEM_TRAY_VERTICAL  : size=event->rect.w; break;
    default                    : size=FXMAX(event->rect.h,event->rect.w); break;
    };

  resize(size,size);

  XSizeHints hint;
  hint.flags=PMinSize|PMaxSize|PBaseSize;
  hint.min_width = hint.max_width = hint.base_width = size;
  hint.min_height = hint.max_height = hint.base_height = size;
  XSetWMNormalHints((Display*)getApp()->getDisplay(),xid,&hint);


  if (icon && icon->getWidth()!=size) {
    delete icon;
    icon=nullptr;
    }

  if (icon==nullptr) {
    if (size<=16) {
      icon = new FXPNGIcon(getApp(),gogglesmm_16_png,0,opaque ? IMAGE_OPAQUE : 0);
      icon->setVisual(getVisual());
      if (size!=16) icon->scale(size,size,FOX_SCALE_BEST);
      }
    else {
      icon = new FXPNGIcon(getApp(),gogglesmm_32_png,0,opaque ? IMAGE_OPAQUE : 0);
      icon->setVisual(getVisual());
      if (size!=32) icon->scale(size,size,FOX_SCALE_BEST);
      }
    icon->blend(GMPlayerManager::instance()->getPreferences().gui_tray_color);
    icon->create();
    }
  return 1;
  }


long GMTrayIcon::onRightBtnRelease(FXObject*,FXSelector,void*ptr){
  FXEvent * event=(FXEvent*)ptr;
  if (event->moved) return 0;
  GMMenuPane pane(this,POPUP_SHRINKWRAP);
  new GMMenuCommand(&pane,pane.tr("Play"),GMIconTheme::instance()->icon_play,GMPlayerManager::instance()->getMainWindow(),GMWindow::ID_PLAYPAUSEMENU);
  new GMMenuCommand(&pane,pane.tr("Stop"),GMIconTheme::instance()->icon_stop,GMPlayerManager::instance()->getMainWindow(),GMWindow::ID_STOP);
  new GMMenuCommand(&pane,pane.tr("Previous Track"),GMIconTheme::instance()->icon_prev,GMPlayerManager::instance()->getMainWindow(),GMWindow::ID_PREV);
  new GMMenuCommand(&pane,pane.tr("Next Track"),GMIconTheme::instance()->icon_next,GMPlayerManager::instance()->getMainWindow(),GMWindow::ID_NEXT);
  new FXMenuSeparator(&pane);
  new GMMenuCommand(&pane,tr("Quit"),GMIconTheme::instance()->icon_exit,GMPlayerManager::instance()->getMainWindow(),GMWindow::ID_QUIT);
  gm_set_window_cursor(&pane,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
  pane.create();
  ewmh_change_window_type(&pane,WINDOWTYPE_POPUP_MENU);
  gm_run_popup_menu(&pane,event->root_x+1,event->root_y+1);
  return 1;
  }


long GMTrayIcon::onLeftBtnPress(FXObject*,FXSelector,void*){
  GMPlayerManager::instance()->cmd_toggle_shown();
  return 1;
  }

long GMTrayIcon::onMiddleBtnPress(FXObject*,FXSelector,void*){
  GMPlayerManager::instance()->cmd_playpause();
  return 1;
  }

long GMTrayIcon::onMouseWheel(FXObject*sender,FXSelector sel,void*ptr){
  GMPlayerManager::instance()->getMainWindow()->handle(sender,sel,ptr);
  return 1;
  }

long GMTrayIcon::onQueryTip(FXObject*sender,FXSelector sel,void* ptr){
  if(FXTopWindow::onQueryTip(sender,sel,ptr)) return 1;
  if((flags&FLAG_TIP) && !tip.empty() && !grabbed() ){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&tip);
    return 1;
    }
  return 0;
  }


long GMTrayIcon::onPaint(FXObject*,FXSelector,void*){
  if (icon) {
    FXDCWindow dc(this);
    dc.drawIcon(icon,0,0);
    }
  return 1;
  }
