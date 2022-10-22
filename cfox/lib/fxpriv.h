/********************************************************************************
*                                                                               *
*              P r i v a t e   I n t e r n a l   F u n c t i o n s              *
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
#ifndef FXPRIV_H
#define FXPRIV_H

namespace FX {

// DND protocol version
#define XDND_PROTOCOL_VERSION   5


#if defined(WIN32)  /////////////// Windows /////////////////////////////////////


// Device input messages
#ifndef WM_INPUT
#define WM_INPUT                0x00FF
#endif

// Definitions for DND messages for Windows
#define WM_DND_REQUEST          (WM_APP+1)
#define WM_DND_ENTER            (WM_APP+2)
#define WM_DND_LEAVE            (WM_APP+3)
#define WM_DND_POSITION_REJECT  (WM_APP+4)
#define WM_DND_POSITION_ASK     (WM_APP+5)
#define WM_DND_POSITION_COPY    (WM_APP+6)
#define WM_DND_POSITION_MOVE    (WM_APP+7)
#define WM_DND_POSITION_LINK    (WM_APP+8)
#define WM_DND_POSITION_PRIVATE (WM_APP+9)
#define WM_DND_STATUS_REJECT    (WM_APP+10)
#define WM_DND_STATUS_ASK       (WM_APP+11)
#define WM_DND_STATUS_COPY      (WM_APP+12)
#define WM_DND_STATUS_MOVE      (WM_APP+13)
#define WM_DND_STATUS_LINK      (WM_APP+14)
#define WM_DND_STATUS_PRIVATE   (WM_APP+15)
#define WM_DND_DROP             (WM_APP+16)
#define WM_DND_REPLY            (WM_APP+17)
#define WM_DND_FINISH_REJECT    (WM_APP+18)
#define WM_DND_FINISH_ASK       (WM_APP+19)
#define WM_DND_FINISH_COPY      (WM_APP+20)
#define WM_DND_FINISH_MOVE      (WM_APP+21)
#define WM_DND_FINISH_LINK      (WM_APP+22)
#define WM_DND_FINISH_PRIVATE   (WM_APP+23)


// Missing in CYGWIN
#ifndef IMAGE_SUBSYSTEM_NATIVE_WINDOWS
#define IMAGE_SUBSYSTEM_NATIVE_WINDOWS 8
#endif
#ifndef IMAGE_SUBSYSTEM_WINDOWS_CE_GUI
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI 9
#endif


// Keyboard stuff
extern FXuint fxmodifierkeys();
extern UINT wkbGetCodePage();
extern FXuint wkbMapKeyCode(UINT iMsg, WPARAM uVirtKey, LPARAM lParam);
extern int (WINAPI *ToUnicodeEx)(UINT, UINT, const BYTE*, LPWSTR, int, UINT, HKL);

// Windows helpers
extern HANDLE fxsendrequest(HWND window,HWND requestor,WPARAM type);
extern HANDLE fxsenddata(HWND window,FXuchar* data,FXuint size);
extern HANDLE fxrecvdata(HANDLE hMap,FXuchar*& data,FXuint& size);


#else ////////////////////////////// Unix ///////////////////////////////////////


// X11 helpers
extern Atom fxsendrequest(Display *display,Window window,Atom selection,Atom prop,Atom type,FXuint time);
extern Atom fxsendreply(Display *display,Window window,Atom selection,Atom prop,Atom target,FXuint time);
extern Atom fxsendtypes(Display *display,Window window,Atom prop,FXDragType* types,FXuint numtypes);
extern Atom fxrecvtypes(Display *display,Window window,Atom prop,FXDragType*& types,FXuint& numtypes,FXbool del);
extern Atom fxsenddata(Display *display,Window window,Atom prop,Atom type,FXuchar* data,FXuint size);
extern Atom fxrecvdata(Display *display,Window window,Atom prop,Atom incr,Atom& type,FXuchar*& data,FXuint& size);


#endif //////////////////////////////////////////////////////////////////////////

}

#endif
