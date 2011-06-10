#ifndef FOX_H
#define FOX_H

#include <new>
#include <fx.h>
#include <FXThread.h>
#include <FXMemMap.h>

#ifndef BadHandle
#ifdef WIN32
#define BadHandle INVALID_HANDLE_VALUE
#else
#define BadHandle -1
#endif
#endif

#define FOXVERSION ((FOX_LEVEL) + (FOX_MINOR*1000) + (FOX_MAJOR*100000))
#define FXVERSION(major,minor,release) ((release)+(minor*1000)+(major*100000))

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


#endif

