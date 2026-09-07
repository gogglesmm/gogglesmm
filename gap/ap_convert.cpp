/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2026 by Sander Jansen. All Rights Reserved      *
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
*                               ---                                            *
* SPDX-License-Identifier: GPL-3.0-or-later                                    *
********************************************************************************/
#include "ap_defs.h"
#include "ap_convert.h"

#define INT16_MIN (-32767-1)
#define INT16_MAX (32767)
#define INT32_MIN (-2147483647-1)
#define INT32_MAX (2147483647)

namespace ap {

static constexpr FXfloat s16_to_float(FXshort x) {
  // 1 / 32768.0f expressed as a precise float literal multiplier
  return static_cast<FXfloat>(x) * (1.0f / 32768.0f);
  }


static constexpr FXint s16_to_s32(FXshort x) {
  // 1. Zero-extend to uint16_t (clears upper bits automatically)
  // 2. Cast to uint32_t for the 32-bit shift
  const auto u = static_cast<FXuint>(static_cast<FXushort>(x));
  return static_cast<FXint>((u << 16) | u);
}


static constexpr FXfloat s24_to_float(FXuint u) {
  // 1. Shift left by 8 to align bit 23 with bit 31
  // 2. Shift right by 8 (arithmetic shift requires casting to signed FXint)
  FXint extended = static_cast<FXint>(u << 8) >> 8;

  // 3. Convert to float using 2^23 scaling
  return static_cast<FXfloat>(extended) * (1.0f / 8388608.0f);
  }


static constexpr FXint s24_to_s32(FXuint u) noexcept {
  // 1. Sign extend 24-bit input to 32-bit
  FXint extended = static_cast<FXint>(u << 8) >> 8;

  // 2. Extract upper 8 bits (the most significant byte)
  const auto top_byte = static_cast<FXuint>(extended >> 16) & 0xFFU;

  // 3. Shift left by 8 and replicate the top byte into the lowest byte
  const auto u32 = (static_cast<FXuint>(extended) << 8) | top_byte;

  return static_cast<FXint>(u32);
}


static constexpr FXshort s24_to_s16(FXuint x) noexcept {
  // 1. Sign-extend 24-bit to 32-bit (C++20 compliant)
  const FXint s32 = static_cast<FXint>(x << 8) >> 8;

  // 2. Add 128 (0x80) for nearest-integer rounding before shifting
  const FXint rounded = s32 + 128;

  // 3. Shift right by 8 bits
  const FXint s16 = rounded >> 8;

  // 4. Clamp to prevent potential overflow on edge cases (+8388607 + 128)
  if (s16 > 32767) return 32767;
  if (s16 < -32768) return -32768;

  return static_cast<FXshort>(s16);
  }


static constexpr FXfloat s32_to_float(FXint x) noexcept {
  // Divide by 2^31 (2147483648.0f) for exact 1-to-1 audio scaling
  return static_cast<FXfloat>(x) * (1.0f / 2147483648.0f);
  }

static constexpr FXshort s32_to_s16(FXint x) {
  // 1. Add 32768 (0x8000) for nearest-integer rounding before bit-shifting.
  // Use 64-bit cast to prevent signed integer overflow on INT32_MAX + 32768.
  const int64_t rounded = static_cast<int64_t>(x) + 32768;

  // 2. Arithmetic right-shift by 16 bits
  const int64_t s16 = rounded >> 16;

  // 3. Clamp to valid 16-bit range [-32768, 32767]
  if (s16 > 32767) return 32767;
  if (s16 < -32768) return -32768;

  return static_cast<FXshort>(s16);
  }


static constexpr FXint float_to_s32(FXfloat x) {
  constexpr FXfloat scale = 2147483648.0f; // 2^31
  FXfloat c = x * scale;
  if (c >= 2147483647.0f) {
    return INT32_MAX;
  }
  if (c <= -2147483648.0f) {
    return INT32_MIN;
  }
  return static_cast<FXint>(lrintf(c));
  }


static constexpr FXshort float_to_s16(FXfloat x) {
  constexpr FXfloat scale = 32768.0f; // 2^15
  FXfloat c = x * scale;
  if (c >= 32767.0f) {
    return INT16_MAX; // +32767
    }
  if (c <= -32768.0f) {
    return INT16_MIN; // -32768
    }
  return static_cast<FXshort>(lrintf(c));
  }

/***********************************************************************************/

void s16_to_float(const FXuchar * buffer, FXuint nsamples, MemoryBuffer & out){
  out.clear();
  out.reserve(nsamples*4);
  const auto * input = reinterpret_cast<const FXshort*>(buffer);
  FXfloat * output  = out.flt();
  for (FXuint i=0;i<nsamples;i++) {
    output[i]=s16_to_float(input[i]);
  }
  out.wroteBytes(nsamples*4);
  }


void s16_to_s32(const FXuchar *buffer, FXuint nsamples, MemoryBuffer & out) {
  out.clear();
  out.reserve(nsamples*4);
  FXint  * output = out.s32();
  const auto * input = reinterpret_cast<const FXshort*>(buffer);
  for (FXuint i=0;i<nsamples;i++) {
    output[i]=s16_to_s32(input[i]);
    }
  out.wroteBytes(nsamples*4);
  }

void s24le3_to_float(const FXuchar * input, FXuint nsamples, MemoryBuffer & out){
  out.clear();
  out.reserve(nsamples*4);
  FXfloat * output = out.flt();
  for (FXuint i=0;i<nsamples;i++,input+=3) {
    output[i] = s24_to_float(input[0]|input[1]<<8|input[2]<<16);
    }
  out.wroteBytes(nsamples*4);
  }


void s24le3_to_s32(const FXuchar * input, FXuint nsamples, MemoryBuffer & out){
  out.clear();
  out.reserve(nsamples*4);
  FXint * output = out.s32();
  for (FXuint i=0;i<nsamples;i++,input+=3) {
    const FXuint u24 = static_cast<FXuint>(input[0]) |
                      (static_cast<FXuint>(input[1]) << 8) |
                      (static_cast<FXuint>(input[2]) << 16);
    output[i] = s24_to_s32(u24);
    }
  out.wroteBytes(nsamples*4);
  }


void s24le3_to_s16(FXuchar* input, FXuint nsamples) {
  auto * output = reinterpret_cast<FXshort*>(input);
  for (FXuint i=0; i<nsamples;i++,input+=3) {
    const FXuint u24 = static_cast<FXuint>(input[0]) |
                      (static_cast<FXuint>(input[1]) << 8) |
                      (static_cast<FXuint>(input[2]) << 16);
    output[i] = s24_to_s16(u24);
  }
}


void s32_to_float(FXuchar * buffer, FXuint nsamples){
  auto * input = reinterpret_cast<FXint*>(buffer);
  auto * output = reinterpret_cast<FXfloat*>(buffer);
  for (FXuint i=0;i<nsamples;i++) {
    output[i]=s32_to_float(input[i]);
    }
  }


void s32_to_s16(FXuchar* buffer, FXuint nsamples) {
  auto * input  = reinterpret_cast<FXint*>(buffer);
  auto * output = reinterpret_cast<FXshort*>(buffer);
  for (FXuint i=0; i<nsamples;i++) {
    output[i] = s32_to_s16(input[i]);
    }
  }


void float_to_s32(FXuchar * buffer,FXuint nsamples){
  auto * input = reinterpret_cast<FXfloat*>(buffer);
  auto *  output = reinterpret_cast<FXint*>(buffer);
  for (FXuint i=0;i<nsamples;i++) {
    output[i]=float_to_s32(input[i]);
    }
  }


void float_to_s16(FXuchar * buffer,FXuint nsamples){
  auto * input  = reinterpret_cast<FXfloat*>(buffer);
  auto * output = reinterpret_cast<FXshort*>(buffer);
  for (FXuint i=0;i<nsamples;i++) {
    output[i]=float_to_s16(input[i]);
    }
  }

}
