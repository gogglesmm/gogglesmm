/********************************************************************************
*                                                                               *
*                   M e m o r y   S t r e a m   C l a s s e s                   *
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
#ifndef FXMEMORYSTREAM_H
#define FXMEMORYSTREAM_H

#ifndef FXSTREAM_H
#include "FXStream.h"
#endif

namespace FX {


/// Memory Store Definition
class FXAPI FXMemoryStream : public FXStream {
protected:
  virtual FXuval writeBuffer(FXuval count);
  virtual FXuval readBuffer(FXuval count);
private:
  FXMemoryStream(const FXMemoryStream&);
  FXMemoryStream &operator=(const FXMemoryStream&);
public:

  /// Create memory stream
  FXMemoryStream(const FXObject* cont=nullptr);

  /// Create and open memory stream
  FXMemoryStream(FXStreamDirection save_or_load,FXuchar* data=nullptr,FXuval size=~0UL,FXbool owned=false);

  /**
  * Open memory stream.
  * When reading from the data buffer, the size parameter is optional.  If not given,
  * the reader will need to know when to stop reading by some other means, like end-of-file
  * markers in the data.  When writing, the size parameter must be set to reflect the actual
  * buffer size, and should be at least 16.
  * If the owned flag is true, the stream becomes the owner of the data buffer; otherwise,
  * the stream will not delete the buffer.
  * Passing NULL for the data buffer will cause the stream to allocate a buffer of the
  * given size.
  */
  FXbool open(FXStreamDirection save_or_load,FXuchar* data=nullptr,FXuval size=~0UL,FXbool owned=false);

  /// Take buffer away from stream
  void takeBuffer(FXuchar*& data,FXuval& size);

  /// Give buffer to stream, making it the owner of this buffer
  void giveBuffer(FXuchar *data,FXuval size);

  /// Get position
  FXlong position() const { return FXStream::position(); }

  /// Move to position
  virtual FXbool position(FXlong offset,FXWhence whence=FXFromStart);

  /// Save single items to stream
  FXMemoryStream& operator<<(const FXuchar& v){ FXStream::operator<<(v); return *this; }
  FXMemoryStream& operator<<(const FXchar& v){ FXStream::operator<<(v); return *this; }
  FXMemoryStream& operator<<(const FXbool& v){ FXStream::operator<<(v); return *this; }
  FXMemoryStream& operator<<(const FXushort& v){ FXStream::operator<<(v); return *this; }
  FXMemoryStream& operator<<(const FXshort& v){ FXStream::operator<<(v); return *this; }
  FXMemoryStream& operator<<(const FXuint& v){ FXStream::operator<<(v); return *this; }
  FXMemoryStream& operator<<(const FXint& v){ FXStream::operator<<(v); return *this; }
  FXMemoryStream& operator<<(const FXfloat& v){ FXStream::operator<<(v); return *this; }
  FXMemoryStream& operator<<(const FXdouble& v){ FXStream::operator<<(v); return *this; }
  FXMemoryStream& operator<<(const FXlong& v){ FXStream::operator<<(v); return *this; }
  FXMemoryStream& operator<<(const FXulong& v){ FXStream::operator<<(v); return *this; }

  /// Save arrays of items to stream
  FXMemoryStream& save(const FXuchar* p,FXuval n){ FXStream::save(p,n); return *this; }
  FXMemoryStream& save(const FXchar* p,FXuval n){ FXStream::save(p,n); return *this; }
  FXMemoryStream& save(const FXbool* p,FXuval n){ FXStream::save(p,n); return *this; }
  FXMemoryStream& save(const FXushort* p,FXuval n){ FXStream::save(p,n); return *this; }
  FXMemoryStream& save(const FXshort* p,FXuval n){ FXStream::save(p,n); return *this; }
  FXMemoryStream& save(const FXuint* p,FXuval n){ FXStream::save(p,n); return *this; }
  FXMemoryStream& save(const FXint* p,FXuval n){ FXStream::save(p,n); return *this; }
  FXMemoryStream& save(const FXfloat* p,FXuval n){ FXStream::save(p,n); return *this; }
  FXMemoryStream& save(const FXdouble* p,FXuval n){ FXStream::save(p,n); return *this; }
  FXMemoryStream& save(const FXlong* p,FXuval n){ FXStream::save(p,n); return *this; }
  FXMemoryStream& save(const FXulong* p,FXuval n){ FXStream::save(p,n); return *this; }

  /// Load single items from stream
  FXMemoryStream& operator>>(FXuchar& v){ FXStream::operator>>(v); return *this; }
  FXMemoryStream& operator>>(FXchar& v){ FXStream::operator>>(v); return *this; }
  FXMemoryStream& operator>>(FXbool& v){ FXStream::operator>>(v); return *this; }
  FXMemoryStream& operator>>(FXushort& v){ FXStream::operator>>(v); return *this; }
  FXMemoryStream& operator>>(FXshort& v){ FXStream::operator>>(v); return *this; }
  FXMemoryStream& operator>>(FXuint& v){ FXStream::operator>>(v); return *this; }
  FXMemoryStream& operator>>(FXint& v){ FXStream::operator>>(v); return *this; }
  FXMemoryStream& operator>>(FXfloat& v){ FXStream::operator>>(v); return *this; }
  FXMemoryStream& operator>>(FXdouble& v){ FXStream::operator>>(v); return *this; }
  FXMemoryStream& operator>>(FXlong& v){ FXStream::operator>>(v); return *this; }
  FXMemoryStream& operator>>(FXulong& v){ FXStream::operator>>(v); return *this; }

  /// Load arrays of items from stream
  FXMemoryStream& load(FXuchar* p,FXuval n){ FXStream::load(p,n); return *this; }
  FXMemoryStream& load(FXchar* p,FXuval n){ FXStream::load(p,n); return *this; }
  FXMemoryStream& load(FXbool* p,FXuval n){ FXStream::load(p,n); return *this; }
  FXMemoryStream& load(FXushort* p,FXuval n){ FXStream::load(p,n); return *this; }
  FXMemoryStream& load(FXshort* p,FXuval n){ FXStream::load(p,n); return *this; }
  FXMemoryStream& load(FXuint* p,FXuval n){ FXStream::load(p,n); return *this; }
  FXMemoryStream& load(FXint* p,FXuval n){ FXStream::load(p,n); return *this; }
  FXMemoryStream& load(FXfloat* p,FXuval n){ FXStream::load(p,n); return *this; }
  FXMemoryStream& load(FXdouble* p,FXuval n){ FXStream::load(p,n); return *this; }
  FXMemoryStream& load(FXlong* p,FXuval n){ FXStream::load(p,n); return *this; }
  FXMemoryStream& load(FXulong* p,FXuval n){ FXStream::load(p,n); return *this; }

  /// Save object
  FXMemoryStream& saveObject(const FXObject* v){ FXStream::saveObject(v); return *this; }

  /// Load object
  FXMemoryStream& loadObject(FXObject*& v){ FXStream::loadObject(v); return *this; }

  /// Load object
  template<class TYPE>
  FXMemoryStream& operator>>(TYPE*& obj){ return loadObject(reinterpret_cast<FXObject*&>(obj)); }

  /// Save object
  template<class TYPE>
  FXMemoryStream& operator<<(const TYPE* obj){ return saveObject(static_cast<const FXObject*>(obj)); }

  /// Destructor
  virtual ~FXMemoryStream();
  };

}

#endif
