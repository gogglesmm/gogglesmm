/********************************************************************************
*                                                                               *
*     U n i c o d e   C h a r a c t e r   E n c o d i n g   S u p p o r t       *
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
#ifndef FXCHAR_H
#define FXCHAR_H


namespace FX {


/***************************  UTF8 Support Functions  **************************/

/// Test if character c is at the start of a UTF8 sequence
static inline FXbool isUTF8(FXchar c){
  return (c&0xC0)!=0x80;
  }

/// Check if c is leader of a UTF8 multi-byte sequence
static inline FXbool leadUTF8(FXchar c){
  return (c&0xC0)==0xC0;
  }

/// Check if c is follower of a UTF8 multi-byte sequence
static inline FXbool followUTF8(FXchar c){
  return (c&0xC0)==0x80;
  }

/// Check if c is part of multi-byte UTF8 sequence
static inline FXbool seqUTF8(FXchar c){
  return (c&0x80)==0x80;
  }

/// Length of UTF8 character, in bytes
static inline FXival lenUTF8(FXchar c){
  return ((0xE5000000>>((c>>3)&0x1E))&3)+1;
  }

/// Length of UTF8 character, in bytes
static inline FXival wclen(const FXchar *ptr){
  return lenUTF8(*ptr);
  }

/// Return wide character from UTF8 string
static inline FXwchar wc(const FXchar* ptr){
  FXwchar w=(FXuchar)ptr[0];
  if(0xC0<=w){ w = (w<<6) ^ (FXuchar)ptr[1] ^ 0x3080;
  if(0x800<=w){ w = (w<<6) ^ (FXuchar)ptr[2] ^ 0x20080;
  if(0x10000<=w){ w = (w<<6) ^ (FXuchar)ptr[3] ^ 0x400080; }}}
  return w;
  }

/// Return wide character from 1-byte UTF8 string
static inline FXwchar wc1(const FXchar* ptr){
  return (FXuchar)ptr[0];
  }

/// Return wide character from 2-byte UTF8 string
static inline FXwchar wc2(const FXchar* ptr){
  return ((FXuchar)ptr[0]<<6)^((FXuchar)ptr[1])^0x3080;
  }

/// Return wide character from 3-byte UTF8 string
static inline FXwchar wc3(const FXchar* ptr){
  return ((FXuchar)ptr[0]<<12)^((FXuchar)ptr[1]<<6)^((FXuchar)ptr[2])^0x0E2080;
  }

/// Return wide character from 4-byte UTF8 string
static inline FXwchar wc4(const FXchar* ptr){
  return ((FXuchar)ptr[0]<<18)^((FXuchar)ptr[1]<<12)^((FXuchar)ptr[2]<<6)^((FXuchar)ptr[3])^0x3C82080;
  }

/// Return wide character from UTF8 string, and go to next wide character
static inline FXwchar wcnxt(const FXchar*& ptr){
  FXwchar w=(FXuchar)*ptr++;
  if(0xC0<=w){ w = (w<<6) ^ (FXuchar)*ptr++ ^ 0x3080;
  if(0x800<=w){ w = (w<<6) ^ (FXuchar)*ptr++ ^ 0x20080;
  if(0x10000<=w){ w = (w<<6) ^ (FXuchar)*ptr++ ^ 0x400080; }}}
  return w;
  }

/// Return wide character from UTF8 string, and go to next wide character
static inline FXwchar wcnxt(FXchar*& ptr){
  FXwchar w=(FXuchar)*ptr++;
  if(0xC0<=w){ w = (w<<6) ^ (FXuchar)*ptr++ ^ 0x3080;
  if(0x800<=w){ w = (w<<6) ^ (FXuchar)*ptr++ ^ 0x20080;
  if(0x10000<=w){ w = (w<<6) ^ (FXuchar)*ptr++ ^ 0x400080; }}}
  return w;
  }

