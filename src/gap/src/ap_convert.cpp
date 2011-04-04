#include "ap_defs.h"
#include "ap_memory_buffer.h"

/*
  TODO:
    - Need audio dithering when converting between sample formats


*/

namespace ap {

void s24le3_to_s16(FXuchar * input,FXuint nsamples) {
  FXshort * output = reinterpret_cast<FXshort*>(input);
  for (FXint i=0;i<nsamples;i++,input+=3) {
    output[i] = input[1] | (input[2]<<8);
    }
  }

void float_to_s16(FXuchar * buffer,FXuint nsamples){
  FXfloat * input  = reinterpret_cast<FXfloat*>(buffer);
  FXshort * output = reinterpret_cast<FXshort*>(buffer);
  register FXint val;
  for (FXint i=0;i<nsamples;i++) {
    val=((FXint)(floor(input[i]*32767.f+.5f)));
    if (__unlikely(val>32767))  val=32767;
    if (__unlikely(val<-32768)) val=-32768;
    output[i]=val;
    }
  }


void s24le3_to_s32(const FXuchar * input,FXuint nsamples,MemoryBuffer & out){
  out.clear();
  out.grow(nsamples*4);
  FXint * output = out.s32();
  for (FXint i=0;i<nsamples;i++,input+=3) {
    output[i] = (input[0]<<8) | (input[1]<<16) | (input[2]<<24) ;
    }
  out.wrote(nsamples*4);
  }

void float_to_s32(FXuchar * buffer,FXuint nsamples){
  FXfloat * input = reinterpret_cast<FXfloat*>(buffer);
  FXint *  output = reinterpret_cast<FXint*>(buffer);
  register FXint val;
  for (FXint i=0;i<nsamples;i++) {
    val=((FXint)(floor(input[i]*2147483647.f+.5f)));
    if (__unlikely(val>2147483647))  val=2147483647;
    if (__unlikely(val<-2147483648)) val=-2147483648;
    output[i]=val;
    }
  }

}
