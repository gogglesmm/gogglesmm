/********************************************************************************
*                                                                               *
*                            J S O N   S t r i n g   I / O                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2018,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxmath.h"
#include "fxascii.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXIODevice.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXException.h"
#include "FXVariant.h"
#include "FXVariantArray.h"
#include "FXVariantMap.h"
#include "FXJSON.h"
#include "FXJSONString.h"

/*
  Notes:

  - JSON Serialization to a string.
  - Should default behaviour be to append to the given string instead of overwriting
    it (if writing)?
*/

using namespace FX;

namespace FX {

/*******************************************************************************/

// Create JSON file i/o object
FXJSONString::FXJSONString(){
  FXTRACE((100,"FXJSONString::FXJSONString\n"));
  }


// Create JSON string i/o object and open it
FXJSONString::FXJSONString(const FXString& string,Direction d){
  FXTRACE((100,"FXJSONString::FXJSONString(\"%.*s\",%s)\n",(FXint)FXMIN(string.length(),16),string.text(),(d==Save)?"Save":(d==Load)?"Load":"Stop"));
  open(string,d);
  }


// Create JSON string i/o object and open it
FXJSONString::FXJSONString(const FXchar* string,FXuval length,Direction d){
  FXTRACE((100,"FXJSONString::FXJSONString(\"%.*s\",%lu,%s)\n",(FXint)FXMIN(length,16),string,length,(d==Save)?"Save":(d==Load)?"Load":"Stop"));
  open(string,length,d);
  }


// Open archive for operation
FXbool FXJSONString::open(const FXString& string,Direction d){
  FXTRACE((101,"FXJSONString::open(\"%.*s\",%s)\n",(FXint)FXMIN(string.length(),16),string.text(),(d==Save)?"Save":(d==Load)?"Load":"Stop"));
  FXASSERT(dir==Stop);
  buffer=string;
  if(FXJSON::open(buffer.text(),buffer.length(),d)){
    // wptr=endptr;             // Append to end of buffer
    return true;
    }
  return false;
  }


// Open archive for operation
FXbool FXJSONString::open(const FXchar* string,FXuval length,Direction d){
  FXTRACE((101,"FXJSONString::open(\"%.*s\",%lu,%s)\n",(FXint)FXMIN(length,16),string,length,(d==Save)?"Save":(d==Load)?"Load":"Stop"));
  FXASSERT(dir==Stop);
  buffer.assign(string,length);
  if(FXJSON::open(buffer.text(),buffer.length(),d)){
    // wptr=endptr;             // Append to end of buffer
    return true;
    }
  return false;
  }


// Read at least count bytes into buffer; return bytes available, or -1 for error
FXival FXJSONString::fill(FXival){
  FXASSERT(dir==Load);
  if(dir==Load){
    return wptr-sptr;
    }
  return -1;
  }


// Write at least count bytes from buffer; return space available, or -1 for error
FXival FXJSONString::flush(FXival count){
  FXASSERT(dir==Save);
  if(dir==Save){
    FXchar *oldptr=begptr;
    buffer.length(buffer.length()+count);
    begptr=buffer.text();
    endptr=buffer.text()+buffer.length();
    rptr=begptr+(rptr-oldptr);
    sptr=begptr+(sptr-oldptr);
    wptr=begptr+(wptr-oldptr);
    return endptr-wptr;
    }
  return -1;
  }


// Close stream and delete buffers
FXbool FXJSONString::close(){
  FXTRACE((101,"FXJSONString::close()\n"));
  return FXJSON::close();
  }


// Close JSON file
FXJSONString::~FXJSONString(){
  FXTRACE((100,"FXJSONString::~FXJSONString\n"));
  close();
  }

}
