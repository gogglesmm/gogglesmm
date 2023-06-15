/********************************************************************************
*                                                                               *
*                         T o p   W i n d o w   O b j e c t                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxchar.h"
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
#include "FXCursor.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXIcon.h"
#include "FXTopWindow.h"
#include "FXMainWindow.h"
#include "FXToolBar.h"
#include "FXToolBarGrip.h"
#include "FXProcess.h"
#include "FXSystem.h"
#include "FX88591Codec.h"
#include "fxpriv.h"


/*
  Notes:
  - Handle zero width/height case similar to FXWindow.
  - Pass Size Hints to Window Manager as per ICCCM.
  - Add padding options, as this is convenient for FXDialogBox subclasses;
    for FXTopWindow/FXMainWindow, padding should default to 0, for FXDialogBox,
    default to something easthetically pleasing...
  - Now observes LAYOUT_FIX_X and LAYOUT_FIX_Y hints.
  - LAYOUT_FIX_WIDTH and LAYOUT_FIX_HEIGHT take precedence over PACK_UNIFORM_WIDTH and
    PACK_UNIFORM_HEIGHT!
*/

// Definitions for Motif-style WM Hints.
#ifndef WIN32
#define MWM_HINTS_FUNCTIONS	(1L << 0)       // Definitions for FXMotifHints.flags
#define MWM_HINTS_DECORATIONS	(1L << 1)
#define MWM_HINTS_INPUT_MODE	(1L << 2)
#define MWM_HINTS_ALL           (MWM_HINTS_FUNCTIONS|MWM_HINTS_DECORATIONS|MWM_HINTS_INPUT_MODE)

#define MWM_FUNC_ALL		(1L << 0)       // Definitions for FXMotifHints.functions
#define MWM_FUNC_RESIZE		(1L << 1)
#define MWM_FUNC_MOVE		(1L << 2)
#define MWM_FUNC_MINIMIZE	(1L << 3)
#define MWM_FUNC_MAXIMIZE	(1L << 4)
#define MWM_FUNC_CLOSE		(1L << 5)

#define MWM_DECOR_ALL		(1L << 0)       // Definitions for FXMotifHints.decorations
#define MWM_DECOR_BORDER	(1L << 1)
#define MWM_DECOR_RESIZEH	(1L << 2)
#define MWM_DECOR_TITLE		(1L << 3)
#define MWM_DECOR_MENU		(1L << 4)
#define MWM_DECOR_MINIMIZE	(1L << 5)
#define MWM_DECOR_MAXIMIZE	(1L << 6)

#define MWM_INPUT_MODELESS		    0   // Values for FXMotifHints.inputmode
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL		    2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3
#endif


// Side layout modes
#define LAYOUT_SIDE_MASK (LAYOUT_SIDE_LEFT|LAYOUT_SIDE_RIGHT|LAYOUT_SIDE_TOP|LAYOUT_SIDE_BOTTOM)

// Layout modes
#define LAYOUT_MASK (LAYOUT_SIDE_MASK|LAYOUT_RIGHT|LAYOUT_CENTER_X|LAYOUT_BOTTOM|LAYOUT_CENTER_Y|LAYOUT_FIX_X|LAYOUT_FIX_Y|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_X|LAYOUT_FILL_Y)

#define DISPLAY(app) ((Display*)((app)->display))

using namespace FX;


/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXTopWindow) FXTopWindowMap[]={
  FXMAPFUNC(SEL_CLOSE,0,FXTopWindow::onCmdClose),
  FXMAPFUNC(SEL_FOCUS_UP,0,FXTopWindow::onFocusUp),
  FXMAPFUNC(SEL_FOCUS_DOWN,0,FXTopWindow::onFocusDown),
  FXMAPFUNC(SEL_FOCUS_LEFT,0,FXTopWindow::onFocusLeft),
  FXMAPFUNC(SEL_FOCUS_RIGHT,0,FXTopWindow::onFocusRight),
  FXMAPFUNC(SEL_RESTORE,0,FXTopWindow::onRestore),
  FXMAPFUNC(SEL_MAXIMIZE,0,FXTopWindow::onMaximize),
  FXMAPFUNC(SEL_MINIMIZE,0,FXTopWindow::onMinimize),
  FXMAPFUNC(SEL_SESSION_NOTIFY,0,FXTopWindow::onSessionNotify),
  FXMAPFUNC(SEL_SESSION_CLOSED,0,FXTopWindow::onSessionClosed),
  FXMAPFUNC(SEL_CHORE,FXTopWindow::ID_CLOSE,FXTopWindow::onCmdClose),
  FXMAPFUNC(SEL_SIGNAL,FXTopWindow::ID_CLOSE,FXTopWindow::onCmdClose),
  FXMAPFUNC(SEL_TIMEOUT,FXTopWindow::ID_CLOSE,FXTopWindow::onCmdClose),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_CLOSE,FXTopWindow::onCmdClose),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_RESTORE,FXTopWindow::onCmdRestore),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_MAXIMIZE,FXTopWindow::onCmdMaximize),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_MINIMIZE,FXTopWindow::onCmdMinimize),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_FULLSCREEN,FXTopWindow::onCmdFullScreen),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_SETSTRINGVALUE,FXTopWindow::onCmdSetStringValue),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_GETSTRINGVALUE,FXTopWindow::onCmdGetStringValue),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_SETICONVALUE,FXTopWindow::onCmdSetIconValue),
  FXMAPFUNC(SEL_COMMAND,FXTopWindow::ID_GETICONVALUE,FXTopWindow::onCmdGetIconValue),
  };


// Object implementation
FXIMPLEMENT_ABSTRACT(FXTopWindow,FXShell,FXTopWindowMap,ARRAYNUMBER(FXTopWindowMap))


// Deserialization
FXTopWindow::FXTopWindow(){
  icon=nullptr;
  miniIcon=nullptr;
  padtop=0;
  padbottom=0;
  padleft=0;
  padright=0;
  hspacing=0;
  vspacing=0;
  }


