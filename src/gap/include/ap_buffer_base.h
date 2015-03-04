/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2015 by Sander Jansen. All Rights Reserved      *
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
#ifndef AP_BUFFER_BASE_H
#define AP_BUFFER_BASE_H

namespace ap {

class BufferBase {
protected:
  FXuchar * begptr;  // Begin of buffer
  FXuchar * endptr;  // End of buffer
  FXuchar * wrptr;   // Write pointer
  FXuchar * rdptr;   // Read pointer
public:
  BufferBase(FXival n=4096);

  // Resize buffer
  FXbool resize(FXival n);

  // Reserve up to free n bytes.
  FXbool reserve(FXival n);

  // Clear buffer by resetting read and write pointers
  void clear();

  ~BufferBase();
  };

}

#endif

