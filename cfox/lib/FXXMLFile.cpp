/********************************************************************************
*                                                                               *
*                             X M L - F i l e   I / O                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2016,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXElement.h"
#include "FXArray.h"
#include "FXString.h"
#include "FXParseBuffer.h"
#include "FXIO.h"
#include "FXIODevice.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXException.h"
#include "FXStringDictionary.h"
#include "FXCallback.h"
#include "FXXML.h"
#include "FXXMLFile.h"


/*
  Notes:

  - XML serialization to a file.
  - To fill buffer, move unread bytes to start, then load as from file as will fit;
    thus, wptr==endptr indicates the file is not fully loaded yet, while wptr<endptr
    means we've reached end of file.
  - To flush buffer, try write all bytes from buffer; if not able to write it all,
    move unwritten bytes to start to have maximum of free space in buffer.
*/


using namespace FX;

namespace FX {

/*******************************************************************************/

// Create XML file i/o object
FXXMLFile::FXXMLFile(){
  FXTRACE((100,"FXXMLFile::FXXMLFile\n"));
  }


// Create XML file i/o object and open it
FXXMLFile::FXXMLFile(const FXString& filename,Direction d,FXuval sz){
  FXTRACE((100,"FXXMLFile::FXXMLFile(\"%s\",%s,%lu)\n",filename.text(),(d==Save)?"Save":(d==Load)?"Load":"Stop",sz));
  open(filename,d,sz);
  }


// Open archive for operation
FXbool FXXMLFile::open(FXInputHandle h,Direction d,FXuval sz){
  FXTRACE((101,"FXXMLFile::open(%lx,%s,%lu)\n",(FXuval)h,(d==Save)?"Save":(d==Load)?"Load":"Stop",sz));
  FXchar *buffer;
  if(allocElms(buffer,sz)){
    if(file.open(h,(d==Save)?FXIO::Writing:FXIO::Reading)){
      if(FXXML::open(buffer,sz,d)){
        rptr=endptr;
        sptr=endptr;
        wptr=endptr;
        return true;
        }
      file.close();
      }
    freeElms(buffer);
    }
  return false;
  }


// Open archive for operation
FXbool FXXMLFile::open(const FXString& filename,Direction d,FXuval sz){
  FXTRACE((101,"FXXMLFile::open(\"%s\",%s,%lu)\n",filename.text(),(d==Save)?"Save":(d==Load)?"Load":"Stop",sz));
  FXchar *buffer;
  FXASSERT(dir==Stop);
  if(allocElms(buffer,sz)){
    if(file.open(filename,(d==Save)?FXIO::Writing:FXIO::Reading,FXIO::AllReadWrite)){
      if(FXXML::open(buffer,sz,d)){
        rptr=endptr;
        sptr=endptr;
        wptr=endptr;
        return true;
        }
      file.close();
      }
    freeElms(buffer);
    }
  return false;
  }


// Read at least count bytes into buffer; return bytes available, or -1 for error
FXival FXXMLFile::fill(FXival){
  FXival nbytes;
  FXASSERT(dir==Load);
  if(file.isReadable()){
    moveElms(begptr,rptr,wptr-rptr);
    wptr=begptr+(wptr-rptr);
    sptr=begptr+(sptr-rptr);
    rptr=begptr;
    nbytes=file.readBlock(wptr,endptr-wptr);
    if(0<=nbytes){
      wptr+=nbytes;
      if(wptr<endptr){ wptr[0]='\0'; }
      return wptr-sptr;                         // Input left to read
      }
    }
  return -1;
  }


// Write at least count bytes from buffer; return space available, or -1 for error
FXival FXXMLFile::flush(FXival){
  FXival nbytes;
  FXASSERT(dir==Save);
  if(file.isWritable()){
    nbytes=file.writeBlock(rptr,wptr-rptr);
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
FXbool FXXMLFile::close(){
  FXTRACE((101,"FXXMLFile::close()\n"));
  FXchar *buffer=begptr;
  if(FXXML::close()){
    freeElms(buffer);
    return file.close();
    }
  return false;
  }


// Close XML file
FXXMLFile::~FXXMLFile(){
  FXTRACE((100,"FXXMLFile::~FXXMLFile\n"));
  close();
  }

}
