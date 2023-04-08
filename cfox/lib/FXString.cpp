/********************************************************************************
*                                                                               *
*                           S t r i n g   O b j e c t                           *
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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxchar.h"
#include "fxmath.h"
#include "fxascii.h"
#include "fxunicode.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXException.h"


/*
  Notes:
  - The special pointer-value null represents an empty "" string.

  - Strings are never NULL:- this speeds things up a lot as there is no
    need to check for NULL strings anymore.

  - In the new representation, '\0' is allowed as a character everywhere; but there
    is always an (uncounted) '\0' at the end.

  - The length preceeds the text in the buffer.

  - UTF-8 Encoding scheme:

      Hex Range                 Binary                          Encoding
      -----------------------   -----------------------------   -----------------------------------
      U-00000000 - U-0000007F   0000 0000 0000 0000 0xxx xxxx   0xxxxxxx
      U-00000080 - U-000007FF   0000 0000 0000 0yyy yyxx xxxx   110yyyyy 10xxxxxx
      U-00000800 - U-0000FFFF   0000 0000 zzzz yyyy yyxx xxxx   1110zzzz 10yyyyyy 10xxxxxx
      U-00010000 - U-001FFFFF   000u uuzz zzzz yyyy yyxx xxxx   11110uuu 10zzzzzz 10yyyyyy 10xxxxxx

  - UTF-16 Encoding scheme:

      Hex Range                 Binary                          Encoding
      -----------------------   -----------------------------   -----------------------------------
      U-00000000 - U-0000FFFF   0000 0000 xxxx xxxx xxxx xxxx   xxxxxxxx xxxxxxxx
      U-00010000 - U-0010FFFF   000y yyyy yyyy yyxx xxxx xxxx   110110zz zzzzzzzz 110111xx xxxxxxxx

    The range U-D800 - U-DFFF is reserved for surrogate pairs; Leading-surrogates or high-surrogates
    are from U-D800 to U-DBFF, and trailing-surrogates or low-surrogates are from U-DC00 to U-DFFF.
    Surrogates CH and CL are computed as follows for U > 0x10000:

      CH = (U >> 10) + 0xD800
      CL = (U & 0x3FF) + 0xDC00

*/


// The string buffer is always rounded to a multiple of ROUNDVAL
// which must be 2^n.  Thus, small size changes will not result in any
// actual resizing of the buffer except when ROUNDVAL is exceeded.
#define ROUNDVAL    16

// Round up to nearest ROUNDVAL
#define ROUNDUP(n)  (((n)+ROUNDVAL-1)&-ROUNDVAL)

// Special empty string value
#define EMPTY       (const_cast<FXchar*>((const FXchar*)(__string__empty__+1)))

using namespace FX;

/*******************************************************************************/

namespace FX {

// For conversion from UTF16 to UTF32
const FXint SURROGATE_OFFSET=0x10000-(0xD800<<10)-0xDC00;

// For conversion of UTF32 to UTF16
const FXint LEAD_OFFSET=0xD800-(0x10000>>10);

// For conversion of UTF32 to UTF16
const FXint TAIL_OFFSET=0xDC00;


// Empty string
extern const FXint __string__empty__[];
const FXint __string__empty__[2]={0,0};


// Special NULL string
const FXchar FXString::null[4]={0,0,0,0};


// Hexadecimal digit of value
const FXchar FXString::value2Digit[36]={
  '0','1','2','3','4','5','6','7','8','9','A','B',
  'C','D','E','F','G','H','I','J','K','L','M','N',
  'O','P','Q','R','S','T','U','V','W','X','Y','Z',
  };


// Hexadecimal value of digit
const FXschar FXString::digit2Value[256]={
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
  -1,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
  25,26,27,28,29,30,31,32,33,34,35,-1,-1,-1,-1,-1,
  -1,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
  25,26,27,28,29,30,31,32,33,34,35,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
  };


// Furnish our own versions
extern FXAPI FXint __vsscanf(const FXchar* string,const FXchar* format,va_list arg_ptr);
extern FXAPI FXint __vsnprintf(FXchar* string,FXint length,const FXchar* format,va_list args);
extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);
extern FXAPI FXlong __strtoll(const FXchar *beg,const FXchar** end=nullptr,FXint base=0,FXbool* ok=nullptr);
extern FXAPI FXulong __strtoull(const FXchar* beg,const FXchar** end=nullptr,FXint base=0,FXbool* ok=nullptr);
extern FXAPI FXint __strtol(const FXchar *beg,const FXchar** end=nullptr,FXint base=0,FXbool* ok=nullptr);
extern FXAPI FXuint __strtoul(const FXchar* beg,const FXchar** end=nullptr,FXint base=0,FXbool* ok=nullptr);
extern FXAPI FXdouble __strtod(const FXchar *beg,const FXchar** end=nullptr,FXbool* ok=nullptr);
extern FXAPI FXfloat __strtof(const FXchar *beg,const FXchar** end=nullptr,FXbool* ok=nullptr);

/*******************************************************************************/

// Length of character string
static inline FXint strlen(const FXchar *src){
  FXint i=0;
  while(src[i]) i++;
  return i;
  }


// Length of narrow character string
static inline FXint strlen(const FXnchar *src){
  FXint i=0;
  while(src[i]) i++;
  return i;
  }


// Length of wide character string
static inline FXint strlen(const FXwchar *src){
  FXint i=0;
  while(src[i]) i++;
  return i;
  }

/*******************************************************************************/

// Change the length of the string to len
FXbool FXString::length(FXint len){
  if(__likely(len!=length())){
    FXchar *ptr;
    if(0<len){
      if(str==EMPTY){
        ptr=(FXchar*)::malloc(ROUNDUP(1+len)+sizeof(FXint));
        }
      else{
        ptr=(FXchar*)::realloc(str-sizeof(FXint),ROUNDUP(1+len)+sizeof(FXint));
        }
      if(__unlikely(!ptr)) return false;
      str=ptr+sizeof(FXint);
      str[len]=0;
      *(((FXint*)str)-1)=len;
      }
    else if(str!=EMPTY){
      ::free(str-sizeof(FXint));
      str=EMPTY;
      }
    }
  return true;
  }


// Initialize to empty
FXString::FXString():str(EMPTY){
  }


// Construct copy of another string
FXString::FXString(const FXString& s):str(EMPTY){
  if(__likely(length(s.length()))){
    memcpy(str,s.text(),s.length());
    }
  }


// Construct and initialize with string s
FXString::FXString(const FXchar* s):str(EMPTY){
  FXint n;
  if(__likely(s && s[0] && length((n=strlen(s))))){
    memcpy(str,s,n);
    }
  }


// Construct and init from narrow character string
FXString::FXString(const FXnchar* s):str(EMPTY){
  FXint m;
  if(__likely(s && s[0] && length((m=ncs2utf(s))))){
    ncs2utf(str,s,m);
    }
  }


// Construct and init from wide character string
FXString::FXString(const FXwchar* s):str(EMPTY){
  FXint m;
  if(__likely(s && s[0] && length((m=wcs2utf(s))))){
    wcs2utf(str,s,m);
    }
  }


// Construct and init with substring
FXString::FXString(const FXchar* s,FXint n):str(EMPTY){
  if(__likely(s && 0<n && length(n))){
    memcpy(str,s,n);
    }
  }


// Construct and init with narrow character substring
FXString::FXString(const FXnchar* s,FXint n):str(EMPTY){
  FXint m;
  if(__likely(s && 0<n && length((m=ncs2utf(s,n))))){
    ncs2utf(str,s,m,n);
    }
  }


// Construct and init with wide character substring
FXString::FXString(const FXwchar* s,FXint n):str(EMPTY){
  FXint m;
  if(__likely(s && 0<n && length((m=wcs2utf(s,n))))){
    wcs2utf(str,s,m,n);
    }
  }


// Construct and fill with constant
FXString::FXString(FXchar c,FXint n):str(EMPTY){
  if(__likely(0<n && length(n))){
    memset(str,c,n);
    }
  }


// Destructor
FXString::~FXString(){
  if(str!=EMPTY){::free(str-sizeof(FXint));}
  }


// Return wide character starting at p
FXwchar FXString::wc(FXint p) const {
  FXwchar w=(FXuchar)str[p];
  if(0xC0<=w){ w = (w<<6) ^ (FXuchar)str[p+1] ^ 0x3080;
  if(0x800<=w){ w = (w<<6) ^ (FXuchar)str[p+2] ^ 0x20080;
  if(0x10000<=w){ w = (w<<6) ^ (FXuchar)str[p+3] ^ 0x400080; }}}
  return w;
  }


//  Return wide character at p and advance to next
FXwchar FXString::wcnxt(FXint& p) const {
  FXwchar w=(FXuchar)str[p++];
  if(0xC0<=w){ w=(w<<6) ^ (FXuchar)str[p++] ^ 0x3080;
  if(0x800<=w){ w=(w<<6) ^ (FXuchar)str[p++] ^ 0x20080;
  if(0x10000<=w){ w=(w<<6) ^ (FXuchar)str[p++] ^ 0x400080; }}}
  return w;
  }


// Retreat to wide character previous to p and return it
FXwchar FXString::wcprv(FXint& p) const {
  FXwchar w=(FXuchar)str[--p];
  if(0x80<=w){ w = ((FXuchar)str[--p]<<6) ^ w ^ 0x3080;
  if(0x1000<=w){ w = ((FXuchar)str[--p]<<12) ^ w ^ 0xE1000;
  if(0x20000<=w){ w = ((FXuchar)str[--p]<<18) ^ w ^ 0x3C60000; }}}
  return w;
  }


// Increment byte offset by one utf8 character
FXint FXString::inc(FXint p) const {
  return (++p>=length() || isUTF8(str[p]) || ++p>=length() || isUTF8(str[p]) || ++p>=length() || isUTF8(str[p]) || ++p), p;
  }


