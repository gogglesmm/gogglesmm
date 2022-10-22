/********************************************************************************
*                                                                               *
*                         C o m p o s e - C o n t e x t                         *
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
#include "FXFont.h"
#include "FXComposeContext.h"
#include "FXException.h"


/*
  Notes:
  - In Asian languages, a text entry widget that's being edited may have
    an input method editor.  During the composition process, the system
    needs to keep track of the state of the composition until it is
    committed as an input to the widget.  This class represents that
    state.
  - Each text entry widget may have a compositon context, while it
    has the focus.
  - The composition context is deleted when the focus is moved to another
    widget away from the entry widget.
*/

#define DISPLAY(app)     ((Display*)((app)->display))


using namespace FX;

/*******************************************************************************/

namespace FX {


// Object implementation
FXIMPLEMENT(FXComposeContext,FXId,nullptr,0)


#ifdef WIN32   //////////////////////////  MS-Windows ///////////////////////////


// Deserialization
FXComposeContext::FXComposeContext():window(nullptr),message(0){
  FXTRACE((100,"FXComposeContext::FXComposeContext %p\n",this));
  }


// Create input context
FXComposeContext::FXComposeContext(FXApp* a,FXWindow* win,FXSelector sel):FXId(a),window(win),message(sel){
  FXTRACE((100,"FXComposeContext::FXComposeContext %p\n",this));
  }


// Realize the input context
void FXComposeContext::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE((100,"%s::create %p\n",getClassName(),this));
      xid=window->id();
      }
    }
  }


// Unrealize the input context
void FXComposeContext::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE((100,"%s::destroy %p\n",getClassName(),this));
      ///////
      }
    }
  }


// Set focus to it
void FXComposeContext::focusIn(){
  if(xid){
    ///////
    }
  }


// Kill focus to it
void FXComposeContext::focusOut(){
  if(xid){
    ///////
    }
  }


// Set the font
void FXComposeContext::setFont(FXFont* font){
  if(xid && font && font->id()){
    HIMC himc=ImmGetContext((HWND)xid);
    LOGFONT lf;
    GetObject((HFONT)font->id(),sizeof(LOGFONT),&lf);
    ImmSetCompositionFont(himc,&lf);
    ImmReleaseContext((HWND)xid,himc);
    }
  }


// Set the spot
void FXComposeContext::setSpot(FXint x,FXint y){
  if(xid){
    HIMC himc=ImmGetContext((HWND)xid);
    COMPOSITIONFORM cf;
    cf.dwStyle=CFS_POINT;
    FXint tox,toy;
    window->translateCoordinatesTo(tox,toy,window->getShell(),x,y);
    cf.ptCurrentPos.x=tox;
    cf.ptCurrentPos.y=toy;
    ImmSetCompositionWindow(himc,&cf);
    ImmReleaseContext((HWND)xid,himc);
    }
  }


// Set the area
void FXComposeContext::setArea(FXint x,FXint y,FXint w,FXint h){
  if(xid){
    ///////
    }
  }


// Translate key event
FXString FXComposeContext::translateEvent(FXRawEvent& event){
  FXString result;
  if(xid){
    HIMC himc=ImmGetContext(event.hwnd);
    LONG mlen=0;
    FXnchar* mstr;
    if(event.lParam&GCS_RESULTSTR){
      mlen=ImmGetCompositionString(himc,GCS_RESULTSTR,nullptr,0);
      mstr=new FXnchar [mlen];
      ImmGetCompositionString(himc,GCS_RESULTSTR,mstr,mlen);
      }
    else{
      mlen=ImmGetCompositionString(himc,GCS_COMPSTR,nullptr,0);
      mstr=new FXnchar [mlen+1];
      ImmGetCompositionString(himc,GCS_COMPSTR,mstr,mlen);
      }
    mstr[mlen/sizeof(FXnchar)]=0;
    ImmReleaseContext(event.hwnd,himc);
    int utf8len=WideCharToMultiByte(CP_UTF8,0,mstr,-1,nullptr,0,nullptr,nullptr);
    FXchar* utf8str=new FXchar [utf8len];
    WideCharToMultiByte(CP_UTF8,0,mstr,-1,utf8str,utf8len,nullptr,nullptr);
    result.assign(utf8str,utf8len);

    delete [] mstr;
    delete [] utf8str;
    }
  return result;
  }


// Delete input context
FXComposeContext::~FXComposeContext(){
  FXTRACE((100,"FXComposeContext::~FXComposeContext %p\n",this));
  }


#else   //////////////////////////////  X-Windows ///////////////////////////////