/// Go to previous wide character from UTF8 string, and return it
static inline FXwchar wcprv(const FXchar*& ptr){
  FXwchar w=(FXuchar)*--ptr;
  if(0x80<=w){ w = ((FXuchar)*--ptr<<6) ^ w ^ 0x3080;
  if(0x1000<=w){ w = ((FXuchar)*--ptr<<12) ^ w ^ 0xE1000;
  if(0x20000<=w){ w = ((FXuchar)*--ptr<<18) ^ w ^ 0x3C60000; }}}
  return w;
  }

/// Go to previous wide character from UTF8 string, and return it
static inline FXwchar wcprv(FXchar*& ptr){
  FXwchar w=(FXuchar)*--ptr;
  if(0x80<=w){ w = ((FXuchar)*--ptr<<6) ^ w ^ 0x3080;
  if(0x1000<=w){ w = ((FXuchar)*--ptr<<12) ^ w ^ 0xE1000;
  if(0x20000<=w){ w = ((FXuchar)*--ptr<<18) ^ w ^ 0x3C60000; }}}
  return w;
  }

/// Increment to start of next wide character in utf8 string
static inline const FXchar* wcinc(const FXchar* ptr){
  return (isUTF8(*++ptr) || isUTF8(*++ptr) || isUTF8(*++ptr) || ++ptr), ptr;
  }

/// Increment to start of next wide character in utf8 string
static inline FXchar* wcinc(FXchar* ptr){
  return (isUTF8(*++ptr) || isUTF8(*++ptr) || isUTF8(*++ptr) || ++ptr), ptr;
  }

/// Decrement to start of previous wide character in utf8 string
static inline const FXchar* wcdec(const FXchar* ptr){
  return (isUTF8(*--ptr) || isUTF8(*--ptr) || isUTF8(*--ptr) || --ptr), ptr;
  }

/// Decrement to start of previous wide character in utf8 string
static inline FXchar* wcdec(FXchar* ptr){
  return (isUTF8(*--ptr) || isUTF8(*--ptr) || isUTF8(*--ptr) || --ptr), ptr;
  }

/// Adjust ptr to point to leader of multi-byte sequence
static inline const FXchar* wcstart(const FXchar* ptr){
  return (isUTF8(*ptr) || isUTF8(*--ptr) || isUTF8(*--ptr) || --ptr), ptr;
  }

/// Adjust ptr to point to leader of multi-byte sequence
static inline FXchar* wcstart(FXchar* ptr){
  return (isUTF8(*ptr) || isUTF8(*--ptr) || isUTF8(*--ptr) || --ptr), ptr;
  }

/**************************  UTF16 Support Functions  **************************/

/// Test if character c is at start of UTF16 sequence
static inline FXbool isUTF16(FXnchar c){
  return (c&0xFC00)!=0xDC00;
  }

/// Check if c is leader of a UTF16 surrogate pair sequence
static inline FXbool leadUTF16(FXnchar c){
  return (c&0xFC00)==0xD800;
  }

/// Check if c is follower of a UTF16 surrogate pair sequence
static inline FXbool followUTF16(FXnchar c){
  return (c&0xFC00)==0xDC00;
  }

/// Check if c is part of multi-word UTF16 sequence
static inline FXbool seqUTF16(FXnchar c){
  return (c&0xF800)==0xD800;
  }

/// Length of UTF16 character, in words
static inline FXival lenUTF16(FXnchar c){
  return leadUTF16(c)+1;
  }

/// Return number of words of narrow character at ptr
static inline FXival wclen(const FXnchar *ptr){
  return lenUTF16(*ptr);
  }

/// Return wide character from utf16 string
static inline FXwchar wc(const FXnchar* ptr){
  FXwchar w=ptr[0];
  if(leadUTF16(w)){ w = (w<<10) + ptr[1] - 0x35FDC00; }
  return w;
  }

/// Return wide character from 1-word utf16 string
static inline FXwchar wc1(const FXnchar* ptr){
  return ptr[0];
  }

/// Return wide character from 2-word utf16 string
static inline FXwchar wc2(const FXnchar* ptr){
  return (ptr[0]<<10)+ptr[1]-0x35FDC00;
  }

/// Return wide character from UTF16 string, and go to next wide character
static inline FXwchar wcnxt(const FXnchar*& ptr){
  FXwchar w=*ptr++;
  if(leadUTF16(w)){ w = (w<<10) + *ptr++ - 0x35FDC00; }
  return w;
  }