// Increment byte offset by n utf8 characters
FXint FXString::inc(FXint p,FXint n) const {
  while(p<length() && 0<n){ p=inc(p); --n; }
  return p;
  }


// Decrement byte offset by one utf8 character
FXint FXString::dec(FXint p) const {
  return (--p<=0 || isUTF8(str[p]) || --p<=0 || isUTF8(str[p]) || --p<=0 || isUTF8(str[p]) || --p), p;
  }


// Decrement byte offset by n utf8 characters
FXint FXString::dec(FXint p,FXint n) const {
  while(0<=p && 0<n){ p=dec(p); --n; }
  return p;
  }


// Count number of utf8 characters in subrange start...end
FXint FXString::count(FXint start,FXint end) const {
  FXint cnt=0;
  while(start<end){
    start+=lenUTF8(str[start]);
    cnt++;
    }
  return cnt;
  }


// Count number of utf8 characters
FXint FXString::count() const {
  FXint len=length();
  FXint cnt=0;
  FXint s=0;
  while(s<len){
    s+=lenUTF8(str[s]);
    cnt++;
    }
  return cnt;
  }


// Return byte offset of utf8 character at index
FXint FXString::offset(FXint indx) const {
  FXint len=length();
  FXint i=0;
  FXint p=0;
  while(i<indx && p<len){
    p+=lenUTF8(str[p]);
    i++;
    }
  return p;
  }


// Return index of utf8 character at byte offset
FXint FXString::index(FXint offs) const {
  FXint len=length();
  FXint i=0;
  FXint p=0;
  while(p<offs && p<len){
    p+=lenUTF8(str[p]);
    i++;
    }
  return i;
  }


// Return start of utf8 character containing position
FXint FXString::validate(FXint p) const {
  return (p<=0 || isUTF8(str[p]) || --p<=0 || isUTF8(str[p]) || --p<=0 || isUTF8(str[p]) || --p), p;
  }


// Assign a constant string to this string
FXString& FXString::operator=(const FXchar* s){
  return assign(s);
  }


// Assign a narrow character string to this string
FXString& FXString::operator=(const FXnchar* s){
  return assign(s);
  }


// Assign a wide character string to this string
FXString& FXString::operator=(const FXwchar* s){
  return assign(s);
  }


// Assign another string to this string
FXString& FXString::operator=(const FXString& s){
  if(__likely(str!=s.str)){ assign(s.str,s.length()); }
  return *this;
  }


// Append single character to this string
FXString& FXString::operator+=(FXchar c){
  return append(c);
  }


// Append constant string after this string
FXString& FXString::operator+=(const FXchar* s){
  return append(s);
  }


// Append narrow character string after this string
FXString& FXString::operator+=(const FXnchar* s){
  return append(s);
  }


// Append wide characeter string after this string
FXString& FXString::operator+=(const FXwchar* s){
  return append(s);
  }


// Append another string after this string
FXString& FXString::operator+=(const FXString& s){
  return append(s);
  }


// Adopt string s, leaving s empty
FXString& FXString::adopt(FXString& s){
  if(__likely(str!=s.str)){
    swap(str,s.str);
    s.clear();
    }
  return *this;
  }


// Assign input character to this string
FXString& FXString::assign(FXchar c){
  if(__likely(length(1))){
    str[0]=c;
    }
  return *this;
  }


// Assign input n characters c to this string
FXString& FXString::assign(FXchar c,FXint n){
  if(__likely(length(n))){
    memset(str,c,n);
    }
  return *this;
  }


// Assign input string to this string
FXString& FXString::assign(const FXchar* s){
  FXint m;
  if(__likely(s && s[0] && length((m=strlen(s))))){
    memmove(str,s,m);
    }
  else{
    length(0);
    }
  return *this;
  }


// Assign narrow character string s to this string
FXString& FXString::assign(const FXnchar* s){
  FXint m;
  if(__likely(s && s[0] && length((m=ncs2utf(s))))){
    ncs2utf(str,s,m);
    }
  else{
    length(0);
    }
  return *this;
  }


// Assign wide character string s to this string
FXString& FXString::assign(const FXwchar* s){
  FXint m;
  if(__likely(s && s[0] && length((m=wcs2utf(s))))){
    wcs2utf(str,s,m);
    }
  else{
    length(0);
    }
  return *this;
  }


// Assign first n characters of input string to this string
FXString& FXString::assign(const FXchar* s,FXint n){
  if(__likely(s && 0<n && length(n))){
    memmove(str,s,n);
    }
  else{
    length(0);
    }
  return *this;
  }


// Assign first n characters of narrow character string s to this string
FXString& FXString::assign(const FXnchar* s,FXint n){
  FXint m;
  if(__likely(s && 0<n && length((m=ncs2utf(s,n))))){
    ncs2utf(str,s,m,n);
    }
  else{
    length(0);
    }
  return *this;
  }


// Assign first n characters of wide character string s to this string
FXString& FXString::assign(const FXwchar* s,FXint n){
  FXint m;
  if(__likely(s && 0<n && length((m=wcs2utf(s,n))))){
    wcs2utf(str,s,m,n);
    }
  else{
    length(0);
    }
  return *this;
  }


// Assign input string to this string
FXString& FXString::assign(const FXString& s){
  if(__likely(str!=s.text())){ assign(s.text(),s.length()); }
  return *this;
  }


// Insert character at position
FXString& FXString::insert(FXint pos,FXchar c){
  FXint len=length();
  if(__likely(length(len+1))){
    if(pos<=0){
      memmove(str+1,str,len);
      str[0]=c;
      }
    else if(pos>=len){
      str[len]=c;
      }
    else{
      memmove(str+pos+1,str+pos,len-pos);
      str[pos]=c;
      }
    }
  return *this;
  }


// Insert n characters c at specified position
FXString& FXString::insert(FXint pos,FXchar c,FXint n){
  FXint len=length();
  if(__likely(0<n && length(len+n))){
    if(pos<=0){
      memmove(str+n,str,len);
      memset(str,c,n);
      }
    else if(pos>=len){
      memset(str+len,c,n);
      }
    else{
      memmove(str+pos+n,str+pos,len-pos);
      memset(str+pos,c,n);
      }
    }
  return *this;
  }


// Insert string at position
FXString& FXString::insert(FXint pos,const FXchar* s){
  FXint len=length(),m;
  if(__likely(s && s[0] && length(len+(m=strlen(s))))){
    if(pos<=0){
      memmove(str+m,str,len);
      memcpy(str,s,m);
      }
    else if(pos>=len){
      memcpy(str+len,s,m);
      }
    else{
      memmove(str+pos+m,str+pos,len-pos);
      memcpy(str+pos,s,m);
      }
    }
  return *this;
  }


// Insert narrow character string at position
FXString& FXString::insert(FXint pos,const FXnchar* s){
  FXint len=length(),m;
  if(__likely(s && s[0] && length(len+(m=ncs2utf(s))))){
    if(pos<=0){
      memmove(str+m,str,len);
      ncs2utf(str,s,m);
      }
    else if(pos>=len){
      ncs2utf(str+len,s,m);
      }
    else{
      memmove(str+pos+m,str+pos,len-pos);
      ncs2utf(str+pos,s,m);
      }
    }
  return *this;
  }


// Insert wide character string at position
FXString& FXString::insert(FXint pos,const FXwchar* s){
  FXint len=length(),m;
  if(__likely(s && s[0] && length(len+(m=wcs2utf(s))))){
    if(pos<=0){
      memmove(str+m,str,len);
      wcs2utf(str,s,m);
      }
    else if(pos>=len){
      wcs2utf(str+len,s,m);
      }
    else{
      memmove(str+pos+m,str+pos,len-pos);
      wcs2utf(str+pos,s,m);
      }
    }
  return *this;
  }



// Insert string at position
FXString& FXString::insert(FXint pos,const FXchar* s,FXint n){
  FXint len=length();
  if(__likely(s && 0<n && length(len+n))){
    if(pos<=0){
      memmove(str+n,str,len);
      memcpy(str,s,n);
      }
    else if(pos>=len){
      memcpy(str+len,s,n);
      }
    else{
      memmove(str+pos+n,str+pos,len-pos);
      memcpy(str+pos,s,n);
      }
    }
  return *this;
  }


// Insert narrow character string at position
FXString& FXString::insert(FXint pos,const FXnchar* s,FXint n){
  FXint len=length(),m;
  if(__likely(s && 0<n && length(len+(m=ncs2utf(s,n))))){
    if(pos<=0){
      memmove(str+m,str,len);
      ncs2utf(str,s,m,n);
      }
    else if(pos>=len){
      ncs2utf(str+len,s,m,n);
      }
    else{
      memmove(str+pos+m,str+pos,len-pos);
      ncs2utf(str+pos,s,m,n);
      }
    }
  return *this;
  }


// Insert wide character string at position
FXString& FXString::insert(FXint pos,const FXwchar* s,FXint n){
  FXint len=length(),m;
  if(__likely(s && 0<n && length(len+(m=wcs2utf(s,n))))){
    if(pos<=0){
      memmove(str+m,str,len);
      wcs2utf(str,s,m,n);
      }
    else if(pos>=len){
      wcs2utf(str+len,s,m,n);
      }
    else{
      memmove(str+pos+m,str+pos,len-pos);
      wcs2utf(str+pos,s,m,n);
      }
    }
  return *this;
  }


// Insert string at position
FXString& FXString::insert(FXint pos,const FXString& s){
  return insert(pos,s.text(),s.length());
  }


// Prepend character
FXString& FXString::prepend(FXchar c){
  FXint len=length();
  if(__likely(length(len+1))){
    memmove(str+1,str,len);
    str[0]=c;
    }
  return *this;
  }


// Prepend string with n characters c
FXString& FXString::prepend(FXchar c,FXint n){
  FXint len=length();
  if(__likely(0<n && length(len+n))){
    memmove(str+n,str,len);
    memset(str,c,n);
    }
  return *this;
  }


