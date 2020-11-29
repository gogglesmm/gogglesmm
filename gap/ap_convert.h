/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2021 by Sander Jansen. All Rights Reserved      *
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
#ifndef CONVERT_H
#define CONVERT_H

#include "ap_buffer.h"

namespace ap {

extern void s16_to_float(FXuchar * buffer, FXuint nsamples, MemoryBuffer & out);
extern void s24le3_to_float(FXuchar * buffer,FXuint nsamples, MemoryBuffer & out);


extern void s24le3_to_s16(FXuchar * buffer,FXuint nsamples);
extern void  float_to_s16(FXuchar * buffer,FXuint nsamples);

extern void s24le3_to_s32(const FXuchar * buffer,FXuint nsamples,MemoryBuffer & out);
extern void  float_to_s32(FXuchar * buffer,FXuint nsamples);

}
#endif