// Create toplevel window object & add to toplevel window list
FXTopWindow::FXTopWindow(FXApp* ap,const FXString& name,FXIcon *ic,FXIcon *mi,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXShell(ap,opts,x,y,w,h){
  title=name;
  icon=ic;
  miniIcon=mi;
  accelTable=new FXAccelTable;
  padtop=pt;
  padbottom=pb;
  padleft=pl;
  padright=pr;
  hspacing=hs;
  vspacing=vs;
  }


// Create toplevel window object & add to toplevel window list
FXTopWindow::FXTopWindow(FXWindow* ow,const FXString& name,FXIcon *ic,FXIcon *mi,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):FXShell(ow,opts,x,y,w,h){
  title=name;
  icon=ic;
  miniIcon=mi;
  accelTable=new FXAccelTable;
  padtop=pt;
  padbottom=pb;
  padleft=pl;
  padright=pr;
  hspacing=hs;
  vspacing=vs;
  }


#ifdef WIN32
const void* FXTopWindow::GetClass() const { return TEXT("FXTopWindow"); }
#endif


// Create window
void FXTopWindow::create(){
  FXShell::create();

  // Create icons
  if(icon) icon->create();
  if(miniIcon) miniIcon->create();

  if(xid){
    if(getApp()->isInitialized()){

      // Set title
      settitle();

      // Set decorations
      setdecorations();

      // Set icon for X-Windows
      seticons();

      // Only shrinkable; size may not be above default size
      if((options&DECOR_SHRINKABLE) && !(options&DECOR_STRETCHABLE)){
        if(width>getDefaultWidth()) width=getDefaultWidth();
        if(height>getDefaultHeight()) height=getDefaultHeight();
        }

      // Only stretchable; size may not be below default size
      else if((options&DECOR_STRETCHABLE) && !(options&DECOR_SHRINKABLE)){
        if(width<getDefaultWidth()) width=getDefaultWidth();
        if(height<getDefaultHeight()) height=getDefaultHeight();
        }

#ifdef WIN32
      RECT rect;
      SetRect(&rect,xpos,ypos,xpos+width,ypos+height);
      DWORD dwStyle=GetWindowLong((HWND)xid,GWL_STYLE);
      DWORD dwExStyle=GetWindowLong((HWND)xid,GWL_EXSTYLE);
      AdjustWindowRectEx(&rect,dwStyle,false,dwExStyle);        // Calculate based on *client* rectangle
      SetWindowPos((HWND)xid,nullptr,rect.left,rect.top,FXMAX(rect.right-rect.left,1),FXMAX(rect.bottom-rect.top,1),SWP_NOZORDER|SWP_NOOWNERZORDER);
#else

      // Set WM protocols
      Atom protocols[4];
      protocols[0]=getApp()->wmDeleteWindow;
      protocols[1]=getApp()->wmTakeFocus;
      protocols[2]=getApp()->wmNetPing;
      protocols[3]=getApp()->wmSaveYourself;
      XSetWMProtocols(DISPLAY(getApp()),xid,protocols,4);       // Catch delete window

      // Set resource and class name for toplevel windows.
      // In a perfect world this would be set in FXTopWindow, but for some strange reasons
      // some window-managers (e.g. fvwm) this will be too late and they will not recognize them.
      XClassHint hint;
      hint.res_name=const_cast<char*>(getApp()->getAppName().text());           // "FoxApp"
      hint.res_class=const_cast<char*>(getApp()->getVendorName().text());       // "FoxWindow"
      XSetClassHint((Display*)getApp()->getDisplay(),xid,&hint);

      // Set client machine name and application pid
      FXString hostname=FXSystem::getHostName();
      XChangeProperty((Display*)getApp()->getDisplay(),xid,getApp()->wmClientMachine,XA_STRING,8,PropModeReplace,(unsigned char*)hostname.text(),hostname.length());
      FXint proccessid=FXProcess::current();
      XChangeProperty((Display*)getApp()->getDisplay(),xid,getApp()->wmNetProcessId,XA_CARDINAL,32,PropModeReplace,(unsigned char*)&proccessid,1);

//#ifdef HAVE_XFIXES_H
//      int evb,erb;
//      if(XFixesQueryExtension(DISPLAY(getApp()),&evb,&erb)){
//        XFixesSelectSelectionInput(DISPLAY(getApp()),xid,getApp()->xcbSelection,XFixesSetSelectionOwnerNotifyMask|XFixesSelectionWindowDestroyNotifyMask|XFixesSelectionClientCloseNotifyMask);
//        }
//#endif
#endif
      }
    }
  }


// Detach window
void FXTopWindow::detach(){
  FXShell::detach();
  if(icon) icon->detach();
  if(miniIcon) miniIcon->detach();
  }


// Destroy window
void FXTopWindow::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
#ifdef WIN32
      HICON icold;
      if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_BIG,0))!=0){
        DestroyIcon(icold);
        }
      if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_SMALL,0))!=0){
        DestroyIcon(icold);
        }
#endif
      }
    }
  FXShell::destroy();
  }


// Focus to this toplevel window
void FXTopWindow::setFocus(){
  FXShell::setFocus();
  if(xid){
#ifdef WIN32
    //SetFocus((HWND)xid);
    SetActiveWindow((HWND)xid);
#else
    XSetInputFocus(DISPLAY(getApp()),xid,RevertToPointerRoot,CurrentTime);
#endif
    }
  }


// Focus away from this toplevel window
void FXTopWindow::killFocus(){
  FXShell::killFocus();
  if(xid){
#ifdef WIN32
/*
    if(GetFocus()==(HWND)xid){
      if(getOwner() && getOwner()->id()){
        FXTRACE((100,"focus back to owner\n"));
        SetFocus((HWND)getOwner()->id());
        }
      else{
        FXTRACE((100,"focus back to NULL\n"));
        SetFocus((HWND)nullptr);
        }
      }
*/
    if(GetActiveWindow()==(HWND)xid){
      if(getOwner() && getOwner()->id()){
        FXTRACE((100,"focus back to owner\n"));
        SetActiveWindow((HWND)getOwner()->getShell()->id());        // Fix from Sander
        //SetForegroundWindow((HWND)getOwner()->getShell()->id());
        }
      }
#else
    Window win;
    int    dum;
    XGetInputFocus(DISPLAY(getApp()),&win,&dum);
    if(win==xid){
      if(getOwner() && getOwner()->id()){
        FXTRACE((100,"focus back to owner\n"));
        XSetInputFocus(DISPLAY(getApp()),getOwner()->id(),RevertToPointerRoot,CurrentTime);
        }
      else{
        FXTRACE((100,"focus back to NULL\n"));
        XSetInputFocus(DISPLAY(getApp()),PointerRoot,RevertToPointerRoot,CurrentTime);
        }
      }
#endif
    }
  }


// Show and raise window
void FXTopWindow::show(){
  FXShell::show();
  raise();
  }



// Show and raise window, placed properly on the screen
void FXTopWindow::show(FXuint placement){
  place(placement);
  FXShell::show();
  raise();
  }



// Hide window
void FXTopWindow::hide(){
  if(flags&FLAG_SHOWN){
    killFocus();
    flags&=~FLAG_SHOWN;
    if(xid){
#ifdef WIN32
//      if(getOwner() && getOwner()->id()){
//        SetActiveWindow((HWND)getOwner()->getShell()->id());  // Fix from Sander
//        }
      ShowWindow((HWND)xid,SW_HIDE);
#else
      XWithdrawWindow(DISPLAY(getApp()),xid,DefaultScreen(DISPLAY(getApp())));
#endif
      }
    }
  }


// Raise and make foreground window
void FXTopWindow::raise(){
  FXShell::raise();
  if(xid){
#ifdef WIN32
//    SetForegroundWindow((HWND)xid);
    SetActiveWindow((HWND)xid);
#else
    XEvent se;
    se.xclient.type=ClientMessage;
    se.xclient.display=DISPLAY(getApp());
    se.xclient.message_type=getApp()->wmNetRestack;
    se.xclient.format=32;
    se.xclient.window=xid;
    se.xclient.data.l[0]=xid;
    se.xclient.data.l[1]=None;
    se.xclient.data.l[2]=Above;
    se.xclient.data.l[3]=0;
    se.xclient.data.l[4]=0;
    XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),false,SubstructureRedirectMask|SubstructureNotifyMask,&se);
#endif
    }
  }


// Lower
void FXTopWindow::lower(){
  FXShell::lower();
#ifndef WIN32
  if(xid){
    XEvent se;
    se.xclient.type=ClientMessage;
    se.xclient.display=DISPLAY(getApp());
    se.xclient.message_type=getApp()->wmNetRestack;
    se.xclient.format=32;
    se.xclient.window=xid;
    se.xclient.data.l[0]=xid;
    se.xclient.data.l[1]=None;
    se.xclient.data.l[2]=Below;
    se.xclient.data.l[3]=0;
    se.xclient.data.l[4]=0;
    XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),false,SubstructureRedirectMask|SubstructureNotifyMask,&se);
    }
#endif
  }


// The code below returns the size of the entire virtual
// screen area instead of just that of the primary display;
// thanks to "Steve Granja" <Steven.Granja@abaqus.com>.
// [Apparently does not work on Win95 and WinNT...]
//xpos=GetSystemMetrics(SM_XVIRTUALSCREEN);
//ypos=GetSystemMetrics(SM_YVIRTUALSCREEN);
//width=GetSystemMetrics(SM_CXVIRTUALSCREEN);
//height=GetSystemMetrics(SM_CYVIRTUALSCREEN);