// Prepend string
FXString& FXString::prepend(const FXchar* s){
  FXint len=length(),m;
  if(__likely(s && s[0] && length(len+(m=strlen(s))))){
    memmove(str+m,str,len);
    memmove(str,s,m);
    }
  return *this;
  }


// Prepend narrow character string
FXString& FXString::prepend(const FXnchar* s){
  FXint len=length(),m;
  if(__likely(s && s[0] && length(len+(m=ncs2utf(s))))){
    memmove(str+m,str,len);
    ncs2utf(str,s,m);
    }
  return *this;
  }


// Prepend wide character string
FXString& FXString::prepend(const FXwchar* s){
  FXint len=length(),m;
  if(__likely(s && s[0] && length(len+(m=wcs2utf(s))))){
    memmove(str+m,str,len);
    wcs2utf(str,s,m);
    }
  return *this;
  }


// Prepend string
FXString& FXString::prepend(const FXchar* s,FXint n){
  FXint len=length();
  if(__likely(s && 0<n && length(len+n))){
    memmove(str+n,str,len);
    memmove(str,s,n);
    }
  return *this;
  }


// Prepend narrow character string
FXString& FXString::prepend(const FXnchar* s,FXint n){
  FXint len=length(),m;
  if(__likely(s && 0<n && length(len+(m=ncs2utf(s,n))))){
    memmove(str+m,str,len);
    ncs2utf(str,s,m,n);
    }
  return *this;
  }


// Prepend wide character string
FXString& FXString::prepend(const FXwchar* s,FXint n){
  FXint len=length(),m;
  if(__likely(s && 0<n && length(len+(m=wcs2utf(s,n))))){
    memmove(str+m,str,len);
    wcs2utf(str,s,m,n);
    }
  return *this;
  }


// Prepend string
FXString& FXString::prepend(const FXString& s){
  return prepend(s.text(),s.length());
  }


// Append character c to this string
FXString& FXString::append(FXchar c){
  FXint len=length();
  if(__likely(length(len+1))){
    str[len]=c;
    }
  return *this;
  }


// Append n characters c to this string
FXString& FXString::append(FXchar c,FXint n){
  FXint len=length();
  if(__likely(0<n && length(len+n))){
    memset(str+len,c,n);
    }
  return *this;
  }


// Append string to this string
FXString& FXString::append(const FXchar* s){
  FXint len=length(),m;
  if(__likely(s && s[0] && length(len+(m=strlen(s))))){
    memmove(str+len,s,m);
    }
  return *this;
  }


// Append string to this string
FXString& FXString::append(const FXnchar* s){
  FXint len=length(),m;
  if(__likely(s && s[0] && length(len+(m=ncs2utf(s))))){
    ncs2utf(str+len,s,m);
    }
  return *this;
  }


// Append string to this string
FXString& FXString::append(const FXwchar* s){
  FXint len=length(),m;
  if(__likely(s && s[0] && length(len+(m=wcs2utf(s))))){
    wcs2utf(str+len,s,m);
    }
  return *this;
  }


// Append string to this string
FXString& FXString::append(const FXchar* s,FXint n){
  FXint len=length();
  if(__likely(s && 0<n && length(len+n))){
    memmove(str+len,s,n);
    }
  return *this;
  }


// Append string to this string
FXString& FXString::append(const FXnchar* s,FXint n){
  FXint len=length(),m;
  if(__likely(s && 0<n && length(len+(m=ncs2utf(s,n))))){
    ncs2utf(str+len,s,m,n);
    }
  return *this;
  }


// Append string to this string
FXString& FXString::append(const FXwchar* s,FXint n){
  FXint len=length(),m;
  if(__likely(s && 0<n && length(len+(m=wcs2utf(s,n))))){
    wcs2utf(str+len,s,m,n);
    }
  return *this;
  }


// Append string to this string
FXString& FXString::append(const FXString& s){
  return append(s.text(),s.length());
  }


// Replace character in string
FXString& FXString::replace(FXint pos,FXchar c){
  str[pos]=c;
  return *this;
  }


// Replace the r characters at pos with n characters c
FXString& FXString::replace(FXint pos,FXint r,FXchar c,FXint n){
  FXint len=length();
  if(__likely(0<=pos && 0<=r && pos+r<=len)){
    if(r<n){
      if(!length(len+n-r)) return *this;
      memmove(str+pos+n,str+pos+r,len-pos-r);
      }
    else if(r>n){
      memmove(str+pos+n,str+pos+r,len-pos-r);
      if(!length(len+n-r)) return *this;
      }
    memset(str+pos,c,n);
    }
  return *this;
  }


// Replace the r characters at pos with string s
FXString& FXString::replace(FXint pos,FXint r,const FXchar* s){
  return replace(pos,r,s,strlen(s));
  }


// Replace the r characters at pos with narrow character string s
FXString& FXString::replace(FXint pos,FXint r,const FXnchar* s){
  return replace(pos,r,s,strlen(s));
  }


// Replace the r characters at pos with wide character string s
FXString& FXString::replace(FXint pos,FXint r,const FXwchar* s){
  return replace(pos,r,s,strlen(s));
  }


// Replaces the r characters at pos with first n characters of string s
FXString& FXString::replace(FXint pos,FXint r,const FXchar* s,FXint n){
  FXint len=length();
  if(__likely(0<=pos && 0<=r && pos+r<=len)){
    if(r<n){
      if(!length(len+n-r)) return *this;
      memmove(str+pos+n,str+pos+r,len-pos-r);
      }
    else if(r>n){
      memmove(str+pos+n,str+pos+r,len-pos-r);
      if(!length(len+n-r)) return *this;
      }
    memcpy(str+pos,s,n);
    }
  return *this;
  }


// Replaces the r characters at pos with first n characters of narrow character string s
FXString& FXString::replace(FXint pos,FXint r,const FXnchar* s,FXint n){
  FXint len=length(),m=ncs2utf(s,n);
  if(__likely(0<=pos && 0<=r && pos+r<=len)){
    if(r<m){
      if(!length(len+m-r)) return *this;
      memmove(str+pos+m,str+pos+r,len-pos-r);
      }
    else if(r>m){
      memmove(str+pos+m,str+pos+r,len-pos-r);
      if(!length(len+m-r)) return *this;
      }
    ncs2utf(str+pos,s,m,n);
    }
  return *this;
  }


// Replaces the r characters at pos with first n characters of wide character string s
FXString& FXString::replace(FXint pos,FXint r,const FXwchar* s,FXint n){
  FXint len=length(),m=wcs2utf(s,n);
  if(__likely(0<=pos && 0<=r && pos+r<=len)){
    if(r<m){
      if(!length(len+m-r)) return *this;
      memmove(str+pos+m,str+pos+r,len-pos-r);
      }
    else if(r>m){
      memmove(str+pos+m,str+pos+r,len-pos-r);
      if(!length(len+m-r)) return *this;
      }
    wcs2utf(str+pos,s,m,n);
    }
  return *this;
  }


// Replace part of string
FXString& FXString::replace(FXint pos,FXint r,const FXString& s){
  return replace(pos,r,s.str,s.length());
  }


// Move range of n characters from src position to dst position
FXString& FXString::move(FXint dst,FXint src,FXint n){
  FXint len=length();
  if(__likely(0<n && 0<=src && src+n<=len)){
    if(dst<0){                                  // Move below begin
      if(dst<-n) dst=-n;
      length(len-dst);
      memmove(str-dst,str,len);
      memmove(str,str-dst+src,n);
      }
    else if(dst+n>len){                         // Move beyond end
      if(dst>len) dst=len;
      length(dst+n);
      memmove(str+dst,str+src,n);
      }
    else{
      memmove(str+dst,str+src,n);               // Move inside
      }
    }
  return *this;
  }


// Remove one character
FXString& FXString::erase(FXint pos){
  FXint len=length();
  if(__likely(0<=pos && pos<len)){
    memmove(str+pos,str+pos+1,len-pos-1);
    length(len-1);
    }
  return *this;
  }


// Remove section from buffer
FXString& FXString::erase(FXint pos,FXint n){
  FXint len=length();
  if(__likely(0<n && 0<pos+n && pos<len)){
    if(pos<0){n+=pos;pos=0;}
    if(len<pos+n){n=len-pos;}
    memmove(str+pos,str+pos+n,len-pos-n);
    length(len-n);
    }
  return *this;
  }


// Truncate string
FXString& FXString::trunc(FXint pos){
  length(FXMAX(FXMIN(pos,length()),0));
  return *this;
  }


// Clean string
FXString& FXString::clear(){
  length(0);
  return *this;
  }


// Return number of occurrences of ch in string
FXint FXString::contains(FXchar ch) const {
  FXint len=length();
  FXint m=0;
  FXint i=0;
  while(i<len){
    m+=(str[i++]==ch);
    }
  return m;
  }


// Return number of occurrences of string sub in string
FXint FXString::contains(const FXchar* sub,FXint n) const {
  FXint len=length()-n;
  FXint m=0;
  FXint i=0;
  while(i<=len){
    if(FXString::compare(str+i,sub,n)==0){
      m++;
      }
    i++;
    }
  return m;
  }


// Return number of occurrences of string sub in string
FXint FXString::contains(const FXchar* sub) const {
  return contains(sub,strlen(sub));
  }


// Return number of occurrences of string sub in string
FXint FXString::contains(const FXString& sub) const {
  return contains(sub.text(),sub.length());
  }


// Substitute one character by another
FXString& FXString::substitute(FXchar org,FXchar sub,FXbool all){
  FXint len=length();
  FXint c=org;
  FXint s=sub;
  FXint i=0;
  while(i<len){
    if(str[i]==c){
      str[i]=s;
      if(!all) break;
      }
    i++;
    }
  return *this;
  }


