/********************************************************************************
*                                                                               *
*                            W i n d o w   O b j e c t                          *
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
#include "FXElement.h"
#include "FXMutex.h"
#include "FXAutoThreadStorageKey.h"
#include "FXRunnable.h"
#include "FXThread.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXRegion.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXVisual.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXImage.h"
#include "FXBitmap.h"
#include "FXIcon.h"
#include "FXCursor.h"
#include "FXWindow.h"
#include "FXFrame.h"
#include "FXComposite.h"
#include "FXRootWindow.h"
#include "FXShell.h"
#include "FXTopWindow.h"
#include "FXException.h"
#include "FXStatusLine.h"
#include "FXTranslator.h"
#include "FXComposeContext.h"
#include "FXTextField.h"
#include "FX88591Codec.h"
#include "FXUTF16Codec.h"
#include "fxpriv.h"


/*
 Notes:
  - Major Contributions for Windows NT by Lyle Johnson.
  - When window is disabled, it should lose focus
  - When no subclass handles SEL_SELECTION_REQUEST, send back None property here
  - Update should only happen if widget is not in some sort of transaction.
  - Not every widget needs help and tip data; move this down to buttons etc.
  - Default constructors [for serialization] should initialize dynamic member
    variables same as regular constructor.  Dynamic variables are those that
    are not saved during serialization.
  - If FLAG_DIRTY gets reset at the END of layout(), it will be safe to have
    show() and hide() to call recalc(), in case they get called from the
    application code.
  - Use INCR mechanism if it gets large.
  - Perhaps should post CLIPBOARD type list in advance also, just like windows insists.
    We will need to keep this list some place [perhaps same place as XDND] and clean it
    after we're done [This would imply the CLIPBOARD type list would be the same as the
    XDND type list, which is probably OK].
  - Add URL jump text also.
  - Can we call parent->recalc() in the constructor?
  - Need other focus models:
      o click to focus (like we have now)
      o focus only by tabs and arrows (buttons should not move focus when clicking)
      o point to focus (useful for canvas type widgets)
    Mouse wheel should probably not cause focus changes...
  - Close v.s. delete messages are not consistent.
*/


#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001
#define TOPIC_KEYBOARD  1009

#ifndef WIN32

// Basic events
#define BASIC_EVENT_MASK   (StructureNotifyMask|ExposureMask|PropertyChangeMask|EnterWindowMask|LeaveWindowMask|KeyPressMask|KeyReleaseMask|KeymapStateMask)
//#define BASIC_EVENT_MASK   (StructureNotifyMask|ExposureMask|PropertyChangeMask|EnterWindowMask|LeaveWindowMask|KeyPressMask|KeyReleaseMask|KeymapStateMask|VisibilityChangeMask)

// Additional events for shell widget events
#define SHELL_EVENT_MASK   (FocusChangeMask|StructureNotifyMask)

// Additional events for enabled widgets
#define ENABLED_EVENT_MASK (ButtonPressMask|ButtonReleaseMask|PointerMotionMask)

// These events are grabbed for mouse grabs
#define GRAB_EVENT_MASK    (ButtonPressMask|ButtonReleaseMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask)

// Do not propagate mask
#define NOT_PROPAGATE_MASK (KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|ButtonMotionMask)

#endif


// Side layout modes
#define LAYOUT_SIDE_MASK (LAYOUT_SIDE_LEFT|LAYOUT_SIDE_RIGHT|LAYOUT_SIDE_TOP|LAYOUT_SIDE_BOTTOM)


// Layout modes
#define LAYOUT_MASK (LAYOUT_SIDE_MASK|LAYOUT_RIGHT|LAYOUT_CENTER_X|LAYOUT_BOTTOM|LAYOUT_CENTER_Y|LAYOUT_FIX_X|LAYOUT_FIX_Y|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_DOCK_SAME|LAYOUT_DOCK_NEXT)


using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXWindow) FXWindowMap[]={
  FXMAPFUNC(SEL_UPDATE,0,FXWindow::onUpdate),
  FXMAPFUNC(SEL_PAINT,0,FXWindow::onPaint),
  FXMAPFUNC(SEL_MOTION,0,FXWindow::onMotion),
  FXMAPFUNC(SEL_CONFIGURE,0,FXWindow::onConfigure),
  FXMAPFUNC(SEL_MOUSEWHEEL,0,FXWindow::onMouseWheel),
  FXMAPFUNC(SEL_MAP,0,FXWindow::onMap),
  FXMAPFUNC(SEL_UNMAP,0,FXWindow::onUnmap),
  FXMAPFUNC(SEL_DRAGGED,0,FXWindow::onDragged),
  FXMAPFUNC(SEL_ENTER,0,FXWindow::onEnter),
  FXMAPFUNC(SEL_LEAVE,0,FXWindow::onLeave),
  FXMAPFUNC(SEL_DESTROY,0,FXWindow::onDestroy),
  FXMAPFUNC(SEL_FOCUSIN,0,FXWindow::onFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT,0,FXWindow::onFocusOut),
  FXMAPFUNC(SEL_SELECTION_LOST,0,FXWindow::onSelectionLost),
  FXMAPFUNC(SEL_SELECTION_GAINED,0,FXWindow::onSelectionGained),
  FXMAPFUNC(SEL_SELECTION_REQUEST,0,FXWindow::onSelectionRequest),
  FXMAPFUNC(SEL_CLIPBOARD_LOST,0,FXWindow::onClipboardLost),
  FXMAPFUNC(SEL_CLIPBOARD_GAINED,0,FXWindow::onClipboardGained),
  FXMAPFUNC(SEL_CLIPBOARD_REQUEST,0,FXWindow::onClipboardRequest),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXWindow::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXWindow::onLeftBtnRelease),
  FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,0,FXWindow::onMiddleBtnPress),
  FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE,0,FXWindow::onMiddleBtnRelease),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,0,FXWindow::onRightBtnPress),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,0,FXWindow::onRightBtnRelease),
  FXMAPFUNC(SEL_SPACEBALLMOTION,0,FXWindow::onSpaceBallMotion),
  FXMAPFUNC(SEL_SPACEBALLBUTTONPRESS,0,FXWindow::onSpaceBallButtonPress),
  FXMAPFUNC(SEL_SPACEBALLBUTTONRELEASE,0,FXWindow::onSpaceBallButtonRelease),
  FXMAPFUNC(SEL_UNGRABBED,0,FXWindow::onUngrabbed),
  FXMAPFUNC(SEL_KEYPRESS,0,FXWindow::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXWindow::onKeyRelease),
  FXMAPFUNC(SEL_DND_ENTER,0,FXWindow::onDNDEnter),
  FXMAPFUNC(SEL_DND_LEAVE,0,FXWindow::onDNDLeave),
  FXMAPFUNC(SEL_DND_DROP,0,FXWindow::onDNDDrop),
  FXMAPFUNC(SEL_DND_MOTION,0,FXWindow::onDNDMotion),
  FXMAPFUNC(SEL_DND_REQUEST,0,FXWindow::onDNDRequest),
  FXMAPFUNC(SEL_FOCUS_SELF,0,FXWindow::onFocusSelf),
  FXMAPFUNC(SEL_BEGINDRAG,0,FXWindow::onBeginDrag),
  FXMAPFUNC(SEL_ENDDRAG,0,FXWindow::onEndDrag),
  FXMAPFUNC(SEL_QUERY_TIP,0,FXWindow::onQueryTip),
  FXMAPFUNC(SEL_QUERY_HELP,0,FXWindow::onQueryHelp),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SHOW,FXWindow::onCmdShow),
  FXMAPFUNC(SEL_CHORE,FXWindow::ID_SHOW,FXWindow::onCmdShow),
  FXMAPFUNC(SEL_TIMEOUT,FXWindow::ID_SHOW,FXWindow::onCmdShow),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_HIDE,FXWindow::onCmdHide),
  FXMAPFUNC(SEL_CHORE,FXWindow::ID_HIDE,FXWindow::onCmdHide),
  FXMAPFUNC(SEL_TIMEOUT,FXWindow::ID_HIDE,FXWindow::onCmdHide),
  FXMAPFUNC(SEL_UPDATE,FXWindow::ID_TOGGLESHOWN,FXWindow::onUpdToggleShown),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_TOGGLESHOWN,FXWindow::onCmdToggleShown),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_RAISE,FXWindow::onCmdRaise),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_LOWER,FXWindow::onCmdLower),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_ENABLE,FXWindow::onCmdEnable),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_DISABLE,FXWindow::onCmdDisable),
  FXMAPFUNC(SEL_UPDATE,FXWindow::ID_TOGGLEENABLED,FXWindow::onUpdToggleEnabled),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_TOGGLEENABLED,FXWindow::onCmdToggleEnabled),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_UPDATE,FXWindow::onCmdUpdate),
  FXMAPFUNC(SEL_UPDATE,FXWindow::ID_DELETE,FXWindow::onUpdYes),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_DELETE,FXWindow::onCmdDelete),
  FXMAPFUNC(SEL_CHORE,FXWindow::ID_DELETE,FXWindow::onCmdDelete),
  FXMAPFUNC(SEL_TIMEOUT,FXWindow::ID_DELETE,FXWindow::onCmdDelete),
  };


// Object implementation
FXIMPLEMENT(FXWindow,FXDrawable,FXWindowMap,ARRAYNUMBER(FXWindowMap))


/*******************************************************************************/


// Drag type names
const FXchar FXWindow::octetTypeName[]="application/octet-stream";
const FXchar FXWindow::deleteTypeName[]="DELETE";
const FXchar FXWindow::textTypeName[]="text/plain";
const FXchar FXWindow::colorTypeName[]="application/x-color";
const FXchar FXWindow::urilistTypeName[]="text/uri-list";
const FXchar FXWindow::utf8TypeName[]="UTF8_STRING";
const FXchar FXWindow::utf16TypeName[]="text/utf16";
const FXchar FXWindow::actionTypeName[]="application/x-kde-cutselection";


// Drag type atoms; first widget to need it should register the type
FXDragType FXWindow::octetType=0;
FXDragType FXWindow::deleteType=0;
FXDragType FXWindow::textType=0;
FXDragType FXWindow::colorType=0;
FXDragType FXWindow::urilistType=0;
FXDragType FXWindow::utf8Type=0;
FXDragType FXWindow::actionType=0;


// The string type is predefined and hardwired
#ifdef WIN32
const FXDragType FXWindow::stringType=CF_TEXT;
#else
const FXDragType FXWindow::stringType=XA_STRING;
#endif


// The image type is predefined and hardwired
#ifdef WIN32
const FXDragType FXWindow::imageType=CF_DIB;
#else
const FXDragType FXWindow::imageType=XA_PIXMAP;
#endif


// The UTF-16 string type is predefined and hardwired
#ifdef WIN32
FXDragType FXWindow::utf16Type=CF_UNICODETEXT;
#else
FXDragType FXWindow::utf16Type=0;
#endif


/*******************************************************************************/


// For deserialization; note that the deserialized window only
// becomes part of the widget tree after its loaded with load().
FXWindow::FXWindow(){
  FXTRACE((TOPIC_CONSTRUCT,"FXWindow::FXWindow %p\n",this));
  parent=nullptr;
  owner=nullptr;
  first=nullptr;
  last=nullptr;
  next=nullptr;
  prev=nullptr;
  focus=nullptr;
  composeContext=nullptr;
  defaultCursor=nullptr;
  dragCursor=nullptr;
  accelTable=nullptr;
  target=nullptr;
  message=0;
  xpos=0;
  ypos=0;
  backColor=0;
  flags=FLAG_DIRTY|FLAG_UPDATE|FLAG_RECALC|FLAG_CURSOR;
  options=0;
  wk=0;
  }


// Only used for the root window
FXWindow::FXWindow(FXApp* a,FXVisual *vis):FXDrawable(a,1,1){
  FXTRACE((TOPIC_CONSTRUCT,"FXWindow::FXWindow %p\n",this));
  getApp()->windowCount++;
  visual=vis;
  parent=nullptr;
  owner=nullptr;
  first=last=nullptr;
  next=prev=nullptr;
  focus=nullptr;
  composeContext=nullptr;
  defaultCursor=getApp()->getDefaultCursor(DEF_ARROW_CURSOR);
  dragCursor=getApp()->getDefaultCursor(DEF_ARROW_CURSOR);
  accelTable=nullptr;
  target=nullptr;
  message=0;
  xpos=0;
  ypos=0;
  backColor=0;
  flags=FLAG_DIRTY|FLAG_SHOWN|FLAG_UPDATE|FLAG_RECALC|FLAG_CURSOR;
  options=LAYOUT_FIX_X|LAYOUT_FIX_Y|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT;
  wk=1;
  }


