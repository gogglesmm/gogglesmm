/********************************************************************************
*                                                                               *
*                     FOX Definitions, Types, and Macros                        *
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


// Byte order
#if !defined(FOX_BIGENDIAN)
#if defined(__GNUC__)
#if defined(__BYTE_ORDER__)
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define FOX_BIGENDIAN 0
#else
#define FOX_BIGENDIAN 1
#endif
#else
#error "FOX_BIGENDIAN macro not set"
#endif
#elif defined(_MSC_VER)
#define FOX_BIGENDIAN 0
#else
#error "FOX_BIGENDIAN macro not set"
#endif
#endif


// Shared library support
#ifdef WIN32
#if defined(__GNUC__)
#define FXLOCAL
#define FXEXPORT __attribute__ ((dllexport))
#define FXIMPORT __attribute__ ((dllimport))
#else
#define FXLOCAL
#define FXEXPORT __declspec(dllexport)
#define FXIMPORT __declspec(dllimport)
#endif
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

// Unreachable part of code
#if defined(__GNUC__) && (__GNUC__ >= 4)
#define __unreachable()    __builtin_unreachable()
#elif defined(_MSC_VER)
#define __unreachable()    __assume(false)
#else
#define __unreachable()
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

// Process handle
#if defined(WIN32)
typedef void*                   FXProcessID;
#else
typedef int                     FXProcessID;
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


// Drag and drop data type
#ifdef WIN32
typedef FXushort                FXDragType;
#else
typedef FXID                    FXDragType;
#endif


// Third logic state: unknown/indeterminate
enum { maybe=2 };

// A time in the far, far future
const FXTime forever=FXLONG(9223372036854775807);

// Search modes for search/replace dialogs
enum {
  SEARCH_BACKWARD   = 1,        /// Search backward
  SEARCH_FORWARD    = 2,        /// Search forward
  SEARCH_NOWRAP     = 0,        /// Don't wrap (default)
  SEARCH_WRAP       = 4,        /// Wrap around to start
  SEARCH_EXACT      = 0,        /// Exact match (default)
  SEARCH_IGNORECASE = 8,        /// Ignore case
  SEARCH_REGEX      = 16,       /// Regular expression match
  SEARCH_PREFIX     = 32,       /// Prefix of subject string
  SEARCH_SUFFIX     = 64,       /// Suffix of subject string
  SEARCH_WORDS      = 128       /// Whole words
  };

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
* FXASSERT_STATIC performs a compile time assert (requires C++11 or newer).
* When assertion (which must be const expression) fails, a compile-time
* error message is generated.  Thus, there is no run-time overhead whatsoever.
* In addition, the condition is checked even if code is never executed.
*/
#if (defined(__cplusplus) && (__cplusplus >= 201103L)) || (defined(_MSC_VER) && (_MSC_VER >= 1600))
#define FXASSERT_STATIC(expr) static_assert(expr,#expr)
#else
#define FXASSERT_STATIC(expr) FXASSERT(expr)
#endif


/**
* FXTRACE() allows you to trace the execution of your application
* with any amount of detail desired.
* The trace topic number determines whether a trace command is
* printed to the output.
* When compiling your application for release, all trace statements
* are compiled out, just like FXASSERT.
* A statement like: FXTRACE((10,"The value of x=%d\n",x)) will
* generate output only if the trace topic 10 is enabled.
* Note the double parentheses!
* Trace topics may be set by command line parameter "-tracetopics"
* followed by a comma-separeted list of topic ranges.  For example,
* parameter "-tracetopics 1000:1023,0:3" selects topics 1000 through
* 1023, and topics 0 through 3.
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

/// Aliasing cast pointer to other type
template <typename to_type>
static inline to_type* alias_cast(void* ptr){
  union UNI { to_type dst[1]; };
  return reinterpret_cast<UNI*>(ptr)->dst;
  }

/// Aliasing cast const-pointer to other type
template <typename to_type>
static inline const to_type* alias_cast(const void* ptr){
  union UNI { to_type dst[1]; };
  return reinterpret_cast<const UNI*>(ptr)->dst;
  }

/// Allocate memory
extern FXAPI FXbool fxmalloc(void** ptr,FXuval size);

/// Allocate cleaned memory
extern FXAPI FXbool fxcalloc(void** ptr,FXuval size);

/// Resize memory
extern FXAPI FXbool fxresize(void** ptr,FXuval size);

/// Free memory, resets ptr to NULL afterward
extern FXAPI void fxfree(void** ptr);

/// Duplicate memory
extern FXAPI FXbool fxmemdup(void** ptr,const void* src,FXuval size);

/// Error output routine; will terminate program after writing message
extern FXAPI __noreturn void fxerror(const FXchar* format,...) FX_PRINTF(1,2) ;

/// Warning routine; will continue program after writing message
extern FXAPI void fxwarning(const FXchar* format,...) FX_PRINTF(1,2) ;

/// Log message to [typically] stderr
extern FXAPI void fxmessage(const FXchar* format,...) FX_PRINTF(1,2) ;

/// Assert failed routine:- usually not called directly but called through FXASSERT
extern FXAPI void fxassert(const FXchar* expression,const FXchar* filename,unsigned int lineno);

/// Verify failed routine:- usually not called directly but called through FXVERIFY
extern FXAPI void fxverify(const FXchar* expression,const FXchar* filename,unsigned int lineno);

/// Trace printout routine:- usually not called directly but called through FXTRACE
extern FXAPI void fxtrace(FXuint level,const FXchar* format,...) FX_PRINTF(2,3) ;

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

/// Version number that the library has been compiled with
extern FXAPI const FXuchar fxversion[3];

/// Get trace topic setting
extern FXAPI FXbool getTraceTopic(FXuint topic);

/// Set trace topic on or off
extern FXAPI void setTraceTopic(FXuint topic,FXbool flag=true);

/// Set tracing for all topics up to and including level
extern FXAPI void setTraceLevel(FXuint level,FXbool flag=true);

/// Set trace topics from a string of the form:
///
/// <topic-list>  : <topic-range> [ ',' <topic-range> ]*
///
/// <topic-range> : <topic> [':' [ <topic> ]? ]?
///
///               : ':' [<topic> ]?
///
/// <topic>       : <digit> [ <digits> ]*
///
extern FXAPI FXbool setTraceTopics(const FXchar* topics,FXbool flag=true);

/// Get operating system version string
extern FXAPI FXival fxosversion(FXchar version[],FXival len);


}

#endif