// Substitute one string by another
FXString& FXString::substitute(const FXchar* org,FXint olen,const FXchar* rep,FXint rlen,FXbool all){
  if(__likely(0<olen && 0<=rlen)){
    FXint len=length();
    FXint pos=0;
    while(pos<=len-olen){
      if(FXString::compare(str+pos,org,olen)==0){
        replace(pos,olen,rep,rlen);
        if(!all) break;
        len+=rlen-olen;
        pos+=rlen;
        continue;
        }
      pos++;
      }
    }
  return *this;
  }


// Substitute one string by another
FXString& FXString::substitute(const FXchar* org,const FXchar* rep,FXbool all){
  return substitute(org,strlen(org),rep,strlen(rep),all);
  }


// Substitute one string by another
FXString& FXString::substitute(const FXString& org,const FXString& rep,FXbool all){
  return substitute(org.text(),org.length(),rep.text(),rep.length(),all);
  }


// Convert to lower case
FXString& FXString::lower(){
  FXint p,ow,nw;
  FXwchar w;
  for(p=0; p<length(); p+=nw){
    w=wc(p);
    ow=wc2utf(w);
    w=Unicode::toLower(w);
    nw=wc2utf(w);
    replace(p,ow,&w,1);
    }
  return *this;
  }


// Convert to upper case
FXString& FXString::upper(){
  FXint p,ow,nw;
  FXwchar w;
  for(p=0; p<length(); p+=nw){
    w=wc(p);
    ow=wc2utf(w);
    w=Unicode::toUpper(w);
    nw=wc2utf(w);
    replace(p,ow,&w,1);
    }
  return *this;
  }


// Simplify whitespace in string
FXString& FXString::simplify(){
  if(str!=EMPTY){
    FXint e=length(),s=0,d=0;
    while(s<e && Ascii::isSpace(str[s])) s++;
    while(1){
      while(s<e && !Ascii::isSpace(str[s])) str[d++]=str[s++];
      while(s<e && Ascii::isSpace(str[s])) s++;
      if(s>=e) break;
      str[d++]=' ';
      }
    length(d);
    }
  return *this;
  }


// Remove leading and trailing whitespace
FXString& FXString::trim(){
  if(str!=EMPTY){
    FXint e=length(),s=0,d=0;
    while(s<e && Ascii::isSpace(str[s])) s++;
    while(s<e && Ascii::isSpace(str[e-1])) e--;
    while(s<e) str[d++]=str[s++];
    length(d);
    }
  return *this;
  }


// Remove leading whitespace
FXString& FXString::trimBegin(){
  if(str!=EMPTY){
    FXint e=length(),s=0,d=0;
    while(s<e && Ascii::isSpace(str[s])) s++;
    while(s<e) str[d++]=str[s++];
    length(d);
    }
  return *this;
  }


// Remove trailing whitespace
FXString& FXString::trimEnd(){
  if(str!=EMPTY){
    FXint e=length();
    while(0<e && Ascii::isSpace(str[e-1])) e--;
    length(e);
    }
  return *this;
  }


// Get leftmost part
FXString FXString::left(FXint n) const {
  FXint len=length();
  if(0<n){
    if(n>len) n=len;
    return FXString(str,n);
    }
  return FXString::null;
  }


// Get rightmost part
FXString FXString::right(FXint n) const {
  FXint len=length();
  if(0<n){
    if(n>len) n=len;
    return FXString(str+len-n,n);
    }
  return FXString::null;
  }


// Get some part in the middle
FXString FXString::mid(FXint pos,FXint n) const {
  FXint len=length();
  if(__likely(0<n && 0<pos+n && pos<len)){
    if(pos<0){n+=pos;pos=0;}
    if(len-pos<n){n=len-pos;}
    return FXString(str+pos,n);
    }
  return FXString::null;
  }


// Return partition of string separated by delimiter delim
FXString FXString::section(FXchar delim,FXint start,FXint num) const {
  FXint len=length(),s,e;
  s=0;
  if(0<start){
    while(s<len){
      ++s;
      if(str[s-1]==delim && --start==0) break;
      }
    }
  e=s;
  if(0<num){
    while(e<len){
      if(str[e]==delim && --num==0) break;
      ++e;
      }
    }
  return FXString(str+s,e-s);
  }


// Return partition of string separated by delimiters in delim
FXString FXString::section(const FXchar* delim,FXint n,FXint start,FXint num) const {
  FXint len=length(),s,e,i;
  FXchar c;
  s=0;
  if(0<start){
    while(s<len){
      c=str[s++];
      i=n;
      while(--i>=0){
        if(delim[i]==c){
          if(--start==0) goto a;
          break;
          }
        }
      }
    }
a:e=s;
  if(0<num){
    while(e<len){
      c=str[e];
      i=n;
      while(--i>=0){
        if(delim[i]==c){
          if(--num==0) goto b;
          break;
          }
        }
      ++e;
      }
    }
b:return FXString(str+s,e-s);
  }


// Return partition of string separated by delimiters in delim
FXString FXString::section(const FXchar* delim,FXint start,FXint num) const {
  return section(delim,strlen(delim),start,num);
  }


// Return partition of string separated by delimiters in delim
FXString FXString::section(const FXString& delim,FXint start,FXint num) const {
  return section(delim.text(),delim.length(),start,num);
  }


// Return all characters before the nth occurrence of ch, searching forward
FXString FXString::before(FXchar c,FXint n) const {
  FXint len=length();
  FXint p=0;
  if(0<n){
    while(p<len){
      if(str[p]==c && --n==0) break;
      p++;
      }
    }
  return FXString(str,p);
  }


// Return all characters before the nth occurrence of ch, searching backward
FXString FXString::rbefore(FXchar c,FXint n) const {
  FXint p=length();
  if(0<n){
    while(0<p){
      p--;
      if(str[p]==c && --n==0) break;
      }
    }
  return FXString(str,p);
  }


// Return all characters after the nth occurrence of ch, searching forward
FXString FXString::after(FXchar c,FXint n) const {
  FXint len=length();
  FXint p=0;
  if(0<n){
    while(p<len){
      p++;
      if(str[p-1]==c && --n==0) break;
      }
    }
  return FXString(str+p,len-p);
  }


// Return all characters after the nth occurrence of ch, searching backward
FXString FXString::rafter(FXchar c,FXint n) const {
  FXint len=length();
  FXint p=len;
  if(0<n){
    while(0<p){
      if(str[p-1]==c && --n==0) break;
      p--;
      }
    }
  return FXString(str+p,len-p);
  }


// Find n-th occurrence of character, searching forward; return position or -1
FXint FXString::find(FXchar c,FXint pos,FXint n) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p<0) p=0;
  if(n<=0) return p;
  while(p<len){
    if(str[p]==cc){ if(--n==0) return p; }
    ++p;
    }
  return -1;
  }


// Find n-th occurrence of character, searching backward; return position or -1
FXint FXString::rfind(FXchar c,FXint pos,FXint n) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p>=len) p=len-1;
  if(n<=0) return p;
  while(0<=p){
    if(str[p]==cc){ if(--n==0) return p; }
    --p;
    }
  return -1;
  }


// Find a character, searching forward; return position or -1
FXint FXString::find(FXchar c,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p<0) p=0;
  while(p<len){ if(str[p]==cc){ return p; } ++p; }
  return -1;
  }


// Find a character, searching backward; return position or -1
FXint FXString::rfind(FXchar c,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p>=len) p=len-1;
  while(0<=p){ if(str[p]==cc){ return p; } --p; }
  return -1;
  }


// Find a substring of length n, searching forward; return position or -1
FXint FXString::find(const FXchar* substr,FXint n,FXint pos) const {
  FXint len=length();
  if(0<=pos && 0<n && n<=len){
    FXint c=substr[0];
    len=len-n+1;
    while(pos<len){
      if(str[pos]==c){
        if(!FXString::compare(str+pos,substr,n)){
          return pos;
          }
        }
      pos++;
      }
    }
  return -1;
  }


// Find a substring, searching forward; return position or -1
FXint FXString::find(const FXchar* substr,FXint pos) const {
  return find(substr,strlen(substr),pos);
  }


// Find a substring, searching forward; return position or -1
FXint FXString::find(const FXString& substr,FXint pos) const {
  return find(substr.text(),substr.length(),pos);
  }


// Find a substring of length n, searching backward; return position or -1
FXint FXString::rfind(const FXchar* substr,FXint n,FXint pos) const {
  FXint len=length();
  if(0<=pos && 0<n && n<=len){
    FXint c=substr[0];
    len-=n;
    if(pos>len) pos=len;
    while(0<=pos){
      if(str[pos]==c){
        if(!FXString::compare(str+pos,substr,n)){
          return pos;
          }
        }
      pos--;
      }
    }
  return -1;
  }


// Find a substring, searching backward; return position or -1
FXint FXString::rfind(const FXchar* substr,FXint pos) const {
  return rfind(substr,strlen(substr),pos);
  }


// Find a substring, searching backward; return position or -1
FXint FXString::rfind(const FXString& substr,FXint pos) const {
  return rfind(substr.text(),substr.length(),pos);
  }


// Find first character in the set of size n, starting from pos; return position or -1
FXint FXString::find_first_of(const FXchar* set,FXint n,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  if(p<0) p=0;
  while(p<len){
    FXint c=str[p];
    FXint i=n;
    while(--i>=0){ if(set[i]==c) return p; }
    p++;
    }
  return -1;
  }


// Find first character in the set, starting from pos; return position or -1
FXint FXString::find_first_of(const FXchar* set,FXint pos) const {
  return find_first_of(set,strlen(set),pos);
  }


// Find first character in the set, starting from pos; return position or -1
FXint FXString::find_first_of(const FXString& set,FXint pos) const {
  return find_first_of(set.text(),set.length(),pos);
  }


// Find first character, starting from pos; return position or -1
FXint FXString::find_first_of(FXchar c,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p<0) p=0;
  while(p<len){ if(str[p]==cc){ return p; } p++; }
  return -1;
  }


