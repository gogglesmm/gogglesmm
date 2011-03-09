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
#ifndef GMCOMMON_H
#define GMCOMMON_H

#include "gmconfig.h"

#include <new>
#include <fx.h>

#define FOXVERSION ((FOX_LEVEL) + (FOX_MINOR*1000) + (FOX_MAJOR*100000))
#define FXVERSION(major,minor,release) ((release)+(minor*1000)+(major*100000))

#include <fx3d.h>
#include <FXArray.h>
#include <FXTextCodec.h>

#if FOXVERSION == FXVERSION(1,7,22)
#include <FXAutoPtr.h>
#endif

#include "GMAutoPtr.h"
#include "GMURL.h"
#if FOXVERSION < FXVERSION(1,7,0)
#include "GMMessageChannel.h"
#endif
#include "fxext.h"

/// for locale_t definition
#include <locale.h>


#if FOXVERSION < FXVERSION(1,7,0)
#define TIME_MSEC(ms) (ms)
#define TIME_SEC(s) 	(1000*s)
#define TIME_MIN(m) 	TIME_SEC(60*m)
#define TIME_HOUR(h) 	TIME_MIN(60*h)
#define TO_NANO_SECONDS(x) ((FXlong)(1000000000LL*x))
#else
#define TIME_MSEC(ms) (1000000LL*ms)
#define TIME_SEC(s) 	(1000000000LL*s)
#define TIME_MIN(m) 	TIME_SEC(60*m)
#define TIME_HOUR(h) 	TIME_MIN(60*h)
#define TO_NANO_SECONDS(x) (x)
#endif

#if FOXVERSION >= FXVERSION(1,7,12)
#define GMStringVal(str) FXString::value(str)
#define GMStringFormat  FXString::value
#define GMFloatVal(str) str.toFloat()
#define GMIntVal(str) str.toInt()
#else
#define GMStringVal(str) FXStringVal(str)
#define GMStringFormat  FXStringFormat
#define GMFloatVal FXFloatVal
#define GMIntVal FXIntVal
#endif

/// Branch prediction optimization
#ifndef __likely
#if __GNUC__ >= 3
#define __likely(cond)    __builtin_expect(!!(cond),1)
#define __unlikely(cond)  __builtin_expect(!!(cond),0)
#else
#define __likely(cond)    (!!(cond))
#define __unlikely(cond)  (!!(cond))
#endif
#endif



//#define NO_FXGETTICKS 1
/// Some debugging macros
#if defined DEBUG && !defined(NO_FXGETTICKS)
namespace FX {
  extern FXlong fxgetticks();
  }
#define GM_TICKS_START() FXlong end,start = fxgetticks();
#define GM_TICKS_END()  end = fxgetticks(); fxmessage("%20s:%20s:%15ld ticks.\n",__FILE__,__func__,end-start)
#define GM_DEBUG_PRINT(format, args...) fxmessage (format , ##args)
#else
#define GM_TICKS_START() ((void)0)
#define GM_TICKS_END() ((void)0)
#define GM_DEBUG_PRINT(arguments, args...) ((void)0)
#endif


typedef FXArray<FXString>     FXStringList;
typedef FXArray<FXint>        FXIntList;
typedef FXArray<FXlong>       FXLongList;
typedef FXAutoPtr<FXCursor>   FXCursorPtr;
typedef FXAutoPtr<FXIcon>     FXIconPtr;
typedef FXAutoPtr<FXImage>    FXImagePtr;
typedef FXAutoPtr<FXMenuPane> FXMenuPtr;
typedef FXAutoPtr<FXFont>     FXFontPtr;
typedef FXAutoPtr<FXPopup>    FXPopupPtr;

extern const FXchar * fxtr(const FXchar *) __attribute__ ((format_arg(1)));

#define notr(x) x
#define fxtrformat(x) fxtr(x)

#endif

