/********************************************************************************
*                                                                               *
*                    A S C I I   C h a r a c t e r   I n f o                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2023 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXASCII_H
#define FXASCII_H


/******** Generated on 2023/06/11 16:58:10 by ascii tool version 3.1.0 *********/


namespace FX {


namespace Ascii {

// Ascii table
extern FXAPI const FXushort ascii_data[256];

// Value to ascii digit table
extern FXAPI const FXchar value_to_digit[256];

// Ascii digit to valuetable
extern FXAPI const FXschar digit_to_value[256];


// Numeric value of ascii digit
static inline FXint digitValue(FXchar asc){
  return digit_to_value[(FXuchar)asc];
  }


// Ascii digit of numeric value
static inline FXint valueDigit(FXuchar asc){
  return value_to_digit[(FXuchar)asc];
  }


// Has upper or lower case variant
static inline FXbool hasCase(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x4000)!=0;
  }


// Is upper case
static inline FXbool isUpper(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x200)!=0;
  }


// Is lower case
static inline FXbool isLower(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x20)!=0;
  }


// Is title case
static inline FXbool isTitle(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x200)!=0;
  }


// Is us-ascii
static inline FXbool isAscii(FXchar asc){
  return ((FXuchar)asc)<128;
  }


// Is letter
static inline FXbool isLetter(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x2)!=0;
  }


// Is decimal digit
static inline FXbool isDigit(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x8)!=0;
  }


// Is letter or digit
static inline FXbool isAlphaNumeric(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x1)!=0;
  }


// Is control character
static inline FXbool isControl(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x4)!=0;
  }


// Is space
static inline FXbool isSpace(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x100)!=0;
  }


// Is blank
static inline FXbool isBlank(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x800)!=0;
  }


// Is punctuation character
static inline FXbool isPunct(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x80)!=0;
  }


// Is graphic character
static inline FXbool isGraph(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x10)!=0;
  }


// Is printing character
static inline FXbool isPrint(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x40)!=0;
  }


// Is hexadecimal digit
static inline FXbool isHexDigit(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x400)!=0;
  }


// Is octal digit
static inline FXbool isOctDigit(FXchar asc){
  return (asc&0xF8)==0x30;
  }


// Is binary digit
static inline FXbool isBinDigit(FXchar asc){
  return (asc&0xFE)==0x30;
  }


// Is word character
static inline FXbool isWord(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x1000)!=0;
  }


// Is delimiter character
static inline FXbool isDelim(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x2000)!=0;
  }


// Convert to upper case
static inline FXchar toUpper(FXchar asc){
  return ((FXuchar)(asc-'a'))<26 ? asc+'A'-'a' : asc;
  }


// Convert to lower case
static inline FXchar toLower(FXchar asc){
  return ((FXuchar)(asc-'A'))<26 ? asc+'a'-'A' : asc;
  }


// Convert to title case
static inline FXchar toTitle(FXchar asc){
  return ((FXuchar)(asc-'a'))<26 ? asc+'A'-'a' : asc;
  }

}

}

#endif