// Find last character in the set of size n, starting from pos; return position or -1
FXint FXString::find_last_of(const FXchar* set,FXint n,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  if(p>=len) p=len-1;
  while(0<=p){
    FXint c=str[p];
    FXint i=n;
    while(--i>=0){ if(set[i]==c) return p; }
    p--;
    }
  return -1;
  }


// Find last character in the set, starting from pos; return position or -1
FXint FXString::find_last_of(const FXchar* set,FXint pos) const {
  return find_last_of(set,strlen(set),pos);
  }


// Find last character in the set, starting from pos; return position or -1
FXint FXString::find_last_of(const FXString& set,FXint pos) const {
  return find_last_of(set.text(),set.length(),pos);
  }


// Find last character, starting from pos; return position or -1
FXint FXString::find_last_of(FXchar c,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p>=len) p=len-1;
  while(0<=p){ if(str[p]==cc){ return p; } p--; }
  return -1;
  }


// Find first character NOT in the set of size n, starting from pos; return position or -1
FXint FXString::find_first_not_of(const FXchar* set,FXint n,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  if(p<0) p=0;
  while(p<len){
    FXint c=str[p];
    FXint i=n;
    while(--i>=0){ if(set[i]==c) goto x; }
    return p;
x:  p++;
    }
  return -1;
  }


// Find first character NOT in the set, starting from pos; return position or -1
FXint FXString::find_first_not_of(const FXchar* set,FXint pos) const {
  return find_first_not_of(set,strlen(set),pos);
  }


// Find first character NOT in the set, starting from pos; return position or -1
FXint FXString::find_first_not_of(const FXString& set,FXint pos) const {
  return find_first_not_of(set.text(),set.length(),pos);
  }


// Find first character NOT equal to c, starting from pos; return position or -1
FXint FXString::find_first_not_of(FXchar c,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p<0) p=0;
  while(p<len){ if(str[p]!=cc){ return p; } p++; }
  return -1;
  }


// Find last character NOT in the set of size n, starting from pos; return position or -1
FXint FXString::find_last_not_of(const FXchar* set,FXint n,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  if(p>=len) p=len-1;
  while(0<=p){
    FXint c=str[p];
    FXint i=n;
    while(--i>=0){ if(set[i]==c) goto x; }
    return p;
x:  p--;
    }
  return -1;
  }


// Find last character NOT in the set, starting from pos; return position or -1
FXint FXString::find_last_not_of(const FXchar* set,FXint pos) const {
  return find_last_not_of(set,strlen(set),pos);
  }


// Find last character NOT in the set, starting from pos; return position or -1
FXint FXString::find_last_not_of(const FXString& set,FXint pos) const {
  return find_last_not_of(set.text(),set.length(),pos);
  }


// Find last character NOT equal to c, starting from pos; return position or -1
FXint FXString::find_last_not_of(FXchar c,FXint pos) const {
  FXint len=length();
  FXint p=pos;
  FXint cc=c;
  if(p>=len) p=len-1;
  while(0<=p){ if(str[p]!=cc){ return p; } p--; }
  return -1;
  }

/*******************************************************************************/

#ifdef WIN32
#ifndef va_copy
#define va_copy(arg,list) ((arg)=(list))
#endif
#endif


// Scan
FXint FXString::vscan(const FXchar* fmt,va_list args) const {
  return __vsscanf(str,fmt,args);
  }


FXint FXString::scan(const FXchar* fmt,...) const {
  FXint result;
  va_list args;
  va_start(args,fmt);
  result=vscan(fmt,args);
  va_end(args);
  return result;
  }


// Print formatted string a-la vprintf
FXint FXString::vformat(const FXchar* fmt,va_list args){
  FXint result=0;
  if(fmt && *fmt){
    va_list ag;
    va_copy(ag,args);
    result=__vsnprintf(str,length(),fmt,ag);       // Try to see if existing buffer fits
    va_end(ag);
    if(length()<result){                           // FOX's own __vsnprintf() truncates at buffer size
      length(result);
      result=__vsnprintf(str,length(),fmt,args);   // Now try again with exactly the right size
      return result;
      }
    }
  length(result);
  return result;
  }


// Print formatted string a-la printf
FXint FXString::format(const FXchar* fmt,...){
  va_list args;
  va_start(args,fmt);
  FXint result=vformat(fmt,args);
  va_end(args);
  return result;
  }

/*******************************************************************************/

// Convert to integer
FXint FXString::toInt(FXint base,FXbool* ok) const {
  return __strtol(str,nullptr,base,ok);
  }


// Convert to unsigned integer
FXuint FXString::toUInt(FXint base,FXbool* ok) const {
  return __strtoul(str,nullptr,base,ok);
  }


// Convert to long integer
FXlong FXString::toLong(FXint base,FXbool* ok) const {
  return __strtoll(str,nullptr,base,ok);
  }


// Convert to unsigned long integer
FXulong FXString::toULong(FXint base,FXbool* ok) const {
  return __strtoull(str,nullptr,base,ok);
  }


// Convert to float
FXfloat FXString::toFloat(FXbool* ok) const {
  return __strtof(str,nullptr,ok);
  }


// Convert to double number
FXdouble FXString::toDouble(FXbool* ok) const {
  return __strtod(str,nullptr,ok);
  }


// Convert from integer
FXString& FXString::fromInt(FXint number,FXint base){
  FXuint nn=FXABS(number);
  FXchar buf[34],*p=buf+sizeof(buf);
  if(base<2 || base>16){ fxerror("FXString::fromInt: base out of range.\n"); }
  do{
    *--p=FXString::value2Digit[nn%base];
    nn/=base;
    }
  while(nn);
  if(number<0) *--p='-';
  return assign(p,buf+sizeof(buf)-p);
  }


// Convert from unsigned integer
FXString& FXString::fromUInt(FXuint number,FXint base){
  FXuint nn=number;
  FXchar buf[34],*p=buf+sizeof(buf);
  if(base<2 || base>16){ fxerror("FXString::fromUInt: base out of range.\n"); }
  do{
    *--p=FXString::value2Digit[nn%base];
    nn/=base;
    }
  while(nn);
  return assign(p,buf+sizeof(buf)-p);
  }


// Convert from long integer
FXString& FXString::fromLong(FXlong number,FXint base){
  FXulong nn=FXABS(number);
  FXchar buf[66],*p=buf+sizeof(buf);
  if(base<2 || base>16){ fxerror("FXString::fromLong: base out of range.\n"); }
  do{
    *--p=FXString::value2Digit[nn%base];
    nn/=base;
    }
  while(nn);
  if(number<0) *--p='-';
  return assign(p,buf+sizeof(buf)-p);
  }


// Convert from unsigned long integer
FXString& FXString::fromULong(FXulong number,FXint base){
  FXulong nn=number;
  FXchar buf[66],*p=buf+sizeof(buf);
  if(base<2 || base>16){ fxerror("FXString::fromULong: base out of range.\n"); }
  do{
    *--p=FXString::value2Digit[nn%base];
    nn/=base;
    }
  while(nn);
  return assign(p,buf+sizeof(buf)-p);
  }


// Formatting for reals
static const char conversionformat[8][8]={"%.*lF","%.*lE","%.*lG","%.*lA","%'.*lF","%'.*lE","%'.*lG","%.*lA"};


// Convert from float
FXString& FXString::fromFloat(FXfloat number,FXint prec,FXint fmt){
  format(conversionformat[fmt&7],prec,(FXdouble)number);
  return *this;
  }


// Convert from double
FXString& FXString::fromDouble(FXdouble number,FXint prec,FXint fmt){
  format(conversionformat[fmt&7],prec,number);
  return *this;
  }


// Return string by converting a integer
FXString FXString::value(FXint num,FXint base){
  FXString result;
  return result.fromInt(num,base);
  }


// Return string by converting an unsigned integer
FXString FXString::value(FXuint num,FXint base){
  FXString result;
  return result.fromUInt(num,base);
  }


// Return string by converting a long integer
FXString FXString::value(FXlong num,FXint base){
  FXString result;
  return result.fromLong(num,base);
  }


// Return string by converting an unsigned long
FXString FXString::value(FXulong num,FXint base){
  FXString result;
  return result.fromULong(num,base);
  }


// Return string by converting a float
FXString FXString::value(FXfloat num,FXint prec,FXint fmt){
  FXString result;
  return result.fromFloat(num,prec,fmt);
  }


// Return string by converting a double
FXString FXString::value(FXdouble num,FXint prec,FXint fmt){
  FXString result;
  return result.fromDouble(num,prec,fmt);
  }


// Return string from printf-like format arguments
FXString FXString::value(const FXchar* fmt,...){
  FXString result;
  va_list args;
  va_start(args,fmt);
  result.vformat(fmt,args);
  va_end(args);
  return result;
  }


// Return string from vprintf-like format arguments
FXString FXString::vvalue(const FXchar* fmt,va_list args){
  FXString result;
  result.vformat(fmt,args);
  return result;
  }

/*******************************************************************************/

// Compute FNV1a hash value of string
FXuint FXString::hash(const FXchar* s,FXint n){
  FXuint result=0x811C9DC5;
  FXuchar c;
  if(0<n){
    do{
      c=*s++;
      result=(result^c)*0x01000193;
      }
    while(--n);
    }
  return result;
  }


// Compute FNV1a hash value of string
FXuint FXString::hash(const FXchar* s){
  FXuint result=0x811C9DC5;
  FXuchar c;
  while((c=*s++)!='\0'){
    result=(result^c)*0x01000193;
    }
  return result;
  }


// Compute hash value of string
FXuint FXString::hash() const {
  return FXString::hash(str);
  }

/*******************************************************************************/

// Compare string and string
FXint FXString::compare(const FXchar* s1,const FXchar* s2){
  if(s1!=s2){
    FXint c1,c2;
    do{
      c1=*s1++;
      c2=*s2++;
      }
    while((c1==c2) && c1);
    return c1-c2;
    }
  return 0;
  }