// Deserialization
FXComposeContext::FXComposeContext():window(nullptr),message(0),fontset(0){
  FXTRACE((100,"FXComposeContext::FXComposeContext %p\n",this));
  }


// Create input context
FXComposeContext::FXComposeContext(FXApp* a,FXWindow* win,FXSelector sel):FXId(a),window(win),message(sel),fontset(0){
  FXTRACE((100,"FXComposeContext::FXComposeContext %p\n",this));
  }


/*

    // Determine list of input styles
    XIMStyles *ximstyles=nullptr;
    XGetIMValues((XIM)xim,XNQueryInputStyle,&ximstyles,nullptr);
    if(ximstyles){
      FXuint s;

      // Try preferred input style
      for(s=0; s<ximstyles->count_styles; s++){
        if(ximstyles->supported_styles[s]==inputstyle) goto match;
        }

      // Try root input style
      inputstyle=XIMPreeditNothing|XIMStatusNothing;
      for(s=0; s<ximstyles->count_styles; s++){
        if(ximstyles->supported_styles[s]==inputstyle) goto match;
        }

      // Try none style
      inputstyle=XIMPreeditNone|XIMStatusNone;
      for(s=0; s<ximstyles->count_styles; s++){
        if(ximstyles->supported_styles[s]==inputstyle) goto match;
        }

      // No style at all
      inputstyle=0;

      // Free list
match:XFree(ximstyles);
*/


/*
FXbool isIMRunning(Display *display){
  const FXchar *p=XSetLocaleModifiers(nullptr);
  if(p){
    FXTRACE((100,"XSetLocaleModifiers=%s\n",p));
    FXString server("@server=");
    server.append(p+4);         // skip "@im="
    FXint pos=server.find('@',1);
    if(0<pos) server.trunc(pos);
    Atom atom=XInternAtom(display,server.text(),False);
    Window win=XGetSelectionOwner(display,atom);
    return win!=None;
    }
  return false;
  }
*/


