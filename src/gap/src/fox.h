#ifndef FOX_H
#define FOX_H

#include <new>

// Basic includes
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

// FOX defines
#include <fxver.h>

#define FOXVERSION ((FOX_LEVEL) + (FOX_MINOR*1000) + (FOX_MAJOR*100000))
#define FXVERSION(major,minor,release) ((release)+(minor*1000)+(major*100000))

#include <fxdefs.h>
#include <fxendian.h>
#include <fxascii.h>
#include <fxunicode.h>

// Containers
#include <FXAutoPtr.h>
#include <FXPtrList.h>
#include <FXElement.h>
#include <FXArray.h>
#include <FXString.h>

// Threading
#include <FXMutex.h>
#include <FXCondition.h>
#include <FXAutoThreadStorageKey.h>
#include <FXThread.h>

// IO
#include <FXIO.h>
#include <FXIODevice.h>
#include <FXFile.h>
#include <FXMemMap.h>

// Events
#include <FXSize.h>
#include <FXRectangle.h>
#include <FXEvent.h>

// System
#include <FXPath.h>
#include <FXSystem.h>
#include <FXStat.h>
#include <FXURL.h>
#include <FXDLL.h>

// FXObject based classes
#include <FXHash.h>
#include <FXStream.h>
#include <FXObject.h>
#include <FXDict.h>
#include <FXStringDict.h>
#include <FXSettings.h>
#include <FXMessageChannel.h>

using namespace FX;











#ifndef BadHandle
#ifdef WIN32
#define BadHandle INVALID_HANDLE_VALUE
#else
#define BadHandle -1
#endif
#endif


typedef FXArray<FXString> FXStringList;

/// Some debugging macros
#ifdef DEBUG
#define GM_TICKS_START() FXTime end,start = fxgetticks();
#define GM_TICKS_END()  end = fxgetticks(); fxmessage("%20s:%15ld ticks.\n",__func__,end-start)
#define GM_DEBUG_PRINT(format, args...) fxmessage (format , ##args)
#else
#define GM_TICKS_START() ((void)0)
#define GM_TICKS_END() ((void)0)
#define GM_DEBUG_PRINT(arguments, args...) ((void)0)
#endif

#define TIME_MSEC(ms) (1000000LL*ms)
#define TIME_SEC(s) 	(1000000000LL*s)
#define TIME_MIN(m) 	TIME_SEC(60*m)
#define TIME_HOUR(h) 	TIME_MIN(60*h)

#endif