// Position the window based on placement
void FXTopWindow::place(FXuint placement){
  FXint rx,ry,rw,rh,ox,oy,ow,oh,wx,wy,ww,wh,x,y;
  FXuint state;
  FXWindow *over;

  // Default placement:- leave it where it was
  wx=getX();
  wy=getY();
  ww=getWidth();
  wh=getHeight();

  // Get root window size
#ifdef WIN32
  RECT rect;
  MONITORINFO minfo;
  HMONITOR monitor;

  // Use mouse position to select screen
  if(placement!=PLACEMENT_OWNER){
    getRoot()->getCursorPosition(x,y,state);
    rect.left=x;
    rect.right=x+1;
    rect.top=y;
    rect.bottom=y+1;
    }

  // Use owner to select screen
  else{
    over=getOwner()?getOwner():getRoot();
    over->translateCoordinatesTo(ox,oy,getRoot(),0,0);
    ow=over->getWidth();
    oh=over->getHeight();
    rect.left=ox;
    rect.right=ox+ow;
    rect.top=oy;
    rect.bottom=oy+oh;
    }

  // Get monitor info if we have this API
  monitor=MonitorFromRect(&rect,MONITOR_DEFAULTTOPRIMARY);
  if(monitor){
    memset(&minfo,0,sizeof(minfo));
    minfo.cbSize=sizeof(minfo);
    GetMonitorInfo(monitor,&minfo);
    rx=minfo.rcWork.left;
    ry=minfo.rcWork.top;
    rw=minfo.rcWork.right-minfo.rcWork.left;
    rh=minfo.rcWork.bottom-minfo.rcWork.top;
    }

  // Otherwise use the work-area
  else{
    SystemParametersInfo(SPI_GETWORKAREA,sizeof(RECT),&rect,0);
    rx=rect.left;
    ry=rect.top;
    rw=rect.right-rect.left;
    rh=rect.bottom-rect.top;
    }
#else
  rx=getRoot()->getX();
  ry=getRoot()->getY();
  rw=getRoot()->getWidth();
  rh=getRoot()->getHeight();
#endif

  // Placement policy
  switch(placement){

    // Place such that it contains the cursor
    case PLACEMENT_CURSOR:

      // Get the owner
      over=getOwner()?getOwner():getRoot();

      // Get owner window size
      ow=over->getWidth();
      oh=over->getHeight();

      // Owner's coordinates to root coordinates
      over->translateCoordinatesTo(ox,oy,getRoot(),0,0);

      // Adjust position over owner
      wx=ox+(ow-ww)/2;
      wy=oy+(oh-wh)/2;

      // Where's the mouse?
      getRoot()->getCursorPosition(x,y,state);

      // Move by the minimal amount
      if(x<wx) wx=x-20; else if(wx+ww<=x) wx=x-ww+20;
      if(y<wy) wy=y-20; else if(wy+wh<=y) wy=y-wh+20;

      // Adjust so dialog is fully visible
      if(wx<rx) wx=rx+10;
      if(wy<ry) wy=ry+10;
      if(wx+ww>rx+rw) wx=rx+rw-ww-10;
      if(wy+wh>ry+rh) wy=ry+rh-wh-10;
      break;

    // Place centered over the owner
    case PLACEMENT_OWNER:

      // Get the owner
      over=getOwner()?getOwner():getRoot();

      // Get owner window size
      ow=over->getWidth();
      oh=over->getHeight();

      // Owner's coordinates to root coordinates
      over->translateCoordinatesTo(ox,oy,getRoot(),0,0);

      // Adjust position
      wx=ox+(ow-ww)/2;
      wy=oy+(oh-wh)/2;

      // Adjust so dialog is fully visible
      if(wx<rx) wx=rx+10;
      if(wy<ry) wy=ry+10;
      if(wx+ww>rx+rw) wx=rx+rw-ww-10;
      if(wy+wh>ry+rh) wy=ry+rh-wh-10;
      break;

    // Place centered on the screen
    case PLACEMENT_SCREEN:

      // Adjust position
      wx=rx+(rw-ww)/2;
      wy=ry+(rh-wh)/2;
      break;

    // Place to make it fully visible
    case PLACEMENT_VISIBLE:

      // Adjust so dialog is fully visible
      if(wx<rx) wx=rx+10;
      if(wy<ry) wy=ry+10;
      if(wx+ww>rx+rw) wx=rx+rw-ww-10;
      if(wy+wh>ry+rh) wy=ry+rh-wh-10;
      break;

    // Place maximized
    case PLACEMENT_MAXIMIZED:
      wx=rx;
      wy=ry;
      ww=rw;                // Yes, I know:- we should subtract the borders;
      wh=rh;                // trouble is, no way to know how big those are....
      break;

    // Default placement
    case PLACEMENT_DEFAULT:
    default:
      break;
    }

  // Place it
  position(wx,wy,ww,wh);
  }


// Set large icon(s)
void FXTopWindow::seticons(){
#ifdef WIN32
  HICON icold,icnew;
  icnew=nullptr;
  if(icon){
    ICONINFO iconinfo;
    iconinfo.fIcon=true;
    iconinfo.xHotspot=0;
    iconinfo.yHotspot=0;
    iconinfo.hbmMask=(HBITMAP)icon->shape;
    iconinfo.hbmColor=(HBITMAP)icon->xid;
    icnew=CreateIconIndirect(&iconinfo);
    }
  if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_BIG,(LPARAM)icnew))!=0){
    DestroyIcon(icold);
    }
  icnew=nullptr;
  if(miniIcon){
    ICONINFO iconinfo;
    iconinfo.fIcon=true;
    iconinfo.xHotspot=0;
    iconinfo.yHotspot=0;
    iconinfo.hbmMask=(HBITMAP)miniIcon->shape;
    iconinfo.hbmColor=(HBITMAP)miniIcon->xid;
    icnew=CreateIconIndirect(&iconinfo);
    }
  if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_SMALL,(LPARAM)icnew))!=0){
    DestroyIcon(icold);
    }
#else
  FXWindow *own=this;
  XWMHints  wmhints;
  wmhints.flags=InputHint|StateHint;
  wmhints.input=true;       // True, but ICCCM says it should be false....
  wmhints.initial_state=NormalState;
  if(icon){
    if(!icon->xid || !icon->shape){ fxerror("%s::setIcon: illegal icon specified.\n",getClassName()); }
    wmhints.flags|=IconPixmapHint|IconMaskHint;
    wmhints.icon_pixmap=icon->xid;
    wmhints.icon_mask=icon->shape;
    }
  else if(miniIcon){
    if(!miniIcon->xid || !miniIcon->shape){ fxerror("%s::setMiniIcon: illegal icon specified.\n",getClassName()); }
    wmhints.flags|=IconPixmapHint|IconMaskHint;
    wmhints.icon_pixmap=miniIcon->xid;
    wmhints.icon_mask=miniIcon->shape;
    }
  while(own->getOwner()){   // Find the ultimate owner of the whole chain
    own=own->getOwner();
    }
  if(own && own->id()){     // Set the window_group id; all windows in the group should be iconified together
    wmhints.flags|=WindowGroupHint;
    wmhints.window_group=own->id();
    }
  XSetWMHints(DISPLAY(getApp()),xid,&wmhints);
#endif
  }



// Set title
void FXTopWindow::settitle(){
  if(!title.empty()){
#ifdef WIN32
#ifdef UNICODE
    FXnchar unititle[1024];
    utf2ncs(unititle,title.text(),ARRAYNUMBER(unititle));
    SetWindowTextW((HWND)xid,unititle);
#else
    SetWindowTextA((HWND)xid,title.text());
#endif
#else
    FX88591Codec ascii;
    FXString string=ascii.utf2mb(title);
    XTextProperty t;
    if(XStringListToTextProperty((char**)&string,1,&t)){
      XSetWMIconName(DISPLAY(getApp()),xid,&t);
      XSetWMName(DISPLAY(getApp()),xid,&t);
      XFree(t.value);
      }

    // Extended window manager hint for true unicode name in title
    XChangeProperty(DISPLAY(getApp()),xid,getApp()->wmNetIconName,utf8Type,8,PropModeReplace,(unsigned char*)title.text(),title.length());
    XChangeProperty(DISPLAY(getApp()),xid,getApp()->wmNetWindowName,utf8Type,8,PropModeReplace,(unsigned char*)title.text(),title.length());
#endif
    }
  }