// This constructor is used for shell windows
FXWindow::FXWindow(FXApp* a,FXWindow* own,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXDrawable(a,w,h){
  FXTRACE((TOPIC_CONSTRUCT,"FXWindow::FXWindow %p\n",this));
  getApp()->windowCount++;
  parent=a->root;
  owner=own;
  visual=getApp()->getDefaultVisual();
  first=last=nullptr;
  prev=parent->last;
  next=nullptr;
  parent->last=this;
  if(prev){
    wk=prev->wk+1;
    prev->next=this;
    }
  else{
    wk=1;
    parent->first=this;
    }
  focus=nullptr;
  composeContext=nullptr;
  defaultCursor=getApp()->getDefaultCursor(DEF_ARROW_CURSOR);
  dragCursor=getApp()->getDefaultCursor(DEF_ARROW_CURSOR);
  accelTable=nullptr;
  target=nullptr;
  message=0;
  xpos=x;
  ypos=y;
  backColor=getApp()->getBaseColor();
  flags=FLAG_DIRTY|FLAG_UPDATE|FLAG_RECALC|FLAG_SHELL|FLAG_CURSOR;
  options=opts;
  }


// This constructor is used for all child windows
FXWindow::FXWindow(FXComposite* p,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXDrawable(p->getApp(),w,h){
  FXTRACE((TOPIC_CONSTRUCT,"FXWindow::FXWindow %p\n",this));
  getApp()->windowCount++;
  parent=p;
  owner=parent;
  visual=parent->getVisual();
  first=last=nullptr;
  prev=parent->last;
  next=nullptr;
  parent->last=this;
  if(prev){
    wk=prev->wk+1;
    prev->next=this;
    }
  else{
    wk=1;
    parent->first=this;
    }
  focus=nullptr;
  composeContext=nullptr;
  defaultCursor=getApp()->getDefaultCursor(DEF_ARROW_CURSOR);
  dragCursor=getApp()->getDefaultCursor(DEF_ARROW_CURSOR);
  accelTable=nullptr;
  target=nullptr;
  message=0;
  xpos=x;
  ypos=y;
  backColor=getApp()->getBaseColor();
  flags=FLAG_DIRTY|FLAG_UPDATE|FLAG_RECALC|FLAG_CURSOR;
  options=opts;
  }


/*******************************************************************************/

// The idea is to recurse down, but not sideways or upward.  Recursing upward
// through the parent or owner, or sideways through the left and right sibling,
// carries the risk of serializing the entire widget tree instead of just the
// one widget and its children.
// Each widget only saves its own data and resources, and loops over its child
// widgets saving each one in turn. A final NULL child pointer is saved to
// indicate the end of the sequence.
// When loading back, we deserialize a child pointer and hang it in the widget
// tree under this widget.  We continue to deserialize child pointers until
// we find the NULL child pointer.


// Save window to stream
void FXWindow::save(FXStream& store) const {
  FXWindow *child=getFirst();
  FXDrawable::save(store);
  while(child){
    store << child;
    child=child->getNext();
    }
  store << child;
  store << accelTable;
  store << defaultCursor;
  store << dragCursor;
  store << target;
  store << message;
  store << xpos;
  store << ypos;
  store << backColor;
  store << tag;
  store << options;
  store << flags;
  store << wk;
  }


// Load window from stream
void FXWindow::load(FXStream& store){
  FXWindow *child=nullptr;
  FXDrawable::load(store);
  getApp()->windowCount++;
  store >> child;
  while(child){
    child->parent=this;
    child->owner=this;
    child->prev=last;
    child->next=nullptr;
    if(last){
      last->next=child;
      }
    else{
      first=child;
      }
    last=child;
    store >> child;
    }
  store >> accelTable;
  store >> defaultCursor;
  store >> dragCursor;
  store >> target;
  store >> message;
  store >> xpos;
  store >> ypos;
  store >> backColor;
  store >> tag;
  store >> options;
  store >> flags;       // FIXME
  store >> wk;
  }


// Get window class; most windows have normal class
FXWindow::WindowClass FXWindow::getWindowClass() const {
  return ClassNormal;
  }


/*******************************************************************************/


// Return a pointer to the shell window
FXWindow* FXWindow::getShell() const {
  FXWindow *win=const_cast<FXWindow*>(this);
  FXWindow *p;
  while((p=win->parent)!=nullptr && p->parent) win=p;
  return win;
  }


// Return a pointer to the root window
FXWindow* FXWindow::getRoot() const {
  FXWindow *win=const_cast<FXWindow*>(this);
  while(win->parent) win=win->parent;
  return win;
  }


// Test if logically inside
FXbool FXWindow::contains(FXint parentx,FXint parenty) const {
  return xpos<=parentx && parentx<xpos+width && ypos<=parenty && parenty<ypos+height;
  }


// Return child window with given window key
FXWindow* FXWindow::getChildWithKey(FXuint k) const {
  for(FXWindow *child=first; child; child=child->next){
    if(child->getKey()==k) return child;
    }
  return nullptr;
  }

/*
  /// Get window from Dewey decimal key
  FXWindow *getChildFromKey(const FXString& key) const;

  /// Get Dewey decimal key from window
  FXString getKeyFromChild(FXWindow* window) const;

// Get window from Dewey decimal key
FXWindow *FXWindow::getChildFromKey(const FXString& key) const {
  FXWindow *window=(FXWindow*)this;
  const FXchar *s=key.text();
  FXuint num;
  do{
    if(!('0'<=*s && *s<='9')) return nullptr;
    for(num=0; '0'<=*s && *s<='9'; s++){ num=10*num+*s-'0'; }
    for(window=window->first; window && window->wk!=num; window=window->next);
    }
  while(*s++ == '.' && window);
  return window;
  }


// Get Dewey decimal key from window
FXString FXWindow::getKeyFromChild(FXWindow* window) const {
  FXchar buf[1024];
  FXchar *p=buf+1023;
  FXuint num;
  *p='\0';
  while(window && window!=this && buf+10<=p){
    num=window->getKey();
    do{ *--p=num%10+'0'; num/=10; }while(num);
    window=window->getParent();
    *--p='.';
    }
  if(*p=='.') p++;
  return p;
  }
*/


// Return true if this window contains child in its subtree
FXbool FXWindow::containsChild(const FXWindow* child) const {
  while(child){
    if(child==this) return true;
    child=child->parent;
    }
  return false;
  }


// Return true if specified window is owned by this window
FXbool FXWindow::isOwnerOf(const FXWindow* window) const {
  while(window){
    if(window==this) return true;
    window=window->owner;
    }
  return false;
  }


// Return true if specified window is ancestor of this window
FXbool FXWindow::isChildOf(const FXWindow* window) const {
  const FXWindow* child=this;
  while(child){
    child=child->parent;
    if(child==window) return true;
    }
  return false;
  }


// Get child at x,y
FXWindow* FXWindow::getChildAt(FXint x,FXint y) const {
  FXWindow *child;
  if(0<=x && 0<=y && x<width && y<height){
    for(child=last; child; child=child->prev){
      if(child->shown() && child->xpos<=x && child->ypos<=y && x<child->xpos+child->width && y<child->ypos+child->height) return child;
      }
    }
  return nullptr;
  }


// Count number of children
FXint FXWindow::numChildren() const {
  const FXWindow *child=first;
  FXint num=0;
  while(child){
    child=child->next;
    num++;
    }
  return num;
  }


// Get index of child window
FXint FXWindow::indexOfChild(const FXWindow *window) const {
  FXint index=0;
  if(!window || window->parent!=this) return -1;
  while(window->prev){
    window=window->prev;
    index++;
    }
  return index;
  }


// Get child window at index
FXWindow* FXWindow::childAtIndex(FXint index) const {
  FXWindow* child=first;
  if(index<0) return nullptr;
  while(index && child){
    child=child->next;
    index--;
    }
  return child;
  }


// Find common ancestor between window a and b
FXWindow* FXWindow::commonAncestor(FXWindow* a,FXWindow* b){
  FXWindow *p1,*p2;
  if(a || b){
    if(!a) return b->getRoot();
    if(!b) return a->getRoot();
    p1=a;
    while(p1){
      p2=b;
      while(p2){
        if(p2==p1) return p1;
        p2=p2->parent;
        }
      p1=p1->parent;
      }
    }
  return nullptr;
  }


// Return true if sibling a <= sibling b in list
FXbool FXWindow::before(const FXWindow *a,const FXWindow* b){
  while(a!=b && a) a=a->next;
  return a==b;
  }


// Return true if sibling a >= sibling b in list
FXbool FXWindow::after(const FXWindow *a,const FXWindow* b){
  while(a!=b && b) b=b->next;
  return a==b;
  }


// Return true if window is a shell window
FXbool FXWindow::isShell() const {
  return (flags&FLAG_SHELL)!=0;
  }


/*******************************************************************************/


// Handle repaint
long FXWindow::onPaint(FXObject*,FXSelector,void* ptr){
  FXDCWindow dc(this,(FXEvent*)ptr);
  dc.setForeground(backColor);
  dc.fillRectangle(0,0,width,height);
  return 1;
  }


// Window was mapped to screen
long FXWindow::onMap(FXObject*,FXSelector,void* ptr){
  FXTRACE((250,"%s::onMap %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_MAP,message),ptr);
  }


// Window was unmapped; the grab is lost
long FXWindow::onUnmap(FXObject*,FXSelector,void* ptr){
  FXTRACE((250,"%s::onUnmap %p\n",getClassName(),this));
  if(getApp()->mouseGrabWindow==this) getApp()->mouseGrabWindow=nullptr;
  if(getApp()->keyboardGrabWindow==this) getApp()->keyboardGrabWindow=nullptr;
  return target && target->tryHandle(this,FXSEL(SEL_UNMAP,message),ptr);
  }


// Handle configure notify
long FXWindow::onConfigure(FXObject*,FXSelector,void* ptr){
  FXTRACE((250,"%s::onConfigure %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_CONFIGURE,message),ptr);
  }


// The window was destroyed; the grab is lost
long FXWindow::onDestroy(FXObject*,FXSelector,void*){
  FXTRACE((250,"%s::onDestroy %p\n",getClassName(),this));
  getApp()->hash.remove((FXptr)xid);
  if(getApp()->mouseGrabWindow==this) getApp()->mouseGrabWindow=nullptr;
  if(getApp()->keyboardGrabWindow==this) getApp()->keyboardGrabWindow=nullptr;
  if(getApp()->cursorWindow==this) getApp()->cursorWindow=parent;
  if(getApp()->activeWindow==this) getApp()->activeWindow=nullptr;
  flags&=~FLAG_FOCUSED;
  xid=0;
  return 1;
  }


// Left button pressed
long FXWindow::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    }
  return 0;
  }


// Left button released
long FXWindow::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;
    }
  return 0;
  }


// Middle button released
long FXWindow::onMiddleBtnPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_MIDDLEBUTTONPRESS,message),ptr)) return 1;
    }
  return 0;
  }


// Middle button pressed
long FXWindow::onMiddleBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    if(target && target->tryHandle(this,FXSEL(SEL_MIDDLEBUTTONRELEASE,message),ptr)) return 1;
    }
  return 0;
  }


// Right button pressed
long FXWindow::onRightBtnPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONPRESS,message),ptr)) return 1;
    }
  return 0;
  }


// Right button released
long FXWindow::onRightBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    if(target && target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONRELEASE,message),ptr)) return 1;
    }
  return 0;
  }


// Mouse motion
long FXWindow::onMotion(FXObject*,FXSelector,void* ptr){
  return isEnabled() && target && target->tryHandle(this,FXSEL(SEL_MOTION,message),ptr);
  }


// Mouse wheel
long FXWindow::onMouseWheel(FXObject*,FXSelector,void* ptr){
  return isEnabled() && target && target->tryHandle(this,FXSEL(SEL_MOUSEWHEEL,message),ptr);
  }


// Keyboard press
long FXWindow::onKeyPress(FXObject*,FXSelector,void* ptr){
  FXTRACE((TOPIC_KEYBOARD,"%s::onKeyPress %p keysym=0x%04x state=%04x\n",getClassName(),this,((FXEvent*)ptr)->code,((FXEvent*)ptr)->state));
  return isEnabled() && target && target->tryHandle(this,FXSEL(SEL_KEYPRESS,message),ptr);
  }


// Keyboard release
long FXWindow::onKeyRelease(FXObject*,FXSelector,void* ptr){
  FXTRACE((TOPIC_KEYBOARD,"%s::onKeyRelease %p keysym=0x%04x state=%04x\n",getClassName(),this,((FXEvent*)ptr)->code,((FXEvent*)ptr)->state));
  return isEnabled() && target && target->tryHandle(this,FXSEL(SEL_KEYRELEASE,message),ptr);
  }


// Space ball motion
long FXWindow::onSpaceBallMotion(FXObject*,FXSelector,void* ptr){
  return isEnabled() && target && target->tryHandle(this,FXSEL(SEL_SPACEBALLMOTION,message),ptr);
  }


// Space ball button press
long FXWindow::onSpaceBallButtonPress(FXObject*,FXSelector,void* ptr){
  return isEnabled() && target && target->tryHandle(this,FXSEL(SEL_SPACEBALLBUTTONPRESS,message),ptr);
  }


// Space ball button release
long FXWindow::onSpaceBallButtonRelease(FXObject*,FXSelector,void* ptr){
  return isEnabled() && target && target->tryHandle(this,FXSEL(SEL_SPACEBALLBUTTONRELEASE,message),ptr);
  }


// Start a drag operation
long FXWindow::onBeginDrag(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_BEGINDRAG,message),ptr);
  }


// End drag operation
long FXWindow::onEndDrag(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_ENDDRAG,message),ptr);
  }


// Dragged stuff around
long FXWindow::onDragged(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_DRAGGED,message),ptr);
  }


// Entering window
long FXWindow::onEnter(FXObject*,FXSelector,void* ptr){
  FXTRACE((150,"%s::onEnter %p (%s)\n",getClassName(),this, (((FXEvent*)ptr)->code==CROSSINGNORMAL) ? "CROSSINGNORMAL" : (((FXEvent*)ptr)->code==CROSSINGGRAB) ? "CROSSINGGRAB" : (((FXEvent*)ptr)->code==CROSSINGUNGRAB)? "CROSSINGUNGRAB" : "?"));
  if(((FXEvent*)ptr)->code!=CROSSINGGRAB){
    if(!(((FXEvent*)ptr)->state&(SHIFTMASK|CONTROLMASK|METAMASK|LEFTBUTTONMASK|MIDDLEBUTTONMASK|RIGHTBUTTONMASK))) flags|=FLAG_TIP;
    flags|=FLAG_HELP;
    }
  if(isEnabled() && target){ target->tryHandle(this,FXSEL(SEL_ENTER,message),ptr); }
  return 1;
  }


// Leaving window
long FXWindow::onLeave(FXObject*,FXSelector,void* ptr){
  FXTRACE((150,"%s::onLeave %p (%s)\n",getClassName(),this, (((FXEvent*)ptr)->code==CROSSINGNORMAL) ? "CROSSINGNORMAL" : (((FXEvent*)ptr)->code==CROSSINGGRAB) ? "CROSSINGGRAB" : (((FXEvent*)ptr)->code==CROSSINGUNGRAB)? "CROSSINGUNGRAB" : "?"));
  if(((FXEvent*)ptr)->code!=CROSSINGUNGRAB){
    flags&=~(FLAG_TIP|FLAG_HELP);
    }
  if(isEnabled() && target){ target->tryHandle(this,FXSEL(SEL_LEAVE,message),ptr); }
  return 1;
  }


// We were asked about tip text
long FXWindow::onQueryTip(FXObject* sender,FXSelector,void* ptr){
  return (flags&FLAG_TIP) && target && target->tryHandle(sender,FXSEL(SEL_QUERY_TIP,message),ptr);
  }


// We were asked about status text
long FXWindow::onQueryHelp(FXObject* sender,FXSelector,void* ptr){
  return (flags&FLAG_HELP) && target && target->tryHandle(sender,FXSEL(SEL_QUERY_HELP,message),ptr);
  }


// True if window under the cursor
FXbool FXWindow::underCursor() const {
  return (getApp()->cursorWindow==this);
  }


