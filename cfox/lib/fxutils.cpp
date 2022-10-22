/********************************************************************************
*                                                                               *
*                          U t i l i t y   F u n c t i o n s                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxchar.h"
#include "fxmath.h"
#include "fxendian.h"
#include "fxascii.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXElement.h"


/*
  Notes:
  - Handy global utility functions.
*/


// Initial space allocated for message; it will grow if needed
#ifndef INITIALMESSAGESIZE
#define INITIALMESSAGESIZE 512
#endif


using namespace FX;

/*******************************************************************************/

namespace FX {


// Furnish our own versions
extern FXAPI FXuint __strtoul(const FXchar *beg,const FXchar** end=nullptr,FXint base=0,FXbool* ok=nullptr);

// Allows GNU autoconfigure to find FOX
extern "C" FXAPI void fxfindfox(void){ }


// Version number that the library has been compiled with
const FXuchar fxversion[3]={FOX_MAJOR,FOX_MINOR,FOX_LEVEL};

/*******************************************************************************/

#ifdef WIN32

// Return true if console application
FXbool fxisconsole(const FXchar *path){
  IMAGE_OPTIONAL_HEADER optional_header;
  IMAGE_FILE_HEADER     file_header;
  IMAGE_DOS_HEADER      dos_header;
  DWORD                 dwCoffHeaderOffset;
  DWORD                 dwNewOffset;
  DWORD                 dwMoreDosHeader[16];
  ULONG                 ulNTSignature;
  HANDLE                hImage;
  DWORD                 dwBytes;
  FXbool                flag=false;     // Assume false on Windows is safest!

  // Open the application file.
  hImage=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(hImage!=INVALID_HANDLE_VALUE){

    // Read MS-Dos image header.
    if(ReadFile(hImage,&dos_header,sizeof(IMAGE_DOS_HEADER),&dwBytes,nullptr)==0) goto x;

    // Test bytes read
    if(dwBytes!=sizeof(IMAGE_DOS_HEADER)) goto x;

    // Test signature
    if(dos_header.e_magic!=IMAGE_DOS_SIGNATURE) goto x;

    // Read more MS-Dos header.
    if(ReadFile(hImage,dwMoreDosHeader,sizeof(dwMoreDosHeader),&dwBytes,nullptr)==0) goto x;

    // Test bytes read
    if(dwBytes!=sizeof(dwMoreDosHeader)) goto x;

    // Move the file pointer to get the actual COFF header.
    dwNewOffset=SetFilePointer(hImage,dos_header.e_lfanew,nullptr,FILE_BEGIN);
    dwCoffHeaderOffset=dwNewOffset+sizeof(ULONG);
    if(dwCoffHeaderOffset==0xFFFFFFFF) goto x;

    // Read NT signature of the file.
    if(ReadFile(hImage,&ulNTSignature,sizeof(ULONG),&dwBytes,nullptr)==0) goto x;

    // Test bytes read
    if(dwBytes!=sizeof(ULONG)) goto x;

    // Test NT signature
    if(ulNTSignature!=IMAGE_NT_SIGNATURE) goto x;

    if(ReadFile(hImage,&file_header,IMAGE_SIZEOF_FILE_HEADER,&dwBytes,nullptr)==0) goto x;

    // Test bytes read
    if(dwBytes!=IMAGE_SIZEOF_FILE_HEADER) goto x;

    // Read the optional header of file.
    if(ReadFile(hImage,&optional_header,sizeof(IMAGE_OPTIONAL_HEADER),&dwBytes,nullptr)==0) goto x;

    // Test bytes read
    if(dwBytes!=sizeof(IMAGE_OPTIONAL_HEADER)) goto x;

    // Switch on systems
    switch(optional_header.Subsystem){
      case IMAGE_SUBSYSTEM_WINDOWS_GUI:     // Windows GUI (2)
      case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:  // Windows CE GUI (9)
        flag=false;
        break;
      case IMAGE_SUBSYSTEM_WINDOWS_CUI:     // Windows Console (3)
      case IMAGE_SUBSYSTEM_OS2_CUI:         // OS/2 Console (5)
      case IMAGE_SUBSYSTEM_POSIX_CUI:       // Posix Console (7)
        flag=true;
        break;
      case IMAGE_SUBSYSTEM_NATIVE:          // Native (1)
      case IMAGE_SUBSYSTEM_NATIVE_WINDOWS:  // Native Win9x (8)
      case IMAGE_SUBSYSTEM_UNKNOWN:         // Unknown (0)
      default:
        flag=false;
        break;
      }
x:  CloseHandle(hImage);
    }
  return flag;
  }

#else

// Return true if console application
FXbool fxisconsole(const FXchar*){
  return true;
  }

#endif

/*******************************************************************************/

// Log message to [typically] stderr
void fxmessage(const FXchar* format,...){
  FXString message('\0',INITIALMESSAGESIZE);
  va_list arguments;
  va_start(arguments,format);
  message.vformat(format,arguments);
  va_end(arguments);
#ifdef WIN32
#ifdef _WINDOWS
  OutputDebugStringA(message.text());
  fputs(message.text(),stderr);         // if a console is available
  fflush(stderr);
#else
  fputs(message.text(),stderr);
  fflush(stderr);
#endif
#else
  fputs(message.text(),stderr);
  fflush(stderr);
#endif
  }


// Error routine
void fxerror(const FXchar* format,...){
  FXString message('\0',INITIALMESSAGESIZE);
  va_list arguments;
  va_start(arguments,format);
  message.vformat(format,arguments);
  va_end(arguments);
#ifdef WIN32
#ifdef _WINDOWS
  OutputDebugStringA(message.text());
  fputs(message.text(),stderr);         // if a console is available
  fflush(stderr);
  MessageBoxA(nullptr,message.text(),nullptr,MB_OK|MB_ICONEXCLAMATION|MB_APPLMODAL);
  DebugBreak();
#else
  fputs(message.text(),stderr);
  fflush(stderr);
  abort();
#endif
#else
  fputs(message.text(),stderr);
  fflush(stderr);
  abort();
#endif
  }


// Warning routine
void fxwarning(const FXchar* format,...){
  FXString message('\0',INITIALMESSAGESIZE);
  va_list arguments;
  va_start(arguments,format);
  message.vformat(format,arguments);
  va_end(arguments);
#ifdef WIN32
#ifdef _WINDOWS
  OutputDebugStringA(message.text());
  fputs(message.text(),stderr);         // if a console is available
  fflush(stderr);
  MessageBoxA(nullptr,message.text(),nullptr,MB_OK|MB_ICONINFORMATION|MB_APPLMODAL);
#else
  fputs(message.text(),stderr);
  fflush(stderr);
#endif
#else
  fputs(message.text(),stderr);
  fflush(stderr);
#endif
  }

/*******************************************************************************/

// Assert failed routine
void fxassert(const FXchar* expression,const FXchar* filename,unsigned int lineno){
#ifdef WIN32
  fxmessage("%s(%d): FXASSERT(%s) failed.\n",filename,lineno,expression);
#else
  if(isatty(fileno(stderr))){
    fxmessage("%s:%d: \033[1;33mFXASSERT(%s)\033[0m failed.\n",filename,lineno,expression);
    }
  else{
    fxmessage("%s:%d: FXASSERT(%s) failed.\n",filename,lineno,expression);
    }
#endif
  }


// Verify failed routine
void fxverify(const FXchar* expression,const FXchar* filename,unsigned int lineno){
#ifdef WIN32
  fxmessage("%s(%d): FXVERIFY(%s) failed.\n",filename,lineno,expression);
#else
  if(isatty(fileno(stderr))){
    fxmessage("%s:%d: \033[1;33mFXVERIFY(%s)\033[0m failed.\n",filename,lineno,expression);
    }
  else{
    fxmessage("%s:%d: FXVERIFY(%s) failed.\n",filename,lineno,expression);
    }
#endif
  }

/*******************************************************************************/

// Trace level environment variable
static const FXchar* fxTraceVariable=nullptr;

// Room for lots of topics
static FXuchar fxTopicArray[1024];


// Set trace topic
void setTraceTopic(FXuint topic,FXbool flag){
  fxTopicArray[topic&(ARRAYNUMBER(fxTopicArray)-1)]=flag;
  }


// Get trace topic setting
FXbool getTraceTopic(FXuint topic){
  return fxTopicArray[topic&(ARRAYNUMBER(fxTopicArray)-1)];
  }


// Set trace level
void setTraceLevel(FXuint level,FXbool flag){
  level=FXMIN(level,ARRAYNUMBER(fxTopicArray)-1);
  fillElms(fxTopicArray,0,ARRAYNUMBER(fxTopicArray));
  fillElms(fxTopicArray,flag,level+1);
  }


// Parse trace topics from string of the form:
//
// <topic-list>  : <topic-range> [ ',' <topic-range> ]*
//
// <topic-range> : <topic> [':' [ <topic> ]? ]?
//
//               : ':' [<topic> ]?
//
// <topic>       : <digit> [ <digits> ]*
//
FXbool setTraceTopics(const FXchar* topics,FXbool flag){
  if(__likely(topics)){
    FXuint f,t;
    while((*topics==':') || ('0'<=*topics && *topics<='9')){
      f=0;
      while('0'<=*topics && *topics<='9'){
        f=f*10+*topics++-'0';
        }
      f=FXMIN(f,ARRAYNUMBER(fxTopicArray)-1);
      t=f;
      if(*topics==':'){
        topics++;
        t=ARRAYNUMBER(fxTopicArray)-1;
        if('0'<=*topics && *topics<='9'){
          t=0;
          while('0'<=*topics && *topics<='9'){
            t=t*10+*topics++-'0';
            }
          t=FXMIN(t,ARRAYNUMBER(fxTopicArray)-1);
          if(f>t) swap(f,t);
          }
        fillElms(&fxTopicArray[f],flag,t-f);
        }
      fxTopicArray[t]=flag;
      if(*topics!=',') break;
      topics++;
      }
    return true;
    }
  return false;
  }


// Trace printout routine
void fxtrace(FXuint level,const FXchar* format,...){
  if(__unlikely(fxTraceVariable==nullptr)){
    const FXchar* str;
    fxTraceVariable="";
    if((str=getenv("FOX_TRACE_TOPICS"))!=nullptr){
      fxTraceVariable=str;
      setTraceTopics(fxTraceVariable,true);
      }
    else if((str=getenv("FOX_TRACE_LEVEL"))!=nullptr){
      fxTraceVariable=str;
      setTraceLevel(__strtoul(fxTraceVariable,nullptr,10),true);
      }
    }
  if(__likely(fxTopicArray[level&(ARRAYNUMBER(fxTopicArray)-1)])){
    FXString message('\0',INITIALMESSAGESIZE);
    va_list arguments;
    va_start(arguments,format);
    message.vformat(format,arguments);
    va_end(arguments);
#ifdef WIN32
#ifdef _WINDOWS
    OutputDebugStringA(message.text());
    fputs(message.text(),stderr);       // if a console is available
    fflush(stderr);
#else
    fputs(message.text(),stderr);
    fflush(stderr);
#endif
#else
    fputs(message.text(),stderr);
    fflush(stderr);
#endif
    }
  }

/*******************************************************************************/

// Safe string copy
FXival fxstrlcpy(FXchar* dst,const FXchar* src,FXival len){
  const FXchar* s=src;
  FXchar* d=dst;
  FXival n=len;
  FXchar c;
  while((c=*s)!='\0'){
    if(1<n){ *d++=c; n--; }
    s++;
    }
  if(0<n){
    *d='\0';
    }
  return s-src;
  }


// Safe string concat
FXival fxstrlcat(FXchar* dst,const FXchar* src,FXival len){
  const FXchar* s=src;
  FXchar* d=dst;
  FXival n=len;
  FXival m;
  FXchar c;
  while(0<n && *d!='\0'){
    d++;
    n--;
    }
  m=d-dst;
  while((c=*s)!='\0'){
    if(1<n){ *d++=c; n--; }
    s++;
    }
  if(0<n){
    *d='\0';
    }
  return (s-src)+m;
  }

/*******************************************************************************/

// Convert string of length len to MSDOS; return new string and new length
FXbool fxtoDOS(FXchar*& string,FXint& len){
  FXint f=0,t=0;
  while(f<len){
    if(string[f++]=='\n') t++;
    t++;
    }
  if(resizeElms(string,t+1)){
    len=t;
    while(0<t){
      if((string[--t]=string[--f])=='\n') string[--t]='\r';
      }
    string[len]='\0';
    return true;
    }
  return false;
  }


// Convert string of length len from MSDOS; return new string and new length
FXbool fxfromDOS(FXchar*& string,FXint& len){
  FXint f=0,t=0,c;
  while(f<len){
    if((c=string[f++])!='\r') string[t++]=c;
    }
  if(resizeElms(string,t+1)){
    len=t;
    string[len]='\0';
    return true;
    }
  return false;
  }


extern FXAPI FILE *fxopen(const char *filename,const char *mode);

FILE *fxopen(const char *filename,const char *mode){
#if defined(WIN32) && defined(UNICODE)
  FXnchar unifile[MAXPATHLEN],unimode[8];
  utf2ncs(unifile,filename,ARRAYNUMBER(unifile));
  utf2ncs(unimode,mode,ARRAYNUMBER(unimode));
  return _wfopen(unifile,unimode);
#else
  return fopen(filename,mode);
#endif
  }

extern FXAPI FILE *fxreopen(const char *filename,const char *mode,FILE * stream);

FILE *fxreopen(const char *filename,const char *mode,FILE * stream){
#if defined(WIN32) && defined(UNICODE)
  FXnchar unifile[MAXPATHLEN],unimode[8];
  utf2ncs(unifile,filename,ARRAYNUMBER(unifile));
  utf2ncs(unimode,mode,ARRAYNUMBER(unimode));
  return _wfreopen(unifile,unimode,stream);
#else
  return freopen(filename,mode,stream);
#endif
  }


/*******************************************************************************/

// Convert RGB to HSV
void fxrgb_to_hsv(FXfloat& h,FXfloat& s,FXfloat& v,FXfloat r,FXfloat g,FXfloat b){
  FXfloat mx=FXMAX3(r,g,b);
  FXfloat mn=FXMIN3(r,g,b);
  FXfloat d=mx-mn;
  h=0.0f;
  s=0.0f;
  v=mx;
  if(__likely(mx>0.0f)){
    s=d/mx;
    if(__likely(s>0.0f)){
      if(r==mx) h=(g-b)/d;
      else if(g==mx) h=2.0f+(b-r)/d;
      else if(b==mx) h=4.0f+(r-g)/d;
      h*=60.0f;
      if(h<0.0f) h+=360.0f;
      }
    }
  }


// Convert HSV to RGB
void fxhsv_to_rgb(FXfloat& r,FXfloat& g,FXfloat& b,FXfloat h,FXfloat s,FXfloat v){
  FXfloat f,w,q,t;
  FXint i;
  r=g=b=v;
  if(__likely(s>0.0f)){
    if(h==360.0f) h=0.0f;
    else h=h/60.0f;
    i=(FXint)h;
    f=h-i;
    w=v*(1.0f-s);
    q=v*(1.0f-(s*f));
    t=v*(1.0f-(s*(1.0f-f)));
    switch(i){
      case 0: r=v; g=t; b=w; break;
      case 1: r=q; g=v; b=w; break;
      case 2: r=w; g=v; b=t; break;
      case 3: r=w; g=q; b=v; break;
      case 4: r=t; g=w; b=v; break;
      case 5: r=v; g=w; b=q; break;
      }
    }
  }


// Convert RGB to HSL
void fxrgb_to_hsl(FXfloat& h,FXfloat& s,FXfloat& l,FXfloat r,FXfloat g,FXfloat b){
  FXfloat mx=FXMAX3(r,g,b);
  FXfloat mn=FXMIN3(r,g,b);
  FXfloat d=mx-mn;
  h=0.0f;
  s=0.0f;
  l=(mx+mn)*0.5f;
  if(__likely(d>0.0f)){
    if(l<=0.5f)
      s=d/(mx+mn);
    else
      s=d/(2.0f-mx-mn);
    if(r==mx) h=(g-b)/d;
    else if(g==mx) h=2.0f+(b-r)/d;
    else if(b==mx) h=4.0f+(r-g)/d;
    h*=60.0f;
    if(h<0.0f) h+=360.0f;
    }
  }


// Convert HSL to RGB
void fxhsl_to_rgb(FXfloat& r,FXfloat& g,FXfloat& b,FXfloat h,FXfloat s,FXfloat l){
  FXfloat f,v,min,mid1,mid2,sv,vsf;
  FXint i;
  r=g=b=l;
  if(l<0.5f)
    v=l+l*s;
  else
    v=l+s-l*s;
  if(v<=0.0f){
    r=g=b=0.0f;
    }
  else{
    if(h==360.0f) h=0.0f;
    else h=h/60.0f;
    min=2.0f*l-v;
    sv=(v-min)/v;
    i=(FXint)h;
    f=h-i;
    vsf=v*sv*f;
    mid1=min+vsf;
    mid2=v-vsf;
    switch(i){
      case 0: r=v; g=mid1; b=min; break;
      case 1: r=mid2; g=v; b=min; break;
      case 2: r=min; g=v; b=mid1; break;
      case 3: r=min; g=mid2; b=v; break;
      case 4: r=mid1; g=min; b=v; break;
      case 5: r=v; g=min; b=mid2; break;
      }
    }
  }


}

