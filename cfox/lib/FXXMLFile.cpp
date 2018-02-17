/********************************************************************************
*                                                                               *
*                             X M L - F i l e   I / O                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2016,2017 by Jeroen van der Zijp.   All Rights Reserved.        *
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
  FXTRACE((1,"FXXMLFile::FXXMLFile\n"));
  }


// Create XML file i/o object and open it
FXXMLFile::FXXMLFile(const FXString& filename,Direction d,FXuval sz){
  FXTRACE((1,"FXXMLFile::FXXMLFile(\"%s\",%s,%lu)\n",filename.text(),(d==Save)?"Save":(d==Load)?"Load":"Stop",sz));
  open(filename,d,sz);
  }


// Open archive for operation
FXbool FXXMLFile::open(FXInputHandle h,Direction d,FXuval sz){
  FXTRACE((2,"FXXMLFile::open(%lx,%s,%lu)\n",h,(d==Save)?"Save":(d==Load)?"Load":"Stop",sz));
  if(FXXML::open(NULL,sz,d)){
    if(file.open(h,(d==Save)?FXIO::Writing:FXIO::Reading)){
      return true;
      }
    close();
    }
  return false;
  }


// Open archive for operation
FXbool FXXMLFile::open(const FXString& filename,Direction d,FXuval sz){
  FXTRACE((2,"FXXMLFile::open(\"%s\",%s,%lu)\n",filename.text(),(d==Save)?"Save":(d==Load)?"Load":"Stop",sz));
  if(FXXML::open(NULL,sz,d)){
    if(file.open(filename,(d==Save)?FXIO::Writing:FXIO::Reading,FXIO::AllReadWrite)){
      return true;
      }
    close();
    }
  return false;
  }


// Fill buffer from file
FXbool FXXMLFile::fill(){
  register FXival n;
  if(file.isReadable()){
    if(sptr<wptr){ moveElms(begptr,sptr,wptr-sptr); }
    wptr=begptr+(wptr-sptr);
    rptr=begptr+(rptr-sptr);
    sptr=begptr;
    n=file.readBlock(wptr,endptr-wptr);
    FXTRACE((2,"FXXMLFile::fill() = %ld\n",n));
    if(0<=n){
      wptr+=n;
      if(wptr<endptr){ wptr[0]='\0'; }
      return rptr<wptr;         // Input left to read
      }
    }
  return false;
  }


// Flush buffer to file
FXbool FXXMLFile::flush(){
  register FXival n;
  if(file.isWritable()){
    n=file.writeBlock(rptr,wptr-rptr);
    FXTRACE((2,"FXXMLFile::flush() = %ld\n",n));
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
FXbool FXXMLFile::close(){
  FXTRACE((2,"FXXMLFile::close()\n"));
  if(FXXML::close()){
    return file.close();
    }
  return false;
  }


// Close XML file
FXXMLFile::~FXXMLFile(){
  FXTRACE((1,"FXXMLFile::~FXXMLFile\n"));
  close();
  }

}