// Set decorations
void FXTopWindow::setdecorations(){
#ifdef WIN32
  // Get old style
  DWORD dwStyle=GetWindowLong((HWND)xid,GWL_STYLE);
  RECT rect;

  // Moved here just in case the size changes behind our backs
  SetRect(&rect,0,0,width,height);

  // Change style setting; note, under Windows, if we want a minimize,
  // maximize, or close button, we also need a window menu style as well.
  // Also, if you want a title, you will need a border.
  if(options&DECOR_BORDER) dwStyle|=WS_BORDER; else dwStyle&=~WS_BORDER;
  if(options&DECOR_TITLE) dwStyle|=WS_CAPTION; else dwStyle&=~WS_DLGFRAME;
  if(options&DECOR_RESIZE) dwStyle|=WS_THICKFRAME; else dwStyle&=~WS_THICKFRAME;
  if(options&DECOR_MENU) dwStyle|=WS_SYSMENU; else dwStyle&=~WS_SYSMENU;
  if(options&DECOR_CLOSE) dwStyle|=WS_SYSMENU;
  if(options&DECOR_MINIMIZE) dwStyle|=(WS_MINIMIZEBOX|WS_SYSMENU); else dwStyle&=~WS_MINIMIZEBOX;
  if(options&DECOR_MAXIMIZE) dwStyle|=(WS_MAXIMIZEBOX|WS_SYSMENU); else dwStyle&=~WS_MAXIMIZEBOX;

  // Set new style
  SetWindowLong((HWND)xid,GWL_STYLE,dwStyle);

  // Patch from Stephane Ancelot <sancelot@wanadoo.fr> and Sander Jansen <sander@knology.net>
  HMENU sysmenu=GetSystemMenu((HWND)xid,false);
  if(sysmenu){
    if(options&DECOR_CLOSE)
      EnableMenuItem(sysmenu,SC_CLOSE,MF_ENABLED);
    else
      EnableMenuItem(sysmenu,SC_CLOSE,MF_GRAYED);
    }

  // Moved here just in case SetWindowLong GWL_STYLE has changed
  // the GWL_EXSTYLE behind the scenes...
  DWORD dwExStyle=GetWindowLong((HWND)xid,GWL_EXSTYLE);

  // Adjust non-client area size based on new style
  AdjustWindowRectEx(&rect,dwStyle,false,dwExStyle);
  SetWindowPos((HWND)xid,nullptr,0,0,FXMAX(rect.right-rect.left,1),FXMAX(rect.bottom-rect.top,1),SWP_NOMOVE|SWP_NOZORDER|SWP_NOOWNERZORDER);
  RedrawWindow((HWND)xid,nullptr,nullptr,RDW_FRAME|RDW_INVALIDATE);
#else
  struct {
    long flags;
    long functions;
    long decorations;
    long inputmode;
    } prop;
  prop.flags=MWM_HINTS_FUNCTIONS|MWM_HINTS_DECORATIONS|MWM_HINTS_INPUT_MODE;
  prop.decorations=0;
  prop.functions=MWM_FUNC_MOVE;
  prop.inputmode=MWM_INPUT_MODELESS;
  if(options&DECOR_TITLE){
    prop.decorations|=MWM_DECOR_TITLE;
    }
  if(options&DECOR_MINIMIZE){
    prop.decorations|=MWM_DECOR_MINIMIZE;
    prop.functions|=MWM_FUNC_MINIMIZE;
    }
  if(options&DECOR_MAXIMIZE){
    prop.decorations|=MWM_DECOR_MAXIMIZE;
    prop.functions|=MWM_FUNC_MAXIMIZE;
    }
  if(options&DECOR_CLOSE){
    prop.functions|=MWM_FUNC_CLOSE;
    }
  if(options&DECOR_BORDER){
    prop.decorations|=MWM_DECOR_BORDER;
    }
  if(options&(DECOR_SHRINKABLE|DECOR_STRETCHABLE)){
    if(options&DECOR_BORDER) prop.decorations|=MWM_DECOR_RESIZEH;       // Only grips if border
    prop.functions|=MWM_FUNC_RESIZE;
    }
  if(options&DECOR_MENU){
    prop.decorations|=MWM_DECOR_MENU;
    prop.functions|=MWM_FUNC_RESIZE;
    }
  if(icon || miniIcon){         // Icons won't show unless we enable menu
    prop.decorations|=MWM_DECOR_MENU;
    }
  XChangeProperty(DISPLAY(getApp()),xid,getApp()->wmMotifHints,getApp()->wmMotifHints,32,PropModeReplace,(unsigned char*)&prop,4);
#endif
  }


// Obtain border sizes added to our window by the window manager
FXbool FXTopWindow::getWMBorders(FXint& left,FXint& right,FXint& top,FXint& bottom){
  left=right=top=bottom=0;
  if(xid){
#ifdef WIN32
#if(WINVER >= 0x0500)
    WINDOWINFO wi;
    GetWindowInfo((HWND)xid,&wi);
    left=wi.rcClient.left-wi.rcWindow.left;
    top=wi.rcClient.top-wi.rcWindow.top;
    right=wi.rcWindow.right-wi.rcClient.right;
    bottom=wi.rcWindow.bottom-wi.rcClient.bottom;
#endif
#else
    unsigned int sx,sy,msx,msy,cn,border,depth;
    Window w,rw,pw,*cw;
    int ox,oy;
    w=xid;
    XGetGeometry(DISPLAY(getApp()),w,&rw,&ox,&oy,&msx,&msy,&border,&depth);
    do{
      XQueryTree(DISPLAY(getApp()),w,&rw,&pw,&cw,&cn);
      XFree(cw);
      XGetGeometry(DISPLAY(getApp()),w,&rw,&ox,&oy,&sx,&sy,&border,&depth);
      if(pw!=rw){
        left+=ox;
        top+=oy;
        }
      w=pw;
      }
    while(w!=rw);
    right=(sx-msx-left);
    bottom=(sy-msy-top);
#endif
    return true;
    }
  return false;
  }


// Change decorations
void FXTopWindow::setDecorations(FXuint decorations){
  FXuint opts=(options&~DECOR_ALL) | (decorations&DECOR_ALL);
  if(options!=opts){
    options=opts;
    if(xid) setdecorations();
    recalc();
    }
  }


// Get decorations
FXuint FXTopWindow::getDecorations() const {
  return options&DECOR_ALL;
  }


// Flash the window to get user's attention
void FXTopWindow::flash(FXbool yes){
  if(xid){
#ifdef WIN32
//    FLASHWINFO flashinfo;
//    flashinfo.cbSize=sizeof(flashinfo);
//    flashinfo.hwnd=xid;
//    flashinfo.dwFlags=FLASHW_ALL;
//    flashinfo.uCount=5;
//    flashinfo.dwTimeout=0;
//    FlashWindowEx(flashinfo);
    FlashWindow((HWND)xid,false);
#else
    XEvent se;
    se.xclient.type=ClientMessage;
    se.xclient.display=DISPLAY(getApp());
    se.xclient.message_type=getApp()->wmNetState;
    se.xclient.format=32;
    se.xclient.window=xid;
    se.xclient.data.l[0]=yes;   // 0=_NET_WM_STATE_REMOVE, 1=_NET_WM_STATE_ADD, 2=_NET_WM_STATE_TOGGLE
    se.xclient.data.l[1]=getApp()->wmNetNeedAttention;
    se.xclient.data.l[2]=0;
    se.xclient.data.l[3]=0;
    se.xclient.data.l[4]=0;
    XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),False,SubstructureRedirectMask|SubstructureNotifyMask,&se);
#endif
    }
  }


// Restore window
FXbool FXTopWindow::restore(FXbool notify){
  if(isMinimized() || isMaximized() || isFullScreen()){
    if(xid){
#ifdef WIN32
      ShowWindow((HWND)xid,SW_RESTORE);
#else
      XEvent se;
      se.xclient.type=ClientMessage;
      se.xclient.display=DISPLAY(getApp());
      se.xclient.message_type=getApp()->wmNetState;
      se.xclient.format=32;
      se.xclient.window=xid;
      se.xclient.data.l[0]=0;   // 0=_NET_WM_STATE_REMOVE, 1=_NET_WM_STATE_ADD, 2=_NET_WM_STATE_TOGGLE
      se.xclient.data.l[1]=getApp()->wmNetHMaximized;
      se.xclient.data.l[2]=getApp()->wmNetVMaximized;
      se.xclient.data.l[3]=0;
      se.xclient.data.l[4]=0;
      XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),False,SubstructureRedirectMask|SubstructureNotifyMask,&se);
      se.xclient.data.l[1]=getApp()->wmNetFullScreen;
      se.xclient.data.l[2]=0;
      se.xclient.data.l[3]=0;
      se.xclient.data.l[4]=0;
      XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),False,SubstructureRedirectMask|SubstructureNotifyMask,&se);
      XMapWindow(DISPLAY(getApp()),xid);