// Handle drag-and-drop enter
long FXWindow::onDNDEnter(FXObject*,FXSelector,void* ptr){
  FXTRACE((150,"%s::onDNDEnter %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_DND_ENTER,message),ptr);
  }


// Handle drag-and-drop leave
long FXWindow::onDNDLeave(FXObject*,FXSelector,void* ptr){
  FXTRACE((150,"%s::onDNDLeave %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_DND_LEAVE,message),ptr);
  }


// Handle drag-and-drop motion
long FXWindow::onDNDMotion(FXObject*,FXSelector,void* ptr){
  FXTRACE((150,"%s::onDNDMotion %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_DND_MOTION,message),ptr);
  }


// Handle drag-and-drop drop
long FXWindow::onDNDDrop(FXObject*,FXSelector,void* ptr){
  FXTRACE((150,"%s::onDNDDrop %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_DND_DROP,message),ptr);
  }


// Request for DND data
long FXWindow::onDNDRequest(FXObject*,FXSelector,void* ptr){
  FXTRACE((150,"%s::onDNDRequest %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_DND_REQUEST,message),ptr);
  }


// Show window
long FXWindow::onCmdShow(FXObject*,FXSelector,void*){
  if(!shown()){ show(); recalc(); }
  return 1;
  }


// Hide window
long FXWindow::onCmdHide(FXObject*,FXSelector,void*){
  if(shown()){ hide(); recalc(); }
  return 1;
  }

// Hide or show window
long FXWindow::onCmdToggleShown(FXObject*,FXSelector,void*){
  shown() ? hide() : show();
  recalc();
  return 1;
  }


// Update hide or show window
long FXWindow::onUpdToggleShown(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SHOW),nullptr);
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SETVALUE),(void*)(FXuval)shown());
  return 1;
  }


// Raise window
long FXWindow::onCmdRaise(FXObject*,FXSelector,void*){
  raise();
  return 1;
  }


// Lower window
long FXWindow::onCmdLower(FXObject*,FXSelector,void*){
  lower();
  return 1;
  }


// Delete window
long FXWindow::onCmdDelete(FXObject*,FXSelector,void*){
  delete this;
  return 1;
  }


// Enable window
long FXWindow::onCmdEnable(FXObject*,FXSelector,void*){
  enable();
  return 1;
  }


// Disable window
long FXWindow::onCmdDisable(FXObject*,FXSelector,void*){
  disable();
  return 1;
  }


// Disable or enable window
long FXWindow::onCmdToggleEnabled(FXObject*,FXSelector,void*){
  isEnabled() ? disable() : enable();
  return 1;
  }


// Update disable or enable window
long FXWindow::onUpdToggleEnabled(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SHOW),nullptr);
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SETVALUE),(void*)(FXuval)isEnabled());
  return 1;
  }


// In combination with autograying/autohiding widgets,
// this could be used to ungray or show when you don't want
// to write a whole handler routine just to do this.
long FXWindow::onUpdYes(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SHOW),nullptr);
  return 1;
  }


// Update (repaint) window
long FXWindow::onCmdUpdate(FXObject*,FXSelector,void*){
  update();
  return 1;
  }


/*******************************************************************************/

// Gained focus
long FXWindow::onFocusIn(FXObject*,FXSelector,void* ptr){
  FXTRACE((150,"%s::onFocusIn %p\n",getClassName(),this));
  flags|=FLAG_FOCUSED;
  if(target){ target->tryHandle(this,FXSEL(SEL_FOCUSIN,message),ptr); }
  if(composeContext){ composeContext->focusIn(); }
  if(focus){ focus->handle(focus,FXSEL(SEL_FOCUSIN,0),nullptr); }
  return 1;
  }


// Lost focus
long FXWindow::onFocusOut(FXObject*,FXSelector,void* ptr){
  FXTRACE((150,"%s::onFocusOut %p\n",getClassName(),this));
  if(focus){ focus->handle(focus,FXSEL(SEL_FOCUSOUT,0),nullptr); }
  if(composeContext){ composeContext->focusOut(); }
  if(target){ target->tryHandle(this,FXSEL(SEL_FOCUSOUT,message),ptr); }
  flags&=~FLAG_FOCUSED;
  return 1;
  }


// Focus on widget itself, if its enabled
long FXWindow::onFocusSelf(FXObject*,FXSelector,void*){
  FXTRACE((150,"%s::onFocusSelf %p\n",getClassName(),this));
  if(isEnabled() && canFocus()){ setFocus(); return 1; }
  return 0;
  }


// Create compose context
void FXWindow::createComposeContext(){
  if(!composeContext){
    composeContext=new FXComposeContext(getApp(),this,0);
    composeContext->create();
    }
  }


// Destroy compose context
void FXWindow::destroyComposeContext(){
  if(composeContext){
    delete composeContext;
    composeContext=nullptr;
    }
  }


// If window can have focus
FXbool FXWindow::canFocus() const { return false; }


// Has window the focus
FXbool FXWindow::hasFocus() const {
  return (flags&FLAG_FOCUSED)!=0;
  }


// Window is in focus chain
FXbool FXWindow::inFocusChain() const {
  return parent->focus==this;
  }


// Set focus to this widget.
// The chain of focus from shell down to a control is changed.
// Widgets now in the chain may or may not gain real focus,
// depending on whether parent window already had a real focus!
// Setting the focus to a composite will cause descendants to loose it.
void FXWindow::setFocus(){
  FXTRACE((140,"%s::setFocus %p\n",getClassName(),this));
  if(parent && parent->focus!=this){
    if(parent->focus) parent->focus->killFocus(); else parent->setFocus();
    parent->focus=this;
    if(parent->hasFocus()) handle(this,FXSEL(SEL_FOCUSIN,0),nullptr);
    }
  flags|=FLAG_HELP;
  }


// Kill focus to this widget.
void FXWindow::killFocus(){
  FXTRACE((140,"%s::killFocus %p\n",getClassName(),this));
  if(parent && parent->focus==this){
    if(focus) focus->killFocus();
    if(hasFocus()) handle(this,FXSEL(SEL_FOCUSOUT,0),nullptr);
    parent->focus=nullptr;
    }
  flags&=~FLAG_HELP;
  }


// Return true if widget is drawn as default
FXbool FXWindow::isDefault() const {
  return (flags&FLAG_DEFAULT)!=0;
  }


// Make widget drawn as default
void FXWindow::setDefault(FXuchar flag){
  FXWindow *win;
  switch(flag){
    case false:
      flags&=~FLAG_DEFAULT;
      break;
    case true:
      if(!(flags&FLAG_DEFAULT)){
        win=getShell()->findDefault();
        if(win) win->setDefault(false);
        flags|=FLAG_DEFAULT;
        }
      break;
    case maybe:
      if(flags&FLAG_DEFAULT){
        flags&=~FLAG_DEFAULT;
        win=getShell()->findInitial();
        if(win) win->setDefault(true);
        }
      break;
    }
  }

// Find default window
FXWindow* FXWindow::findDefault() const {
  FXWindow *win=const_cast<FXWindow*>(this);
  while(1){
    if(win->isDefault()) return win;
    if(win->getFirst()){ win=win->getFirst(); continue; }
nxt:if(win==this) break;
    if(win->getNext()){ win=win->getNext(); continue; }
    win=win->getParent();
    goto nxt;
    }
  return nullptr;
  }



// Return true if this is the initial default window
FXbool FXWindow::isInitial() const {
  return (flags&FLAG_INITIAL)!=0;
  }


// Make this window the initial default window
void FXWindow::setInitial(FXbool flag){
  FXWindow *win;
  if((flags&FLAG_INITIAL) && !flag){
    flags&=~FLAG_INITIAL;
    }
  if(!(flags&FLAG_INITIAL) && flag){
    win=getShell()->findInitial();
    if(win) win->setInitial(false);
    flags|=FLAG_INITIAL;
    }
  }


// Find initial window
FXWindow* FXWindow::findInitial() const {
  FXWindow *win=const_cast<FXWindow*>(this);
  while(1){
    if(win->isInitial()) return win;
    if(win->getFirst()){ win=win->getFirst(); continue; }
nxt:if(win==this) break;
    if(win->getNext()){ win=win->getNext(); continue; }
    win=win->getParent();
    goto nxt;
    }
  return nullptr;
  }

/*******************************************************************************/


#ifndef WIN32

// Add this window to the list of colormap windows
void FXWindow::addColormapWindows(){
  Window windows[2],*windowsReturn,*windowList;
  int countReturn,i;

  // Check to see if there is already a property
  Status status=XGetWMColormapWindows((Display*)getApp()->getDisplay(),getShell()->id(),&windowsReturn,&countReturn);

  // If no property, just create one
  if(!status){
    windows[0]=id();
    windows[1]=getShell()->id();
    XSetWMColormapWindows((Display*)getApp()->getDisplay(),getShell()->id(),windows,2);
    }

  // There was a property, add myself to the beginning
  else{
    windowList=(Window*)malloc((sizeof(Window))*(countReturn+1));
    windowList[0]=id();
    for(i=0; i<countReturn; i++) windowList[i+1]=windowsReturn[i];
    XSetWMColormapWindows((Display*)getApp()->getDisplay(),getShell()->id(),windowList,countReturn+1);
    XFree((char*)windowsReturn);
    free(windowList);
    }
  }


// Remove it from colormap windows
void FXWindow::remColormapWindows(){
  Window *windowsReturn;
  int countReturn,i;
  Status status=XGetWMColormapWindows((Display*)getApp()->getDisplay(),getShell()->id(),&windowsReturn,&countReturn);
  if(status){
    for(i=0; i<countReturn; i++){
      if(windowsReturn[i]==id()){
        for(i++; i<countReturn; i++) windowsReturn[i-1]=windowsReturn[i];
        XSetWMColormapWindows((Display*)getApp()->getDisplay(),getShell()->id(),windowsReturn,countReturn-1);
        break;
        }
      }
    XFree((char*)windowsReturn);
    }
  }

#endif


/*******************************************************************************/

// Create X window
void FXWindow::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE((TOPIC_CREATION,"%s::create %p\n",getClassName(),this));

      // Gotta have a parent already created!
      if(!parent->id()){ fxerror("%s::create: trying to create window before creating parent window.\n",getClassName()); }

      // If window has owner, owner should have been created already
      if(owner && !owner->id()){ fxerror("%s::create: trying to create window before creating owner window.\n",getClassName()); }

      // Got to have a visual
      if(!visual){ fxerror("%s::create: trying to create window without a visual.\n",getClassName()); }

      // Initialize visual
      visual->create();

      // Create default cursor
      if(defaultCursor) defaultCursor->create();

      // Create drag cursor
      if(dragCursor) dragCursor->create();

#ifdef WIN32
      // Most windows use these style bits
      DWORD dwStyle=WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
      DWORD dwExStyle=WS_EX_NOPARENTNOTIFY;
      HWND hParent=(HWND)parent->id();
      if(isShell()){
        if(isMemberOf(FXMETACLASS(FXTopWindow))){
          dwStyle=WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
          }
        else if(doesOverrideRedirect()){
          // To control window placement or control decoration, a window manager
          // often needs to redirect map or configure requests. Popup windows, however,
          // often need to be mapped without a window manager getting in the way.
          dwStyle=WS_POPUP|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
          dwExStyle=WS_EX_TOOLWINDOW;
          }
        else{
          // Other top-level shell windows (like dialogs)
          dwStyle=WS_POPUP|WS_CAPTION|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
          dwExStyle=WS_EX_DLGMODALFRAME|WS_EX_TOOLWINDOW;
          }
        if(owner) hParent=(HWND)owner->id();
        }

      FXASSERT_STATIC(sizeof(FXID)>=sizeof(HWND));

      // Create this window
      xid=CreateWindowEx(dwExStyle,(TCHAR*)GetClass(),nullptr,dwStyle,xpos,ypos,FXMAX(width,1),FXMAX(height,1),hParent,nullptr,(HINSTANCE)getApp()->getDisplay(),this);

      // Uh-oh, we failed
      if(!xid){ throw FXWindowException("unable to create window."); }

      // Store for xid to C++ object mapping
      getApp()->hash.insert((FXptr)xid,this);

      // We put the XdndAware property on all toplevel windows, so that
      // when dragging, we need to search no further than the toplevel window.
      if(isShell()){
        HANDLE propdata=(HANDLE)XDND_PROTOCOL_VERSION;
        SetProp((HWND)xid,(LPCTSTR)MAKELONG(getApp()->xdndAware,0),propdata);
        }

      // To keep it on top
      //SetWindowPos((HWND)xid,HWND_TOPMOST,xpos,ypos,FXMAX(width,1),FXMAX(height,1),0);

      // Disable if not enabled (FIXME this does not work)
      if(!isEnabled()){
 //       EnableWindow((HWND)xid,false);
        }

      // Show if it was supposed to be.  Apparently, initial state
      // is neither shown nor hidden, so an explicit hide is needed.
      // Patch thanks to "Glenn Shen" <shen@hks.com>
      if(shown())
        ShowWindow((HWND)xid,SW_SHOWNOACTIVATE);
      else
        ShowWindow((HWND)xid,SW_HIDE);

#else                   // X11

      XSetWindowAttributes wattr;
      unsigned long mask=(CWBackPixmap|CWWinGravity|CWBitGravity|CWBorderPixel|CWEventMask|CWDontPropagate|CWOverrideRedirect|CWSaveUnder|CWColormap);

      // Events for normal windows
      wattr.event_mask=BASIC_EVENT_MASK;

      // Events for shell windows
      if(isShell()) wattr.event_mask|=SHELL_EVENT_MASK;

      // If enabled, turn on some more events
      if(isEnabled()) wattr.event_mask|=ENABLED_EVENT_MASK;

      // For input methods
//#ifndef NO_XIM
//      if(getApp()->xic){
//        long filterevents=0;
//        XGetICValues((XIC)getApp()->xic,XNFilterEvents,&filterevents,nullptr);
//        wattr.event_mask|=filterevents;
//        }
//#endif

      // FOX will not propagate events to ancestor windows
      wattr.do_not_propagate_mask=NOT_PROPAGATE_MASK;

      // Obtain colormap
      wattr.colormap=visual->colormap;

      // This is needed for OpenGL
      wattr.border_pixmap=None;
      wattr.border_pixel=0;

      // Background
      //wattr.background_pixmap=ParentRelative;
      wattr.background_pixmap=None;
      wattr.background_pixel=0;

      // Preserving content during resize will be faster:- not turned
      // on yet as we will have to recode all widgets to decide when to
      // repaint or not to repaint the display when resized...
      wattr.bit_gravity=NorthWestGravity;
      wattr.bit_gravity=ForgetGravity;

      // The window gravity is NorthWestGravity, which means
      // if a child keeps same position relative to top/left
      // of its parent window, nothing extra work is incurred.
      wattr.win_gravity=NorthWestGravity;

      // Determine override redirect
      wattr.override_redirect=doesOverrideRedirect();

      // Backing store/planes/pixel not passed
      wattr.backing_store=0;
      wattr.backing_planes=0;
      wattr.backing_pixel=0;

      // Determine save-unders
      wattr.save_under=doesSaveUnder();

      // Set cursor
      if(defaultCursor){
        wattr.cursor=defaultCursor->id();
        mask|=CWCursor;
        }

      FXASSERT_STATIC(sizeof(FXID)>=sizeof(Window));

      // Finally, create the window
      xid=XCreateWindow((Display*)getApp()->getDisplay(),parent->id(),xpos,ypos,FXMAX(width,1),FXMAX(height,1),0,visual->depth,InputOutput,(Visual*)visual->visual,mask,&wattr);

      // Uh-oh, we failed
      if(!xid){ throw FXWindowException("unable to create window."); }

      // Store for xid to C++ object mapping
      getApp()->hash.insert((FXptr)xid,this);

      // We put the XdndAware property on all toplevel windows, so that
      // when dragging, we need to search no further than the toplevel window.
      if(isShell()){
        Atom propdata=(Atom)XDND_PROTOCOL_VERSION;
        XChangeProperty((Display*)getApp()->getDisplay(),xid,getApp()->xdndAware,XA_ATOM,32,PropModeReplace,(unsigned char*)&propdata,1);
        }

      // If window is a shell and it has an owner, make it stay on top of the owner
      if(isShell() && owner){
        XSetTransientForHint((Display*)getApp()->getDisplay(),xid,owner->getShell()->id());
        }

      // If colormap different, set WM_COLORMAP_WINDOWS property properly
      if(visual->colormap!=DefaultColormap((Display*)getApp()->getDisplay(),DefaultScreen((Display*)getApp()->getDisplay()))){
        addColormapWindows();
        }

      // Show if it was supposed to be
      if(shown() && 0<width && 0<height) XMapWindow((Display*)getApp()->getDisplay(),xid);

