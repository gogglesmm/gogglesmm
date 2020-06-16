/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2018 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#include "ap_defs.h"
#include "ap_convert.h"

#define INT8_MIN (-128)
#define INT8_MAX (127)
#define INT16_MIN (-32767-1)
#define INT16_MAX (32767)
#define INT24_MIN (-8388608)
#define INT24_MAX (8388607)
#define INT32_MIN (-2147483647-1)
#define INT32_MAX (2147483647)


/*
  TODO:
    - Need audio dithering when converting between sample formats
*/

namespace ap {



template <typename T, unsigned B>
inline T signextend(const T x)
{
  struct {T x:B;} s;
  return s.x = x;
}


static FXint s16_to_s32(FXshort x) {
  if (x==INT16_MIN)
    return INT32_MIN;
  else
    return (x<<16)+(x+x)+((x+16383)>>15);
  }

static FXfloat s16_to_float(FXshort x) {
  FXfloat c =  (float)x / (float)INT16_MAX;
  if (c > 1.0)
    return 1.0;
  else if (c < -1.0)
    return -1.0;
  else
    return c;
  }

static FXint s24_to_s32(FXint x) {
  if (x==INT24_MIN)
    return INT32_MIN;
  else
    return (x<<8)+(x>>16)+(x>>16)+((x+4194303)>>23);
  }


static FXfloat s24_to_float(FXint x) {
  struct {signed int x:24;} s;
  FXint e = s.x = x;
  FXfloat c = (float)e / (float)INT24_MAX;
  if (c > 1.0)
    return 1.0;
  else if (c < -1.0)
    return -1.0;
  else
    return c;
  }


static FXshort float_to_s16(FXfloat x) {
  FXfloat c = x*INT16_MAX;
  if (c>=INT16_MAX)
    return INT16_MAX;
  else if (c<=INT16_MIN)
    return INT16_MIN;
  else
    return lrintf(c);
  }

static FXint float_to_s32(FXfloat x) {
  FXfloat c = x*INT32_MAX;
  if (c>=INT32_MAX)
    return INT32_MAX;
  else if (c<=INT32_MIN)
    return INT32_MIN;
  else
    return lrintf(c);
  }



void s16_to_s24le3(const FXuchar * buffer, FXuint nsamples, MemoryBuffer & out){
  }

void s16_to_s32(const FXuchar *buffer,FXuint nsamples,MemoryBuffer & out) {
  out.clear();
  out.reserve(nsamples*4);
  FXint  * output = out.s32();
  const FXshort * input = reinterpret_cast<const FXshort*>(buffer);
  for (FXuint i=0;i<nsamples;i++) {
    output[i]=s16_to_s32(input[i]);
    }
  out.wroteBytes(nsamples*4);
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
  for (FXuint i=0;i<nsamples;i++) {
    output[i]=float_to_s16(input[i]);
    }
  }


void s16_to_float(FXuchar * buffer, FXuint nsamples, MemoryBuffer & out){
  out.clear();
  out.reserve(nsamples*4);
  FXshort * input = reinterpret_cast<FXshort*>(buffer);
  FXfloat * output  = out.flt();
  for (FXuint i=0;i<nsamples;i++) {
    output[i]=s16_to_float(input[i]);
    }
  out.wroteBytes(nsamples*4);
  }

void s24le3_to_float(FXuchar * input,FXuint nsamples, MemoryBuffer & out){
  out.clear();
  out.reserve(nsamples*4);
  FXfloat * output = out.flt();
  for (FXuint i=0;i<nsamples;i++,input+=3) {
    output[i] = s24_to_float(input[0]|input[1]<<8|input[2]<<16);
    }
  out.wroteBytes(nsamples*4);
  }


void s24le3_to_s32(const FXuchar * input,FXuint nsamples,MemoryBuffer & out){
  out.clear();
  out.reserve(nsamples*4);
  FXint * output = out.s32();
  for (FXuint i=0;i<nsamples;i++,input+=3) {
    output[i] = s24_to_s32(input[0]|input[1]<<8|input[2]<<16);
    }
  out.wroteBytes(nsamples*4);
  }

void float_to_s32(FXuchar * buffer,FXuint nsamples){
  FXfloat * input = reinterpret_cast<FXfloat*>(buffer);
  FXint *  output = reinterpret_cast<FXint*>(buffer);
  for (FXuint i=0;i<nsamples;i++) {
    output[i]=float_to_s32(input[i]);
    }
  }

}
