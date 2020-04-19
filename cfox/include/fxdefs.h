/********************************************************************************
*                                                                               *
*                     FOX Definitions, Types, and Macros                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2019 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXDEFS_H
#define FXDEFS_H



/********************************  Definitions  ********************************/

// Placement new
#include <new>


// Path separator
#ifdef WIN32
#define PATHSEP '\\'
#define PATHSEPSTRING "\\"
#define PATHLISTSEP ';'
#define PATHLISTSEPSTRING ";"
#define ISPATHSEP(c) ((c)=='\\' || (c)=='/')
#else
#define PATHSEP '/'
#define PATHSEPSTRING "/"
#define PATHLISTSEP ':'
#define PATHLISTSEPSTRING ":"
#define ISPATHSEP(c) ((c)=='/')
#endif


// End Of Line
#ifdef WIN32
#define ENDLINE "\r\n"
#else
#define ENDLINE "\n"
#endif


// For Windows
#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif
#ifdef _NDEBUG
#ifndef NDEBUG
#define NDEBUG
#endif
#endif


// Shared library support
#ifdef WIN32
#define FXLOCAL
#define FXEXPORT __declspec(dllexport)
#define FXIMPORT __declspec(dllimport)
#else
#if defined(__GNUC__) && (__GNUC__ >= 4)
#define FXLOCAL  __attribute__((visibility("hidden")))
#define FXEXPORT __attribute__((visibility("default")))
#define FXIMPORT __attribute__((visibility("default")))
#else
#define FXLOCAL
#define FXEXPORT
#define FXIMPORT
#endif
#endif


// Define FXAPI for DLL builds
#ifdef FOXDLL
#ifdef FOXDLL_EXPORTS
#define FXAPI FXEXPORT
#define FXTEMPLATE_EXTERN
#else
#define FXAPI FXIMPORT
#define FXTEMPLATE_EXTERN extern
#endif
#else
#define FXAPI
#define FXTEMPLATE_EXTERN
#endif


// Data alignment attribute
#if defined(__GNUC__)
#define __align(x)    __attribute__((aligned(x)))
#elif defined(_MSC_VER)
#define __align(x)    __declspec(align(x))
#else
#define __align(x)
#endif

// Get alignment of pointer p to b=2^n bytes, returning 0..b-1
#define __alignment(p,b)   (((FXival)(p))&((b)-1))

// Check if pointer p is aligned to b=2^n bytes
#define __isaligned(p,b)   (__alignment(p,b)==0)

// Align pointer to b=2^n bytes
#define __alignto(p,b)     ((void*)((((FXival)(p))+((FXival)((b)-1)))&~((FXival)((b)-1))))


// Thread-local storage attribute
#if defined(__GNUC__)
#define __threadlocal   __thread
#elif defined(_MSC_VER)
#define __threadlocal   __declspec(thread)
#else
#define __threadlocal
#endif


// Non-returning function
#if defined(__GNUC__)
#define __noreturn      __attribute__((__noreturn__))
#elif (_MSC_VER >= 1400)
#define __noreturn      __declspec(noreturn)
#else
#define __noreturn
#endif

// Branch prediction optimization
#if (__GNUC__ >= 3)
#define __likely(cond)    __builtin_expect(!!(cond),1)
#define __unlikely(cond)  __builtin_expect(!!(cond),0)
#else
#define __likely(cond)    (!!(cond))
#define __unlikely(cond)  (!!(cond))
#endif


// Prefetch address
#if (__GNUC__ >= 4) && (defined(__i386__) || defined(__x86_64__))
#define __prefetch(addr)   __builtin_prefetch((addr),0)
#define __prefetchw(addr)  __builtin_prefetch((addr),1)
#else
#define __prefetch(addr)
#define __prefetchw(addr)
#endif


// Standard call calling sequence
#ifdef WIN32
#ifndef CALLBACK
#define CALLBACK __stdcall
#endif
#endif