#endif
      flags|=FLAG_OWNED;
      }
    }
  }


// Attach to a window belonging to another application
void FXWindow::attach(FXID w){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE((TOPIC_CREATION,"%s::attach %p\n",getClassName(),this));

      // Gotta have a parent already created!
      if(!parent->id()){ fxerror("%s::attach: trying to attach window before creating parent window.\n",getClassName()); }

      // If window has owner, owner should have been created already
      if(owner && !owner->id()){ fxerror("%s::attach: trying to attach window before creating owner window.\n",getClassName()); }

      // Got to have a visual
      if(!visual){ fxerror("%s::attach: trying to attach window without a visual.\n",getClassName()); }

      // Uh-oh, we failed
      if(!w){ throw FXWindowException("unable to attach window."); }

      // Initialize visual
      visual->create();

      // Create default cursor
      if(defaultCursor) defaultCursor->create();

      // Create drag cursor
      if(dragCursor) dragCursor->create();

      // Simply assign to xid
      xid=w;

      // Store for xid to C++ object mapping
      getApp()->hash.insert((FXptr)xid,this);

#ifdef WIN32

      // Reparent under what WE think the parent is
      SetParent((HWND)xid,(HWND)parent->id());

#else

      XSetWindowAttributes wattr;
      unsigned long mask=(CWEventMask|CWDontPropagate);

      // Set event mask
      wattr.event_mask=BASIC_EVENT_MASK;
      if(isEnabled()) wattr.event_mask|=ENABLED_EVENT_MASK;

      // Events that should not propagate
      wattr.do_not_propagate_mask=NOT_PROPAGATE_MASK;

      // Set cursor
      if(defaultCursor){
        wattr.cursor=defaultCursor->id();
        mask|=CWCursor;
        }

      // Change window attributes
      XChangeWindowAttributes((Display*)getApp()->getDisplay(),xid,mask,&wattr);

      // Reparent window
      XReparentWindow((Display*)getApp()->getDisplay(),xid,parent->id(),0,0);
#endif
      flags&=~FLAG_OWNED;
      }
    }
  }


// Detach window
void FXWindow::detach(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE((TOPIC_CREATION,"%s::detach %p\n",getClassName(),this));

      // Remove from xid to C++ object mapping
      getApp()->hash.remove((FXptr)xid);

      // Detach visual
      visual->detach();

      // Detach default cursor
      if(defaultCursor) defaultCursor->detach();

      // Detach drag cursor
      if(dragCursor) dragCursor->detach();
      }

    // No longer grabbed
    if(getApp()->mouseGrabWindow==this) getApp()->mouseGrabWindow=nullptr;
    if(getApp()->keyboardGrabWindow==this) getApp()->keyboardGrabWindow=nullptr;
    if(getApp()->cursorWindow==this) getApp()->cursorWindow=parent;
    if(getApp()->activeWindow==this) getApp()->activeWindow=nullptr;
    flags&=~FLAG_FOCUSED;
    flags&=~FLAG_OWNED;
    xid=0;
    }
  }


// Destroy; only destroy window if we own it
void FXWindow::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE((TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this));

      // Remove from xid to C++ object mapping
      getApp()->hash.remove((FXptr)xid);

      // Its our own window, so destroy it
      if(flags&FLAG_OWNED){
#ifdef WIN32
        // Delete the XdndAware property
        if(isShell()){
          RemoveProp((HWND)xid,(LPCTSTR)MAKELONG(getApp()->xdndAware,0));
          }

        // Zap the window
        DestroyWindow((HWND)xid);
#else
        // If colormap different, set WM_COLORMAP_WINDOWS property properly
        if(visual->colormap!=DefaultColormap((Display*)getApp()->getDisplay(),DefaultScreen((Display*)getApp()->getDisplay()))){
          remColormapWindows();
          }

        // Delete the XdndAware property
        if(isShell()){
          XDeleteProperty((Display*)getApp()->getDisplay(),xid,getApp()->xdndAware);
          }

        // Delete the window
        XDestroyWindow((Display*)getApp()->getDisplay(),xid);
#endif
        }
      }

    // No longer grabbed
    if(getApp()->mouseGrabWindow==this) getApp()->mouseGrabWindow=nullptr;
    if(getApp()->keyboardGrabWindow==this) getApp()->keyboardGrabWindow=nullptr;
    if(getApp()->cursorWindow==this) getApp()->cursorWindow=parent;
    if(getApp()->activeWindow==this) getApp()->activeWindow=nullptr;
    flags&=~FLAG_FOCUSED;
    flags&=~FLAG_OWNED;
    xid=0;
    }
  }


#ifdef WIN32

// Get this window's device context
FXID FXWindow::GetDC() const {
  return ::GetDC((HWND)xid);
  }


// Release it
int FXWindow::ReleaseDC(FXID hdc) const {
  return ::ReleaseDC((HWND)xid,(HDC)hdc);
  }

// Window class
const void* FXWindow::GetClass() const { return TEXT("FXWindow"); }

#endif


/*******************************************************************************/


// Set window shape by means of region
void FXWindow::setShape(const FXRegion& region){
  if(xid){
#ifdef WIN32
    // Make a copy so as to leave original region intact
    HRGN rgn=CreateRectRgn(0,0,0,0);
    CombineRgn(rgn,(HRGN)region.region,rgn,RGN_COPY);
    SetWindowRgn((HWND)xid,rgn,false);
#else
#if defined(HAVE_XSHAPE_H)
    XShapeCombineRegion((Display*)getApp()->getDisplay(),xid,ShapeBounding,0,0,(Region)region.region,ShapeSet);
#endif
#endif
    }
  }


// Builds a region from a bitmap
#ifdef WIN32
static HRGN makeregion(HDC hdc,FXint w,FXint h){
  const COLORREF clear=RGB(255,255,255);
  HRGN shape,run;
  FXint x,y,z;
  shape=CreateRectRgn(0,0,0,0);
  for(y=0; y<h; y++){
    for(x=0; x<w; ){
      while(x<w && GetPixel(hdc,x,y)==clear) ++x;
      z=x;
      while(x<w && GetPixel(hdc,x,y)!=clear) ++x;
      if(z<x){
        run=CreateRectRgn(z,y,x,y+1);
        CombineRgn(shape,shape,run,RGN_OR);
        DeleteObject(run);
        }
      }
    }
  return shape;
  }
#endif


// Set window shape by means of bitmap
void FXWindow::setShape(FXBitmap* bitmap){
  if(!bitmap || !bitmap->id()){ fxerror("%s::setShape: illegal bitmap specified.\n",getClassName()); }
  if(xid){
#ifdef WIN32
    HDC hdc=CreateCompatibleDC(nullptr);
    SelectObject(hdc,bitmap->id());
    HRGN rgn=makeregion(hdc,bitmap->getWidth(),bitmap->getHeight());
    SetWindowRgn((HWND)xid,rgn,false);
    DeleteDC(hdc);
#else
#if defined(HAVE_XSHAPE_H)
    XShapeCombineMask((Display*)getApp()->getDisplay(),xid,ShapeBounding,0,0,bitmap->id(),ShapeSet);
#endif
#endif
    }
  }


// Set window shape by means of bitmap
void FXWindow::setShape(FXIcon* icon){
  if(!icon || !icon->shape){ fxerror("%s::setShape: illegal icon specified.\n",getClassName()); }
  if(xid){
#ifdef WIN32
    HDC hdc=CreateCompatibleDC(nullptr);
    SelectObject(hdc,icon->shape);
    HRGN rgn=makeregion(hdc,icon->getWidth(),icon->getHeight());
    SetWindowRgn((HWND)xid,rgn,false);
    DeleteDC(hdc);
#else
#if defined(HAVE_XSHAPE_H)
    XShapeCombineMask((Display*)getApp()->getDisplay(),xid,ShapeBounding,0,0,icon->shape,ShapeSet);
#endif
#endif
    }
  }


// Clear window shape to default rectangle
void FXWindow::clearShape(){
  if(xid){
#ifdef WIN32
    SetWindowRgn((HWND)xid,nullptr,false);
#else
#if defined(HAVE_XSHAPE_H)
    XShapeCombineMask((Display*)getApp()->getDisplay(),xid,ShapeBounding,0,0,None,ShapeSet);
#endif
#endif
    }
  }


/*******************************************************************************/


// Test if active
FXbool FXWindow::isActive() const {
  return (flags&FLAG_ACTIVE)!=0;
  }


// Get default width
FXint FXWindow::getDefaultWidth(){
  return 1;
  }


// Get default height
FXint FXWindow::getDefaultHeight(){
  return 1;
  }


// Return width for given height
FXint FXWindow::getWidthForHeight(FXint){
  return getDefaultWidth();
  }


// Return height for given width
FXint FXWindow::getHeightForWidth(FXint){
  return getDefaultHeight();
  }


// Set X position
void FXWindow::setX(FXint x){
  xpos=x;
  recalc();
  }


// Set Y position
void FXWindow::setY(FXint y){
  ypos=y;
  recalc();
  }


// Set width
void FXWindow::setWidth(FXint w){
  if(w<0) w=0;
  width=w;
  recalc();
  }


// Set height
void FXWindow::setHeight(FXint h){
  if(h<0) h=0;
  height=h;
  recalc();
  }


// Change layout
void FXWindow::setLayoutHints(FXuint lout){
  FXuint opts=(options&~LAYOUT_MASK) | (lout&LAYOUT_MASK);
  if(options!=opts){
    options=opts;
    recalc();
    }
  }


// Get layout hints
FXuint FXWindow::getLayoutHints() const {
  return (options&LAYOUT_MASK);
  }


// Is widget a composite
FXbool FXWindow::isComposite() const {
  return false;
  }


// Window does override-redirect
FXbool FXWindow::doesOverrideRedirect() const {
  return false;
  }


// Window does save-unders
FXbool FXWindow::doesSaveUnder() const {
  return false;
  }


/*******************************************************************************/


// Add hot key to closest ancestor's accelerator table
void FXWindow::addHotKey(FXHotKey code){
  FXAccelTable *accel=nullptr;
  FXWindow *win=this;
  while(win && (accel=win->getAccelTable())==nullptr) win=win->parent;
  if(accel) accel->addAccel(code,this,FXSEL(SEL_KEYPRESS,ID_HOTKEY),FXSEL(SEL_KEYRELEASE,ID_HOTKEY));
  }


// Remove hot key from closest ancestor's accelerator table
void FXWindow::remHotKey(FXHotKey code){
  FXAccelTable *accel=nullptr;
  FXWindow *win=this;
  while(win && (accel=win->getAccelTable())==nullptr) win=win->parent;
  if(accel) accel->removeAccel(code);
  }


/*******************************************************************************/


// Is cursor shown
FXbool FXWindow::cursorShown() const {
  return (flags&FLAG_CURSOR)!=0;
  }


// Show or hide the cursor
void FXWindow::showCursor(FXbool flag){
  if(!cursorShown() && flag){
    if(xid){
#ifdef WIN32
      ShowCursor(true);
#else
      XDefineCursor((Display*)getApp()->getDisplay(),xid,defaultCursor->id());
#endif
      }
    flags|=FLAG_CURSOR;
    }
  else if(cursorShown() && !flag){
    if(xid){
#ifdef WIN32
      ShowCursor(false);
#else
      XDefineCursor((Display*)getApp()->getDisplay(),xid,getApp()->getDefaultCursor(DEF_BLANK_CURSOR)->id());
#endif
      }
    flags&=~FLAG_CURSOR;
    }
  }


// Set cursor
void FXWindow::setDefaultCursor(FXCursor* cur){
  if(defaultCursor!=cur){
    if(cur==nullptr){ fxerror("%s::setDefaultCursor: NULL cursor argument.\n",getClassName()); }
    if(xid){
      if(cur->id()==0){ fxerror("%s::setDefaultCursor: Cursor has not been created yet.\n",getClassName()); }
      if(cursorShown()){
#ifdef WIN32
        if(underCursor() && !grabbed()) SetCursor((HCURSOR)cur->id());
#else
        XDefineCursor((Display*)getApp()->getDisplay(),xid,cur->id());
#endif
        }
      }
    defaultCursor=cur;
    }
  }


// Set drag cursor
void FXWindow::setDragCursor(FXCursor* cur){
  if(dragCursor!=cur){
    if(cur==nullptr){ fxerror("%s::setDragCursor: NULL cursor argument.\n",getClassName()); }
    if(xid){
      if(cur->id()==0){ fxerror("%s::setDragCursor: Cursor has not been created yet.\n",getClassName()); }
      if(grabbed()){
#ifdef WIN32
        SetCursor((HCURSOR)cur->id());
#else
        XChangeActivePointerGrab((Display*)getApp()->getDisplay(),GRAB_EVENT_MASK,cur->id(),CurrentTime);
#endif
        }
      }
    dragCursor=cur;
    }
  }


// Set window background
void FXWindow::setBackColor(FXColor clr){
  if(clr!=backColor){
    backColor=clr;
//    if(xid){
//#ifdef WIN32
//      // FIXME //
//#else
//      XSetWindowBackground((Display*)getApp()->getDisplay(),xid,visual->getPixel(backColor));
//#endif
//      }
    update();
    }
  }



/*******************************************************************************/


