/********************************************************************************
*                                                                               *
*                            J S O N   F i l e   I / O                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXParseBuffer.h"
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

/*******************************************************************************/

namespace FX {

// Create JSON file i/o object
FXJSONFile::FXJSONFile(){
  FXTRACE((100,"FXJSONFile::FXJSONFile\n"));
  }


// Create JSON file i/o object and open it
FXJSONFile::FXJSONFile(const FXString& filename,Direction d,FXuval sz){
  FXTRACE((100,"FXJSONFile::FXJSONFile(\"%s\",%s,%ld)\n",filename.text(),(d==Save)?"Save":(d==Load)?"Load":"Stop",sz));
  open(filename,d,sz);
  }


// Open JSON file for load or save
FXbool FXJSONFile::open(const FXString& filename,Direction d,FXuval sz){
  FXTRACE((101,"FXJSONFile::open(\"%s\",%s,%ld)\n",filename.text(),(d==Save)?"Save":(d==Load)?"Load":"Stop",sz));
  if(dir==Stop){
    FXchar *buffer;
    if(allocElms(buffer,sz)){
      if(file.open(filename,(d==Save)?FXIO::Writing:FXIO::Reading,FXIO::AllReadWrite)){
        if(FXJSON::open(buffer,sz,d)){
          rptr=endptr;
          sptr=endptr;
          wptr=endptr;
          return true;
          }
        file.close();
        }
      freeElms(buffer);
      }
    }
  return false;
  }


// Read at least count bytes into buffer; return bytes available, or -1 for error
FXival FXJSONFile::fill(FXival){
  if(dir==Load){
    FXival nbytes;
    moveElms(begptr,rptr,wptr-rptr);
    wptr=begptr+(wptr-rptr);
    sptr=begptr+(sptr-rptr);
    rptr=begptr;
    nbytes=file.readBlock(wptr,endptr-wptr);
    FXTRACE((104,"FXJSONFile::fill() = %ld\n",nbytes));
    if(0<=nbytes){
      wptr+=nbytes;
      if(wptr<endptr){ wptr[0]='\0'; }
      return wptr-sptr;                         // Input left to read
      }
    }
  return -1;
  }


// Write at least count bytes from buffer; return space available, or -1 for error
FXival FXJSONFile::flush(FXival){
  if(dir==Save){
    FXival nbytes;
    nbytes=file.writeBlock(rptr,wptr-rptr);
    FXTRACE((104,"FXJSONFile::flush() = %ld\n",nbytes));
    if(0<=nbytes){
      rptr+=nbytes;
      moveElms(begptr,rptr,wptr-rptr);
      wptr=begptr+(wptr-rptr);
      rptr=begptr;
      return endptr-wptr;                       // Space left to write
      }
    }
  return -1;
  }


// Close stream and delete buffers
FXbool FXJSONFile::close(){
  FXTRACE((101,"FXJSONFile::close()\n"));
  FXchar *buffer=begptr;
  if(FXJSON::close()){
    freeElms(buffer);
    return file.close();
    }
  return false;
  }


// Close JSON file
FXJSONFile::~FXJSONFile(){
  FXTRACE((100,"FXJSONFile::~FXJSONFile\n"));
  close();
  }

}
