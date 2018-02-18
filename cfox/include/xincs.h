/********************************************************************************
*                                                                               *
*              F O X   P r i v a t e   I n c l u d e   F i l e s                *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2017 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef XINCS_H
#define XINCS_H


////////////////////  DO NOT INCLUDE THIS PRIVATE HEADER FILE  //////////////////

// Thread safe
#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS
#endif

// GNU extras if we can get them
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// Use 64-bit files
#ifndef WIN32
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#endif


// Basic includes
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <locale.h>
#include <fcntl.h>
#include <sys/stat.h>

// Platform includes
#if defined(WIN32)  /////////////// Windows /////////////////////////////////////

// Windows 2000 is minimum now
#if _WIN32_WINNT < 0x0500
#define _WIN32_WINNT 0x0500
#endif

// Enforce handle types
#ifndef STRICT
#define STRICT 1
#endif

// Skip some stuff
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Common headers
#include <windows.h>            // Core Windows stuff
#include <winspool.h>           // Printer stuff
#include <io.h>                 // For _access()
#ifndef __CYGWIN__
#include <winsock2.h>
#endif
#include <commctrl.h>           // For _TrackMouseEvent
#include <shellapi.h>
#include <imm.h>                // IME
#ifdef UNICODE
#include <wchar.h>              // Wide character support
#endif
#ifndef PROCESS_SUSPEND_RESUME
#define PROCESS_SUSPEND_RESUME 0x0800
#endif
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif
#if (_MSC_VER >= 1400)          // VC++ 2005 or newer
#include <intrin.h>
#endif

// OpenGL includes
#ifdef HAVE_GL_H
#include <GL/gl.h>
#endif
//#ifdef _WIN32
//#include <GL/glext.h>
//#endif
#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif
#ifndef GLAPI
#define GLAPI
#endif
#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif
//#ifdef HAVE_GLU_H
//#include <GL/glu.h>
//#endif

#else ////////////////////////////// Unix ///////////////////////////////////////

// Common headers
#include <grp.h>
#include <pwd.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_UNISTD_H
#include <sys/types.h>
#include <unistd.h>
#endif
#ifdef HAVE_SYS_FILIO_H         // Get FIONREAD on Solaris
#include <sys/filio.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#ifdef HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#ifdef HAVE_NDIR_H
#include <ndir.h>
#endif
#endif
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_EPOLL_H
#include <sys/epoll.h>
#endif
#ifdef HAVE_SYS_TIMERFD_H
#include <sys/timerfd.h>
#endif
#ifdef HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif
#ifdef HAVE_SYS_SHM_H
#include <sys/shm.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_SYS_PSTAT_H
#include <sys/pstat.h>
#endif
#if defined(__APPLE__)
#include <libkern/OSAtomic.h>
#endif
#include <pthread.h>
#ifdef HAVE_SEMAPHORE_H
#include <semaphore.h>
#endif
#if defined(HAVE_PTHREAD_SETAFFINITY_NP)
#if defined(__FreeBSD__)
#include <osreldate.h>
#if __FreeBSD_version >= 702000
#include <pthread_np.h>
#include <sys/cpuset.h>
typedef cpuset_t   cpu_set_t;
#endif
#endif
#endif
#if defined(__minix)            // MINIX
#ifdef sleep
#undef sleep                    // We mean sleep not __pthread_sleep
#endif
#ifdef read
#undef read                     // We mean read not __pthread_read
#endif
#ifdef write
#undef write                    // We mean write not __pthread_write
#endif
#ifdef select
#undef select                   // We mean select not __pthread_select
#endif
#endif

// Dynamic library loading
#ifdef HAVE_SHL_LOAD
#include <dl.h>                 // HP-UX
#ifndef	DYNAMIC_PATH
#define DYNAMIC_PATH 0
#endif
#ifndef	BIND_RESTRICTED
#define BIND_RESTRICTED	0
#endif
#else
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>              // POSIX
#endif
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0           // Does not exist on DEC
#endif
#ifndef RTLD_NOLOAD             // Older GLIBC libraries
#define RTLD_NOLOAD 0
#endif
#ifndef RTLD_NOW                // for OpenBSD
#define RTLD_NOW DL_LAZY
#endif

// SSE Intrinsics only if available and turned on
#if ((defined(__GNUC__) || defined(__INTEL_COMPILER)) && (defined(__i386__) || defined(__x86_64__)))
#if defined(HAVE_IMMINTRIN_H)
#include <immintrin.h>
#if defined(__SSE__)
#define FOX_HAS_SSE
#endif
#if defined(__SSE2__)
#define FOX_HAS_SSE2
#endif
#if defined(__SSE3__)
#define FOX_HAS_SSE3
#endif
#if defined(__SSSE3__)
#define FOX_HAS_SSSE3
#endif
#if defined(__SSE4_2__)
#define FOX_HAS_SSE4
#endif
#if defined(__AVX__)
#define FOX_HAS_AVX
#endif
#if defined(__AVX2__)
#define FOX_HAS_AVX2
#endif
#if defined(__FMA__)
#define FOX_HAS_FMA
#endif
#endif
#endif


// X11 includes
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xcms.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#ifdef HAVE_XSHM_H
#include <X11/extensions/XShm.h>
#endif
#ifdef HAVE_XCURSOR_H
#include <X11/Xcursor/Xcursor.h>
#endif
#ifdef HAVE_XFT_H
#include <X11/Xft/Xft.h>
#endif
#ifdef HAVE_XSHAPE_H
#include <X11/extensions/shape.h>
#endif
#ifdef HAVE_XRANDR_H
#include <X11/extensions/Xrandr.h>
#endif
#ifdef HAVE_XFIXES_H
#include <X11/extensions/Xfixes.h>
#endif
#ifdef HAVE_XRENDER_H
#include <X11/extensions/Xrender.h>
#endif
#ifdef HAVE_XINPUT2_H
#include <X11/extensions/XInput2.h>
#endif
#ifndef NO_XIM
#ifndef XlibSpecificationRelease        // Not defined until X11R5
#define NO_XIM
#elif XlibSpecificationRelease < 6      // Need at least Xlib X11R6
#define NO_XIM
#endif
#endif

// OpenGL includes
#ifdef HAVE_GL_H
#ifndef SUN_OGL_NO_VERTEX_MACROS
#define SUN_OGL_NO_VERTEX_MACROS
#endif
#ifndef HPOGL_SUPPRESS_FAST_API
#define HPOGL_SUPPRESS_FAST_API
#endif
#include <GL/gl.h>
#ifdef HAVE_GLX_H
#include <GL/glx.h>
#endif
#endif
//#ifdef HAVE_GLU_H
//#include <GL/glu.h>
//#endif

#endif //////////////////////////////////////////////////////////////////////////

// Maximum path length
#ifndef MAXPATHLEN
#if defined(PATH_MAX)
#define MAXPATHLEN   PATH_MAX
#elif defined(_MAX_PATH)
#define MAXPATHLEN   _MAX_PATH
#elif defined(MAX_PATH)
#define MAXPATHLEN   MAX_PATH
#else
#define MAXPATHLEN   2048
#endif
#endif

// Maximum host name length
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

// Some systems don't have it
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

// Remove crap
#ifdef VOID
#undef VOID
#endif

#endif
