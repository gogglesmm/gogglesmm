#include "ap_defs.h"
#include "ap_memory_buffer.h"

/*
  TODO:
    - Need audio dithering when converting between sample formats


*/

namespace ap {

const FXint S8_MIN  = -128;
const FXint S8_MAX  = 127;
const FXint S16_MIN = -32768;
const FXint S16_MAX = 32767;
const FXint S24_MIN = -8388608;
const FXint S24_MAX = 8388607;
const FXint S32_MIN = -2147483648;
const FXint S32_MAX = 2147483647;


static FXint s16_to_s32(FXshort x) {
  if (x==S16_MIN)
    return S32_MIN;
  else
    return (x<<16)+(x+x)+((x+16383)>>15);
  }

static FXint s24le3_to_s32(FXint x) {
  if (x==S24_MIN)
    return S32_MIN;
  else
    return (x<<8)+(x>>16)+(x>>16)+((x+4194303)>>23);
  }


static FXshort float_to_s16(FXfloat x) {
  FXfloat c = x*S16_MAX;
  if (c>=S16_MAX)
    return S16_MAX;
  else if (c<=S16_MIN)
    return S16_MIN;
  else
    return lrintf(c);
  }

static FXint float_to_s32(FXfloat x) {
  FXfloat c = x*S32_MAX;
  if (c>=S32_MAX)
    return S32_MAX;
  else if (c<=S32_MIN)
    return S32_MIN;
  else
    return lrintf(c);
  }



void s16_to_s32(const FXuchar *buffer,FXuint nsamples,MemoryBuffer & out) {
  out.clear();
  out.grow(nsamples*4);
  FXint  * output = out.s32();
  const FXshort * input = reinterpret_cast<const FXshort*>(buffer);
  for (FXuint i=0;i<nsamples;i++) {
    output[i]=s16_to_s32(input[i]);
    }
  out.wrote(nsamples*4);
  }

void s24le3_to_s16(FXuchar * input,FXuint nsamples) {
  FXshort * output = reinterpret_cast<FXshort*>(input);
  for (FXuint i=0;i<nsamples;i++,input+=3) {
    output[i] = input[1] | (input[2]<<8);
    }
  }

void float_to_s16(FXuchar * buffer,FXuint nsamples){
  FXfloat * input  = reinterpret_cast<FXfloat*>(buffer);
  FXshort * output = reinterpret_cast<FXshort*>(buffer);
  register FXint val;
  for (FXuint i=0;i<nsamples;i++) {
    output[i]=float_to_s16(input[i]);

/*
    val=((FXint)(floor(input[i]*32767.f+.5f)));
    if (__unlikely(val>32767))  val=32767;
    if (__unlikely(val<-32768)) val=-32768;
    output[i]=val;
*/
    }
  }


void s24le3_to_s32(const FXuchar * input,FXuint nsamples,MemoryBuffer & out){
  out.clear();
  out.grow(nsamples*4);
  FXint * output = out.s32();
  for (FXuint i=0;i<nsamples;i++,input+=3) {
    output[i] = s24le3_to_s32(input[0]|input[1]<<8|input[2]<<16);

//    output[i] = (input[0]<<8) | (input[1]<<16) | (input[2]<<24) ;
    }
  out.wrote(nsamples*4);
  }

void float_to_s32(FXuchar * buffer,FXuint nsamples){
  FXfloat * input = reinterpret_cast<FXfloat*>(buffer);
  FXint *  output = reinterpret_cast<FXint*>(buffer);
  register FXlong val;
  for (FXuint i=0;i<nsamples;i++) {
    output[i]=float_to_s32(input[i]);
/*

    val=((FXint)(floor(input[i]*2147483647.f+.5f)));
    if (__unlikely(val>2147483647))  val=2147483647;
    if (__unlikely(val<-2147483648)) val=-2147483648;
    output[i]=val;
*/
    }
  }

}