// Compare string and FXString
FXint FXString::compare(const FXchar* s1,const FXString& s2){
  return FXString::compare(s1,s2.text());
  }


// Compare FXString and string
FXint FXString::compare(const FXString& s1,const FXchar* s2){
  return FXString::compare(s1.text(),s2);
  }


// Compare FXString and FXString
FXint FXString::compare(const FXString& s1,const FXString& s2){
  return FXString::compare(s1.text(),s2.text());
  }

/*******************************************************************************/

// Compare string and string, up to n
FXint FXString::compare(const FXchar* s1,const FXchar* s2,FXint n){
  if(s1!=s2 && n>0){
    FXint c1,c2;
    do{
      c1=*s1++;
      c2=*s2++;
      }
    while((c1==c2) && c1 && --n);
    return c1-c2;
    }
  return 0;
  }


// Compare string and FXString, up to n
FXint FXString::compare(const FXchar* s1,const FXString& s2,FXint n){
  return FXString::compare(s1,s2.text(),n);
  }


// Compare FXString and string, up to n
FXint FXString::compare(const FXString& s1,const FXchar* s2,FXint n){
  return FXString::compare(s1.text(),s2,n);
  }


// Compare FXString and FXString, up to n
FXint FXString::compare(const FXString& s1,const FXString& s2,FXint n){
  return FXString::compare(s1.text(),s2.text(),n);
  }

/*******************************************************************************/

// Compare string and string case insensitive
FXint FXString::comparecase(const FXchar* s1,const FXchar* s2){
  if(s1!=s2){
    FXint c1,c2;
    do{
      c1=Ascii::toLower(*s1++);
      c2=Ascii::toLower(*s2++);
      }
    while((c1==c2) && c1);
    return c1-c2;
    }
  return 0;
  }


// Compare string and FXString case insensitive
FXint FXString::comparecase(const FXchar* s1,const FXString& s2){
  return FXString::comparecase(s1,s2.text());
  }


// Compare FXString and string case insensitive
FXint FXString::comparecase(const FXString& s1,const FXchar* s2){
  return FXString::comparecase(s1.text(),s2);
  }


// Compare FXString and FXString case insensitive
FXint FXString::comparecase(const FXString& s1,const FXString& s2){
  return FXString::comparecase(s1.text(),s2.text());
  }

/*******************************************************************************/

// Compare string and string case insensitive, up to n
FXint FXString::comparecase(const FXchar* s1,const FXchar* s2,FXint n){
  if(s1!=s2 && n>0){
    FXint c1,c2;
    do{
      c1=Ascii::toLower(*s1++);
      c2=Ascii::toLower(*s2++);
      }
    while((c1==c2) && c1 && --n);
    return c1-c2;
    }
  return 0;
  }


// Compare string and FXString case insensitive, up to n
FXint FXString::comparecase(const FXchar* s1,const FXString& s2,FXint n){
  return FXString::comparecase(s1,s2.text(),n);
  }


// Compare FXString and string case insensitive, up to n
FXint FXString::comparecase(const FXString& s1,const FXchar* s2,FXint n){
  return FXString::comparecase(s1.text(),s2,n);
  }


// Compare FXString and FXString case insensitive, up to n
FXint FXString::comparecase(const FXString& s1,const FXString& s2,FXint n){
  return FXString::comparecase(s1.text(),s2.text(),n);
  }


/*******************************************************************************/

// Compare with natural interpretation of decimal numbers
FXint FXString::comparenatural(const FXchar* s1,const FXchar* s2){
  FXint diff=0;
  if(s1!=s2){
    const FXchar *ns1,*ne1,*ns2,*ne2;
    FXint c1=0,c2=0,d;
    while((c1=(FXuchar)*s1)!='\0' && (c2=(FXuchar)*s2)!='\0'){

      // Both are numbers: special treatment
      if(c1<='9' && c2<='9' && '0'<=c1 && '0'<=c2){

        // Parse over leading zeroes
        for(ns1=s1; *ns1=='0'; ++ns1){ }
        for(ns2=s2; *ns2=='0'; ++ns2){ }

        // Use number of leading zeroes as tie-breaker
        if(diff==0){ diff=(ns1-s1)-(ns2-s2); }

        // Parse over numbers
        for(ne1=ns1; '0'<=*ne1 && *ne1<='9'; ++ne1){ }
        for(ne2=ns2; '0'<=*ne2 && *ne2<='9'; ++ne2){ }

        // Check length difference of the numbers
        if((d=(ne1-ns1)-(ne2-ns2))!=0){ return d; }

        // Compare the numbers
        while(ns1<ne1){
          if((d=*ns1++ - *ns2++)!=0){ return d; }
          }

        // Continue with the rest
        s1=ne1;
        s2=ne2;
        continue;
        }

      // Characters differ
      if(c1!=c2){ return c1-c2; }

      // Advance
      s1++;
      s2++;
      }

    // Characters differ
    if(c1!=c2){ return c1-c2; }
    }
  return diff;
  }


// Compare with natural interpretation of decimal numbers
FXint FXString::comparenatural(const FXchar* s1,const FXString& s2){
  return FXString::comparenatural(s1,s2.text());
  }


// Compare with natural interpretation of decimal numbers
FXint FXString::comparenatural(const FXString& s1,const FXchar* s2){
  return FXString::comparenatural(s1.text(),s2);
  }


// Compare with natural interpretation of decimal numbers
FXint FXString::comparenatural(const FXString& s1,const FXString& s2){
  return FXString::comparenatural(s1.text(),s2.text());
  }

/*******************************************************************************/

// Compare case insensitive with natural interpretation of decimal numbers
FXint FXString::comparenaturalcase(const FXchar* s1,const FXchar* s2){
  FXint diff=0;
  if(s1!=s2){
    const FXchar *ns1,*ne1,*ns2,*ne2;
    FXint c1=0,c2=0,d;
    while((c1=(FXuchar)*s1)!='\0' && (c2=(FXuchar)*s2)!='\0'){

      // Both are numbers: special treatment
      if(c1<='9' && c2<='9' && '0'<=c1 && '0'<=c2){

        // Parse over leading zeroes
        for(ns1=s1; *ns1=='0'; ++ns1){ }
        for(ns2=s2; *ns2=='0'; ++ns2){ }

        // Use number of leading zeroes as tie-breaker
        if(diff==0){ diff=(ns1-s1)-(ns2-s2); }

        // Parse over numbers
        for(ne1=ns1; '0'<=*ne1 && *ne1<='9'; ++ne1){ }
        for(ne2=ns2; '0'<=*ne2 && *ne2<='9'; ++ne2){ }

        // Check length difference of the numbers
        if((d=(ne1-ns1)-(ne2-ns2))!=0){ return d; }

        // Compare the numbers
        while(ns1<ne1){
          if((d=*ns1++ - *ns2++)!=0){ return d; }
          }

        // Continue with the rest
        s1=ne1;
        s2=ne2;
        continue;
        }

      // Get lower-case
      c1=Ascii::toLower(c1);
      c2=Ascii::toLower(c2);

      // Characters differ
      if(c1!=c2){ return c1-c2; }

      // Advance
      s1++;
      s2++;
      }

    // Characters differ
    if(c1!=c2){ return c1-c2; }
    }
  return diff;
  }


// Compare case insensitive with natural interpretation of decimal numbers
FXint FXString::comparenaturalcase(const FXchar* s1,const FXString& s2){
  return FXString::comparenaturalcase(s1,s2.text());
  }


// Compare case insensitive with natural interpretation of decimal numbers
FXint FXString::comparenaturalcase(const FXString& s1,const FXchar* s2){
  return FXString::comparenaturalcase(s1.text(),s2);
  }


// Compare case insensitive with natural interpretation of decimal numbers
FXint FXString::comparenaturalcase(const FXString& s1,const FXString& s2){
  return FXString::comparenaturalcase(s1.text(),s2.text());
  }

/*******************************************************************************/

// Strings may have to be escaped in case any of the following is true:
//
//   1) There are leading or trailing spaces or tabs;
//   2) The string contains control characters;
//   3) The escape character (\) is encountered;
//   4) The opening and/or closing quote is encountered;
//   5) A bad unicode character is encountered;
//   6) Flag says all unicode should be escaped.
//
FXbool FXString::shouldEscape(const FXchar* str,FXint num,FXchar lquote,FXchar rquote,FXint flag){
  const FXchar* end=str+num;
  if(str<end){
    FXuchar c;
    if((c=str[0])<=' ') return true;
    if((c=end[-1])<=' ') return true;
    do{
      c=*str++;
      if(c<' ') return true;
      if(c=='\\') return true;
      if(c==lquote) return true;
      if(c==rquote) return true;
      if(0x7F<=c){
        if(0x7F==c) return true;
        if(0xF8<=c) return true;
        if(flag) return true;
        }
      }
    while(str<end);
    }
  return false;
  }

// Check if the string should be escaped
FXbool FXString::shouldEscape(const FXchar* str,FXchar lquote,FXchar rquote,FXint flag){
  return FXString::shouldEscape(str,strlen(str),lquote,rquote,flag);
  }


// Check if the string should be escaped
FXbool FXString::shouldEscape(const FXString& str,FXchar lquote,FXchar rquote,FXint flag){
  return FXString::shouldEscape(str.text(),str.length(),lquote,rquote,flag);
  }

/*******************************************************************************/