// C Language calling sequence
#ifdef WIN32
#ifndef CDECL
#define CDECL __cdecl
#endif
#else
#ifndef CDECL
#define CDECL
#endif
#endif


// Checking printf and scanf format strings
#if defined(_CC_GNU_) || defined(__GNUG__) || defined(__GNUC__)
#define FX_PRINTF(fmt,arg) __attribute__((format(printf,fmt,arg)))
#define FX_SCANF(fmt,arg)  __attribute__((format(scanf,fmt,arg)))
#define FX_FORMAT(arg) __attribute__((format_arg(arg)))
#else
#define FX_PRINTF(fmt,arg)
#define FX_SCANF(fmt,arg)
#define FX_FORMAT(arg)
#endif


// Word size issues
#if defined(_MSC_VER) || defined(__MINGW32__) // Windows
#if defined(_WIN64)
#define LLP64  1                // Long longs and pointers are 64 bit
#else
#define ILP32  1                // Ints, longs, and pointers are 32 bit
#endif
#elif defined(__LP64__) || defined(_LP64) || (_MIPS_SZLONG == 64) || (__WORDSIZE == 64)
#define LP64   1                // Longs and pointers are 64 bit
#else
#define ILP32  1                // Longs, integers, and pointers are 32 bit
#endif

// Suffixes for 64-bit constants
#if defined(LP64)
#define FXLONG(c)  c ## L       // Long suffix for 64 bit
#define FXULONG(c) c ## UL
#elif defined(_MSC_VER) && (_MSC_VER < 1900)
#define FXLONG(c)  c ## i64     // Special suffix for 64 bit
#define FXULONG(c) c ## ui64
#else
#define FXLONG(c)  c ## LL      // Long long suffix for 64 bit
#define FXULONG(c) c ## ULL
#endif


// Raw event type
#ifdef WIN32
struct tagMSG;
#else
union _XEvent;
#endif


