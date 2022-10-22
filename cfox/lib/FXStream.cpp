/********************************************************************************
*                                                                               *
*       P e r s i s t e n t   S t o r a g e   S t r e a m   C l a s s e s       *
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
#include "fxmath.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXObject.h"
#include "FXDLL.h"


/*
  Notes:
  - Defer malloc in FXStream::open till object is actually being saved/loaded!
  - Programming errors are punished by calling fxerror(); for example, saving
    into a stream which is set for loading, NULL buffer pointers, and so on.
  - Status codes are set when a correct program encounters errors such as
    writing to full disk, running out of memory, and so on.
  - Single character insert/extract operators are virtual so subclasses can
    overload these specific cases for greater speed.
  - Copy byte at a time because don't know about alignment of stream buffer;
    unaligned accesses are disallowed on some RISC cpus.
  - Buffer can not be written till after first call to writeBuffer().
  - Need to haul some memory stream stuff up (buffer mgmt).
  - Need to have load() and save() API's return number of elements ACTUALLY
    transferred (not bytes but whole numbers of elements!).
*/


#define MAXCLASSNAME       256          // Maximum class name length


using namespace FX;


/*******************************************************************************/

namespace FX {


// Create PersistentStore object
FXStream::FXStream(const FXObject *cont){
  parent=cont;
  begptr=nullptr;
  endptr=nullptr;
  wrptr=nullptr;
  rdptr=nullptr;
  pos=0L;
  dir=FXStreamDead;
  code=FXStreamOK;
  seq=0x80000000;
  swap=false;
  owns=false;
  }


// Set stream to big endian mode if true
void FXStream::setBigEndian(FXbool big){
  swap=(big^FOX_BIGENDIAN);
  }


// Return true if big endian mode.
FXbool FXStream::isBigEndian() const {
  return (swap^FOX_BIGENDIAN);
  }


// Destroy PersistentStore object
FXStream::~FXStream(){
  if(owns){freeElms(begptr);}
  parent=(FXObject*)-1L;
  begptr=(FXuchar*)-1L;
  endptr=(FXuchar*)-1L;
  wrptr=(FXuchar*)-1L;
  rdptr=(FXuchar*)-1L;
  }


// Write at least count bytes from the buffer; the default
// implementation simply discards all data in the buffer.
// It returns the amount of room available to be written.
FXuval FXStream::writeBuffer(FXuval){
  rdptr=begptr;
  wrptr=begptr;
  return endptr-wrptr;
  }


// Read at least count bytes into the buffer; the default
// implementation reads an endless stream of zero's.
// It returns the amount of data available to be read.
FXuval FXStream::readBuffer(FXuval){
  rdptr=begptr;
  wrptr=endptr;
  return wrptr-rdptr;
  }


// Set status code
void FXStream::setError(FXStreamStatus err){
  code=err;
  }


// Get available space
FXuval FXStream::getSpace() const {
  return endptr-begptr;
  }


// Set available space
void FXStream::setSpace(FXuval size){
  if(code==FXStreamOK){

    // Changed size?
    if(begptr+size!=endptr){

      // Old buffer location
      FXuchar *oldbegptr=begptr;

      // Only resize if owned
      if(!owns){ fxerror("FXStream::setSpace: cannot resize external data buffer.\n"); }

      // Resize the buffer
      if(!resizeElms(begptr,size)){ code=FXStreamAlloc; return; }

      // Adjust pointers, buffer may have moved
      endptr=begptr+size;
      wrptr=begptr+(wrptr-oldbegptr);
      rdptr=begptr+(rdptr-oldbegptr);
      if(wrptr>endptr) wrptr=endptr;
      if(rdptr>endptr) rdptr=endptr;
      }
    }
  }


// Open for save or load
FXbool FXStream::open(FXStreamDirection save_or_load,FXuchar* data,FXuval size,FXbool owned){
  if(save_or_load!=FXStreamSave && save_or_load!=FXStreamLoad){fxerror("FXStream::open: illegal stream direction.\n");}
  if(!dir){

    // Use external buffer space
    if(data){
      begptr=data;
      if(size==~0UL)
        endptr=(FXuchar*)(FXival)-1L;
      else
        endptr=begptr+size;
      wrptr=begptr;
      rdptr=begptr;
      owns=owned;
      }

    // Use internal buffer space
    else{
      if(!callocElms(begptr,size)){ code=FXStreamAlloc; return false; }
      endptr=begptr+size;
      wrptr=begptr;
      rdptr=begptr;
      owns=true;
      }

    // Reset variables
    hash.clear();
    dir=save_or_load;
    seq=0x80000000;
    pos=0;

    // Append container object to hash table
    if(parent){
      addObject(parent);
      }

    // So far, so good
    code=FXStreamOK;

    return true;
    }
  return false;
  }


// Flush buffer
FXbool FXStream::flush(){
  if(dir!=FXStreamSave){fxerror("FXStream::flush: illegal stream direction.\n");}
  writeBuffer(0);
  return code==FXStreamOK;
  }


// Close store; return true if no errors have been encountered
FXbool FXStream::close(){
  if(dir){
    hash.clear();
    dir=FXStreamDead;
    if(owns){freeElms(begptr);}
    begptr=nullptr;
    wrptr=nullptr;
    rdptr=nullptr;
    endptr=nullptr;
    owns=false;
    return code==FXStreamOK;
    }
  return false;
  }


// Move to position
FXbool FXStream::position(FXlong offset,FXWhence whence){
  if(dir==FXStreamDead){fxerror("FXStream::position: stream is not open.\n");}
  if(code==FXStreamOK){
    if(whence==FXFromCurrent) offset=offset+pos;
    else if(whence==FXFromEnd) offset=offset+endptr-begptr;
    pos=offset;
    return true;
    }
  return false;
  }


/******************************  Save Basic Types  *****************************/

// Write one byte
FXStream& FXStream::operator<<(const FXuchar& v){
  if(code==FXStreamOK){
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    if(wrptr+1>endptr && writeBuffer(1)<1){ code=FXStreamFull; return *this; }
    FXASSERT(wrptr+1<=endptr);
    *wrptr++ = v;
    pos++;
    }
  return *this;
  }


// Write one short
FXStream& FXStream::operator<<(const FXushort& v){
  if(code==FXStreamOK){
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    if(wrptr+2>endptr && writeBuffer((wrptr-endptr)+2)<2){ code=FXStreamFull; return *this; }
    FXASSERT(wrptr+2<=endptr);
    if(swap){
      wrptr[0]=((const FXuchar*)&v)[1];
      wrptr[1]=((const FXuchar*)&v)[0];
      }
    else{
      wrptr[0]=((const FXuchar*)&v)[0];
      wrptr[1]=((const FXuchar*)&v)[1];
      }
    wrptr+=2;
    pos+=2;
    }
  return *this;
  }


// Write one int
FXStream& FXStream::operator<<(const FXuint& v){
  if(code==FXStreamOK){
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    if(wrptr+4>endptr && writeBuffer((wrptr-endptr)+4)<4){ code=FXStreamFull; return *this; }
    FXASSERT(wrptr+4<=endptr);
    if(swap){
      wrptr[0]=((const FXuchar*)&v)[3];
      wrptr[1]=((const FXuchar*)&v)[2];
      wrptr[2]=((const FXuchar*)&v)[1];
      wrptr[3]=((const FXuchar*)&v)[0];
      }
    else{
      wrptr[0]=((const FXuchar*)&v)[0];
      wrptr[1]=((const FXuchar*)&v)[1];
      wrptr[2]=((const FXuchar*)&v)[2];
      wrptr[3]=((const FXuchar*)&v)[3];
      }
    wrptr+=4;
    pos+=4;
    }
  return *this;
  }


// Write one double
FXStream& FXStream::operator<<(const FXdouble& v){
  if(code==FXStreamOK){
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    if(wrptr+8>endptr && writeBuffer((wrptr-endptr)+8)<8){ code=FXStreamFull; return *this; }
    FXASSERT(wrptr+8<=endptr);
    if(swap){
      wrptr[0]=((const FXuchar*)&v)[7];
      wrptr[1]=((const FXuchar*)&v)[6];
      wrptr[2]=((const FXuchar*)&v)[5];
      wrptr[3]=((const FXuchar*)&v)[4];
      wrptr[4]=((const FXuchar*)&v)[3];
      wrptr[5]=((const FXuchar*)&v)[2];
      wrptr[6]=((const FXuchar*)&v)[1];
      wrptr[7]=((const FXuchar*)&v)[0];
      }
    else{
      wrptr[0]=((const FXuchar*)&v)[0];
      wrptr[1]=((const FXuchar*)&v)[1];
      wrptr[2]=((const FXuchar*)&v)[2];
      wrptr[3]=((const FXuchar*)&v)[3];
      wrptr[4]=((const FXuchar*)&v)[4];
      wrptr[5]=((const FXuchar*)&v)[5];
      wrptr[6]=((const FXuchar*)&v)[6];
      wrptr[7]=((const FXuchar*)&v)[7];
      }
    wrptr+=8;
    pos+=8;
    }
  return *this;
  }


/************************  Save Blocks of Basic Types  *************************/

// Write array of bytes
FXStream& FXStream::save(const FXuchar* p,FXuval n){
  if(code==FXStreamOK){
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    while(0<n){
      if(wrptr+n>endptr && writeBuffer((wrptr-endptr)+n)<1){ code=FXStreamFull; return *this; }
      FXASSERT(wrptr<endptr);
      do{
        *wrptr++=*p++;
        pos++;
        n--;
        }
      while(0<n && wrptr<endptr);
      }
    }
  return *this;
  }


// Write array of shorts
FXStream& FXStream::save(const FXushort* p,FXuval n){
  const FXuchar *q=(const FXuchar*)p;
  if(code==FXStreamOK){
    n<<=1;
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    if(swap){
      while(0<n){
        if(wrptr+n>endptr && writeBuffer((wrptr-endptr)+n)<2){ code=FXStreamFull; return *this; }
        FXASSERT(wrptr+2<=endptr);
        do{
          wrptr[0]=q[1];
          wrptr[1]=q[0];
          wrptr+=2;
          pos+=2;
          q+=2;
          n-=2;
          }
        while(0<n && wrptr+2<=endptr);
        }
      }
    else{
      while(0<n){
        if(wrptr+n>endptr && writeBuffer((wrptr-endptr)+n)<2){ code=FXStreamFull; return *this; }
        FXASSERT(wrptr+2<=endptr);
        do{
          wrptr[0]=q[0];
          wrptr[1]=q[1];
          wrptr+=2;
          pos+=2;
          q+=2;
          n-=2;
          }
        while(0<n && wrptr+2<=endptr);
        }
      }
    }
  return *this;
  }


// Write array of ints
FXStream& FXStream::save(const FXuint* p,FXuval n){
  const FXuchar *q=(const FXuchar*)p;
  if(code==FXStreamOK){
    n<<=2;
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    if(swap){
      while(0<n){
        if(wrptr+n>endptr && writeBuffer((wrptr-endptr)+n)<4){ code=FXStreamFull; return *this; }
        FXASSERT(wrptr+4<=endptr);
        do{
          wrptr[0]=q[3];
          wrptr[1]=q[2];
          wrptr[2]=q[1];
          wrptr[3]=q[0];
          wrptr+=4;
          pos+=4;
          q+=4;
          n-=4;
          }
        while(0<n && wrptr+4<=endptr);
        }
      }
    else{
      while(0<n){
        if(wrptr+n>endptr && writeBuffer((wrptr-endptr)+n)<4){ code=FXStreamFull; return *this; }
        FXASSERT(wrptr+4<=endptr);
        do{
          wrptr[0]=q[0];
          wrptr[1]=q[1];
          wrptr[2]=q[2];
          wrptr[3]=q[3];
          wrptr+=4;
          pos+=4;
          q+=4;
          n-=4;
          }
        while(0<n && wrptr+4<=endptr);
        }
      }
    }
  return *this;
  }


// Write array of doubles
FXStream& FXStream::save(const FXdouble* p,FXuval n){
  const FXuchar *q=(const FXuchar*)p;
  if(code==FXStreamOK){
    n<<=3;
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    if(swap){
      while(0<n){
        if(wrptr+n>endptr && writeBuffer((wrptr-endptr)+n)<8){ code=FXStreamFull; return *this; }
        FXASSERT(wrptr+8<=endptr);
        do{
          wrptr[0]=q[7];
          wrptr[1]=q[6];
          wrptr[2]=q[5];
          wrptr[3]=q[4];
          wrptr[4]=q[3];
          wrptr[5]=q[2];
          wrptr[6]=q[1];
          wrptr[7]=q[0];
          wrptr+=8;
          pos+=8;
          q+=8;
          n-=8;
          }
        while(0<n && wrptr+8<=endptr);
        }
      }
    else{
      while(0<n){
        if(wrptr+n>endptr && writeBuffer((wrptr-endptr)+n)<8){ code=FXStreamFull; return *this; }
        FXASSERT(wrptr+8<=endptr);
        do{
          wrptr[0]=q[0];
          wrptr[1]=q[1];
          wrptr[2]=q[2];
          wrptr[3]=q[3];
          wrptr[4]=q[4];
          wrptr[5]=q[5];
          wrptr[6]=q[6];
          wrptr[7]=q[7];
          wrptr+=8;
          pos+=8;
          q+=8;
          n-=8;
          }
        while(0<n && wrptr+8<=endptr);
        }
      }
    }
  return *this;
  }


/*****************************  Load Basic Types  ******************************/

// Read one byte
FXStream& FXStream::operator>>(FXuchar& v){
  if(code==FXStreamOK){
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    if(rdptr+1>wrptr && readBuffer(1)<1){ code=FXStreamEnd; return *this; }
    FXASSERT(rdptr+1<=wrptr);
    v=*rdptr++;
    pos++;
    }
  return *this;
  }


// Read one short
FXStream& FXStream::operator>>(FXushort& v){
  if(code==FXStreamOK){
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    if(rdptr+2>wrptr && readBuffer((rdptr-wrptr)+2)<2){ code=FXStreamEnd; return *this; }
    FXASSERT(rdptr+2<=wrptr);
    if(swap){
      ((FXuchar*)&v)[1]=rdptr[0];
      ((FXuchar*)&v)[0]=rdptr[1];
      }
    else{
      ((FXuchar*)&v)[0]=rdptr[0];
      ((FXuchar*)&v)[1]=rdptr[1];
      }
    rdptr+=2;
    pos+=2;
    }
  return *this;
  }


// Read one int
FXStream& FXStream::operator>>(FXuint& v){
  if(code==FXStreamOK){
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    if(rdptr+4>wrptr && readBuffer((rdptr-wrptr)+4)<4){ code=FXStreamEnd; return *this; }
    FXASSERT(rdptr+4<=wrptr);
    if(swap){
      ((FXuchar*)&v)[3]=rdptr[0];
      ((FXuchar*)&v)[2]=rdptr[1];
      ((FXuchar*)&v)[1]=rdptr[2];
      ((FXuchar*)&v)[0]=rdptr[3];
      }
    else{
      ((FXuchar*)&v)[0]=rdptr[0];
      ((FXuchar*)&v)[1]=rdptr[1];
      ((FXuchar*)&v)[2]=rdptr[2];
      ((FXuchar*)&v)[3]=rdptr[3];
      }
    rdptr+=4;
    pos+=4;
    }
  return *this;
  }


// Read one double
FXStream& FXStream::operator>>(FXdouble& v){
  if(code==FXStreamOK){
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    if(rdptr+8>wrptr && readBuffer((rdptr-wrptr)+8)<8){ code=FXStreamEnd; return *this; }
    FXASSERT(rdptr+8<=wrptr);
    if(swap){
      ((FXuchar*)&v)[7]=rdptr[0];
      ((FXuchar*)&v)[6]=rdptr[1];
      ((FXuchar*)&v)[5]=rdptr[2];
      ((FXuchar*)&v)[4]=rdptr[3];
      ((FXuchar*)&v)[3]=rdptr[4];
      ((FXuchar*)&v)[2]=rdptr[5];
      ((FXuchar*)&v)[1]=rdptr[6];
      ((FXuchar*)&v)[0]=rdptr[7];
      }
    else{
      ((FXuchar*)&v)[0]=rdptr[0];
      ((FXuchar*)&v)[1]=rdptr[1];
      ((FXuchar*)&v)[2]=rdptr[2];
      ((FXuchar*)&v)[3]=rdptr[3];
      ((FXuchar*)&v)[4]=rdptr[4];
      ((FXuchar*)&v)[5]=rdptr[5];
      ((FXuchar*)&v)[6]=rdptr[6];
      ((FXuchar*)&v)[7]=rdptr[7];
      }
    rdptr+=8;
    pos+=8;
    }
  return *this;
  }


/************************  Load Blocks of Basic Types  *************************/

// Read array of bytes
FXStream& FXStream::load(FXuchar* p,FXuval n){
  if(code==FXStreamOK){
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    while(0<n){
      if(rdptr+n>wrptr && readBuffer((rdptr-wrptr)+n)<1){ code=FXStreamEnd; return *this; }
      FXASSERT(rdptr<wrptr);
      do{
        *p++=*rdptr++;
        pos++;
        n--;
        }
      while(0<n && rdptr<wrptr);
      }
    }
  return *this;
  }


// Read array of shorts
FXStream& FXStream::load(FXushort* p,FXuval n){
  FXuchar *q=(FXuchar*)p;
  if(code==FXStreamOK){
    n<<=1;
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    if(swap){
      while(0<n){
        if(rdptr+n>wrptr && readBuffer((rdptr-wrptr)+n)<2){ code=FXStreamEnd; return *this; }
        FXASSERT(rdptr+2<=wrptr);
        do{
          q[1]=rdptr[0];
          q[0]=rdptr[1];
          rdptr+=2;
          pos+=2;
          q+=2;
          n-=2;
          }
        while(0<n && rdptr+2<=wrptr);
        }
      }
    else{
      while(0<n){
        if(rdptr+n>wrptr && readBuffer((rdptr-wrptr)+n)<2){ code=FXStreamEnd; return *this; }
        FXASSERT(rdptr+2<=wrptr);
        do{
          q[0]=rdptr[0];
          q[1]=rdptr[1];
          rdptr+=2;
          pos+=2;
          q+=2;
          n-=2;
          }
        while(0<n && rdptr+2<=wrptr);
        }
      }
    }
  return *this;
  }


// Read array of ints
FXStream& FXStream::load(FXuint* p,FXuval n){
  FXuchar *q=(FXuchar*)p;
  if(code==FXStreamOK){
    n<<=2;
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    if(swap){
      while(0<n){
        if(rdptr+n>wrptr && readBuffer((rdptr-wrptr)+n)<4){ code=FXStreamEnd; return *this; }
        FXASSERT(rdptr+4<=wrptr);
        do{
          q[3]=rdptr[0];
          q[2]=rdptr[1];
          q[1]=rdptr[2];
          q[0]=rdptr[3];
          rdptr+=4;
          pos+=4;
          q+=4;
          n-=4;
          }
        while(0<n && rdptr+4<=wrptr);
        }
      }
    else{
      while(0<n){
        if(rdptr+n>wrptr && readBuffer((rdptr-wrptr)+n)<4){ code=FXStreamEnd; return *this; }
        FXASSERT(rdptr+4<=wrptr);
        do{
          q[0]=rdptr[0];
          q[1]=rdptr[1];
          q[2]=rdptr[2];
          q[3]=rdptr[3];
          rdptr+=4;
          pos+=4;
          q+=4;
          n-=4;
          }
        while(0<n && rdptr+4<=wrptr);
        }
      }
    }
  return *this;
  }


// Read array of doubles
FXStream& FXStream::load(FXdouble* p,FXuval n){
  FXuchar *q=(FXuchar*)p;
  if(code==FXStreamOK){
    n<<=3;
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    if(swap){
      while(0<n){
        if(rdptr+n>wrptr && readBuffer((rdptr-wrptr)+n)<8){ code=FXStreamEnd; return *this; }
        FXASSERT(rdptr+4<=wrptr);
        do{
          q[7]=rdptr[0];
          q[6]=rdptr[1];
          q[5]=rdptr[2];
          q[4]=rdptr[3];
          q[3]=rdptr[4];
          q[2]=rdptr[5];
          q[1]=rdptr[6];
          q[0]=rdptr[7];
          rdptr+=8;
          pos+=8;
          q+=8;
          n-=8;
          }
        while(0<n && rdptr+8<=wrptr);
        }
      }
    else{
      while(0<n){
        if(rdptr+n>wrptr && readBuffer((rdptr-wrptr)+n)<8){ code=FXStreamEnd; return *this; }
        FXASSERT(rdptr+4<=wrptr);
        do{
          q[0]=rdptr[0];
          q[1]=rdptr[1];
          q[2]=rdptr[2];
          q[3]=rdptr[3];
          q[4]=rdptr[4];
          q[5]=rdptr[5];
          q[6]=rdptr[6];
          q[7]=rdptr[7];
          rdptr+=8;
          pos+=8;
          q+=8;
          n-=8;
          }
        while(0<n && rdptr+8<=wrptr);
        }
      }
    }
  return *this;
  }


/*********************************  Add Object  ********************************/


// Add object without saving or loading
FXStream& FXStream::addObject(const FXObject* v){
  if(v){
    void* ref=(void*)(FXuval)seq++;
    if(dir==FXStreamSave){
      hash.insert(v,ref);
      }
    else if(dir==FXStreamLoad){
      hash.insert(ref,const_cast<FXObject*>(v));
      }
    }
  return *this;
  }


/********************************  Save Object  ********************************/


// Save object
FXStream& FXStream::saveObject(const FXObject* v){
  if(dir!=FXStreamSave){ fxerror("FXStream::saveObject: wrong stream direction.\n"); }
  if(code==FXStreamOK){
    const FXMetaClass *cls;
    const FXchar *name;
    FXuint tag,zero=0;
    void* ref;
    if(v==nullptr){                             // Its a NULL
      *this << zero;                            // Save special null-object tag
      return *this;
      }
    ref=hash.at(v);                             // Reference from table
    tag=(FXuint)(FXuval)ref;
    if(tag){                                    // Already in table
      *this << tag;
      return *this;
      }
    cls=v->getMetaClass();
    name=cls->getClassName();
    tag=strlen(name)+1;
    if(tag>MAXCLASSNAME){                       // Class name too long
      code=FXStreamFormat;
      return *this;
      }
    ref=(void*)(FXuval)seq++;                   // New reference
    hash.insert(v,ref);                         // Map object to reference
    *this << tag;                               // Save tag
    *this << zero;                              // Save escape code
    save(name,tag);                             // Save class name
    FXTRACE((100,"%08ld: saveObject(%s)\n",(FXuval)pos,v->getClassName()));
    v->save(*this);                             // Save object
    }
  return *this;
  }


/*******************************  Load Object  *********************************/

// Load object
FXStream& FXStream::loadObject(FXObject*& v){
  if(dir!=FXStreamLoad){ fxerror("FXStream::loadObject: wrong stream direction.\n"); }
  if(code==FXStreamOK){
    FXchar name[MAXCLASSNAME+1];
    const FXMetaClass *cls;
    FXuint tag,esc;
    void* ref;
    *this >> tag;
    if(tag==0){                                 // Was a NULL
      v=nullptr;
      return *this;
      }
    if(tag>=0x80000000){
      ref=(void*)(FXuval)tag;
      v=(FXObject*)hash.at(ref);                // Object referenced by tag
      if(!v){
        code=FXStreamFormat;                    // Bad format in stream
        }
      return *this;
      }
    if(tag>MAXCLASSNAME){                       // Class name too long
      code=FXStreamFormat;                      // Bad format in stream
      return *this;
      }
    *this >> esc;                               // Read escape code
    if(esc!=0){                                 // Escape code is wrong
      code=FXStreamFormat;                      // Bad format in stream
      return *this;
      }
    load(name,tag);                             // Load class name
    cls=FXMetaClass::getMetaClassFromName(name);
    if(cls==nullptr){                           // We don't have a class with this name
      code=FXStreamUnknown;                     // Unknown class
      return *this;
      }
    ref=(void*)(FXuval)seq++;                   // New reference
    v=cls->makeInstance();                      // Make instance of class
    hash.insert(ref,v);                         // Map reference to object
    FXTRACE((100,"%08ld: loadObject(%s)\n",(FXuval)pos,v->getClassName()));
    v->load(*this);
    }
  return *this;
  }

}