// Realize the input context
void FXComposeContext::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE((100,"%s::create %p\n",getClassName(),this));
#ifndef NO_XIM
      XIMCallback statusStartStruct;
      XIMCallback statusDoneStruct;
      XIMCallback statusDrawStruct;
      XIMCallback editStartStruct;
      XIMCallback editDoneStruct;
      XIMCallback editDrawStruct;
      XIMCallback editCaretStruct;
      XVaNestedList editAttr;
      XVaNestedList statusAttr;
      XIMStyles *ximstyles=nullptr;
      XRectangle rect;
      XPoint spot;
      FXuint style,s;

      // Check if input methods are available
      if(!getApp()->hasInputMethod()){ fxerror("FXComposeContext: no input methods\n"); }

      // We must have a window
      if(!window || !window->id()){ fxerror("FXComposeContext: illegal window parameter\n"); }

      // Get input style
      if(FXString::comparecase(getApp()->inputstyle,"onthespot")==0)
        style=XIMPreeditCallbacks|XIMStatusNothing;
      else if(FXString::comparecase(getApp()->inputstyle,"overthespot")==0)
        style=XIMPreeditPosition|XIMStatusNothing;
      else if(FXString::comparecase(getApp()->inputstyle,"offthespot")==0)
        style=XIMPreeditArea|XIMStatusArea;
      else if(FXString::comparecase(getApp()->inputstyle,"root")==0)
        style=XIMPreeditNothing|XIMStatusNothing;
      else
        style=XIMPreeditNone|XIMStatusNone;

      // Determine list of input styles
      XGetIMValues((XIM)getApp()->xim,XNQueryInputStyle,&ximstyles,nullptr);
      if(ximstyles){

        // Try preferred input style
        for(s=0; s<ximstyles->count_styles; s++){
          if(ximstyles->supported_styles[s]==style) goto m;
          }

        // Try root input style
        style=XIMPreeditNothing|XIMStatusNothing;
        for(s=0; s<ximstyles->count_styles; s++){
          if(ximstyles->supported_styles[s]==style) goto m;
          }

        // Try none style
        style=XIMPreeditNone|XIMStatusNone;
        for(s=0; s<ximstyles->count_styles; s++){
          if(ximstyles->supported_styles[s]==style) goto m;
          }

        // Pick first
        if(ximstyles->count_styles){
          style=ximstyles->supported_styles[0];
          }

        // Free list
m:      XFree(ximstyles);
        }

      // On the spot method
      if(style&XIMPreeditCallbacks){
        editStartStruct.client_data=(XPointer)this;
        editStartStruct.callback=(XIMProc)editStartCallback;
        editDoneStruct.client_data=(XPointer)this;
        editDoneStruct.callback=(XIMProc)editDoneCallback;
        editDrawStruct.client_data=(XPointer)this;
        editDrawStruct.callback=(XIMProc)editDrawCallback;
        editCaretStruct.client_data=(XPointer)this;
        editCaretStruct.callback=(XIMProc)editCaretCallback;
        editAttr=XVaCreateNestedList(0,XNPreeditStartCallback,&editStartStruct,XNPreeditDrawCallback,&editDrawStruct,XNPreeditDoneCallback,&editDoneStruct,XNPreeditCaretCallback,&editCaretStruct,nullptr);

        // Have status callbacks
        if(style&XIMStatusCallbacks){
          FXTRACE((100,"On the Spot/Status\n"));
          statusStartStruct.client_data=(XPointer)this;
          statusStartStruct.callback=(XIMProc)statusStartCallback;
          statusDoneStruct.client_data=(XPointer)this;
          statusDoneStruct.callback=(XIMProc)statusDoneCallback;
          statusDrawStruct.client_data=(XPointer)this;
          statusDrawStruct.callback=(XIMProc)statusDrawCallback;
          statusAttr=XVaCreateNestedList(0,XNStatusStartCallback,&statusStartStruct,XNStatusDoneCallback,&statusDoneStruct,XNStatusDrawCallback,&statusDrawStruct,nullptr);
          xid=(FXID)XCreateIC((XIM)getApp()->xim,XNInputStyle,XIMPreeditCallbacks|XIMStatusCallbacks,XNClientWindow,window->id(),XNPreeditAttributes,editAttr,XNStatusAttributes,statusAttr,nullptr);
          XFree(statusAttr);
          }

        // No status callbacks
        else{
          FXTRACE((100,"On the Spot\n"));
          xid=(FXID)XCreateIC((XIM)getApp()->xim,XNInputStyle,XIMPreeditCallbacks|XIMStatusNothing,XNClientWindow,window->id(),XNPreeditAttributes,editAttr,nullptr);
          }
        XFree(editAttr);
        }

      // Off the spot method
      else if(style&XIMPreeditArea){
        FXTRACE((100,"Off the Spot\n"));
        rect.x=0;
        rect.y=0;
        rect.width=window->getWidth();
        rect.height=window->getHeight();
        editAttr=XVaCreateNestedList(0,XNArea,&rect,nullptr);
        xid=(FXID)XCreateIC((XIM)getApp()->xim,XNInputStyle,XIMPreeditArea|XIMStatusArea,XNClientWindow,window->id(),XNPreeditAttributes,editAttr,nullptr);
        XFree(editAttr);
        }

      // Over the spot method
      else if(style&XIMPreeditPosition){
        FXTRACE((100,"Over the Spot\n"));
        spot.x=1;
        spot.y=1;
        int missing_charcount;
        char** missing_charsetlist;
        char* def_string;
        fontset=XCreateFontSet(DISPLAY(getApp()),"10x20,10x20",&missing_charsetlist,&missing_charcount,&def_string);
        editAttr=XVaCreateNestedList(0,XNSpotLocation,&spot,XNFontSet,fontset,nullptr);
        xid=(FXID)XCreateIC((XIM)getApp()->xim,XNInputStyle,XIMPreeditPosition|XIMStatusNothing,XNClientWindow,window->id(),XNPreeditAttributes,editAttr,nullptr);
        XFreeStringList(missing_charsetlist);
        XFree(editAttr);
        }

      // Root method
      else{
        FXTRACE((100,"Root\n"));
        xid=(FXID)XCreateIC((XIM)getApp()->xim,XNInputStyle,XIMPreeditNothing|XIMStatusNothing,XNClientWindow,window->id(),nullptr);
        }

      // Reset context
      if(xid){
        //long filterevents=0;
        //XGetICValues((XIC)xid,XNFilterEvents,&filterevents,nullptr);
        //XSelectInput((Display*)getApp()->getDisplay(),window->id(),BASIC_EVENT_MASK|ENABLED_EVENT_MASK|filterevents);
        XmbResetIC((XIC)xid);
        }
#endif
      }
    }
  }


// Unrealize the input context
void FXComposeContext::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE((100,"%s::destroy %p\n",getClassName(),this));
#ifndef NO_XIM
      XDestroyIC((XIC)xid);
#endif
      }
    }
  }


// Set focus to it
void FXComposeContext::focusIn(){
#ifndef NO_XIM
  if(xid){
    XSetICFocus((XIC)xid);
    }
#endif
  }