#endif
      }
    if(notify && target){target->tryHandle(this,FXSEL(SEL_RESTORE,message),nullptr);}
    return true;
    }
  return false;
  }


// Iconify window
FXbool FXTopWindow::maximize(FXbool notify){
  if(!isMaximized()){
    if(xid){
#ifdef WIN32
      ShowWindow((HWND)xid,SW_MAXIMIZE);
#else
      XEvent se;
      se.xclient.type=ClientMessage;
      se.xclient.display=DISPLAY(getApp());
      se.xclient.message_type=getApp()->wmNetState;
      se.xclient.format=32;
      se.xclient.window=xid;
      se.xclient.data.l[0]=2;   // 0=_NET_WM_STATE_REMOVE, 1=_NET_WM_STATE_ADD, 2=_NET_WM_STATE_TOGGLE
      se.xclient.data.l[1]=getApp()->wmNetHMaximized;
      se.xclient.data.l[2]=getApp()->wmNetVMaximized;
      se.xclient.data.l[3]=0;
      se.xclient.data.l[4]=0;
      XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),false,SubstructureRedirectMask|SubstructureNotifyMask,&se);
      XMapWindow(DISPLAY(getApp()),xid);
#endif
      }
    if(notify && target){target->tryHandle(this,FXSEL(SEL_MAXIMIZE,message),nullptr);}
    return true;
    }
  return false;
  }


// Miminize or iconify window
FXbool FXTopWindow::minimize(FXbool notify){
  if(!isMinimized()){
    if(xid){
#ifdef WIN32
      ShowWindow((HWND)xid,SW_MINIMIZE);
#else
      XIconifyWindow(DISPLAY(getApp()),xid,DefaultScreen(DISPLAY(getApp())));
#endif
      }
    if(notify && target){target->tryHandle(this,FXSEL(SEL_MINIMIZE,message),nullptr);}
    return true;
    }
  return false;
  }

#if 0

    // Go to fullscreen
    DWORD dWStyle=GetWindowLong((HWND)xid,GWL_STYLE);
    WINDOWPLACEMENT windowplacement={sizeof(WINDOWPLACEMENT)};
    if(GetWindowPlacement((HWND)xid,&windowplacement)){
      HANDLE hmonitor=fxMonitorFromWindow((HWND)xid,MONITOR_DEFAULTTOPRIMARY);
      if(hmonitor){
        MYMONITORINFO monitorinfo={sizeof(MYMONITORINFO)};
        if(fxGetMonitorInfo(hmonitor,&monitorinfo)){
          SetWindowLong((HWND)xid,GWL_STYLE,dwStyle&~WS_OVERLAPPEDWINDOW);
          SetWindowPos((HWND)xid,HWND_TOP,monitorinfo.rcMonitor.left,monitorinfo.rcMonitor.top,monitorinfo.rcMonitor.right-monitorinfo.rcMonitor.left,monitorinfo.rcMonitor.bottom-monitorinfo.rcMonitor.top,SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
          }
        }
      }

    // Go back to normal
    DWORD dWStyle=GetWindowLong((HWND)xid,GWL_STYLE);
    SetWindowLong((HWND)xid,GWL_STYLE,dwStyle|WS_OVERLAPPEDWINDOW);
    SetWindowPlacement((HWND)xid,&windowplacement);
    SetWindowPos((HWND)xid,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_FRAMECHANGED);


#endif

// Make window full screen, return true if success
FXbool FXTopWindow::fullScreen(FXbool notify){
  if(!isFullScreen()){
    if(xid){
#ifdef WIN32
      ShowWindow((HWND)xid,SW_MAXIMIZE);        // FIXME
#else
      XEvent se;
      se.xclient.type=ClientMessage;
      se.xclient.display=DISPLAY(getApp());
      se.xclient.message_type=getApp()->wmNetState;
      se.xclient.format=32;
      se.xclient.window=xid;
      se.xclient.data.l[0]=2;   // 0=_NET_WM_STATE_REMOVE, 1=_NET_WM_STATE_ADD, 2=_NET_WM_STATE_TOGGLE
      se.xclient.data.l[1]=getApp()->wmNetFullScreen;
      se.xclient.data.l[2]=0;
      se.xclient.data.l[3]=0;
      se.xclient.data.l[4]=0;
      XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),false,SubstructureRedirectMask|SubstructureNotifyMask,&se);
      XMapWindow(DISPLAY(getApp()),xid);
#endif
      }
    if(notify && target){target->tryHandle(this,FXSEL(SEL_MAXIMIZE,message),nullptr);}
    return true;
    }
  return false;
  }


