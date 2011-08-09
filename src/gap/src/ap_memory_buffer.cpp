#include "ap_defs.h"
#include "ap_memory_buffer.h"

#define ROUNDVAL    16
#define ROUNDUP(n)  (((n)+ROUNDVAL-1)&-ROUNDVAL)

namespace ap {

MemoryBuffer::MemoryBuffer(FXival cap) : data_buffer(NULL),data_capacity(cap),data_size(0) {
  allocElms(data_buffer,data_capacity);
  }

void MemoryBuffer::append(const void *buf,FXival sz) {
  if (data_capacity < data_size+sz ) {
    data_capacity = ROUNDUP(data_size+sz);
    resizeElms(data_buffer,data_capacity);
    }
  memcpy(&data_buffer[data_size],buf,sz);
  data_size+=sz;
  }


void MemoryBuffer::appendZero(FXival sz) {
  if (data_capacity < data_size+sz ) {
    data_capacity = ROUNDUP(data_size+sz);
    resizeElms(data_buffer,data_capacity);
    }
  memset(&data_buffer[data_size],0,sz);
  data_size+=sz;
  }

void MemoryBuffer::wrote(FXival sz) {
  FXASSERT(data_capacity>=data_size+sz);
  data_size+=sz;
  }


void MemoryBuffer::read(FXival sz) {
  if (sz<data_size) {
    memmove(data_buffer,data_buffer+sz,data_size-sz);
    data_size-=sz;
    }
  else {
    data_size=0;
    }
  }


void MemoryBuffer::trimBegin(FXival sz) {
  FXASSERT(sz<=data_size);
  memmove(data_buffer,data_buffer+sz,data_size-sz);
  data_size-=sz;
  }

void MemoryBuffer::trimEnd(FXival sz) {
  data_size-=sz;
  if (data_size<0) data_size=0;
  }



void MemoryBuffer::trimBefore(const FXuchar * p) {
  FXASSERT(p>=data_buffer && p<(data_buffer+data_size));
  if (p>data_buffer && p<data_buffer+data_size) {
    memmove(data_buffer,p,(data_buffer+data_size)-p);
    data_size=(data_buffer+data_size)-p;
    }
  }

void MemoryBuffer::clear() {
  data_size=0;
  memset(data_buffer,0,data_capacity);
  }

MemoryBuffer::~MemoryBuffer() {
  freeElms(data_buffer);
  }

void MemoryBuffer::stats() {
  fxmessage("buffer %ld / %ld\n",data_size,data_capacity);
  }
/*
void MemoryBuffer::grow(FXival sz) {
  if (data_capacity<ROUNDUP(data_capacity+sz)){
    data_capacity=ROUNDUP(data_capacity+sz);
    resizeElms(data_buffer,data_capacity);
    }
  }
*/
void MemoryBuffer::grow(FXival sz) {
  if (data_capacity<ROUNDUP(sz)) {
    data_capacity=ROUNDUP(sz);
    resizeElms(data_buffer,data_capacity);
    }
  }
















MemoryStream::MemoryStream(FXival cap) : buffer(NULL),buffersize(cap),sr(NULL),sw(NULL){
  if (buffersize) {
    allocElms(buffer,buffersize);
    sr=sw=buffer;
    }
  }

MemoryStream::~MemoryStream() {
  freeElms(buffer);
  sr=sw=NULL;
  }

void MemoryStream::clear() {
  sr=sw=buffer;
  }

void MemoryStream::append(const void *buf,FXival sz) {
  reserve(sz);
  memcpy(sw,buf,sz);
  sw+=sz;
  }

void MemoryStream::padding(FXival sz) {
  reserve(sz);
  memset(sw,0,sz);
  sw+=sz;
  }


FXival MemoryStream::read(void *buf,FXival sz) {
  FXival n=FXMIN(sz,size());
  memcpy(buf,sr,n);
  sr+=n;
  return n;
  }

FXival MemoryStream::copy(void *buf,FXival sz) {
  FXival n=FXMIN(sz,size());
  memcpy(buf,sr,n);
  return n;
  }


void MemoryStream::wrote(FXival sz) {
  FXASSERT(sz<(buffersize-(sw-buffer)));
  sw+=sz;
  }

void MemoryStream::read(FXival sz) {
  FXASSERT(sz<=size());
  sr+=sz;
  }


void MemoryStream::reserve(FXival sz) {
  if (buffer) {
    FXival sp = (buffersize-(sw-buffer));
    if (sp<sz) {
      if (sr>buffer) {
        if (sw>sr) {
          memmove(buffer,sr,sw-sr);
          sw-=(sr-buffer);
          sr=buffer;
          }
        else {
          sw=sr=buffer;
          }
        }
      }
    sp = (buffersize-(sw-buffer));
    if (sp<sz) {
      buffersize+=sz-sp;
      sp=sw-buffer;
      resizeElms(buffer,buffersize);
      sw=sr=buffer;
      sw+=sp;
      }
    }
  else {
    buffersize=sz;
    allocElms(buffer,buffersize);
    sw=sr=buffer;
    }
  }


}