/// Return wide character from UTF16 string, and go to next wide character
static inline FXwchar wcnxt(FXnchar*& ptr){
  FXwchar w=*ptr++;
  if(leadUTF16(w)){ w = (w<<10) + *ptr++ - 0x35FDC00; }
  return w;
  }

/// Go to previous wide character from UTF16 string, and return it
static inline FXwchar wcprv(const FXnchar*& ptr){
  FXwchar w=*--ptr;
  if(followUTF16(w)){ w = (*--ptr<<10) + w - 0x35FDC00; }
  return w;
  }

/// Go to previous wide character from UTF16 string, and return it
static inline FXwchar wcprv(FXnchar*& ptr){
  FXwchar w=*--ptr;
  if(followUTF16(w)){ w = (*--ptr<<10) + w - 0x35FDC00; }
  return w;
  }

/// Safely go to begin of next utf8 character
static inline const FXnchar* wcinc(const FXnchar* ptr){
  return (isUTF16(*++ptr) || ++ptr), ptr;
  }

/// Safely go to begin of next utf8 character
static inline FXnchar* wcinc(FXnchar* ptr){
  return (isUTF16(*++ptr) || ++ptr), ptr;
  }

/// Safely go to begin of previous utf8 character
static inline const FXnchar* wcdec(const FXnchar* ptr){
  return (isUTF16(*--ptr) || --ptr), ptr;
  }

/// Safely go to begin of previous utf8 character
static inline FXnchar* wcdec(FXnchar* ptr){
  return (isUTF16(*--ptr) || --ptr), ptr;
  }

/// Adjust ptr to point to leader of surrogate pair sequence
static inline const FXnchar* wcstart(const FXnchar *ptr){
  return (isUTF16(*ptr) || --ptr), ptr;
  }

/// Adjust ptr to point to leader of surrogate pair sequence
static inline FXnchar* wcstart(FXnchar *ptr){
  return (isUTF16(*ptr) || --ptr), ptr;
  }

/// Test if c is a legal utf32 character
static inline FXbool isUTF32(FXwchar c){
  return c<0x110000;
  }

/***********************  Measure  Encoding  Conversions  **********************/

/// Return number of bytes for utf8 representation of wide character w
static inline FXival wc2utf(FXwchar w){ return 1+(0x80<=w)+(0x800<=w)+(0x10000<=w); }

/// Return number of narrow characters for utf16 representation of wide character w
static inline FXival wc2nc(FXwchar w){ return 1+(0x10000<=w); }

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


/************************  Encoding  Conversions  ******************************/

/// Convert wide character to utf8 string; return number of items written to dst
extern FXAPI FXival wc2utf(FXchar *dst,FXwchar w);

/// Convert wide character to narrow character string; return number of items written to dst
extern FXAPI FXival wc2nc(FXnchar *dst,FXwchar w);

/// Convert wide character string to utf8 string; return number of items written to dst
extern FXAPI FXival wcs2utf(FXchar *dst,const FXwchar* src,FXival dstlen,FXival srclen);
extern FXAPI FXival wcs2utf(FXchar *dst,const FXwchar* src,FXival dstlen);

/// Convert narrow character string to utf8 string; return number of items written to dst
extern FXAPI FXival ncs2utf(FXchar *dst,const FXnchar* src,FXival dstlen,FXival srclen);
extern FXAPI FXival ncs2utf(FXchar *dst,const FXnchar* src,FXival dstlen);

/// Convert utf8 string to wide character string; return number of items written to dst
extern FXAPI FXival utf2wcs(FXwchar *dst,const FXchar* src,FXival dstlen,FXival srclen);
extern FXAPI FXival utf2wcs(FXwchar *dst,const FXchar* src,FXival dstlen);

/// Convert utf8 string to narrow character string; return number of items written to dst
extern FXAPI FXival utf2ncs(FXnchar *dst,const FXchar* src,FXival dstlen,FXival srclen);
extern FXAPI FXival utf2ncs(FXnchar *dst,const FXchar* src,FXival dstlen);

}

#endif