// Escape special characters, and optionally enclose with left and right quotes
// and escape utf8 as \xHH if flag=1, or as \uHHHH if flag=2.
// UTF8 characters may be encoded as hex (in the form of: \xHH), or as Unicode
// escape sequences (of the form \uHHHH).  Code points exceeding 16-bits will be
// encoded as hex-encoded surrogate-pairs (\uHHHH\uHHHH) in Unicode escape mode.
// UTF8 followers will be always escaped if not preceeded by UTF8 leaders, if
// escaping is enabled.
FXString FXString::escape(const FXchar* str,FXint num,FXchar lquote,FXchar rquote,FXint flag){
  FXString result;
  FXint p,q,w,v;
  FXuchar c,cc;
  p=q=0;
  if(lquote) q++;                               // Opening quote
  while(p<num){                                 // Measure length of converted string
    c=str[p++];
    switch(c){
      case '\a':                                // Special control characters
      case '\b':
      case '\t':
      case '\n':
      case '\v':
      case '\f':
      case '\r':
      case '\\':
        q+=2;
        continue;
      case 0x00:                                // Non-special control characters
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x0E:
      case 0x0F:
      case 0x10:
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17:
      case 0x18:
      case 0x19:
      case 0x1A:
      case 0x1B:
      case 0x1C:
      case 0x1D:
      case 0x1E:
      case 0x1F:
      case 0x7F:                                // DEL
      case 0xF8:                                // Bad UTF8 leaders
      case 0xF9:
      case 0xFA:
      case 0xFB:
      case 0xFC:
      case 0xFD:
      case 0xFF:
hex1:   q+=4;                                   // Escape as \xHH
        continue;
      case 0x80:                                // UTF8 followers
      case 0x81:
      case 0x82:
      case 0x83:
      case 0x84:
      case 0x85:
      case 0x86:
      case 0x87:
      case 0x88:
      case 0x89:
      case 0x8A:
      case 0x8B:
      case 0x8C:
      case 0x8D:
      case 0x8E:
      case 0x8F:
      case 0x90:
      case 0x91:
      case 0x92:
      case 0x93:
      case 0x94:
      case 0x95:
      case 0x96:
      case 0x97:
      case 0x98:
      case 0x99:
      case 0x9A:
      case 0x9B:
      case 0x9C:
      case 0x9D:
      case 0x9E:
      case 0x9F:
      case 0xA0:
      case 0xA1:
      case 0xA2:
      case 0xA3:
      case 0xA4:
      case 0xA5:
      case 0xA6:
      case 0xA7:
      case 0xA8:
      case 0xA9:
      case 0xAA:
      case 0xAB:
      case 0xAC:
      case 0xAD:
      case 0xAE:
      case 0xAF:
      case 0xB0:
      case 0xB1:
      case 0xB2:
      case 0xB3:
      case 0xB4:
      case 0xB5:
      case 0xB6:
      case 0xB7:
      case 0xB8:
      case 0xB9:
      case 0xBA:
      case 0xBB:
      case 0xBC:
      case 0xBD:
      case 0xBE:
      case 0xBF:
        if(flag==0) goto nml1;                  // Pass UTF8 through
        goto hex1;                              // Escape UTF8 follower not preceeded by UTF8 leader
      case 0xC0:                                // 2-byte UTF8 sequences
      case 0xC1:
      case 0xC2:
      case 0xC3:
      case 0xC4:
      case 0xC5:
      case 0xC6:
      case 0xC7:
      case 0xC8:
      case 0xC9:
      case 0xCA:
      case 0xCB:
      case 0xCC:
      case 0xCD:
      case 0xCE:
      case 0xCF:
      case 0xD0:
      case 0xD1:
      case 0xD2:
      case 0xD3:
      case 0xD4:
      case 0xD5:
      case 0xD6:
      case 0xD7:
      case 0xD8:
      case 0xD9:
      case 0xDA:
      case 0xDB:
      case 0xDC:
      case 0xDD:
      case 0xDE:
      case 0xDF:
      case 0xE0:                                // 3-byte UTF8 sequences
      case 0xE1:
      case 0xE2:
      case 0xE3:
      case 0xE4:
      case 0xE5:
      case 0xE6:
      case 0xE7:
      case 0xE8:
      case 0xE9:
      case 0xEA:
      case 0xEB:
      case 0xEC:
      case 0xED:
      case 0xEE:
      case 0xEF:
      case 0xF0:                                // 4-byte UTF8 sequences
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
        if(flag==0) goto nml1;                  // Pass UTF8 through
        if(flag==1) goto hex1;                  // Simple hex escaping
        cc=str[p];
        if(!followUTF8(cc)) goto hex1;          // UTF8 follower?
        w=(c<<6)^cc^0x3080;
        if(0x800<=w){
          cc=str[p+1];
          if(!followUTF8(cc)) goto hex1;        // UTF8 follower?
          w=(w<<6)^cc^0x20080;
          if(0x10000<=w){                       // Surrogate pair needed
            cc=str[p+2];
            if(!followUTF8(cc)) goto hex1;      // UTF8 follower?
            w=(w<<6)^cc^0x400080;
            if(0x110000<=w) goto hex1;          // Beyond assigned code space?
            q+=6;
            p++;
            }
          p++;
          }
        q+=6;
        p++;
        continue;
      default:
        if(c==lquote && lquote){                // Escape opening quote if found in string
          q+=2;
          continue;
          }
        if(c==rquote && rquote){                // Escape closing quote if found in string
          q+=2;
          continue;
          }
nml1:   q+=1;                                   // Normal characters
        continue;
      }
    }
  if(rquote) q++;                               // Closing quote
  result.length(q);
  p=q=0;
  if(lquote) result[q++]=lquote;                // Opening quote
  while(p<num){                                 // Then convert the string
    c=str[p++];
    switch(c){
      case '\a':                                // Special control characters
        result[q++]='\\';
        result[q++]='a';
        continue;
      case '\b':
        result[q++]='\\';
        result[q++]='b';
        continue;
      case '\t':
        result[q++]='\\';
        result[q++]='t';
        continue;
      case '\n':
        result[q++]='\\';
        result[q++]='n';
        continue;
      case '\v':
        result[q++]='\\';
        result[q++]='v';
        continue;
      case '\f':
        result[q++]='\\';
        result[q++]='f';
        continue;
      case '\r':
        result[q++]='\\';
        result[q++]='r';
        continue;
      case '\\':
        result[q++]='\\';
        result[q++]='\\';
        continue;
      case 0x00:                                // Non-special control characters
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x0E:
      case 0x0F:
      case 0x10:
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17:
      case 0x18:
      case 0x19:
      case 0x1A:
      case 0x1B:
      case 0x1C:
      case 0x1D:
      case 0x1E:
      case 0x1F:
      case 0x7F:                                // DEL
      case 0xF8:                                // Bad UTF8 leaders
      case 0xF9:
      case 0xFA:
      case 0xFB:
      case 0xFC:
      case 0xFD:
      case 0xFF:
hex2:   result[q++]='\\';                       // Escape as \xHH
        result[q++]='x';
        result[q++]=FXString::value2Digit[(c>>4)&15];
        result[q++]=FXString::value2Digit[c&15];
        continue;
      case 0x80:                                // UTF8 followers
      case 0x81:
      case 0x82:
      case 0x83:
      case 0x84:
      case 0x85:
      case 0x86:
      case 0x87:
      case 0x88:
      case 0x89:
      case 0x8A:
      case 0x8B:
      case 0x8C:
      case 0x8D:
      case 0x8E:
      case 0x8F:
      case 0x90:
      case 0x91:
      case 0x92:
      case 0x93:
      case 0x94:
      case 0x95:
      case 0x96:
      case 0x97:
      case 0x98:
      case 0x99:
      case 0x9A:
      case 0x9B:
      case 0x9C:
      case 0x9D:
      case 0x9E:
      case 0x9F:
      case 0xA0:
      case 0xA1:
      case 0xA2:
      case 0xA3:
      case 0xA4:
      case 0xA5:
      case 0xA6:
      case 0xA7:
      case 0xA8:
      case 0xA9:
      case 0xAA:
      case 0xAB:
      case 0xAC:
      case 0xAD:
      case 0xAE:
      case 0xAF:
      case 0xB0:
      case 0xB1:
      case 0xB2:
      case 0xB3:
      case 0xB4:
      case 0xB5:
      case 0xB6:
      case 0xB7:
      case 0xB8:
      case 0xB9:
      case 0xBA:
      case 0xBB:
      case 0xBC:
      case 0xBD:
      case 0xBE:
      case 0xBF:
        if(flag==0) goto nml2;                  // Pass UTF8 through
        goto hex2;                              // Escape UTF8 follower not preceeded by UTF8 leader
      case 0xC0:                                // 2-byte UTF8 sequences
      case 0xC1:
      case 0xC2:
      case 0xC3:
      case 0xC4:
      case 0xC5:
      case 0xC6:
      case 0xC7:
      case 0xC8:
      case 0xC9:
      case 0xCA:
      case 0xCB:
      case 0xCC:
      case 0xCD:
      case 0xCE:
      case 0xCF:
      case 0xD0:
      case 0xD1:
      case 0xD2:
      case 0xD3:
      case 0xD4:
      case 0xD5:
      case 0xD6:
      case 0xD7:
      case 0xD8:
      case 0xD9:
      case 0xDA:
      case 0xDB:
      case 0xDC:
      case 0xDD:
      case 0xDE:
      case 0xDF:
      case 0xE0:                                // 3-byte UTF8 sequences
      case 0xE1:
      case 0xE2:
      case 0xE3:
      case 0xE4:
      case 0xE5:
      case 0xE6:
      case 0xE7:
      case 0xE8:
      case 0xE9:
      case 0xEA:
      case 0xEB:
      case 0xEC:
      case 0xED:
      case 0xEE:
      case 0xEF:
      case 0xF0:                                 // 4-byte UTF8 sequences
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
        if(flag==0) goto nml2;                  // Pass UTF8 through
        if(flag==1) goto hex2;                  // Simple hex escaping
        cc=str[p];
        if(!followUTF8(cc)) goto hex2;          // UTF8 follower?
        w=(c<<6)^cc^0x3080;
        if(0x800<=w){
          cc=str[p+1];
          if(!followUTF8(cc)) goto hex2;        // UTF8 follower?
          w=(w<<6)^cc^0x20080;
          if(0x10000<=w){                       // Surrogate pair needed
            cc=str[p+2];
            if(!followUTF8(cc)) goto hex2;      // UTF8 follower?
            w=(w<<6)^cc^0x400080;
            if(0x110000<=w) goto hex2;          // Beyond assigned code space?
            v=LEAD_OFFSET+(w>>10);
            w=TAIL_OFFSET+(w&0x3FF);
            result[q++]='\\';                   // Escape as \uHHHH
            result[q++]='u';
            result[q++]=FXString::value2Digit[(v>>12)&15];
            result[q++]=FXString::value2Digit[(v>>8)&15];
            result[q++]=FXString::value2Digit[(v>>4)&15];
            result[q++]=FXString::value2Digit[v&15];
            p++;
            }
          p++;
          }
        result[q++]='\\';                       // Escape as \uHHHH
        result[q++]='u';
        result[q++]=FXString::value2Digit[(w>>12)&15];
        result[q++]=FXString::value2Digit[(w>>8)&15];
        result[q++]=FXString::value2Digit[(w>>4)&15];
        result[q++]=FXString::value2Digit[w&15];
        p++;
        continue;
      default:
        if(__unlikely(c==lquote && lquote)){    // Escape opening quote if found in string
          result[q++]='\\';
          result[q++]=lquote;
          continue;
          }
        if(__unlikely(c==rquote && rquote)){    // Escape closing quote if found in string
          result[q++]='\\';
          result[q++]=rquote;
          continue;
          }
nml2:   result[q++]=c;                          // Normal characters
        continue;
      }
    }
  if(rquote) result[q++]=rquote;                // Closing quote
  FXASSERT(q==result.length());
  return result;
  }