// Kill focus to it
void FXComposeContext::focusOut(){
#ifndef NO_XIM
  if(xid){
    XUnsetICFocus((XIC)xid);
    }
#endif
  }


// Set the font
void FXComposeContext::setFont(FXFont* font){
  if(xid && font && font->id()){
#ifndef NO_XIM
      ///
#endif
    }
  }


// Set the spot
void FXComposeContext::setSpot(FXint x,FXint y){
#ifndef NO_XIM
  if(xid){
    XVaNestedList editAttr;
    XPoint spot;
    spot.x=x;
    spot.y=y;
    editAttr=XVaCreateNestedList(0,XNSpotLocation,&spot,nullptr);
    XSetICValues((XIC)xid,XNPreeditAttributes,editAttr,nullptr);
    XFree(editAttr);
    }
#endif
  }


// Set the area
void FXComposeContext::setArea(FXint x,FXint y,FXint w,FXint h){
#ifndef NO_XIM
  if(xid){
    XVaNestedList editAttr;
    XRectangle rect;
    rect.x=x;
    rect.y=y;
    rect.width=w;
    rect.height=h;
    editAttr=XVaCreateNestedList(0,XNArea,&rect,nullptr);
    XSetICValues((XIC)xid,XNPreeditAttributes,editAttr,nullptr);
    XFree(editAttr);
    }
#endif
  }


// Translate key event
FXString FXComposeContext::translateEvent(FXRawEvent& event){
  FXString result;
#ifndef NO_XIM
  if(xid){
    char* buffer=new char [513];
    KeySym sym; Status s; int n;
    n=XmbLookupString((XIC)xid,&event.xkey,buffer,512,&sym,&s);
    if(s==XBufferOverflow){
      delete [] buffer;
      buffer=new char [n+1];
      n=XmbLookupString((XIC)xid,&event.xkey,buffer,n,&sym,&s);
      }
    if(s!=XLookupChars && s!=XLookupBoth) n=0;
    // FIXME decode buffer based on XLocaleOfIM(XIMOfIC((XIC)xid))
    buffer[n]=0;
    FXTRACE((100,"XLocaleOfIM=%s\n",XLocaleOfIM(XIMOfIC((XIC)xid))));
    result.assign(buffer,n);
    delete [] buffer;
    }
#endif
  return result;
  }


int FXComposeContext::editStartCallback(void*,FXComposeContext* cc,void*){
  FXTRACE((100,"editStartCallback\n"));
  return -1;			// No length limit
  }


void FXComposeContext::editDoneCallback(void*,FXComposeContext* cc,void*){
  FXTRACE((100,"editDoneCallback\n"));
  }


void FXComposeContext::editDrawCallback(void*,FXComposeContext* cc,void* ptr){
#ifndef NO_XIM
  XIMPreeditDrawCallbackStruct *drawstruct=(XIMPreeditDrawCallbackStruct*)ptr;
  XIMText *ximtext=drawstruct->text;
  FXTRACE((100,"editDrawCallback caret=%d first=%d len=%d\n",drawstruct->caret,drawstruct->chg_first,drawstruct->chg_length));
#endif
  }


void FXComposeContext::editCaretCallback(void*,FXComposeContext* cc,void* ptr){
#ifndef NO_XIM
  XIMPreeditCaretCallbackStruct *caretstruct=(XIMPreeditCaretCallbackStruct*)ptr;
  FXTRACE((100,"editCaretCallback position=%d direction=%d style=%d\n",caretstruct->position,caretstruct->direction,caretstruct->style));
#endif
  }


void FXComposeContext::statusStartCallback(void*,FXComposeContext* cc,void*){
  FXTRACE((100,"statusStartCallback\n"));
  }


void FXComposeContext::statusDoneCallback(void*,FXComposeContext* cc,void*){
  FXTRACE((100,"statusDoneCallback\n"));
  }


void FXComposeContext::statusDrawCallback(void*,FXComposeContext* cc,void* ptr){
#ifndef NO_XIM
  XIMStatusDrawCallbackStruct* drawstruct=(XIMStatusDrawCallbackStruct*)ptr;
  FXTRACE((100,"statusDrawCallback\n"));
#endif
  }


// Delete input context
FXComposeContext::~FXComposeContext(){
  FXTRACE((100,"FXComposeContext::~FXComposeContext %p\n",this));
  destroy();
  window=(FXWindow*)-1L;
  if(fontset) XFreeFontSet(DISPLAY(getApp()),(XFontSet)fontset);
  fontset=(XFontSet)-1L;
  }

#endif  /////////////////////////////////////////////////////////////////////////


}
