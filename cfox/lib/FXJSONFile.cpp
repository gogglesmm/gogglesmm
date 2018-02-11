/********************************************************************************
*                                                                               *
*                            J S O N   F i l e   I / O                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2017 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXJSONFile.h"

/*
  Notes:

  - JSON Serialization to a file.
*/

using namespace FX;

namespace FX {

/*******************************************************************************/

// Create JSON file i/o object
FXJSONFile::FXJSONFile(){
  FXTRACE((1,"FXJSONFile::FXJSONFile\n"));
  }


// Create JSON file i/o object and open it
FXJSONFile::FXJSONFile(const FXString& filename,Direction d,FXuval sz){
  FXTRACE((1,"FXJSONFile::FXJSONFile(\"%s\",%s,%ld)\n",filename.text(),(d==Save)?"Save":(d==Load)?"Load":"Stop",sz));
  open(filename,d,sz);
  }


// Open archive for operation
FXbool FXJSONFile::open(FXInputHandle h,Direction d,FXuval sz){
  FXTRACE((1,"FXJSONFile::open(%lx,%s,%ld)\n",h,(d==Save)?"Save":(d==Load)?"Load":"Stop",sz));
  if(FXJSON::open(NULL,sz,d)){
    if(file.open(h,(d==Save)?FXIO::Writing:FXIO::Reading)){
      return true;
      }
    close();
    }
  return false;
  }


// Open archive for operation
FXbool FXJSONFile::open(const FXString& filename,Direction d,FXuval sz){
  FXTRACE((1,"FXJSONFile::open(\"%s\",%s,%ld)\n",filename.text(),(d==Save)?"Save":(d==Load)?"Load":"Stop",sz));
  if(FXJSON::open(NULL,sz,d)){
    if(file.open(filename,(d==Save)?FXIO::Writing:FXIO::Reading,FXIO::AllReadWrite)){
      return true;
      }
    close();
    }
  return false;
  }


// Fill buffer from file
FXbool FXJSONFile::fill(){
  register FXival n;
  if(file.isReadable()){
    if(sptr<wptr){ moveElms(begptr,sptr,wptr-sptr); }
    wptr=begptr+(wptr-sptr);
    rptr=begptr+(rptr-sptr);
    sptr=begptr;
    n=file.readBlock(wptr,endptr-wptr);
    FXTRACE((2,"FXJSONFile::fill() = %ld\n",n));
    if(0<=n){
      wptr+=n;
      if(wptr<endptr){ wptr[0]='\0'; }
      return rptr<wptr;         // Input left to read
      }
    }
  return false;
  }


// Flush buffer to file
FXbool FXJSONFile::flush(){
  register FXival n;
  if(file.isWritable()){
    n=file.writeBlock(rptr,wptr-rptr);
    FXTRACE((2,"FXJSONFile::flush() = %ld\n",n));
    if(0<=n){
      rptr+=n;
      if(rptr<wptr){ moveElms(begptr,rptr,wptr-rptr); }
      wptr=begptr+(wptr-rptr);
      rptr=begptr;
      return wptr<endptr;       // Space left to write
      }
    }
  return false;
  }


// Close stream and delete buffers
FXbool FXJSONFile::close(){
  FXTRACE((2,"FXJSONFile::close()\n"));
  if(FXJSON::close()){
    return file.close();
    }
  return false;
  }


// Close JSON file
FXJSONFile::~FXJSONFile(){
  FXTRACE((1,"FXJSONFile::~FXJSONFile\n"));
  close();
  }

}