// Lost the selection
long FXWindow::onSelectionLost(FXObject*,FXSelector,void* ptr){
  FXTRACE((100,"%s::onSelectionLost %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_SELECTION_LOST,message),ptr);
  }


// Gained the selection
long FXWindow::onSelectionGained(FXObject*,FXSelector,void* ptr){
  FXTRACE((100,"%s::onSelectionGained %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_SELECTION_GAINED,message),ptr);
  }


// Somebody wants our the selection
long FXWindow::onSelectionRequest(FXObject*,FXSelector,void* ptr){
  FXTRACE((100,"%s::onSelectionRequest %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_SELECTION_REQUEST,message),ptr);
  }


// Has this window the selection
FXbool FXWindow::hasSelection() const {
  return (getApp()->selectionWindow==this);
  }


// Acquire the selection.
// We always generate SEL_SELECTION_LOST and SEL_SELECTION_GAINED
// because we assume the selection types may have changed, and want
// to give target opportunity to allocate the new data for these types.
FXbool FXWindow::acquireSelection(const FXDragType *types,FXuint numtypes){
  if(!types || !numtypes){ fxerror("%s::acquireSelection: should have at least one type to select.\n",getClassName()); }
  if(getApp()->selectionWindow){
    getApp()->selectionWindow->handle(getApp(),FXSEL(SEL_SELECTION_LOST,0),&getApp()->event);
    getApp()->selectionWindow=nullptr;
    freeElms(getApp()->xselTypeList);
    getApp()->xselNumTypes=0;
    }
  if(xid){
#ifndef WIN32
    XSetSelectionOwner((Display*)getApp()->getDisplay(),XA_PRIMARY,xid,getApp()->event.time);
    if(XGetSelectionOwner((Display*)getApp()->getDisplay(),XA_PRIMARY)!=xid) return false;
#endif
    }
  if(!getApp()->selectionWindow){
    getApp()->selectionWindow=this;
    getApp()->selectionWindow->handle(getApp(),FXSEL(SEL_SELECTION_GAINED,0),&getApp()->event);
    resizeElms(getApp()->xselTypeList,numtypes);
    memcpy(getApp()->xselTypeList,types,sizeof(FXDragType)*numtypes);
    getApp()->xselNumTypes=numtypes;
    }
  return true;
  }


// Release the selection
FXbool FXWindow::releaseSelection(){
  if(getApp()->selectionWindow==this){
    getApp()->selectionWindow->handle(getApp(),FXSEL(SEL_SELECTION_LOST,0),&getApp()->event);
    getApp()->selectionWindow=nullptr;
    freeElms(getApp()->xselTypeList);
    getApp()->xselNumTypes=0;
    if(xid){
#ifndef WIN32
      XSetSelectionOwner((Display*)getApp()->getDisplay(),XA_PRIMARY,None,getApp()->event.time);
#endif
      }
    return true;
    }
  return false;
  }


/*******************************************************************************/


// Lost the selection
long FXWindow::onClipboardLost(FXObject*,FXSelector,void* ptr){
  FXTRACE((100,"%s::onClipboardLost %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_CLIPBOARD_LOST,message),ptr);
  }


// Gained the selection
long FXWindow::onClipboardGained(FXObject*,FXSelector,void* ptr){
  FXTRACE((100,"%s::onClipboardGained %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_CLIPBOARD_GAINED,message),ptr);
  }


// Somebody wants our the selection
long FXWindow::onClipboardRequest(FXObject*,FXSelector,void* ptr){
  FXTRACE((100,"%s::onClipboardRequest %p\n",getClassName(),this));
  return target && target->tryHandle(this,FXSEL(SEL_CLIPBOARD_REQUEST,message),ptr);
  }


// Has this window the selection
FXbool FXWindow::hasClipboard() const {
  return (getApp()->clipboardWindow==this);
  }


// Acquire the clipboard
// We always generate SEL_CLIPBOARD_LOST and SEL_CLIPBOARD_GAINED
// because we assume the clipboard types may have changed, and want
// to give target opportunity to allocate the new data for these types.
FXbool FXWindow::acquireClipboard(const FXDragType *types,FXuint numtypes){
  if(!types || !numtypes){ fxerror("%s::acquireClipboard: should have at least one type to select.\n",getClassName()); }
  if(getApp()->clipboardWindow){
    getApp()->clipboardWindow->handle(getApp(),FXSEL(SEL_CLIPBOARD_LOST,0),&getApp()->event);
    getApp()->clipboardWindow=nullptr;
#ifndef WIN32
    freeElms(getApp()->xcbTypeList);
    getApp()->xcbNumTypes=0;
#endif
    }
  if(xid){
#ifndef WIN32
    XSetSelectionOwner((Display*)getApp()->getDisplay(),getApp()->xcbSelection,xid,getApp()->event.time);
    if(XGetSelectionOwner((Display*)getApp()->getDisplay(),getApp()->xcbSelection)!=xid) return false;
#else
    if(!OpenClipboard((HWND)xid)) return false;
    EmptyClipboard();
    for(FXuint i=0; i<numtypes; i++){ SetClipboardData(types[i],nullptr); }
    CloseClipboard();
    if(GetClipboardOwner()!=xid) return false;
#endif
    }
  if(!getApp()->clipboardWindow){
    getApp()->clipboardWindow=this;
    getApp()->clipboardWindow->handle(getApp(),FXSEL(SEL_CLIPBOARD_GAINED,0),&getApp()->event);
#ifndef WIN32
    resizeElms(getApp()->xcbTypeList,numtypes);
    memcpy(getApp()->xcbTypeList,types,sizeof(FXDragType)*numtypes);
    getApp()->xcbNumTypes=numtypes;
#endif
    }
  return true;
  }


// Release the clipboard
FXbool FXWindow::releaseClipboard(){
  if(getApp()->clipboardWindow==this){
    getApp()->clipboardWindow->handle(getApp(),FXSEL(SEL_CLIPBOARD_LOST,0),&getApp()->event);
    getApp()->clipboardWindow=nullptr;
#ifndef WIN32
    freeElms(getApp()->xcbTypeList);
    getApp()->xcbNumTypes=0;
#endif
    if(xid){
#ifndef WIN32
      XSetSelectionOwner((Display*)getApp()->getDisplay(),getApp()->xcbSelection,None,getApp()->event.time);
#else
      if(OpenClipboard((HWND)xid)){EmptyClipboard();CloseClipboard();}
#endif
      }
    return true;
    }
  return false;
  }


/*******************************************************************************/


// Get pointer location (in window coordinates)
FXbool FXWindow::getCursorPosition(FXint& x,FXint& y,FXuint& buttons) const {
  if(xid){
#ifdef WIN32
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient((HWND)xid,&pt);
    x=pt.x; y=pt.y;
    buttons=fxmodifierkeys();
    return true;
#else
    Window dum; int rx,ry;
    return XQueryPointer((Display*)getApp()->getDisplay(),xid,&dum,&dum,&rx,&ry,&x,&y,&buttons);
#endif
    }
  return false;
  }


// Set pointer location (in window coordinates)
// Contributed by David Heath <dave@hipgraphics.com>
FXbool FXWindow::setCursorPosition(FXint x,FXint y){
  if(xid){
#ifdef WIN32
    POINT pt;
    pt.x=x;
    pt.y=y;
    ClientToScreen((HWND)xid,&pt);
    SetCursorPos(pt.x,pt.y);
    return true;
#else
    XWarpPointer((Display*)getApp()->getDisplay(),None,xid,0,0,0,0,x,y);
    return true;
#endif
    }
  return false;
  }


// Update this widget by sending SEL_UPDATE to its target.
// If there is no target, onUpdate returns 0 on behalf of widgets
// which have the autogray feature enabled.  If there is a target
// but we're not updating because the user is manipulating the
// widget, then onUpdate returns 1 to prevent it from graying
// out during manipulation when the autogray feature is enabled.
// Otherwise, onUpdate returns the value returned by the SEL_UPDATE
// callback to the target.
long FXWindow::onUpdate(FXObject*,FXSelector,void*){
  return target && (!(flags&FLAG_UPDATE) || target->tryHandle(this,FXSEL(SEL_UPDATE,message),nullptr));
  }


// Perform layout immediately
void FXWindow::layout(){
  flags&=~FLAG_DIRTY;
  }


// Mark this window's layout as dirty for later layout
void FXWindow::recalc(){
  if(parent) parent->recalc();
  flags|=FLAG_DIRTY;
  }


// Force GUI refresh of this window and all of its children
void FXWindow::forceRefresh(){
  FXWindow *win=this;
  while(1){
    win->handle(win,FXSEL(SEL_UPDATE,0),nullptr);
    if(win->getFirst()){ win=win->getFirst(); continue; }
nxt:if(win==this) break;
    if(win->getNext()){ win=win->getNext(); continue; }
    win=win->getParent();
    goto nxt;
    }
  }


// Update dirty rectangle
void FXWindow::update(FXint x,FXint y,FXint w,FXint h) const {
  if(xid){

    // We toss out rectangles outside the visible area
    if(x>=width || y>=height || x+w<=0 || y+h<=0) return;

    // Intersect with the window
    if(x<0){w+=x;x=0;}
    if(y<0){h+=y;y=0;}
    if(x+w>width){w=width-x;}
    if(y+h>height){h=height-y;}

    // Append the rectangle; it is a synthetic expose event!!
    if(w>0 && h>0){
#ifdef WIN32
      RECT r;
      r.left=x;
      r.top=y;
      r.right=x+w;
      r.bottom=y+h;
      InvalidateRect((HWND)xid,&r,true);
#else
      getApp()->addRepaint(xid,x,y,w,h,true);
#endif
      }
    }
  }


// Update dirty window
void FXWindow::update() const {
  update(0,0,width,height);
  }


// If marked but not yet painted, paint the given area
void FXWindow::repaint(FXint x,FXint y,FXint w,FXint h) const {
  if(xid){

    // We toss out rectangles outside the visible area
    if(x>=width || y>=height || x+w<=0 || y+h<=0) return;

    // Intersect with the window
    if(x<0){w+=x;x=0;}
    if(y<0){h+=y;y=0;}
    if(x+w>width){w=width-x;}
    if(y+h>height){h=height-y;}

    if(w>0 && h>0){
#ifdef WIN32
      RECT r;
      r.left=x;
      r.top=y;
      r.right=x+w;
      r.bottom=y+h;
      RedrawWindow((HWND)xid,&r,nullptr,RDW_UPDATENOW);
#else
      getApp()->removeRepaints(xid,x,y,w,h);
#endif
      }
    }
  }


// If marked but not yet painted, paint the entire window
void FXWindow::repaint() const {
  repaint(0,0,width,height);
  }


// Move window
void FXWindow::move(FXint x,FXint y){
  FXTRACE((200,"%s::move: x=%d y=%d\n",getClassName(),x,y));
  if((flags&FLAG_DIRTY)||(x!=xpos)||(y!=ypos)){
    xpos=x;
    ypos=y;
    if(xid){

      // Similar as for position(), we have to generate protocol
      // here so as to make the display reflect reality...
#ifdef WIN32
      SetWindowPos((HWND)xid,nullptr,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOOWNERZORDER);
#else
      XMoveWindow((Display*)getApp()->getDisplay(),xid,x,y);
#endif
      if(flags&FLAG_DIRTY) layout();
      }
    }
  }


// Move and resize
void FXWindow::position(FXint x,FXint y,FXint w,FXint h){
  FXint ow=width;
  FXint oh=height;
  FXTRACE((200,"%s::position: x=%d y=%d w=%d h=%d\n",getClassName(),x,y,w,h));
  if(w<0) w=0;
  if(h<0) h=0;
  if((flags&FLAG_DIRTY)||(x!=xpos)||(y!=ypos)||(w!=ow)||(h!=oh)){
    xpos=x;
    ypos=y;
    width=w;
    height=h;
    if(xid){

      // Alas, we have to generate some protocol here even if the placement
      // as recorded in the widget hasn't actually changed.  This is because
      // there are ways to change the placement w/o going through position()!
#ifdef WIN32
      SetWindowPos((HWND)xid,nullptr,x,y,w,h,SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOOWNERZORDER);
#else
      if(0<w && 0<h){
        if(shown() && (ow<=0 || oh<=0)){
          XMapWindow((Display*)getApp()->getDisplay(),xid);
          }
        XMoveResizeWindow((Display*)getApp()->getDisplay(),xid,x,y,w,h);
        }
      else if(0<ow && 0<oh){
        XUnmapWindow((Display*)getApp()->getDisplay(),xid);
        }
#endif

      // We don't have to layout the interior of this widget unless
      // the size has changed or it was marked as dirty:- this is
      // a very good optimization as it's applied recursively!
      if((flags&FLAG_DIRTY)||(w!=ow)||(h!=oh)) layout();
      }
    }
  }


// Resize
void FXWindow::resize(FXint w,FXint h){
  FXint ow=width;
  FXint oh=height;
  FXTRACE((200,"%s::resize: w=%d h=%d\n",getClassName(),w,h));
  if(w<0) w=0;
  if(h<0) h=0;
  if((flags&FLAG_DIRTY)||(w!=ow)||(h!=oh)){
    width=w;
    height=h;
    if(xid){

      // Similar as for position(), we have to generate protocol here..
#ifdef WIN32
      SetWindowPos((HWND)xid,nullptr,0,0,w,h,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOOWNERZORDER);
#else
      if(0<w && 0<h){
        if(shown() && (ow<=0 || oh<=0)){
          XMapWindow((Display*)getApp()->getDisplay(),xid);
          }
        XResizeWindow((Display*)getApp()->getDisplay(),xid,w,h);
        }
      else if(0<ow && 0<oh){
        XUnmapWindow((Display*)getApp()->getDisplay(),xid);
        }
#endif

      // And of course the size has changed so layout is needed
      layout();
      }
    }
  }


// Scroll rectangle x,y,w,h by a shift of dx,dy
void FXWindow::scroll(FXint x,FXint y,FXint w,FXint h,FXint dx,FXint dy) const {
  if(xid && 0<w && 0<h && (dx || dy)){
#ifdef WIN32
    RECT rect;
    rect.left=x;
    rect.top=y;
    rect.right=x+w;
    rect.bottom=y+h;
    ScrollWindowEx((HWND)xid,dx,dy,&rect,&rect,nullptr,nullptr,SW_INVALIDATE);
#else

    // No overlap:- repaint the whole thing
    if((w<=FXABS(dx)) || (h<=FXABS(dy))){
      getApp()->addRepaint(xid,x,y,w,h,true);
      }

    // Has overlap, so blit contents and repaint the exposed parts
    else{
      FXint  tx,ty,fx,fy,ex,ey,ew,eh;
      XEvent event;

      // Force server to catch up
      XSync((Display*)getApp()->getDisplay(),False);

      // Pull any outstanding repaint events into our own repaint rectangle list
      while(XCheckWindowEvent((Display*)getApp()->getDisplay(),xid,ExposureMask,&event)){
        if(event.xany.type==NoExpose) continue;
        getApp()->addRepaint(xid,event.xexpose.x,event.xexpose.y,event.xexpose.width,event.xexpose.height,false);
        if(event.xgraphicsexpose.count==0) break;
        }

      // Scroll all repaint rectangles of this window by the dx,dy
      getApp()->scrollRepaints(xid,dx,dy);

      // Compute blitted area
      if(dx>0){             // Content shifted right
        fx=x;
        tx=x+dx;
        ex=x;
        ew=dx;
        }
      else{                 // Content shifted left
        fx=x-dx;
        tx=x;
        ex=x+w+dx;
        ew=-dx;
        }
      if(dy>0){             // Content shifted down
        fy=y;
        ty=y+dy;
        ey=y;
        eh=dy;
        }
      else{                 // Content shifted up
        fy=y-dy;
        ty=y;
        ey=y+h+dy;
        eh=-dy;
        }

      // BLIT the contents
      XCopyArea((Display*)getApp()->getDisplay(),xid,xid,(GC)visual->scrollgc,fx,fy,w-ew,h-eh,tx,ty);

      // Post additional rectangles for the uncovered areas
      if(dy){
        getApp()->addRepaint(xid,x,ey,w,eh,true);
        }
      if(dx){
        getApp()->addRepaint(xid,ex,y,ew,h,true);
        }
      }
#endif
    }
  }


// Check if logically shown
FXbool FXWindow::shown() const {
  return (flags&FLAG_SHOWN)!=0;
  }


// Show window
void FXWindow::show(){
  FXTRACE((160,"%s::show %p\n",getClassName(),this));
  if(!shown()){
    flags|=FLAG_SHOWN;
    if(xid){
#ifdef WIN32
      ShowWindow((HWND)xid,SW_SHOWNOACTIVATE);
#else
      if(0<width && 0<height) XMapWindow((Display*)getApp()->getDisplay(),xid);
#endif
      }
    }
  }


// Hide window
void FXWindow::hide(){
  FXTRACE((160,"%s::hide %p\n",getClassName(),this));
  if(shown()){
    killFocus();
    flags&=~FLAG_SHOWN;
    if(xid){
#ifdef WIN32
      if(getApp()->mouseGrabWindow==this){
        ReleaseCapture();
        SetCursor((HCURSOR)defaultCursor->id());
        handle(this,FXSEL(SEL_UNGRABBED,0),&getApp()->event);
        getApp()->mouseGrabWindow=nullptr;
        }
      if(getApp()->keyboardGrabWindow==this){
        getApp()->keyboardGrabWindow=nullptr;
        }
      ShowWindow((HWND)xid,SW_HIDE);
#else
      if(getApp()->mouseGrabWindow==this){
        XUngrabPointer((Display*)getApp()->getDisplay(),CurrentTime);
        XFlush((Display*)getApp()->getDisplay());
        handle(this,FXSEL(SEL_UNGRABBED,0),&getApp()->event);
        getApp()->mouseGrabWindow=nullptr;
        }
      if(getApp()->keyboardGrabWindow==this){
        XUngrabKeyboard((Display*)getApp()->getDisplay(),getApp()->event.time);
        XFlush((Display*)getApp()->getDisplay());
        getApp()->keyboardGrabWindow=nullptr;
        }
      XUnmapWindow((Display*)getApp()->getDisplay(),xid);
#endif
      }
    }
  }


// Reparent window under a new parent
void FXWindow::reparent(FXWindow* father,FXWindow* other){
  FXbool hadfocus=inFocusChain();
  if(father==nullptr){ fxerror("%s::reparent: NULL parent specified.\n",getClassName()); }
  if(parent==nullptr){ fxerror("%s::reparent: cannot reparent root window.\n",getClassName()); }
  if(parent==getRoot() || father==getRoot()){ fxerror("%s::reparent: cannot reparent toplevel window.\n",getClassName()); }
  if(other && father!=other->getParent()){ fxerror("%s::reparent: other window has different parent.\n",getClassName()); }
  if(this!=other){

    // Check for funny cases
    if(containsChild(father)){ fxerror("%s::reparent: new parent is child of window.\n",getClassName()); }

    // Both windows created or both non-created
    if(xid && !father->id()){ fxerror("%s::reparent: new parent not created yet.\n",getClassName()); }
    if(!xid && father->id()){ fxerror("%s::reparent: window not created yet.\n",getClassName()); }

    // Kill focus chain through this window
    if(hadfocus) killFocus();

    // Recalc old path
    recalc();

    // Unlink from old parent
    if(prev) prev->next=next; else parent->first=next;
    if(next) next->prev=prev; else parent->last=prev;

    // Link to new parent
    if(other){
      next=other;
      prev=other->prev;
      other->prev=this;
      }
    else{
      next=nullptr;
      prev=father->last;
      father->last=this;
      }
    if(prev){
      prev->next=this;
      }
    else{
      father->first=this;
      }

    // Moved between parents
    if(parent!=father){

      // New parent and owner
      parent=father;
      owner=father;

      // Hook up to new window in server too
      if(xid && parent->id()){
#ifdef WIN32
        SetParent((HWND)xid,(HWND)parent->id());
#else
        // See remarks in FXToolBarGrip
        XReparentWindow((Display*)getApp()->getDisplay(),xid,parent->id(),0,0);
        XFlush((Display*)getApp()->getDisplay());
#endif
        }
      }

    // Set focus back if we had it
    if(hadfocus) setFocus();

    // Recalc new path
    recalc();
    }
  }


// Is window enabled
FXbool FXWindow::isEnabled() const {
  return (flags&FLAG_ENABLED)!=0;
  }


// Enable the window
void FXWindow::enable(){
  if(!isEnabled()){
    flags|=FLAG_ENABLED;
    if(xid){
#ifdef WIN32
      EnableWindow((HWND)xid,true);
#else
      FXuint events=BASIC_EVENT_MASK|ENABLED_EVENT_MASK;
      if(isShell()) events|=SHELL_EVENT_MASK;
      XSelectInput((Display*)getApp()->getDisplay(),xid,events);
#endif
      }
    }
  }


// Disable the window
void FXWindow::disable(){
  killFocus();
  if(isEnabled()){
    flags&=~FLAG_ENABLED;
    if(xid){
#ifdef WIN32
      EnableWindow((HWND)xid,false);
      if(getApp()->mouseGrabWindow==this){
        ReleaseCapture();
        SetCursor((HCURSOR)defaultCursor->id());
        handle(this,FXSEL(SEL_UNGRABBED,0),&getApp()->event);
        getApp()->mouseGrabWindow=nullptr;
        }
      if(getApp()->keyboardGrabWindow==this){
        getApp()->keyboardGrabWindow=nullptr;
        }
#else
      FXuint events=BASIC_EVENT_MASK;
      if(isShell()) events|=SHELL_EVENT_MASK;
      XSelectInput((Display*)getApp()->getDisplay(),xid,events);
      if(getApp()->mouseGrabWindow==this){
        XUngrabPointer((Display*)getApp()->getDisplay(),CurrentTime);
        XFlush((Display*)getApp()->getDisplay());
        handle(this,FXSEL(SEL_UNGRABBED,0),&getApp()->event);
        getApp()->mouseGrabWindow=nullptr;
        }
      if(getApp()->keyboardGrabWindow==this){
        XUngrabKeyboard((Display*)getApp()->getDisplay(),getApp()->event.time);
        XFlush((Display*)getApp()->getDisplay());
        getApp()->keyboardGrabWindow=nullptr;
        }
#endif
      }
    }
  }


// Raise (but do not activate!)
void FXWindow::raise(){
  if(xid){
#ifdef WIN32
    SetWindowPos((HWND)xid,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOOWNERZORDER);
#else
    XRaiseWindow((Display*)getApp()->getDisplay(),xid);
#endif
    }
  }


// Lower
void FXWindow::lower(){
  if(xid){
#ifdef WIN32
    SetWindowPos((HWND)xid,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOOWNERZORDER);
#else
    XLowerWindow((Display*)getApp()->getDisplay(),xid);
#endif
    }
  }


#if 0
// Get coordinates from another window (for symmetry)
void FXWindow::translateCoordinatesFrom(FXint& tox,FXint& toy,const FXWindow* fromwindow,FXint fromx,FXint fromy) const {
  if(fromwindow==nullptr){ fxerror("%s::translateCoordinatesFrom: from-window is NULL.\n",getClassName()); }
  for(const FXWindow* w=fromwindow; w; w=w->getParent()){
    fromx+=w->getX();
    fromy+=w->getY();
    }
  for(const FXWindow* w=this; w; w=w->getParent()){
    fromx-=w->getX();
    fromy-=w->getY();
    }
  tox=fromx;
  toy=fromy;
  }


// Get coordinates to another window (for symmetry)
void FXWindow::translateCoordinatesTo(FXint& tox,FXint& toy,const FXWindow* towindow,FXint fromx,FXint fromy) const {
  if(towindow==nullptr){ fxerror("%s::translateCoordinatesTo: to-window is NULL.\n",getClassName()); }
  for(const FXWindow* w=this; w; w=w->getParent()){
    fromx+=w->getX();
    fromy+=w->getY();
    }
  for(const FXWindow* w=towindow; w; w=w->getParent()){
    fromx-=w->getX();
    fromy-=w->getY();
    }
  tox=fromx;
  toy=fromy;
  }
#endif

#if 1
// Get coordinates from another window (for symmetry)
void FXWindow::translateCoordinatesFrom(FXint& tox,FXint& toy,const FXWindow* fromwindow,FXint fromx,FXint fromy) const {
  if(fromwindow==nullptr){ fxerror("%s::translateCoordinatesFrom: from-window is NULL.\n",getClassName()); }
  if(xid && fromwindow->id()){
#ifdef WIN32
    POINT pt;
    pt.x=fromx;
    pt.y=fromy;
    ClientToScreen((HWND)fromwindow->id(),&pt);
    ScreenToClient((HWND)xid,&pt);
    tox=pt.x;
    toy=pt.y;
#else
    Window tmp;
    XTranslateCoordinates((Display*)getApp()->getDisplay(),fromwindow->id(),xid,fromx,fromy,&tox,&toy,&tmp);
#endif
    }
  }


// Get coordinates to another window (for symmetry)
void FXWindow::translateCoordinatesTo(FXint& tox,FXint& toy,const FXWindow* towindow,FXint fromx,FXint fromy) const {
  if(towindow==nullptr){ fxerror("%s::translateCoordinatesTo: to-window is NULL.\n",getClassName()); }
  if(xid && towindow->id()){
#ifdef WIN32
    POINT pt;
    pt.x=fromx;
    pt.y=fromy;
    ClientToScreen((HWND)xid,&pt);
    ScreenToClient((HWND)towindow->id(),&pt);
    tox=pt.x;
    toy=pt.y;
#else
    Window tmp;
    XTranslateCoordinates((Display*)getApp()->getDisplay(),xid,towindow->id(),fromx,fromy,&tox,&toy,&tmp);
#endif
    }
  }
#endif

/*******************************************************************************/


// Return true if active grab is in effect
FXbool FXWindow::grabbed() const {
  return getApp()->mouseGrabWindow==this;
  }


// Acquire grab; also switches to the drag cursor
void FXWindow::grab(){
  if(xid){
    FXTRACE((150,"%s::grab %p\n",getClassName(),this));
    if(dragCursor->id()==0){ fxerror("%s::grab: Cursor has not been created yet.\n",getClassName()); }
    if(!shown()){ fxwarning("%s::grab: Window is not visible.\n",getClassName()); }
#ifdef WIN32
    SetCapture((HWND)xid);
    if(GetCapture()!=(HWND)xid){
      SetCapture((HWND)xid);
      }
    SetCursor((HCURSOR)dragCursor->id());
#else
    if(GrabSuccess!=XGrabPointer((Display*)getApp()->getDisplay(),xid,false,GRAB_EVENT_MASK,GrabModeAsync,GrabModeAsync,None,dragCursor->id(),getApp()->event.time)){
      XGrabPointer((Display*)getApp()->getDisplay(),xid,false,GRAB_EVENT_MASK,GrabModeAsync,GrabModeAsync,None,dragCursor->id(),CurrentTime);
      }
#endif
    getApp()->mouseGrabWindow=this;
    }
  }


// Release grab; also switches back to the normal cursor
void FXWindow::ungrab(){
  if(xid){
    FXTRACE((150,"%s::ungrab %p\n",getClassName(),this));
    getApp()->mouseGrabWindow=nullptr;
#ifdef WIN32
    ReleaseCapture();
    SetCursor((HCURSOR)defaultCursor->id());
#else
    XUngrabPointer((Display*)getApp()->getDisplay(),getApp()->event.time);
    XFlush((Display*)getApp()->getDisplay());
#endif
    }
  }


// Return true if active grab is in effect
FXbool FXWindow::grabbedKeyboard() const {
  return getApp()->keyboardGrabWindow==this;
  }


// Grab keyboard device; note grabbing keyboard generates
// SEL_FOCUSIN on the grab-window and SEL_FOCUSOUT on the old
// focus-window
void FXWindow::grabKeyboard(){
  if(xid){
    FXTRACE((150,"%s::grabKeyboard %p\n",getClassName(),this));
    if(!shown()){ fxwarning("%s::ungrabKeyboard: Window is not visible.\n",getClassName()); }
#ifdef WIN32
    SetActiveWindow((HWND)xid); // FIXME Check this
#else
    XGrabKeyboard((Display*)getApp()->getDisplay(),xid,false,GrabModeAsync,GrabModeAsync,getApp()->event.time);
#endif
    getApp()->keyboardGrabWindow=this;
    }
  }


// Ungrab keyboard device; note ungrabbing keyboard generates
// SEL_FOCUSOUT on the grab-window and SEL_FOCUSIN on the
// current focus-window
void FXWindow::ungrabKeyboard(){
  if(xid){
    FXTRACE((150,"%s::ungrabKeyboard %p\n",getClassName(),this));
    getApp()->keyboardGrabWindow=nullptr;
#ifndef WIN32
    XUngrabKeyboard((Display*)getApp()->getDisplay(),getApp()->event.time);
#endif
    }
  }


// The widget lost the grab for some reason [Windows].
// Subclasses should try to clean up the mess...
long FXWindow::onUngrabbed(FXObject*,FXSelector,void* ptr){
  FXTRACE((150,"%s::onUngrabbed\n",getClassName()));
  if(target) target->tryHandle(this,FXSEL(SEL_UNGRABBED,message),ptr);
//#ifdef WIN32
//  SetCursor((HCURSOR)defaultCursor->id());    // FIXME Maybe should be done with WM_SETCURSOR?
//#endif
  return 1;
  }


/*******************************************************************************/


// Translate message
const FXchar* FXWindow::tr(const FXchar* text,const FXchar* hint,FXint count) const {
  FXTranslator *translator=getApp()->getTranslator();
  if(translator){
    return translator->tr(getClassName(),text,hint,count);
    }
  return text;
  }


/*******************************************************************************/


// Is window drop enabled
FXbool FXWindow::isDropEnabled() const {
  return (flags&FLAG_DROPTARGET)!=0;
  }


// Enable widget as drop target
void FXWindow::dropEnable(){
  flags|=FLAG_DROPTARGET;
  }


// Disable widget as drop target
void FXWindow::dropDisable(){
  flags&=~FLAG_DROPTARGET;
  }


// Set drag rectangle to block events while inside
void FXWindow::setDragRectangle(FXint x,FXint y,FXint w,FXint h,FXbool wantupdates) const {
  if(xid==0){ fxerror("%s::setDragRectangle: window has not yet been created.\n",getClassName()); }
#ifdef WIN32
  POINT pt;
  pt.x=x;
  pt.y=y;
  ClientToScreen((HWND)parent->id(),&pt);
  getApp()->xdndRect.x=(short)pt.x;
  getApp()->xdndRect.y=(short)pt.y;
#else
  Window tmp;
  int tox,toy;
  XTranslateCoordinates((Display*)getApp()->getDisplay(),xid,XDefaultRootWindow((Display*)getApp()->getDisplay()),x,y,&tox,&toy,&tmp);
  getApp()->xdndRect.x=tox;
  getApp()->xdndRect.y=toy;
  getApp()->xdndWantUpdates=wantupdates;
#endif
  getApp()->xdndRect.w=w;
  getApp()->xdndRect.h=h;
  }


// Clear drag rectangle
void FXWindow::clearDragRectangle() const {
  if(xid==0){ fxerror("%s::clearDragRectangle: window has not yet been created.\n",getClassName()); }
  getApp()->xdndRect.x=0;
  getApp()->xdndRect.y=0;
  getApp()->xdndRect.w=0;
  getApp()->xdndRect.h=0;
#ifndef WIN32
  getApp()->xdndWantUpdates=true; // Isn't this bullshit?
#endif
  }


// Get dropped data; called in response to DND enter or DND drop
FXbool FXWindow::getDNDData(FXDNDOrigin origin,FXDragType targettype,FXuchar*& ptr,FXuint& size) const {
  if(xid==0){ fxerror("%s::getDNDData: window has not yet been created.\n",getClassName()); }
  switch(origin){
    case FROM_DRAGNDROP:
      getApp()->dragdropGetData(this,targettype,ptr,size);
      break;
    case FROM_CLIPBOARD:
      getApp()->clipboardGetData(this,targettype,ptr,size);
      break;
    case FROM_SELECTION:
      getApp()->selectionGetData(this,targettype,ptr,size);
      break;
    }
  return ptr!=nullptr;
  }


// Get dropped string; called in response to DND enter or DND drop
FXbool FXWindow::getDNDData(FXDNDOrigin origin,FXDragType targettype,FXString& string) const {
  FXchar *ptr; FXuint size;
  string=FXString::null;
  if(getDNDData(origin,targettype,reinterpret_cast<FXuchar*&>(ptr),size)){
    if(targettype==utf16Type){                                  // UTF16
      FXUTF16LECodec unicode;
      string.length(unicode.mb2utflen(ptr,size));
      unicode.mb2utf(string.text(),string.length(),ptr,size);
      }
    else if(targettype==textType || targettype==stringType){    // ASCII
      FX88591Codec ascii;
      string.length(ascii.mb2utflen(ptr,size));
      ascii.mb2utf(string.text(),string.length(),ptr,size);
      }
    else{                                                       // AS-IS
      string.assign(ptr,size);
      }
    freeElms(ptr);
    return true;
    }
  return false;
  }


// Set drop data; data array will be deleted by the system automatically!
FXbool FXWindow::setDNDData(FXDNDOrigin origin,FXDragType targettype,FXuchar* ptr,FXuint size) const {
  if(xid==0){ fxerror("%s::setDNDData: window has not yet been created.\n",getClassName()); }
  switch(origin){
    case FROM_DRAGNDROP:
      getApp()->dragdropSetData(this,targettype,ptr,size);
      break;
    case FROM_CLIPBOARD:
      getApp()->clipboardSetData(this,targettype,ptr,size);
      break;
    case FROM_SELECTION:
      getApp()->selectionSetData(this,targettype,ptr,size);
      break;
    }
  return true;
  }


// Set drop data; data array will be deleted by the system automatically!
FXbool FXWindow::setDNDData(FXDNDOrigin origin,FXDragType targettype,const FXString& string) const {
  FXchar* ptr; FXuint size;
  if(targettype==utf16Type){                                    // UTF16
    FXUTF16LECodec unicode;
    size=unicode.utf2mblen(string.text(),string.length());
    if(callocElms(ptr,size+2)){
      unicode.utf2mb(ptr,size,string.text(),string.length());
      return setDNDData(origin,targettype,reinterpret_cast<FXuchar*>(ptr),size);
//      return setDNDData(origin,targettype,reinterpret_cast<FXuchar*>(ptr),size+2);
      }
    }
  else if(targettype==textType || targettype==stringType){      // ASCII
    FX88591Codec ascii;
    size=ascii.utf2mblen(string.text(),string.length());
    if(callocElms(ptr,size+2)){
      ascii.utf2mb(ptr,size,string.text(),string.length());
      return setDNDData(origin,targettype,reinterpret_cast<FXuchar*>(ptr),size);
//      return setDNDData(origin,targettype,reinterpret_cast<FXuchar*>(ptr),size+1);
      }
    }
  else{                                                         // AS-IS
    size=string.length();
    if(callocElms(ptr,size+2)){
      memcpy(ptr,string.text(),string.length());
      return setDNDData(origin,targettype,reinterpret_cast<FXuchar*>(ptr),size);
//      return setDNDData(origin,targettype,reinterpret_cast<FXuchar*>(ptr),size);
      }
    }
  return false;
  }


// Inquire about types being dragged or available on the clipboard or selection
FXbool FXWindow::inquireDNDTypes(FXDNDOrigin origin,FXDragType*& types,FXuint& numtypes) const {
  if(xid==0){ fxerror("%s::inquireDNDTypes: window has not yet been created.\n",getClassName()); }
  switch(origin){
    case FROM_DRAGNDROP:
      getApp()->dragdropGetTypes(this,types,numtypes);
      break;
    case FROM_CLIPBOARD:
      getApp()->clipboardGetTypes(this,types,numtypes);
      break;
    case FROM_SELECTION:
      getApp()->selectionGetTypes(this,types,numtypes);
      break;
    }
  return types!=nullptr;
  }


// Is a certain type being offered via drag and drop, clipboard, or selection?
FXbool FXWindow::offeredDNDType(FXDNDOrigin origin,FXDragType type) const {
  if(xid==0){ fxerror("%s::offeredDNDType: window has not yet been created.\n",getClassName()); }
  FXbool offered=false;
  FXDragType *types;
  FXuint i,ntypes;
  if(inquireDNDTypes(origin,types,ntypes)){
    for(i=0; i<ntypes; i++){
      if(type==types[i]){ offered=true; break; }
      }
    freeElms(types);
    }
  return offered;
  }


// Target accepts or rejects drop, and may suggest different action
void FXWindow::acceptDrop(FXDragAction action) const {
  getApp()->ansAction=DRAG_REJECT;
  if(action!=DRAG_REJECT){
    getApp()->ansAction=getApp()->ddeAction;
    if(action!=DRAG_ACCEPT) getApp()->ansAction=action;
    }
  }


// Sent by the drop target
void FXWindow::dropFinished(FXDragAction action) const {
  if(!getApp()->xdndFinishSent){
    if(action==DRAG_ACCEPT) action=getApp()->ansAction;
#ifdef WIN32
    PostMessage((HWND)getApp()->xdndSource,WM_DND_FINISH_REJECT+action,0,(LPARAM)xid);
#else
    XEvent se;
    se.xclient.type=ClientMessage;
    se.xclient.display=(Display*)getApp()->getDisplay();
    se.xclient.message_type=getApp()->xdndFinished;
    se.xclient.format=32;
    se.xclient.window=getApp()->xdndSource;
    se.xclient.data.l[0]=xid;
    se.xclient.data.l[1]=(action==DRAG_REJECT)?0:1;
    se.xclient.data.l[2]=getApp()->xdndActionList[action];
    se.xclient.data.l[3]=0;
    se.xclient.data.l[4]=0;
    XSendEvent((Display*)getApp()->getDisplay(),getApp()->xdndSource,True,NoEventMask,&se);
    XFlush((Display*)getApp()->getDisplay());
#endif
    getApp()->xdndFinishSent=true;
    }
  }


// Target wants to know drag action of source
FXDragAction FXWindow::inquireDNDAction() const {
  return getApp()->ddeAction;
  }


// Source wants to find out if target accepted
FXDragAction FXWindow::didAccept() const {
  return getApp()->ansAction;
  }


// True if we're in a drag operation
FXbool FXWindow::isDragging() const {
  return (getApp()->dragWindow==this);
  }


// True if this window is drop target
FXbool FXWindow::isDropTarget() const {
  return (getApp()->dropWindow==this);
  }


// Start a drag on the types mentioned
FXbool FXWindow::beginDrag(const FXDragType *types,FXuint numtypes){
  if(xid==0){ fxerror("%s::beginDrag: window has not yet been created.\n",getClassName()); }
  if(!isDragging()){
    if(types==nullptr || numtypes<1){ fxerror("%s::beginDrag: should have at least one type to drag.\n",getClassName()); }
#ifdef WIN32
    getApp()->xdndTypes=CreateFileMappingA(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,(numtypes+1)*sizeof(FXDragType),"XdndTypeList");
    if(getApp()->xdndTypes){
      FXDragType *dragtypes=(FXDragType*)MapViewOfFile(getApp()->xdndTypes,FILE_MAP_WRITE,0,0,0);
      if(dragtypes){
        dragtypes[0]=numtypes;
        memcpy(&dragtypes[1],types,numtypes*sizeof(FXDragType));
        UnmapViewOfFile(dragtypes);
        }
      }
    getApp()->xdndTarget=0;
    getApp()->ansAction=DRAG_REJECT;
    getApp()->xdndStatusPending=false;
    getApp()->xdndStatusReceived=false;
    getApp()->xdndRect.x=0;
    getApp()->xdndRect.y=0;
    getApp()->xdndRect.w=0;
    getApp()->xdndRect.h=0;
#else
    XSetSelectionOwner((Display*)getApp()->getDisplay(),getApp()->xdndSelection,xid,getApp()->event.time);
    if(XGetSelectionOwner((Display*)getApp()->getDisplay(),getApp()->xdndSelection)!=xid){
      fxwarning("%s::beginDrag: failed to acquire DND selection.\n",getClassName());
      return false;
      }
    resizeElms(getApp()->xdndTypeList,numtypes);
    memcpy(getApp()->xdndTypeList,types,sizeof(FXDragType)*numtypes);
    getApp()->xdndNumTypes=numtypes;
    XChangeProperty((Display*)getApp()->getDisplay(),xid,getApp()->xdndTypes,XA_ATOM,32,PropModeReplace,(unsigned char*)getApp()->xdndTypeList,getApp()->xdndNumTypes);
    XChangeProperty((Display*)getApp()->getDisplay(),xid,getApp()->xdndActions,XA_ATOM,32,PropModeReplace,(unsigned char*)getApp()->xdndActionList,6);
    getApp()->xdndTarget=0;
    getApp()->xdndProxyTarget=0;
    getApp()->ansAction=DRAG_REJECT;
    getApp()->xdndStatusPending=false;
    getApp()->xdndStatusReceived=false;
    getApp()->xdndWantUpdates=true;
    getApp()->xdndRect.x=0;
    getApp()->xdndRect.y=0;
    getApp()->xdndRect.w=0;
    getApp()->xdndRect.h=0;
#endif
    getApp()->dragWindow=this;
    return true;
    }
  return false;
  }


// Drag to new position
FXbool FXWindow::handleDrag(FXint x,FXint y,FXDragAction action){
  if(xid==0){ fxerror("%s::handleDrag: window has not yet been created.\n",getClassName()); }
  if(action<DRAG_ASK || DRAG_PRIVATE<action){ fxerror("%s::handleDrag: illegal drag action.\n",getClassName()); }
  if(isDragging()){

#ifdef WIN32            // WINDOWS

    HANDLE version=0;
    FXbool forcepos=false;
    POINT point;
    HWND window;

    // Get drop window
    point.x=x;
    point.y=y;
    window=WindowFromPoint(point);      // FIXME wrong for disabled windows
    while(window){
//      version=(FXuint)GetProp(window,(LPCTSTR)MAKELONG(getApp()->xdndAware,0));
      version=GetProp(window,(LPCTSTR)MAKELONG(getApp()->xdndAware,0));
      if(version) break;
      window=GetParent(window);
      }

    // Changed windows
    if(window!=getApp()->xdndTarget){

      // Moving OUT of XDND aware window?
      if(getApp()->xdndTarget!=0){
        PostMessage((HWND)getApp()->xdndTarget,WM_DND_LEAVE,0,(LPARAM)xid);
        }

      // Reset XDND variables
      getApp()->xdndTarget=window;
      getApp()->ansAction=DRAG_REJECT;
      getApp()->xdndStatusPending=false;
      getApp()->xdndStatusReceived=false;
      getApp()->xdndRect.x=x;
      getApp()->xdndRect.y=y;
      getApp()->xdndRect.w=1;
      getApp()->xdndRect.h=1;

      // Moving INTO XDND aware window?
      if(getApp()->xdndTarget!=0){
        PostMessage((HWND)getApp()->xdndTarget,WM_DND_ENTER,0,(LPARAM)xid);
        forcepos=true;
        }
      }

    // Now send a position message
    if(getApp()->xdndTarget!=0){

      // Send position if we're outside the mouse box or ignoring it
     if(forcepos || x<getApp()->xdndRect.x || y<getApp()->xdndRect.y || getApp()->xdndRect.x+getApp()->xdndRect.w<=x || getApp()->xdndRect.y+getApp()->xdndRect.h<=y){

        // No outstanding status message, so send new pos right away
        if(!getApp()->xdndStatusPending){
          PostMessage((HWND)getApp()->xdndTarget,WM_DND_POSITION_REJECT+action,MAKELONG(x,y),(LPARAM)xid);
          getApp()->xdndStatusPending=true;       // Waiting for the other app to respond
          }
        }
      }

#else                   // X11

    Window proxywindow,window,child,win,proxywin,root;
    unsigned long ni,ba;
    unsigned char *ptr1;
    unsigned char *ptr2;
    unsigned char *ptr3;
    Atom    typ;
    int     fmt;
    Atom    version;
    int     dropx;
    int     dropy;
    XEvent  se;
    FXbool    forcepos=false;

    // Find XDND aware window at the indicated location
    root=XDefaultRootWindow((Display*)getApp()->getDisplay());
    window=0;
    proxywindow=0;
    version=0;
    win=root;
    while(1){
      if(!XTranslateCoordinates((Display*)getApp()->getDisplay(),root,win,x,y,&dropx,&dropy,&child)) break;
      proxywin=win;
      if(XGetWindowProperty((Display*)getApp()->getDisplay(),win,getApp()->xdndProxy,0,1,False,AnyPropertyType,&typ,&fmt,&ni,&ba,&ptr1)==Success){
        if(typ==XA_WINDOW && fmt==32 && ni>0){
          if(XGetWindowProperty((Display*)getApp()->getDisplay(),*((Window*)ptr1),getApp()->xdndProxy,0,1,False,AnyPropertyType,&typ,&fmt,&ni,&ba,&ptr2)==Success){
            if(typ==XA_WINDOW && fmt==32 && ni>0 && *((Window*)ptr2) == *((Window*)ptr1)){
              proxywin=*((Window*)ptr1);
              }
            XFree(ptr2);
            }
          }
        XFree(ptr1);
        }
      if(XGetWindowProperty((Display*)getApp()->getDisplay(),proxywin,getApp()->xdndAware,0,1,False,AnyPropertyType,&typ,&fmt,&ni,&ba,&ptr3)==Success){
        if(typ==XA_ATOM && fmt==32 && ni>0 && *((Atom*)ptr3)>=3){
          window=win;
          proxywindow=proxywin;
          version=FXMIN(*((Atom*)ptr3),XDND_PROTOCOL_VERSION);
          if(window!=root){XFree(ptr3);break;}
          }
        XFree(ptr3);
        }
      if(child==None) break;
      win=child;
      }

    // Changed windows
    if(window!=getApp()->xdndTarget){

      // Moving OUT of XDND aware window?
      if(getApp()->xdndTarget!=0){
        se.xclient.type=ClientMessage;
        se.xclient.display=(Display*)getApp()->getDisplay();
        se.xclient.message_type=getApp()->xdndLeave;
        se.xclient.format=32;
        se.xclient.window=getApp()->xdndTarget;
        se.xclient.data.l[0]=xid;
        se.xclient.data.l[1]=0;
        se.xclient.data.l[2]=0;
        se.xclient.data.l[3]=0;
        se.xclient.data.l[4]=0;
        XSendEvent((Display*)getApp()->getDisplay(),getApp()->xdndProxyTarget,True,NoEventMask,&se);
        }

      // Reset XDND variables
      getApp()->xdndTarget=window;
      getApp()->xdndProxyTarget=proxywindow;
      getApp()->ansAction=DRAG_REJECT;
      getApp()->xdndStatusPending=false;
      getApp()->xdndStatusReceived=false;
      getApp()->xdndWantUpdates=true;
      getApp()->xdndRect.x=x;
      getApp()->xdndRect.y=y;
      getApp()->xdndRect.w=1;
      getApp()->xdndRect.h=1;

      // Moving INTO XDND aware window?
      if(getApp()->xdndTarget!=0){
        se.xclient.type=ClientMessage;
        se.xclient.display=(Display*)getApp()->getDisplay();
        se.xclient.message_type=getApp()->xdndEnter;
        se.xclient.format=32;
        se.xclient.window=getApp()->xdndTarget;
        se.xclient.data.l[0]=xid;
        se.xclient.data.l[1]=version<<24;
        se.xclient.data.l[2]=getApp()->xdndNumTypes>=1 ? getApp()->xdndTypeList[0] : None;
        se.xclient.data.l[3]=getApp()->xdndNumTypes>=2 ? getApp()->xdndTypeList[1] : None;
        se.xclient.data.l[4]=getApp()->xdndNumTypes>=3 ? getApp()->xdndTypeList[2] : None;
        if(getApp()->xdndNumTypes>3) se.xclient.data.l[1]|=1;
        XSendEvent((Display*)getApp()->getDisplay(),getApp()->xdndProxyTarget,True,NoEventMask,&se);
        forcepos=true;
        }
      }

    // Now send a position message
    if(getApp()->xdndTarget!=0){

      // Send position if we're outside the mouse box or ignoring it
      if(forcepos || getApp()->xdndRect.w==0 || getApp()->xdndRect.h==0 || getApp()->xdndWantUpdates || x<getApp()->xdndRect.x || y<getApp()->xdndRect.y || getApp()->xdndRect.x+getApp()->xdndRect.w<=x || getApp()->xdndRect.y+getApp()->xdndRect.h<=y){

        // No outstanding status message, so send new pos right away
        if(!getApp()->xdndStatusPending){
          se.xclient.type=ClientMessage;
          se.xclient.display=(Display*)getApp()->getDisplay();
          se.xclient.message_type=getApp()->xdndPosition;
          se.xclient.format=32;
          se.xclient.window=getApp()->xdndTarget;
          se.xclient.data.l[0]=xid;
          se.xclient.data.l[1]=0;
          se.xclient.data.l[2]=MKUINT(y,x);                               // Coordinates
          se.xclient.data.l[3]=getApp()->event.time;                      // Time stamp
          se.xclient.data.l[4]=getApp()->xdndActionList[action];
          XSendEvent((Display*)getApp()->getDisplay(),getApp()->xdndProxyTarget,True,NoEventMask,&se);
          getApp()->xdndStatusPending=true;       // Waiting for the other app to respond
          }
        }
      }

#endif

    return true;
    }
  return false;
  }


#ifndef WIN32

struct XDNDMatch {
  Atom xdndStatus;
  Atom xdndPosition;
  Atom xdndFinished;
  Atom xdndDrop;
  Atom xdndEnter;
  Atom xdndLeave;
  };


// Scan event queue for XDND messages or selection requests
static Bool matchxdnd(Display*,XEvent* event,XPointer ptr){
  XDNDMatch *m=(XDNDMatch*)ptr;
  return event->type==SelectionRequest || (event->type==ClientMessage && (event->xclient.message_type==m->xdndStatus || event->xclient.message_type==m->xdndPosition || event->xclient.message_type==m->xdndFinished || event->xclient.message_type==m->xdndDrop || event->xclient.message_type==m->xdndEnter || event->xclient.message_type==m->xdndLeave));
  }

#endif


// Terminate the drag; if drop flag is false, don't drop even if accepted.
FXDragAction FXWindow::endDrag(FXbool drop){
  FXDragAction action=DRAG_REJECT;
  if(xid==0){ fxerror("%s::endDrag: window has not yet been created.\n",getClassName()); }
  if(isDragging()){

#if defined(WIN32)      // WINDOWS

    FXbool nodrop=true;
    FXuint loops;
    MSG msg;

    // Ever received a status
    if(getApp()->xdndStatusReceived && drop){

      // If a status message is still pending, wait for it
      if(getApp()->xdndStatusPending){
        loops=1000;
        do{
          FXTRACE((100,"Waiting for XdndStatus\n"));
          if(PeekMessage(&msg,nullptr,0,0,PM_REMOVE)){
            getApp()->dispatchEvent(msg);

            // We got the status update, finally
            if(WM_DND_STATUS_REJECT<=msg.message && msg.message<=WM_DND_STATUS_PRIVATE){
              FXTRACE((100,"Got XdndStatus\n"));
              getApp()->xdndStatusPending=false;
              break;
              }

            // We got a selection request message, so there is still hope that
            // the target is functioning; lets give it a bit more time...
            if(msg.message==WM_DND_REQUEST){
              FXTRACE((100,"Got SelectionRequest\n"));
              loops=1000;
              }
            }
          FXThread::sleep(10000000);
          }
        while(--loops);
        FXTRACE((100,"Waiting for pending XdndStatus\n"));
        getApp()->xdndStatusPending=false;
        }

      // Got our status message
      if(!getApp()->xdndStatusPending && getApp()->ansAction!=DRAG_REJECT){

        FXTRACE((100,"Sending XdndDrop\n"));
        PostMessage((HWND)getApp()->xdndTarget,WM_DND_DROP,0,(LPARAM)xid);

        // Wait until the target has processed the drop; since the
        // target may be us, we keep processing all XDND related messages
        // while we're waiting here.  After some time, we assume the target
        // may have core dumped and we fall out of the loop.....
        loops=1000;
        do{
          FXTRACE((100,"Waiting for XdndFinish\n"));
          if(PeekMessage(&msg,nullptr,0,0,PM_REMOVE)){
            getApp()->dispatchEvent(msg);

            // Got the finish message; we now know which action was taken
            if(WM_DND_FINISH_REJECT<=msg.message && msg.message<=WM_DND_FINISH_PRIVATE){
              action=(FXDragAction)(msg.message-WM_DND_FINISH_REJECT);
              FXTRACE((100,"Got XdndFinish action=%d\n",action));
              break;
              }

            // We got a selection request message, so there is still hope that
            // the target is functioning; lets give it a bit more time...
            if(msg.message==WM_DND_REQUEST){
              FXTRACE((100,"Got SelectionRequest\n"));
              loops=1000;
              }
            }
          FXThread::sleep(10000000);
          }
        while(--loops);

        // We tried a drop
        nodrop=false;
        }
      }

    // Didn't drop, or didn't get any response, so just send a leave
    if(nodrop){
      FXTRACE((100,"Sending XdndLeave\n"));
      PostMessage((HWND)getApp()->xdndTarget,WM_DND_LEAVE,0,(LPARAM)xid);
      }


    // Get rid of the shared memory block containing the types list
    if(getApp()->xdndTypes) CloseHandle(getApp()->xdndTypes);
    getApp()->xdndTypes=nullptr;

    // Clean up
    getApp()->xdndTarget=0;
    getApp()->ansAction=DRAG_REJECT;
    getApp()->xdndStatusPending=false;
    getApp()->xdndStatusReceived=false;
    getApp()->xdndRect.x=0;
    getApp()->xdndRect.y=0;
    getApp()->xdndRect.w=0;
    getApp()->xdndRect.h=0;
    getApp()->dragWindow=nullptr;

#else                   // X11

    FXbool nodrop=true;
    XEvent se;
    FXuint loops;
    XDNDMatch match;

    // Fill up match struct
    match.xdndStatus=getApp()->xdndStatus;
    match.xdndPosition=getApp()->xdndPosition;
    match.xdndFinished=getApp()->xdndFinished;
    match.xdndDrop=getApp()->xdndDrop;
    match.xdndEnter=getApp()->xdndEnter;
    match.xdndLeave=getApp()->xdndLeave;

    // Ever received a status
    if(getApp()->xdndStatusReceived && drop){

      // If a status message is still pending, wait for it
      if(getApp()->xdndStatusPending){
        loops=1000;
        do{
          FXTRACE((100,"Waiting for pending XdndStatus\n"));
          if(XCheckIfEvent((Display*)getApp()->getDisplay(),&se,matchxdnd,(char*)&match)){
            getApp()->dispatchEvent(se);

            // We got the status update, finally
            if(se.xclient.type==ClientMessage && se.xclient.message_type==getApp()->xdndStatus){
              FXTRACE((100,"Got XdndStatus\n"));
              getApp()->xdndStatusPending=false;
              break;
              }

            // We got a selection request message, so there is still hope that
            // the target is functioning; lets give it a bit more time...
            if(se.xselection.type==SelectionRequest && se.xselectionrequest.selection==getApp()->xdndSelection){
              FXTRACE((100,"Got SelectionRequest\n"));
              loops=1000;
              }
            }
          FXThread::sleep(10000000);
          }
        while(--loops);
        }

      // Got our status message
      if(!getApp()->xdndStatusPending && getApp()->ansAction!=DRAG_REJECT){

        FXTRACE((100,"Sending XdndDrop\n"));
        se.xclient.type=ClientMessage;
        se.xclient.display=(Display*)getApp()->getDisplay();
        se.xclient.message_type=getApp()->xdndDrop;
        se.xclient.format=32;
        se.xclient.window=getApp()->xdndTarget;
        se.xclient.data.l[0]=xid;
        se.xclient.data.l[1]=0;
        se.xclient.data.l[2]=getApp()->event.time;
        se.xclient.data.l[3]=0;
        se.xclient.data.l[4]=0;
        XSendEvent((Display*)getApp()->getDisplay(),getApp()->xdndProxyTarget,True,NoEventMask,&se);

        // Wait until the target has processed the drop; since the
        // target may be us, we keep processing all XDND related messages
        // while we're waiting here.  After some time, we assume the target
        // may have core dumped and we fall out of the loop.....
        loops=1000;
        do{
          FXTRACE((100,"Waiting for XdndFinish\n"));
          if(XCheckIfEvent((Display*)getApp()->getDisplay(),&se,matchxdnd,(char*)&match)){
            getApp()->dispatchEvent(se);

            // Got the finish message; we now know the drop has been completed and processed
            if(se.xclient.type==ClientMessage && se.xclient.message_type==getApp()->xdndFinished){
              if(se.xclient.data.l[1]&1){
                action=DRAG_ACCEPT;
                if((FXID)se.xclient.data.l[2]==getApp()->xdndActionList[DRAG_ASK]) action=DRAG_ASK;
                else if((FXID)se.xclient.data.l[2]==getApp()->xdndActionList[DRAG_MOVE]) action=DRAG_MOVE;
                else if((FXID)se.xclient.data.l[2]==getApp()->xdndActionList[DRAG_COPY]) action=DRAG_COPY;
                else if((FXID)se.xclient.data.l[2]==getApp()->xdndActionList[DRAG_LINK]) action=DRAG_LINK;
                else if((FXID)se.xclient.data.l[2]==getApp()->xdndActionList[DRAG_PRIVATE]) action=DRAG_PRIVATE;
                }
              FXTRACE((100,"Got XdndFinish action=%u\n",action));
              break;
              }

            // We got a selection request message, so there is still hope that
            // the target is functioning; lets give it a bit more time...
            if(se.xselection.type==SelectionRequest && se.xselectionrequest.selection==getApp()->xdndSelection){
              FXTRACE((100,"Got SelectionRequest\n"));
              loops=1000;
              }
            }
          FXThread::sleep(10000000);
          }
        while(--loops);

        // We tried a drop but failed
        nodrop=false;
        }
      }

    // Didn't drop, or didn't get any response, so just send a leave
    if(nodrop){
      FXTRACE((100,"Sending XdndLeave\n"));
      se.xclient.type=ClientMessage;
      se.xclient.display=(Display*)getApp()->getDisplay();
      se.xclient.message_type=getApp()->xdndLeave;
      se.xclient.format=32;
      se.xclient.window=getApp()->xdndTarget;
      se.xclient.data.l[0]=xid;
      se.xclient.data.l[1]=0;
      se.xclient.data.l[2]=0;
      se.xclient.data.l[3]=0;
      se.xclient.data.l[4]=0;
      XSendEvent((Display*)getApp()->getDisplay(),getApp()->xdndProxyTarget,True,NoEventMask,&se);
      }

    // Clean up
    XSetSelectionOwner((Display*)getApp()->getDisplay(),getApp()->xdndSelection,None,getApp()->event.time);
    XDeleteProperty((Display*)getApp()->getDisplay(),xid,getApp()->xdndTypes);
    XDeleteProperty((Display*)getApp()->getDisplay(),xid,getApp()->xdndActions);
    freeElms(getApp()->xdndTypeList);
    getApp()->xdndNumTypes=0;
    getApp()->xdndTarget=0;
    getApp()->xdndProxyTarget=0;
    getApp()->ansAction=DRAG_REJECT;
    getApp()->xdndStatusPending=false;
    getApp()->xdndStatusReceived=false;
    getApp()->xdndWantUpdates=true;
    getApp()->xdndRect.x=0;
    getApp()->xdndRect.y=0;
    getApp()->xdndRect.w=0;
    getApp()->xdndRect.h=0;
    getApp()->dragWindow=nullptr;
#endif
    }
  return action;
  }


/*******************************************************************************/


// Delete window
FXWindow::~FXWindow(){
  FXTRACE((TOPIC_CONSTRUCT,"FXWindow::~FXWindow %p\n",this));
  getApp()->windowCount--;
  destroy();
  delete accelTable;
  delete composeContext;
  if(prev) prev->next=next; else if(parent) parent->first=next;
  if(next) next->prev=prev; else if(parent) parent->last=prev;
  if(parent && parent->focus==this) parent->focus=nullptr;
  if(getApp()->activeWindow==this) getApp()->activeWindow=nullptr;
  if(getApp()->cursorWindow==this) getApp()->cursorWindow=parent;
  if(getApp()->mouseGrabWindow==this) getApp()->mouseGrabWindow=nullptr;
  if(getApp()->keyboardGrabWindow==this) getApp()->keyboardGrabWindow=nullptr;
  if(getApp()->keyWindow==this) getApp()->keyWindow=nullptr;
  if(getApp()->selectionWindow==this) getApp()->selectionWindow=nullptr;
  if(getApp()->clipboardWindow==this) getApp()->clipboardWindow=nullptr;
  if(getApp()->dragWindow==this) getApp()->dragWindow=nullptr;
  if(getApp()->dropWindow==this) getApp()->dropWindow=nullptr;
  if(getApp()->refresherstop==this) getApp()->refresherstop=parent;
  if(getApp()->refresher==this) getApp()->refresher=parent;
  if(parent) parent->recalc();
  parent=(FXWindow*)-1L;
  owner=(FXWindow*)-1L;
  first=last=(FXWindow*)-1L;
  next=prev=(FXWindow*)-1L;
  focus=(FXWindow*)-1L;
  composeContext=(FXComposeContext*)-1L;
  defaultCursor=(FXCursor*)-1L;
  dragCursor=(FXCursor*)-1L;
  accelTable=(FXAccelTable*)-1L;
  target=(FXObject*)-1L;
  }

}