// Special stacking order level
FXbool FXTopWindow::stackingOrder(FXuint order){
  if(xid){
#ifdef WIN32
    switch(order){
      case STACK_BOTTOM:
        SetWindowPos((HWND)xid,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
        break;
      case STACK_TOP:
        SetWindowPos((HWND)xid,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
        break;
      default:
        SetWindowPos((HWND)xid,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
        break;
      }
#else
    XEvent se;
    switch(order){
      case STACK_BOTTOM:
        se.xclient.data.l[0]=1;   // 0=_NET_WM_STATE_REMOVE, 1=_NET_WM_STATE_ADD, 2=_NET_WM_STATE_TOGGLE
        se.xclient.data.l[1]=getApp()->wmNetBelowOthers;
        se.xclient.data.l[2]=0;
        break;
      case STACK_TOP:
        se.xclient.data.l[0]=1;   // 0=_NET_WM_STATE_REMOVE, 1=_NET_WM_STATE_ADD, 2=_NET_WM_STATE_TOGGLE
        se.xclient.data.l[1]=getApp()->wmNetAboveOthers;
        se.xclient.data.l[2]=0;
        break;
      default:
        se.xclient.data.l[0]=0;   // 0=_NET_WM_STATE_REMOVE, 1=_NET_WM_STATE_ADD, 2=_NET_WM_STATE_TOGGLE
        se.xclient.data.l[1]=getApp()->wmNetAboveOthers;
        se.xclient.data.l[2]=getApp()->wmNetBelowOthers;
        break;
      }
    se.xclient.type=ClientMessage;
    se.xclient.display=DISPLAY(getApp());
    se.xclient.message_type=getApp()->wmNetState;
    se.xclient.format=32;
    se.xclient.window=xid;
    se.xclient.data.l[3]=0;
    se.xclient.data.l[4]=0;
    XSendEvent(DISPLAY(getApp()),XDefaultRootWindow(DISPLAY(getApp())),false,SubstructureRedirectMask|SubstructureNotifyMask,&se);
#endif
    return true;
    }
  return false;
  }


// Attempt to close the window, return true if actually closed
FXbool FXTopWindow::close(FXbool notify){
  FXWindow *window;

  // Ask target if desired
  if(!notify || !target || !target->tryHandle(this,FXSEL(SEL_CLOSE,message),nullptr)){

    // Target will receive no further messages from us
    setTarget(nullptr);
    setSelector(0);

    // If there was another main level window still visible, that's all we do
    for(window=getRoot()->getFirst(); window; window=window->getNext()){
      if(window!=this && window->isMemberOf(FXMETACLASS(FXMainWindow))){
        goto x;
        }
      }

    // We've just hidden the last remaining top level window:- quit the application
    getApp()->handle(this,FXSEL(SEL_COMMAND,FXApp::ID_QUIT),nullptr);

    // Self destruct
x:  delete this;

    // Was closed
    return true;
    }
  return false;
  }


// Return true if window has been maximized
FXbool FXTopWindow::isMaximized() const {
  FXbool result=false;
  if(xid){
#ifdef WIN32
    result=IsZoomed((HWND)xid)!=0;
#else
    // For Window Managers supporting the Extended Window Manager Hints
    // See http://www.freedesktop.org/ for the official documentation of EWMH
    unsigned long n,i; Atom type; unsigned char *prop; int format;
    if(Success==XGetWindowProperty(DISPLAY(getApp()),xid,getApp()->wmNetState,0,2,false,AnyPropertyType,&type,&format,&n,&i,&prop)){
      if(type==XA_ATOM && format==32){
        for(i=0; i<n; i++){
          if(((FXID*)prop)[i]==getApp()->wmNetHMaximized) result=true;
          if(((FXID*)prop)[i]==getApp()->wmNetVMaximized) result=true;
          }
        }
      XFree(prop);
      }
#endif
    }
  return result;
  }


// Return true if window has been minimized
FXbool FXTopWindow::isMinimized() const {
  FXbool result=false;
  if(xid){
#ifdef WIN32
    result=IsIconic((HWND)xid)!=0;
#else
    // This is ICCCM compliant method to ask about WM_STATE
    unsigned long n,i; unsigned char *prop; Atom type; int format;
    if(Success==XGetWindowProperty(DISPLAY(getApp()),xid,getApp()->wmState,0,2,false,AnyPropertyType,&type,&format,&n,&i,&prop)){
      if(format==32){ result=(IconicState==*((FXuint*)prop)); }
      XFree(prop);
      }
#endif
    }
  return result;
  }


// Return true if full screen
FXbool FXTopWindow::isFullScreen() const {
  FXbool result=false;
  if(xid){
#ifdef WIN32
    result=IsZoomed((HWND)xid)!=0;          // FIXME
#else
    // For Window Managers supporting the Extended Window Manager Hints
    // See http://www.freedesktop.org/ for the official documentation of EWMH
    unsigned long n,i; Atom type; unsigned char *prop; int format;
    if(Success==XGetWindowProperty(DISPLAY(getApp()),xid,getApp()->wmNetState,0,2,false,AnyPropertyType,&type,&format,&n,&i,&prop)){
      if(type==XA_ATOM && format==32){
        for(i=0; i<n; i++){
          if(((FXID*)prop)[i]==getApp()->wmNetFullScreen) result=true;
          }
        }
      XFree(prop);
      }
#endif
    }
  return result;
  }


// Request for toplevel window move
void FXTopWindow::move(FXint x,FXint y){
  if((x!=xpos) || (y!=ypos)){
    xpos=x;
    ypos=y;
    if(xid){
#ifdef WIN32
      RECT rect;
      SetRect(&rect,xpos,ypos,0,0);
      DWORD dwStyle=GetWindowLong((HWND)xid,GWL_STYLE);
      DWORD dwExStyle=GetWindowLong((HWND)xid,GWL_EXSTYLE);
      AdjustWindowRectEx(&rect,dwStyle,false,dwExStyle);        // Calculate based on *client* rectangle
      SetWindowPos((HWND)xid,nullptr,rect.left,rect.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);
#else
      XWindowChanges cw;
      cw.x=xpos;
      cw.y=ypos;
      XReconfigureWMWindow(DISPLAY(getApp()),xid,DefaultScreen(DISPLAY(getApp())),CWX|CWY,&cw);
#endif
      }
    }
  }


// Request for toplevel window resize
void FXTopWindow::resize(FXint w,FXint h){
  if((flags&FLAG_DIRTY) || (w!=width) || (h!=height)){
    width=FXMAX(w,1);
    height=FXMAX(h,1);
    if(xid){
#ifdef WIN32
      RECT rect;
      SetRect(&rect,0,0,width,height);
      DWORD dwStyle=GetWindowLong((HWND)xid,GWL_STYLE);
      DWORD dwExStyle=GetWindowLong((HWND)xid,GWL_EXSTYLE);
      AdjustWindowRectEx(&rect,dwStyle,false,dwExStyle);        // Calculate based on *client* rectangle
      SetWindowPos((HWND)xid,nullptr,0,0,FXMAX(rect.right-rect.left,1),FXMAX(rect.bottom-rect.top,1),SWP_NOMOVE|SWP_NOZORDER|SWP_NOOWNERZORDER);
#else
      XWindowChanges changes;
      XSizeHints size;
      size.flags=USSize|PSize|PWinGravity|USPosition|PPosition;
      size.x=xpos;
      size.y=ypos;
      size.width=width;
      size.height=height;
      size.min_width=0;
      size.min_height=0;
      size.max_width=0;
      size.max_height=0;
      size.width_inc=0;
      size.height_inc=0;
      size.min_aspect.x=0;
      size.min_aspect.y=0;
      size.max_aspect.x=0;
      size.max_aspect.y=0;
      size.base_width=0;
      size.base_height=0;
      size.win_gravity=NorthWestGravity;                        // Tim Alexeevsky <realtim@mail.ru>
      size.win_gravity=StaticGravity;                           // Account for border (ICCCM)
      if(!(options&DECOR_SHRINKABLE)){
        if(!(options&DECOR_STRETCHABLE)){                       // Cannot change at all
          size.flags|=PMinSize|PMaxSize;
          size.min_width=size.max_width=width;
          size.min_height=size.max_height=height;
          }
        else{                                                   // Cannot get smaller than default
          size.flags|=PMinSize;
          size.min_width=getDefaultWidth();
          size.min_height=getDefaultHeight();
          }
        }
      else if(!(options&DECOR_STRETCHABLE)){                    // Cannot get larger than default
        size.flags|=PMaxSize;
        size.max_width=getDefaultWidth();
        size.max_height=getDefaultHeight();
        }
      XSetWMNormalHints(DISPLAY(getApp()),xid,&size);
      changes.x=0;
      changes.y=0;
      changes.width=width;
      changes.height=height;
      changes.border_width=0;
      changes.sibling=None;
      changes.stack_mode=Above;
      XReconfigureWMWindow(DISPLAY(getApp()),xid,DefaultScreen(DISPLAY(getApp())),CWWidth|CWHeight,&changes);
#endif
      layout();
      }
    }
  }


// Request for toplevel window reposition
void FXTopWindow::position(FXint x,FXint y,FXint w,FXint h){
  if((flags&FLAG_DIRTY) || (x!=xpos) || (y!=ypos) || (w!=width) || (h!=height)){
    xpos=x;
    ypos=y;
    width=FXMAX(w,1);
    height=FXMAX(h,1);
    if(xid){
#ifdef WIN32
      RECT rect;
      SetRect(&rect,xpos,ypos,xpos+width,ypos+height);
      DWORD dwStyle=GetWindowLong((HWND)xid,GWL_STYLE);
      DWORD dwExStyle=GetWindowLong((HWND)xid,GWL_EXSTYLE);
      AdjustWindowRectEx(&rect,dwStyle,false,dwExStyle);        // Calculate based on *client* rectangle
      SetWindowPos((HWND)xid,nullptr,rect.left,rect.top,FXMAX(rect.right-rect.left,1),FXMAX(rect.bottom-rect.top,1),SWP_NOZORDER|SWP_NOOWNERZORDER);
#else
      XWindowChanges changes;
      XSizeHints size;
      size.flags=USSize|PSize|PWinGravity|USPosition|PPosition;
      size.x=xpos;
      size.y=ypos;
      size.width=width;
      size.height=height;
      size.min_width=0;
      size.min_height=0;
      size.max_width=0;
      size.max_height=0;
      size.width_inc=0;
      size.height_inc=0;
      size.min_aspect.x=0;
      size.min_aspect.y=0;
      size.max_aspect.x=0;
      size.max_aspect.y=0;
      size.base_width=0;
      size.base_height=0;
      size.win_gravity=NorthWestGravity;                        // Tim Alexeevsky <realtim@mail.ru>
      size.win_gravity=StaticGravity;                           // Account for border (ICCCM)
      if(!(options&DECOR_SHRINKABLE)){
        if(!(options&DECOR_STRETCHABLE)){                       // Cannot change at all
          size.flags|=PMinSize|PMaxSize;
          size.min_width=size.max_width=width;
          size.min_height=size.max_height=height;
          }
        else{                                                   // Cannot get smaller than default
          size.flags|=PMinSize;
          size.min_width=getDefaultWidth();
          size.min_height=getDefaultHeight();
          }
        }
      else if(!(options&DECOR_STRETCHABLE)){                    // Cannot get larger than default
        size.flags|=PMaxSize;
        size.max_width=getDefaultWidth();
        size.max_height=getDefaultHeight();
        }
      XSetWMNormalHints(DISPLAY(getApp()),xid,&size);
      changes.x=xpos;
      changes.y=ypos;
      changes.width=width;
      changes.height=height;
      changes.border_width=0;
      changes.sibling=None;
      changes.stack_mode=Above;
      XReconfigureWMWindow(DISPLAY(getApp()),xid,DefaultScreen(DISPLAY(getApp())),CWX|CWY|CWWidth|CWHeight,&changes);
#endif
      layout();
      }
    }
  }


// Compute minimum width based on child layout hints
FXint FXTopWindow::getDefaultWidth(){
  FXint w,wcum,wmax,mw;
  FXWindow* child;
  FXuint hints;
  wmax=wcum=mw=0;
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  for(child=getLast(); child; child=child->getPrev()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else if(options&PACK_UNIFORM_WIDTH) w=mw;
      else w=child->getDefaultWidth();
      if((hints&LAYOUT_RIGHT)&&(hints&LAYOUT_CENTER_X)){    // Fixed X
        w=child->getX()+w;
        if(w>wmax) wmax=w;
        }
      else if(hints&LAYOUT_SIDE_LEFT){                      // Left or right
        if(child->getNext()) wcum+=hspacing;
        wcum+=w;
        }
      else{
        if(w>wcum) wcum=w;
        }
      }
    }
  wcum+=padleft+padright;
  return FXMAX(wcum,wmax);
  }


// Compute minimum height based on child layout hints
FXint FXTopWindow::getDefaultHeight(){
  FXint h,hcum,hmax,mh;
  FXWindow* child;
  FXuint hints;
  hmax=hcum=mh=0;
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();
  for(child=getLast(); child; child=child->getPrev()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
      else if(options&PACK_UNIFORM_HEIGHT) h=mh;
      else h=child->getDefaultHeight();
      if((hints&LAYOUT_BOTTOM)&&(hints&LAYOUT_CENTER_Y)){   // Fixed Y
        h=child->getY()+h;
        if(h>hmax) hmax=h;
        }
      else if(!(hints&LAYOUT_SIDE_LEFT)){                   // Top or bottom
        if(child->getNext()) hcum+=vspacing;
        hcum+=h;
        }
      else{
        if(h>hcum) hcum=h;
        }
      }
    }
  hcum+=padtop+padbottom;
  return FXMAX(hcum,hmax);
  }


// Recalculate layout
void FXTopWindow::layout(){
  FXint left,right,top,bottom,x,y,w,h;
  FXint mw=0,mh=0;
  FXWindow* child;
  FXuint hints;

  // Placement rectangle; right/bottom non-inclusive
  left=padleft;
  right=width-padright;
  top=padtop;
  bottom=height-padbottom;

  // Get maximum child size
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();

  // Pack them in the cavity
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      x=child->getX();
      y=child->getY();

      // Vertical
      if(hints&LAYOUT_SIDE_LEFT){

        // Height
        if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else if(hints&LAYOUT_FILL_Y) h=bottom-top;
        else h=child->getDefaultHeight();

        // Width
        if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else if(hints&LAYOUT_FILL_X) w=right-left;
        else w=child->getWidthForHeight(h);             // Width is a function of height!

        // Y
        if(!((hints&LAYOUT_BOTTOM)&&(hints&LAYOUT_CENTER_Y))){
          if(hints&LAYOUT_CENTER_Y) y=top+(bottom-top-h)/2;
          else if(hints&LAYOUT_BOTTOM) y=bottom-h;
          else y=top;
          }

        // X
        if(!((hints&LAYOUT_RIGHT)&&(hints&LAYOUT_CENTER_X))){
          if(hints&LAYOUT_CENTER_X) x=left+(right-left-w)/2;
          else if(hints&LAYOUT_SIDE_BOTTOM){            // Right
            x=right-w;
            right-=(w+hspacing);
            }
          else{                                         // Left
            x=left;
            left+=(w+hspacing);
            }
          }
        }

      // Horizontal
      else{

        // Width
        if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else if(hints&LAYOUT_FILL_X) w=right-left;
        else w=child->getDefaultWidth();

        // Height
        if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else if(hints&LAYOUT_FILL_Y) h=bottom-top;
        else h=child->getHeightForWidth(w);             // Height is a function of width!

        // X
        if(!((hints&LAYOUT_RIGHT)&&(hints&LAYOUT_CENTER_X))){
          if(hints&LAYOUT_CENTER_X) x=left+(right-left-w)/2;
          else if(hints&LAYOUT_RIGHT) x=right-w;
          else x=left;
          }

        // Y
        if(!((hints&LAYOUT_BOTTOM)&&(hints&LAYOUT_CENTER_Y))){
          if(hints&LAYOUT_CENTER_Y) y=top+(bottom-top-h)/2;
          else if(hints&LAYOUT_SIDE_BOTTOM){            // Bottom
            y=bottom-h;
            bottom-=(h+vspacing);
            }
          else{                                         // Top
            y=top;
            top+=(h+vspacing);
            }
          }
        }
      child->position(x,y,w,h);
      }
    }
  flags&=~FLAG_DIRTY;
  }


