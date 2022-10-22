/********************************************************************************
*                                                                               *
*                    A S C I I   C h a r a c t e r   I n f o                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxdefs.h"
#include "fxascii.h"
#include "FXString.h"



/******** Generated on 2013/07/17 09:50:17 by ascii tool version 3.0.0 *********/


using namespace FX;

namespace FX {

namespace Ascii {

// Ascii table
const unsigned short ascii_data[256]={
  0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x0904,0x0104,0x0104,0x0104,0x0104,0x0004,0x0004,
  0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,0x0004,
  0x0940,0x20d0,0x20d0,0x20d0,0x20d0,0x20d0,0x20d0,0x20d0,0x20d0,0x20d0,0x20d0,0x20d0,0x20d0,0x20d0,0x20d0,0x20d0,
  0x1459,0x1459,0x1459,0x1459,0x1459,0x1459,0x1459,0x1459,0x1459,0x1459,0x20d0,0x20d0,0x20d0,0x20d0,0x20d0,0x20d0,
  0x20d0,0x5653,0x5653,0x5653,0x5653,0x5653,0x5653,0x5253,0x5253,0x5253,0x5253,0x5253,0x5253,0x5253,0x5253,0x5253,
  0x5253,0x5253,0x5253,0x5253,0x5253,0x5253,0x5253,0x5253,0x5253,0x5253,0x5253,0x20d0,0x20d0,0x20d0,0x20d0,0x10d0,
  0x20d0,0x5473,0x5473,0x5473,0x5473,0x5473,0x5473,0x5073,0x5073,0x5073,0x5073,0x5073,0x5073,0x5073,0x5073,0x5073,
  0x5073,0x5073,0x5073,0x5073,0x5073,0x5073,0x5073,0x5073,0x5073,0x5073,0x5073,0x20d0,0x20d0,0x20d0,0x20d0,0x0004,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
  };


FXint digitValue(FXchar asc){
  return FXString::digit2Value[(FXuchar)asc];
  }


FXbool hasCase(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x4000)!=0;
  }


FXbool isUpper(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x200)!=0;
  }


FXbool isLower(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x20)!=0;
  }


FXbool isTitle(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x200)!=0;
  }


FXbool isAscii(FXchar asc){
  return ((FXuchar)asc)<128;
  }


FXbool isLetter(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x2)!=0;
  }


FXbool isDigit(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x8)!=0;
  }


FXbool isAlphaNumeric(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x1)!=0;
  }


FXbool isControl(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x4)!=0;
  }


FXbool isSpace(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x100)!=0;
  }


FXbool isBlank(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x800)!=0;
  }


FXbool isPunct(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x80)!=0;
  }


FXbool isGraph(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x10)!=0;
  }


FXbool isPrint(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x40)!=0;
  }


FXbool isHexDigit(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x400)!=0;
  }


FXbool isOctDigit(FXchar asc){
  return (asc&0xF8)==0x30;
  }


FXbool isBinDigit(FXchar asc){
  return (asc&0xFE)==0x30;
  }


FXbool isWord(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x1000)!=0;
  }


FXbool isDelim(FXchar asc){
  return (ascii_data[(FXuchar)asc]&0x2000)!=0;
  }


FXchar toUpper(FXchar asc){
  return ('a'<=asc && asc<='z') ? (asc-'a'+'A') : asc;
  }


FXchar toLower(FXchar asc){
  return ('A'<=asc && asc<='Z') ? (asc-'A'+'a') : asc;
  }


FXchar toTitle(FXchar asc){
  return ('a'<=asc && asc<='z') ? (asc-'a'+'A') : asc;
  }

}

}