// Escape special characters
FXString FXString::escape(const FXchar* str,FXchar lquote,FXchar rquote,FXint flag){
  return FXString::escape(str,strlen(str),lquote,rquote,flag);
  }


// Escape special characters
FXString FXString::escape(const FXString& str,FXchar lquote,FXchar rquote,FXint flag){
  return FXString::escape(str.text(),str.length(),lquote,rquote,flag);
  }

/*******************************************************************************/

// Unescape special characters in a string; optionally strip quote characters
FXString FXString::unescape(const FXchar* str,FXint num,FXchar lquote,FXchar rquote){
  FXString result;
  FXint p,q,w,c;
  p=q=c=0;
  if(str[p]==lquote && lquote) p++;     // Opening quote
  while(p<num){                         // Measure length of converted string
    w=c;                                // Keep previous decoded character
    c=str[p++];
    if(c==rquote && rquote) break;      // Closing quote
    if(c=='\\' && p<num){               // Escape sequence
      switch((c=str[p++])){
        case 'u':                       // Unicode escape
          c=0;
          if(Ascii::isHexDigit(str[p])){
            c=(c<<4)+Ascii::digitValue(str[p++]);
            if(Ascii::isHexDigit(str[p])){
              c=(c<<4)+Ascii::digitValue(str[p++]);
              if(Ascii::isHexDigit(str[p])){
                c=(c<<4)+Ascii::digitValue(str[p++]);
                if(Ascii::isHexDigit(str[p])){
                  c=(c<<4)+Ascii::digitValue(str[p++]);
                  }
                }
              }
            }
          if(leadUTF16(c)) continue;
          if(followUTF16(c)){
            if(!leadUTF16(w)) continue;
            c=SURROGATE_OFFSET+(w<<10)+c;
            }
          q+=wc2utf(c);
          continue;
        case 'x':                       // Hex escape
          if(Ascii::isHexDigit(str[p])){
            p++;
            if(Ascii::isHexDigit(str[p])) p++;
            }
          q++;
          continue;
        case '\r':                      // End-of-line continuation
          if(str[p]=='\n') p++;         // Eat both <CR> and <LF> of <CR><LF>
          continue;
        case '\n':                      // End-of-line continuation
          continue;
        case '0':                       // Octal escape
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
          if(Ascii::isOctDigit(str[p])){
            p++;
            if(Ascii::isOctDigit(str[p])) p++;
            }
          q++;
          continue;
        case 'n':                       // Special characters
        case 'r':
        case 'b':
        case 'v':
        case 'a':
        case 'e':                       // Escape
        case 'f':
        case 't':
        case '\\':
        default:                        // Unneccessarily escaped character
          q++;
          continue;
        }
      }
    q++;                                // Normal characters
    }
  result.length(q);                     // Resize result string
  p=q=c=0;
  if(str[p]==lquote && lquote) p++;     // Opening quote
  while(p<num){                         // Then convert the string
    w=c;                                // Keep previous decoded character
    c=str[p++];
    if(c==rquote && rquote) break;      // Closing quote
    if(c=='\\' && p<num){               // Escape sequence
      switch((c=str[p++])){
        case 'u':                       // Unicode escape
          if(Ascii::isHexDigit(str[p])){
            c=Ascii::digitValue(str[p++]);
            if(Ascii::isHexDigit(str[p])){
              c=(c<<4)+Ascii::digitValue(str[p++]);
              if(Ascii::isHexDigit(str[p])){
                c=(c<<4)+Ascii::digitValue(str[p++]);
                if(Ascii::isHexDigit(str[p])){
                  c=(c<<4)+Ascii::digitValue(str[p++]);
                  }
                }
              }
            }
          if(leadUTF16(c)) continue;
          if(followUTF16(c)){
            if(!leadUTF16(w)) continue;
            c=SURROGATE_OFFSET+(w<<10)+c;
            }
          q+=wc2utf(&result[q],c);
          continue;
        case 'x':                       // Hex escape
          if(Ascii::isHexDigit(str[p])){
            c=Ascii::digitValue(str[p++]);
            if(Ascii::isHexDigit(str[p])){
              c=(c<<4)+Ascii::digitValue(str[p++]);
              }
            }
          result[q++]=c;
          continue;
        case '\r':                      // End-of-line continuation
          if(str[p]=='\n') p++;         // Eat both <CR> and <LF> of <CR><LF>
          continue;
        case '\n':                      // End-of-line continuation
          continue;
        case '0':                       // Octal escape
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
          c=c-'0';
          if(Ascii::isOctDigit(str[p])){
            c=(c<<3)+str[p++]-'0';
            if(Ascii::isOctDigit(str[p])){
              c=(c<<3)+str[p++]-'0';
              }
            }
          result[q++]=c;
          continue;
        case 'n':                       // Special characters
          result[q++]='\n';
          continue;
        case 'r':
          result[q++]='\r';
          continue;
        case 'b':
          result[q++]='\b';
          continue;
        case 'v':
          result[q++]='\v';
          continue;
        case 'a':
          result[q++]='\a';
          continue;
        case 'e':                       // Escape
          result[q++]='\033';
          continue;
        case 'f':
          result[q++]='\f';
          continue;
        case 't':
          result[q++]='\t';
          continue;
        case '\\':
          result[q++]='\\';
          continue;
        default:                        // Unneccessarily escaped character
          result[q++]=c;
          continue;
        }
      }
    result[q++]=c;                      // Normal characters
    }
  FXASSERT(q==result.length());
  return result;
  }


// Unescape special characters
FXString FXString::unescape(const FXchar* str,FXchar lquote,FXchar rquote){
  return FXString::unescape(str,strlen(str),lquote,rquote);
  }


// Unescape special characters
FXString FXString::unescape(const FXString& str,FXchar lquote,FXchar rquote){
  return FXString::unescape(str.text(),str.length(),lquote,rquote);
  }

/*******************************************************************************/

// Save
FXStream& operator<<(FXStream& store,const FXString& s){
  FXint len=s.length();
  store << len;
  store.save(s.str,len);
  return store;
  }


// Load
FXStream& operator>>(FXStream& store,FXString& s){
  FXint len;
  store >> len;
  s.length(len);
  store.load(s.str,len);
  return store;
  }

/*******************************************************************************/

// Concatenate two FXStrings
FXString operator+(const FXString& s1,const FXString& s2){
  FXString result(s1);
  return result.append(s2);
  }


// Concatenate FXString and string
FXString operator+(const FXString& s1,const FXchar* s2){
  FXString result(s1);
  return result.append(s2);
  }


// Concatenate FXString and narrow character string
FXString operator+(const FXString& s1,const FXnchar* s2){
  FXString result(s1);
  return result.append(s2);
  }


// Concatenate FXString and wide character string
FXString operator+(const FXString& s1,const FXwchar* s2){
  FXString result(s1);
  return result.append(s2);
  }


// Concatenate string and FXString
FXString operator+(const FXchar* s1,const FXString& s2){
  FXString result(s1);
  return result.append(s2);
  }


// Concatenate narrow character string and FXString
FXString operator+(const FXnchar* s1,const FXString& s2){
  FXString result(s1);
  return result.append(s2);
  }


// Concatenate wide character string and FXString
FXString operator+(const FXwchar* s1,const FXString& s2){
  FXString result(s1);
  return result.append(s2);
  }


// Concatenate FXString and character
FXString operator+(const FXString& s,FXchar c){
  FXString result(s);
  return result.append(c);
  }


// Concatenate character and FXString
FXString operator+(FXchar c,const FXString& s){
  FXString result(&c,1);
  return result.append(s);
  }

/*******************************************************************************/

// Convert unix string to dos string by replacing NL by CRNL
FXString& unixToDos(FXString& str){
  FXint i=0,j=0,t=0,c;
  while(j<str.length()){
    str[i++]=c=str[j++];
    if(c=='\n') t++;            // Add CR for every NL
    if(c=='\r') i--;            // But don't double them
    t++;
    }
  str.length(t);
  while(0<t){
    str[--t]=c=str[--i];
    if(c=='\n') str[--t]='\r';
    }
  return str;
  }


// Convert dos string to unix string by replacing CRNL by CR
FXString& dosToUnix(FXString& str){
  FXint i=0,j=0,c;
  while(j<str.length()){
    str[i++]=c=str[j++];
    if(c=='\r') i--;            // Remove CR
    }
  str.length(i);
  return str;
  }

}