// Update value from a message
long FXTopWindow::onCmdSetStringValue(FXObject*,FXSelector,void* ptr){
  setTitle(*((FXString*)ptr));
  return 1;
  }


// Obtain value from text field
long FXTopWindow::onCmdGetStringValue(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getTitle();
  return 1;
  }


// Update icon from a message
long FXTopWindow::onCmdSetIconValue(FXObject*,FXSelector,void* ptr){
  setMiniIcon(*((FXIcon**)ptr));
  return 1;
  }


// Obtain icon from text field
long FXTopWindow::onCmdGetIconValue(FXObject*,FXSelector,void* ptr){
  *((FXIcon**)ptr)=getMiniIcon();
  return 1;
  }


// Restore the window
long FXTopWindow::onCmdRestore(FXObject*,FXSelector,void*){
  restore(true);
  return 1;
  }


// Maximize the window
long FXTopWindow::onCmdMaximize(FXObject*,FXSelector,void*){
  maximize(true);
  return 1;
  }


// Minimize or iconify the window
long FXTopWindow::onCmdMinimize(FXObject*,FXSelector,void*){
  minimize(true);
  return 1;
  }


// Full screen the window
long FXTopWindow::onCmdFullScreen(FXObject*,FXSelector,void*){
  fullScreen(true);
  return 1;
  }


// Close window; ask target before doing close
long FXTopWindow::onCmdClose(FXObject*,FXSelector,void*){
  close(true);
  return 1;
  }


// Session is about to close, give opportunity to save data
long FXTopWindow::onSessionNotify(FXObject*,FXSelector,void* ptr){
  FXTRACE((100,"%s::onSessionNotify %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_SESSION_NOTIFY,message),ptr);
  }


// Session has closed, close the window with prejudice
long FXTopWindow::onSessionClosed(FXObject*,FXSelector,void* ptr){
  FXTRACE((100,"%s::onSessionClosed %p\n",getClassName(),this));
  if(target) target->tryHandle(this,FXSEL(SEL_SESSION_CLOSED,message),ptr);
  close(false);
  return 1;
  }


// Focus moved up
long FXTopWindow::onFocusUp(FXObject*,FXSelector,void* ptr){
  FXWindow *child,*c;
  FXint cury,childy;
  if(getFocus()){
    cury=getFocus()->getY();
    while(1){
      child=nullptr;
      childy=-10000000;
      for(c=getFirst(); c; c=c->getNext()){
        if(c->shown() && c->getY()<cury && childy<c->getY()){ childy=c->getY(); child=c; }
        }
      if(!child) return 0;
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_UP,0),ptr)) return 1;
      cury=childy;
      }
    }
  else{
    child=getLast();
    while(child){
      if(child->shown()){
        if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
        if(child->handle(this,FXSEL(SEL_FOCUS_UP,0),ptr)) return 1;
        }
      child=child->getPrev();
      }
    }
  return 0;
  }


