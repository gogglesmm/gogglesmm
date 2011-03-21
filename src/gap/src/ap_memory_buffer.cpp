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
















MemoryStream::MemoryStream(FXival cap) : data_buffer(NULL),data_ptr(NULL),data_capacity(cap),data_size(0){
  allocElms(data_buffer,data_capacity);
  data_ptr=data_buffer;
  }

MemoryStream::~MemoryStream() {
  freeElms(data_buffer);
  data_ptr=NULL;
  }

void MemoryStream::clear() {
  data_size=0;
  data_ptr=data_buffer;
  }

void MemoryStream::append(const FXuchar *buf,FXival sz) {
  if (data_ptr>data_buffer) {
    FXival nbytes = (data_ptr-data_buffer);
    memmove(data_buffer,data_ptr,nbytes);
    data_size-=nbytes;
    data_ptr=data_buffer;
    }
  if (data_capacity < data_size+sz ) {
    FXival nbytes=(data_ptr-data_buffer);
    data_capacity = (data_size+sz);
    resizeElms(data_buffer,data_capacity);
    data_ptr=data_buffer+nbytes;
    }
  memcpy(&data_buffer[data_size],buf,sz);
  data_size+=sz;
  }

void MemoryStream::read(FXuchar *buf,FXival sz) {
  FXASSERT(sz<=size());
  memcpy(buf,data_ptr,sz);
  data_ptr+=sz;
  }

void MemoryStream::read(FXival sz) {
  FXASSERT(sz<=size());
  data_ptr+=sz;
  }

void MemoryStream::padding(FXival sz) {
  if (data_ptr>data_buffer) {
    FXival nbytes = (data_ptr-data_buffer);
    memmove(data_buffer,data_ptr,nbytes);
    data_size-=nbytes;
    data_ptr=data_buffer;
    }
  if (data_capacity < data_size+sz ) {
    FXival nbytes=(data_ptr-data_buffer);
    data_capacity = (data_size+sz);
    resizeElms(data_buffer,data_capacity);
    data_ptr=data_buffer+nbytes;
    }
  memset(&data_buffer[data_size],0,sz);
  data_size+=sz;
  }

}
