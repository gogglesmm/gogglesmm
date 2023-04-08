/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2021 by Sander Jansen. All Rights Reserved      *
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

#ifdef HAVE_OPENGL
#include <epoxy/gl.h>
#include <epoxy/glx.h>
#include <fx3d.h>
#endif

#include <FXArray.h>
#include <FXTextCodec.h>
#include <FXAutoPtr.h>
#include "fxext.h"

/// for locale_t definition
#include <locale.h>

#define TIME_MSEC(ms) (1000000LL*ms)
#define TIME_SEC(s) 	(1000000000LL*s)
#define TIME_MIN(m) 	TIME_SEC(60*m)
#define TIME_HOUR(h) 	TIME_MIN(60*h)

#define NANOSECONDS_PER_SECOND  1000000000LL
#define NANOSECONDS_PER_MICROSECOND 1000LL
#define NANOSECONDS_PER_MILLISECOND 1000000LL

constexpr FXTime operator"" _s(unsigned long long int value)
{
  return value * NANOSECONDS_PER_SECOND;
}

constexpr FXTime operator"" _ms(unsigned long long int value)
{
  return value * NANOSECONDS_PER_MILLISECOND;
}

#if FOXVERSION >= FXVERSION(1, 7, 82)
#define fxgetticks FXThread::ticks
#endif


//#define NO_FXGETTICKS 1
/// Some debugging macros
#if defined DEBUG && !defined(NO_FXGETTICKS)
#define GM_TICKS_START() FXTime end,start = fxgetticks();
#define GM_TICKS_END()  end = fxgetticks(); fxmessage("%20s:%20s:%15ld ticks.\n",__FILE__,__func__,end-start)
#define GM_DEBUG_PRINT(format, ...) fxmessage (format ,##__VA_ARGS__)
#else
#define GM_TICKS_START() ((void)0)
#define GM_TICKS_END() ((void)0)
#define GM_DEBUG_PRINT(arguments, ...) ((void)0)
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

extern const FXchar * fxtr(const FXchar *) FX_FORMAT(1);

#define notr(x) x
#define fxtrformat(x) fxtr(x)

// Best Image Scaler in FXImage
#define FOX_SCALE_BEST 2


#if FOXVERSION < FXVERSION(1, 7, 80)
#define followUTF8 FXISFOLLOWUTF8
#define leadUTF16 FXISLEADUTF16
#define followUTF16 FXISFOLLOWUTF16
#define seqUTF16 FXISSEQUTF16
#endif

#endif