// Focus moved down
long FXTopWindow::onFocusDown(FXObject*,FXSelector,void* ptr){
  FXWindow *child,*c;
  FXint cury,childy;
  if(getFocus()){
    cury=getFocus()->getY();
    while(1){
      child=nullptr;
      childy=10000000;
      for(c=getFirst(); c; c=c->getNext()){
        if(c->shown() && cury<c->getY() && c->getY()<childy){ childy=c->getY(); child=c; }
        }
      if(!child) return 0;
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_DOWN,0),ptr)) return 1;
      cury=childy;
      }
    }
  else{
    child=getFirst();
    while(child){
      if(child->shown()){
        if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
        if(child->handle(this,FXSEL(SEL_FOCUS_DOWN,0),ptr)) return 1;
        }
      child=child->getNext();
      }
    }
  return 0;
  }


// Focus moved to left
long FXTopWindow::onFocusLeft(FXObject*,FXSelector,void* ptr){
  FXWindow *child,*c;
  FXint curx,childx;
  if(getFocus()){
    curx=getFocus()->getX();
    while(1){
      child=nullptr;
      childx=-10000000;
      for(c=getFirst(); c; c=c->getNext()){
        if(c->shown() && c->getX()<curx && childx<c->getX()){ childx=c->getX(); child=c; }
        }
      if(!child) return 0;
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_LEFT,0),ptr)) return 1;
      curx=childx;
      }
    }
  else{
    child=getLast();
    while(child){
      if(child->shown()){
        if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
        if(child->handle(this,FXSEL(SEL_FOCUS_LEFT,0),ptr)) return 1;
        }
      child=child->getPrev();
      }
    }
  return 0;
  }


// Focus moved to right
long FXTopWindow::onFocusRight(FXObject*,FXSelector,void* ptr){
  FXWindow *child,*c;
  FXint curx,childx;
  if(getFocus()){
    curx=getFocus()->getX();
    while(1){
      child=nullptr;
      childx=10000000;
      for(c=getFirst(); c; c=c->getNext()){
        if(c->shown() && curx<c->getX() && c->getX()<childx){ childx=c->getX(); child=c; }
        }
      if(!child) return 0;
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_RIGHT,0),ptr)) return 1;
      curx=childx;
      }
    }
  else{
    child=getFirst();
    while(child){
      if(child->shown()){
        if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
        if(child->handle(this,FXSEL(SEL_FOCUS_RIGHT,0),ptr)) return 1;
        }
      child=child->getNext();
      }
    }
  return 0;
  }


// Handle restore notify
long FXTopWindow::onRestore(FXObject*,FXSelector,void* ptr){
  FXTRACE((100,"%s::onRestore %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_RESTORE,message),ptr);
  }


// Handle maximize notify
long FXTopWindow::onMaximize(FXObject*,FXSelector,void* ptr){
  FXTRACE((100,"%s::onMaximize %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_MAXIMIZE,message),ptr);
  }


// Handle minimize notify
long FXTopWindow::onMinimize(FXObject*,FXSelector,void* ptr){
  FXTRACE((100,"%s::onMinimize %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_MINIMIZE,message),ptr);
  }


// Change regular icon
void FXTopWindow::setIcon(FXIcon* ic){
  if(icon!=ic){
    icon=ic;
    if(xid){
#ifdef WIN32
      HICON icold=nullptr;
      HICON icnew=nullptr;
      if(icon){
        ICONINFO iconinfo;
        iconinfo.fIcon=true;
        iconinfo.xHotspot=0;
        iconinfo.yHotspot=0;
        iconinfo.hbmMask=(HBITMAP)icon->shape;
        iconinfo.hbmColor=(HBITMAP)icon->xid;
        icnew=CreateIconIndirect(&iconinfo);
        }
      if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_BIG,(LPARAM)icnew))!=0){
        DestroyIcon(icold);
        }
#else
      seticons();
#endif
      }
    }
  }


// Change mini icon
void FXTopWindow::setMiniIcon(FXIcon *ic){
  if(miniIcon!=ic){
    miniIcon=ic;
    if(xid){
#ifdef WIN32
      HICON icold=nullptr;
      HICON icnew=nullptr;
      if(miniIcon){
        ICONINFO iconinfo;
        iconinfo.fIcon=true;
        iconinfo.xHotspot=0;
        iconinfo.yHotspot=0;
        iconinfo.hbmMask=(HBITMAP)miniIcon->shape;
        iconinfo.hbmColor=(HBITMAP)miniIcon->xid;
        icnew=CreateIconIndirect(&iconinfo);
        }
      if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_SMALL,(LPARAM)icnew))!=0){
        DestroyIcon(icold);
        }
#else
      seticons();
#endif
      }
    }
  }


// Set new window title
void FXTopWindow::setTitle(const FXString& name){
  if(title!=name){
    title=name;
    if(xid) settitle();
    }
  }


// Change packing hints
void FXTopWindow::setPackingHints(FXuint ph){
  FXuint opts=(options&~(PACK_UNIFORM_HEIGHT|PACK_UNIFORM_WIDTH)) | (ph&(PACK_UNIFORM_HEIGHT|PACK_UNIFORM_WIDTH));
  if(opts!=options){
    options=opts;
    recalc();
    update();
    }
  }


// Get packing hints
FXuint FXTopWindow::getPackingHints() const {
  return (options&(PACK_UNIFORM_HEIGHT|PACK_UNIFORM_WIDTH));
  }


// Change top padding
void FXTopWindow::setPadTop(FXint pt){
  if(padtop!=pt){
    padtop=pt;
    recalc();
    update();
    }
  }


// Change bottom padding
void FXTopWindow::setPadBottom(FXint pb){
  if(padbottom!=pb){
    padbottom=pb;
    recalc();
    update();
    }
  }


// Change left padding
void FXTopWindow::setPadLeft(FXint pl){
  if(padleft!=pl){
    padleft=pl;
    recalc();
    update();
    }
  }


// Change right padding
void FXTopWindow::setPadRight(FXint pr){
  if(padright!=pr){
    padright=pr;
    recalc();
    update();
    }
  }


// Change horizontal spacing
void FXTopWindow::setHSpacing(FXint hs){
  if(hspacing!=hs){
    hspacing=hs;
    recalc();
    update();
    }
  }


// Change vertical spacing
void FXTopWindow::setVSpacing(FXint vs){
  if(vspacing!=vs){
    vspacing=vs;
    recalc();
    update();
    }
  }


// Save object to stream
void FXTopWindow::save(FXStream& store) const {
  FXShell::save(store);
  store << title;
  store << icon;
  store << miniIcon;
  store << padtop;
  store << padbottom;
  store << padleft;
  store << padright;
  store << hspacing;
  store << vspacing;
  }


// Load object from stream
void FXTopWindow::load(FXStream& store){
  FXShell::load(store);
  store >> title;
  store >> icon;
  store >> miniIcon;
  store >> padtop;
  store >> padbottom;
  store >> padleft;
  store >> padright;
  store >> hspacing;
  store >> vspacing;
  }


// Remove this one from toplevel window list
FXTopWindow::~FXTopWindow(){
#ifdef WIN32
  HICON icold;
  if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_BIG,0))!=0){           // FIXME move to destroy()?
    DestroyIcon(icold);
    }
  if((icold=(HICON)SendMessage((HWND)xid,WM_SETICON,ICON_SMALL,0))!=0){
    DestroyIcon(icold);
    }
#endif
  icon=(FXIcon*)-1L;
  miniIcon=(FXIcon*)-1L;
  }

}