namespace FX {


/// Third logic state: unknown/indeterminate
enum { maybe=2 };


/// Exponent display
enum FXExponent {
  EXP_NEVER=0,                          /// Never use exponential notation
  EXP_ALWAYS=1,                         /// Always use exponential notation
  EXP_AUTO=2                            /// Use exponential notation if needed
  };


/// Search modes for search/replace dialogs
enum {
  SEARCH_BACKWARD   = 1,                            /// Search backward
  SEARCH_FORWARD    = 2,                            /// Search forward
  SEARCH_NOWRAP     = 0,                            /// Don't wrap (default)
  SEARCH_WRAP       = 4,                            /// Wrap around to start
  SEARCH_EXACT      = 0,                            /// Exact match (default)
  SEARCH_IGNORECASE = 8,                            /// Ignore case
  SEARCH_REGEX      = 16,                           /// Regular expression match
  SEARCH_PREFIX     = 32,                           /// Prefix of subject string
  SEARCH_SUFFIX     = 64                            /// Suffix of subject string
  };


/*********************************  Typedefs  **********************************/

// Forward declarations
class   FXObject;
class   FXStream;
class   FXString;


// Streamable types; these are fixed size!
typedef char                    FXchar;
typedef signed char             FXschar;
typedef unsigned char           FXuchar;
typedef bool                    FXbool;
typedef unsigned short          FXushort;
typedef short                   FXshort;
typedef unsigned int            FXuint;
typedef int                     FXint;
typedef float                   FXfloat;
typedef double                  FXdouble;
#if defined(WIN32)
typedef unsigned int            FXwchar;
#if defined(_MSC_VER) && !defined(_NATIVE_WCHAR_T_DEFINED)
typedef unsigned short          FXnchar;
#elif defined(__WATCOMC__) && !defined(_WCHAR_T_DEFINED)
typedef long char               FXnchar;
#else
typedef wchar_t                 FXnchar;
#endif
#else
typedef wchar_t                 FXwchar;
typedef unsigned short          FXnchar;
#endif
#if defined(LP64)
typedef long                    FXlong;
typedef unsigned long           FXulong;
#elif defined(_MSC_VER) && (_MSC_VER < 1900)
typedef __int64                 FXlong;
typedef unsigned __int64        FXulong;
#else
typedef long long               FXlong;
typedef unsigned long long      FXulong;
#endif

// Integral types large enough to hold value of a pointer
#if defined(LP64) || defined(ILP32)     // Long for LP64 and ILP32 models
typedef long                    FXival;
typedef unsigned long           FXuval;
#elif defined(LLP64)                    // Long long for LLP64 models
#if defined(_MSC_VER) && (_MSC_VER < 1900)
typedef __int64                 FXival;
typedef unsigned __int64        FXuval;
#else
typedef long long               FXival;
typedef unsigned long long      FXuval;
#endif
#endif

// Generic void pointer
typedef void*                   FXptr;


// Handle to something in server
#ifdef WIN32
typedef void*                   FXID;
#else
typedef unsigned long           FXID;
#endif

// Time since January 1, 1970 (UTC)
typedef FXlong                  FXTime;

// Pixel type (could be color index)
typedef unsigned long           FXPixel;

// RGBA pixel value
typedef FXuint                  FXColor;

// Hot key
typedef FXuint                  FXHotKey;

// Input source handle type
#ifdef WIN32
typedef void*                   FXInputHandle;
#else
typedef FXint                   FXInputHandle;
#endif

// Thread ID type
#if defined(WIN32)
typedef void*                   FXThreadID;
#else
typedef unsigned long           FXThreadID;
#endif

// Thread-local storage key
typedef FXuval                  FXThreadStorageKey;

// Raw event type
#ifdef WIN32
typedef tagMSG                  FXRawEvent;
#else
typedef _XEvent                 FXRawEvent;
#endif


/// Drag and drop data type
#ifdef WIN32
typedef FXushort                FXDragType;
#else
typedef FXID                    FXDragType;
#endif


/// A time in the far, far future
const FXTime forever=FXLONG(9223372036854775807);


/**********************************  Macros  ***********************************/

/// Get bit b from val
#define FXBIT(val,b)       (((val)>>(b))&1)

/// Abolute value
#define FXABS(val)         (((val)>=0)?(val):-(val))

/// Return 1 if val >= 0 and -1 otherwise
#define FXSGN(val)         (((val)<0)?-1:1)

/// Return 1 if val > 0, -1 if val < 0, and 0 otherwise
#define FXSGNZ(val)        ((val)<0?-1:(val)>0?1:0)

/// Sign-extend bit-field of b bits to 32 bit signed integer
#define FXSGNX(x,b)        (((FXint)((x)<<(32-(b))))>>(32-(b)))

/// Return the maximum of a or b
#define FXMAX(a,b)         (((a)>(b))?(a):(b))

/// Return the minimum of a or b
#define FXMIN(a,b)         (((a)>(b))?(b):(a))

/// Return the minimum of x, y and z
#define FXMIN3(x,y,z)      ((x)<(y)?FXMIN(x,z):FXMIN(y,z))

/// Return the maximum of x, y and z
#define FXMAX3(x,y,z)      ((x)>(y)?FXMAX(x,z):FXMAX(y,z))

/// Return the minimum of x, y, z, and w
#define FXMIN4(x,y,z,w)    (FXMIN(FXMIN(x,y),FXMIN(z,w)))

/// Return the maximum of of x, y, z, and w
#define FXMAX4(x,y,z,w)    (FXMAX(FXMAX(x,y),FXMAX(z,w)))

/// Return minimum and maximum of a, b
#define FXMINMAX(lo,hi,a,b) ((a)<(b)?((lo)=(a),(hi)=(b)):((lo)=(b),(hi)=(a)))

/// Clamp value x to range [lo..hi]
#define FXCLAMP(lo,x,hi)   ((x)<(lo)?(lo):((x)>(hi)?(hi):(x)))

/// Swap a pair of numbers
#define FXSWAP(a,b,t)      ((t)=(a),(a)=(b),(b)=(t))

/// Linear interpolation between a and b, where 0<=f<=1
#define FXLERP(a,b,f)      ((a)+((b)-(a))*(f))

/// Offset of member in a structure
#define STRUCTOFFSET(str,member) (((char *)(&(((str *)0)->member)))-((char *)0))

/// Number of elements in a static array
#define ARRAYNUMBER(array) (sizeof(array)/sizeof(array[0]))

/// Container class of a member class
#define CONTAINER(ptr,str,mem) ((str*)(((char*)(ptr))-STRUCTOFFSET(str,mem)))

/// Make int out of two shorts
#define MKUINT(l,h)        ((((FX::FXuint)(l))&0xffff) | (((FX::FXuint)(h))<<16))

/// Make selector from message type and message id
#define FXSEL(type,id)     ((((FX::FXuint)(id))&0xffff) | (((FX::FXuint)(type))<<16))

/// Get type from selector
#define FXSELTYPE(s)       ((FX::FXushort)(((s)>>16)&0xffff))

/// Get ID from selector
#define FXSELID(s)         ((FX::FXushort)((s)&0xffff))

/// Test if character c is at the start of a utf8 sequence (not a follower byte)
#define FXISUTF8(c)        (((c)&0xC0)!=0x80)

/// Check if c is leader/follower of a utf8 multi-byte sequence
#define FXISLEADUTF8(c)    (((c)&0xC0)==0xC0)
#define FXISFOLLOWUTF8(c)  (((c)&0xC0)==0x80)

/// Check if c is part of a utf8 multi-byte sequence
#define FXISSEQUTF8(c)     (((c)&0x80)==0x80)

/// Number of FXchars in utf8 sequence
#define FXUTF8LEN(c)       (((0xE5000000>>((((FXuchar)(c))>>4)<<1))&3)+1)

/// Test if character c is at start of utf16 sequence (not a follower from surrogate pair)
#define FXISUTF16(c)       (((c)&0xFC00)!=0xDC00)

/// Check if c is leader/follower of a utf16 surrogate pair sequence
#define FXISLEADUTF16(c)   (((c)&0xFC00)==0xD800)
#define FXISFOLLOWUTF16(c) (((c)&0xFC00)==0xDC00)

/// Check if c is part of a utf16 surrogate pair sequence
#define FXISSEQUTF16(c)    (((c)&0xF800)==0xD800)

/// Number of FXnchars in utf16 sequence
#define FXUTF16LEN(c)      (FXISLEADUTF16(c)+1)

/// Test if c is a legal utf32 character
#define FXISUTF32(c)       ((c)<0x110000)

/// Average of two FXColor ca and FXColor cb
#define FXAVGCOLOR(ca,cb)  (((ca)&(cb))+((((ca)^(cb))&0xFEFEFEFE)>>1))


// Definitions for big-endian machines
#if FOX_BIGENDIAN == 1

/// Make RGBA color
#define FXRGBA(r,g,b,a)    (((FX::FXuint)(FX::FXuchar)(a)) | ((FX::FXuint)(FX::FXuchar)(r)<<8) | ((FX::FXuint)(FX::FXuchar)(g)<<16) | ((FX::FXuint)(FX::FXuchar)(b)<<24))

/// Make RGB color
#define FXRGB(r,g,b)       (((FX::FXuint)(FX::FXuchar)(r)<<8) | ((FX::FXuint)(FX::FXuchar)(g)<<16) | ((FX::FXuint)(FX::FXuchar)(b)<<24) | 0x000000ff)

/// Get red value from RGBA color
#define FXREDVAL(rgba)     ((FX::FXuchar)(((rgba)>>8)&0xff))

/// Get green value from RGBA color
#define FXGREENVAL(rgba)   ((FX::FXuchar)(((rgba)>>16)&0xff))

/// Get blue value from RGBA color
#define FXBLUEVAL(rgba)    ((FX::FXuchar)(((rgba)>>24)&0xff))

/// Get alpha value from RGBA color
#define FXALPHAVAL(rgba)   ((FX::FXuchar)((rgba)&0xff))

/// Get component value of RGBA color
#define FXRGBACOMPVAL(rgba,comp) ((FX::FXuchar)(((rgba)>>((comp)<<3))&0xff))

/// Get RGB color from COLORREF
#define FXCOLORREF2RGB(ref) (FX::FXuint)((((ref)<<8)&0xff000000) | (((ref)<<8)&0xff0000) | (((ref)<<8)&0xff00) | 0x000000ff)

/// Get COLORREF from RGB color
#define FXRGB2COLORREF(rgb) (FX::FXuint)((((rgb)>>8)&0xff0000) | (((rgb)>>8)&0xff00) | (((rgb)>>8)&0xff))

#endif


// Definitions for little-endian machines
#if FOX_BIGENDIAN == 0

/// Make RGBA color
#define FXRGBA(r,g,b,a)    (((FX::FXuint)(FX::FXuchar)(a)<<24) | ((FX::FXuint)(FX::FXuchar)(r)<<16) | ((FX::FXuint)(FX::FXuchar)(g)<<8) | ((FX::FXuint)(FX::FXuchar)(b)))

/// Make RGB color
#define FXRGB(r,g,b)       (((FX::FXuint)(FX::FXuchar)(r)<<16) | ((FX::FXuint)(FX::FXuchar)(g)<<8) | ((FX::FXuint)(FX::FXuchar)(b)) | 0xff000000)

/// Get red value from RGBA color
#define FXREDVAL(rgba)     ((FX::FXuchar)(((rgba)>>16)&0xff))

/// Get green value from RGBA color
#define FXGREENVAL(rgba)   ((FX::FXuchar)(((rgba)>>8)&0xff))

/// Get blue value from RGBA color
#define FXBLUEVAL(rgba)    ((FX::FXuchar)((rgba)&0xff))

/// Get alpha value from RGBA color
#define FXALPHAVAL(rgba)   ((FX::FXuchar)(((rgba)>>24)&0xff))

/// Get component value of RGBA color
#define FXRGBACOMPVAL(rgba,comp) ((FX::FXuchar)(((rgba)>>((3-(comp))<<3))&0xff))

/// Get RGB color from COLORREF
#define FXCOLORREF2RGB(ref) (FX::FXuint)((((ref)>>16)&0xff) | ((ref)&0xff00) | (((ref)<<16)&0xff0000) | 0xff000000)

/// Get COLORREF from RGB color
#define FXRGB2COLORREF(rgb) (FX::FXuint)((((rgb)<<16)&0xff0000) | ((rgb)&0xff00) | (((rgb)>>16)&0xff))

#endif


/**
* FXASSERT() prints out a message when the expression fails,
* and nothing otherwise.  Unlike assert(), FXASSERT() will not
* terminate the execution of the application.
* When compiling your application for release, all assertions
* are compiled out; thus there is no impact on execution speed.
*/
#ifndef NDEBUG
#define FXASSERT(exp) (__likely(exp)?((void)0):(void)FX::fxassert(#exp,__FILE__,__LINE__))
#else
#define FXASSERT(exp) ((void)0)
#endif


/**
* FXVERIFY prints out a message when the expression fails,
* and nothing otherwise.
* When compiling your application for release, these messages
* are compiled out, but unlike FXASSERT, FXVERIFY will still execute
* the expression.
*/
#ifndef NDEBUG
#define FXVERIFY(exp) (__likely(exp)?((void)0):(void)FX::fxverify(#exp,__FILE__,__LINE__))
#else
#define FXVERIFY(exp) ((void)(exp))
#endif


/**
* FXTRACE() allows you to trace the execution of your application
* with increasing levels of detail the higher the trace level.
* The trace level is determined by variable fxTraceLevel, which
* may be set from the command line with "-tracelevel <level>".
* When compiling your application for release, all trace statements
* are compiled out, just like FXASSERT.
* A statement like: FXTRACE((10,"The value of x=%d\n",x)) will
* generate output only if fxTraceLevel is set to 11 or greater.
* The default value fxTraceLevel=0 will block all trace outputs.
* Note the double parentheses!
*/
#ifndef NDEBUG
#define FXTRACE(arguments) FX::fxtrace arguments
#else
#define FXTRACE(arguments) ((void)0)
#endif

/**
* Allocate a memory block of no elements of type and store a pointer
* to it into the address pointed to by ptr.
* Return false if size!=0 and allocation fails, true otherwise.
* An allocation of a zero size block returns a NULL pointer.
*/
#define FXMALLOC(ptr,type,no)     (FX::fxmalloc((void **)(ptr),sizeof(type)*(no)))

/**
* Allocate a zero-filled memory block no elements of type and store a pointer
* to it into the address pointed to by ptr.
* Return false if size!=0 and allocation fails, true otherwise.
* An allocation of a zero size block returns a NULL pointer.
*/
#define FXCALLOC(ptr,type,no)     (FX::fxcalloc((void **)(ptr),sizeof(type)*(no)))

/**
* Resize the memory block referred to by the pointer at the address ptr, to a
* hold no elements of type.
* Returns false if size!=0 and reallocation fails, true otherwise.
* If reallocation fails, pointer is left to point to old block; a reallocation
* to a zero size block has the effect of freeing it.
* The ptr argument must be the address where the pointer to the allocated
* block is to be stored.
*/
#define FXRESIZE(ptr,type,no)     (FX::fxresize((void **)(ptr),sizeof(type)*(no)))

/**
* Allocate and initialize memory from another block.
* Return false if size!=0 and source!=NULL and allocation fails, true otherwise.
* An allocation of a zero size block returns a NULL pointer.
* The ptr argument must be the address where the pointer to the allocated
* block is to be stored.
*/
#define FXMEMDUP(ptr,src,type,no) (FX::fxmemdup((void **)(ptr),(const void*)(src),sizeof(type)*(no)))

/**
* Free a block of memory allocated with either FXMALLOC, FXCALLOC, FXRESIZE, or FXMEMDUP.
* It is OK to call free a NULL pointer.  The argument must be the address of the
* pointer to the block to be released.  The pointer is set to NULL to prevent
* any further references to the block after releasing it.
*/
#define FXFREE(ptr)               (FX::fxfree((void **)(ptr)))

/**********************************  Globals  **********************************/

/// Allocate memory
extern FXAPI FXbool fxmalloc(void** ptr,FXuval size);

/// Allocate cleaned memory
extern FXAPI FXbool fxcalloc(void** ptr,FXuval size);

/// Resize memory
extern FXAPI FXbool fxresize(void** ptr,FXuval size);

/// Duplicate memory
extern FXAPI FXbool fxmemdup(void** ptr,const void* src,FXuval size);

/// Free memory, resets ptr to NULL afterward
extern FXAPI void fxfree(void** ptr);

/// Error routine
extern FXAPI __noreturn void fxerror(const FXchar* format,...) FX_PRINTF(1,2) ;

/// Warning routine
extern FXAPI void fxwarning(const FXchar* format,...) FX_PRINTF(1,2) ;

/// Log message to [typically] stderr
extern FXAPI void fxmessage(const FXchar* format,...) FX_PRINTF(1,2) ;

/// Assert failed routine:- usually not called directly but called through FXASSERT
extern FXAPI void fxassert(const FXchar* expression,const FXchar* filename,unsigned int lineno);

/// Verify failed routine:- usually not called directly but called through FXVERIFY
extern FXAPI void fxverify(const FXchar* expression,const FXchar* filename,unsigned int lineno);

/// Trace printout routine:- usually not called directly but called through FXTRACE
extern FXAPI void fxtrace(FXint level,const FXchar* format,...) FX_PRINTF(2,3) ;

/// Convert string of length len to MSDOS; return new string and new length
extern FXAPI FXbool fxtoDOS(FXchar*& string,FXint& len);

/// Convert string of length len from MSDOS; return new string and new length
extern FXAPI FXbool fxfromDOS(FXchar*& string,FXint& len);

/// Duplicate string
extern FXAPI FXchar *fxstrdup(const FXchar* str);

/// Calculate a hash value from a string
extern FXAPI FXuint fxstrhash(const FXchar* str);

/// Safe string copy
extern FXAPI FXival fxstrlcpy(FXchar* dst,const FXchar* src,FXival len);

/// Safe string concat
extern FXAPI FXival fxstrlcat(FXchar* dst,const FXchar* src,FXival len);

/// Convert RGB to HSV
extern FXAPI void fxrgb_to_hsv(FXfloat& h,FXfloat& s,FXfloat& v,FXfloat r,FXfloat g,FXfloat b);

/// Convert HSV to RGB
extern FXAPI void fxhsv_to_rgb(FXfloat& r,FXfloat& g,FXfloat& b,FXfloat h,FXfloat s,FXfloat v);

/// Convert RGB to HSL
extern FXAPI void fxrgb_to_hsl(FXfloat& h,FXfloat& s,FXfloat& l,FXfloat r,FXfloat g,FXfloat b);

/// Convert HSL to RGB
extern FXAPI void fxhsl_to_rgb(FXfloat& r,FXfloat& g,FXfloat& b,FXfloat h,FXfloat s,FXfloat l);

/// Encode src to dst in base64
extern FXchar* fxencode64(FXchar* dst,FXchar* dstend,const FXchar* src,const FXchar* srcend);

/// Decode src to dst from base64
extern FXchar* fxdecode64(FXchar* dst,FXchar* dstend,const FXchar* src,const FXchar* srcend);

/// Encode src to dst in base85
extern FXchar* fxencode85(FXchar* dst,FXchar* dstend,const FXchar* src,const FXchar* srcend);

/// Decode src to dst from base85
extern FXchar* fxdecode85(FXchar* dst,FXchar* dstend,const FXchar* src,const FXchar* srcend);

/// Convert keysym to unicode character
extern FXAPI FXwchar fxkeysym2ucs(FXwchar sym);

/// Convert unicode character to keysym
extern FXAPI FXwchar fxucs2keysym(FXwchar ucs);

/// Parse geometry, a-la X11 geometry specification
extern FXAPI FXint fxparsegeometry(const FXchar *string,FXint& x,FXint& y,FXint& w,FXint& h);

/// True if executable with given path is a console application
extern FXAPI FXbool fxisconsole(const FXchar *path);

/// Return clock ticks from cpu tick-counter
extern FXAPI FXTime fxgetticks();

/// Version number that the library has been compiled with
extern FXAPI const FXuchar fxversion[3];

/// Controls tracing level
extern FXAPI FXint fxTraceLevel;


/// Return wide character from utf8 string at ptr
extern FXAPI FXwchar wc(const FXchar *ptr);

/// Return wide character from utf16 string at ptr
extern FXAPI FXwchar wc(const FXnchar *ptr);


/// Increment to start of next wide character in utf8 string
extern FXAPI const FXchar* wcinc(const FXchar* ptr);

/// Increment to start of next wide character in utf8 string
extern FXAPI FXchar* wcinc(FXchar* ptr);

/// Increment to start of next wide character in utf16 string
extern FXAPI const FXnchar* wcinc(const FXnchar* ptr);

/// Increment to start of next wide character in utf16 string
extern FXAPI FXnchar* wcinc(FXnchar* ptr);

/// Decrement to start of previous wide character in utf8 string
extern FXAPI const FXchar* wcdec(const FXchar* ptr);

/// Decrement to start of previous wide character in utf8 string
extern FXAPI FXchar* wcdec(FXchar* ptr);

/// Decrement to start of previous wide character in utf16 string
extern FXAPI const FXnchar* wcdec(const FXnchar* ptr);

/// Decrement to start of previous wide character in utf16 string
extern FXAPI FXnchar* wcdec(FXnchar* ptr);

/// Adjust ptr to point to leader of multi-byte sequence
extern FXAPI const FXchar* wcstart(const FXchar* ptr);

/// Adjust ptr to point to leader of multi-byte sequence
extern FXAPI FXchar* wcstart(FXchar* ptr);

/// Adjust ptr to point to leader of surrogate pair sequence
extern FXAPI const FXnchar* wcstart(const FXnchar *ptr);

/// Adjust ptr to point to leader of surrogate pair sequence
extern FXAPI FXnchar* wcstart(FXnchar *ptr);

/// Return number of FXchar's of wide character at ptr
extern FXAPI FXival wclen(const FXchar *ptr);

/// Return number of FXnchar's of narrow character at ptr
extern FXAPI FXival wclen(const FXnchar *ptr);

/// Check if valid utf8 wide character representation; returns length or 0
extern FXAPI FXival wcvalid(const FXchar* ptr);

/// Check if valid utf16 wide character representation; returns length or 0
extern FXAPI FXival wcvalid(const FXnchar* ptr);


/// Return number of bytes for utf8 representation of wide character w
extern FXAPI FXival wc2utf(FXwchar w);

/// Return number of narrow characters for utf16 representation of wide character w
extern FXAPI FXival wc2nc(FXwchar w);

/// Return number of bytes for utf8 representation of wide character string
extern FXAPI FXival wcs2utf(const FXwchar* src,FXival srclen);
extern FXAPI FXival wcs2utf(const FXwchar* src);

/// Return number of bytes for utf8 representation of narrow character string
extern FXAPI FXival ncs2utf(const FXnchar* src,FXival srclen);
extern FXAPI FXival ncs2utf(const FXnchar* src);

/// Return number of wide characters for utf8 character string
extern FXAPI FXival utf2wcs(const FXchar src,FXival srclen);
extern FXAPI FXival utf2wcs(const FXchar *src);

/// Return number of narrow characters for utf8 character string
extern FXAPI FXival utf2ncs(const FXchar *src,FXival srclen);
extern FXAPI FXival utf2ncs(const FXchar *src);


/// Convert wide character to utf8 string; return number of items written to dst
extern FXAPI FXival wc2utf(FXchar *dst,FXwchar w);

/// Convert wide character to narrow character string; return number of items written to dst
extern FXAPI FXival wc2nc(FXnchar *dst,FXwchar w);

/// Convert wide character string to utf8 string; return number of items written to dst
extern FXAPI FXival wcs2utf(FXchar *dst,const FXwchar* src,FXival dstlen,FXival srclen);
extern FXAPI FXival wcs2utf(FXchar *dst,const FXwchar* src,FXival dstlen);

/// Convert narrow character string to utf8 string; return number of items written to dst
extern FXAPI FXival ncs2utf(FXchar *dst,const FXnchar* src,FXival dsrlen,FXival srclen);
extern FXAPI FXival ncs2utf(FXchar *dst,const FXnchar* src,FXival dsrlen);

/// Convert utf8 string to wide character string; return number of items written to dst
extern FXAPI FXival utf2wcs(FXwchar *dst,const FXchar* src,FXival dsrlen,FXival srclen);
extern FXAPI FXival utf2wcs(FXwchar *dst,const FXchar* src,FXival dsrlen);

/// Convert utf8 string to narrow character string; return number of items written to dst
extern FXAPI FXival utf2ncs(FXnchar *dst,const FXchar* src,FXival dsrlen,FXival srclen);
extern FXAPI FXival utf2ncs(FXnchar *dst,const FXchar* src,FXival dsrlen);

/// Swap non-overlapping arrays
extern FXAPI void memswap(void* dst,void* src,FXuval n);

}

#endif
