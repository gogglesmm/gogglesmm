#ifndef AP_MEMORY_BUFFER_H
#define AP_MEMORY_BUFFER_H

namespace ap {

class MemoryBuffer {
protected:
  FXuchar * data_buffer;
  FXival    data_capacity;
  FXival    data_size;
public:
  /// Constructor
  MemoryBuffer(FXival cap=32768);

  /// Number of bytes in buffer
  FXival size() const { return data_size; }

  /// Size of the total buffer
  FXival capacity() const { return data_capacity; }

  /// Space left in buffer
  FXival space() const { return data_capacity-data_size; }

  /// Data
  FXuchar * data() const { return data_buffer; }

  /// Append buf of size sz.
  void append(const void * buf,FXival sz);

  /// Append sz zero bytes
  void appendZero(FXival sz);

  /// Clear buffer
  void clear();

  /// Grow buffer to sz
  void grow(FXival sz);

  /// Grow buffer b sz bytes
  //void grow(FXival sz);

  /// Trim bytes for p
  void trimBefore(const FXuchar * p);

  /// Trim sz bytes at beginning
  void trimBegin(FXival sz);

  /// Trim sz bytes at end
  void trimEnd(FXival sz);


  void stats();

  /// Returns the write ptr.
  FXuchar * ptr() const { return data_buffer+data_size; }
  FXchar*   s8() const { return reinterpret_cast<FXchar*>(data_buffer+data_size); }
  FXshort*  s16() const { return reinterpret_cast<FXshort*>(data_buffer+data_size); }
  FXint*    s32() const { return reinterpret_cast<FXint*>(data_buffer+data_size); }

  /// Wrote sz bytes in buffer
  void wrote(FXival sz);

  /// Read sz bytes from buffer
  void read(FXival sz);

  /// Destructor
  ~MemoryBuffer();
  };


class MemoryStream {
public:
  FXuchar * data_buffer;
  FXuchar * data_ptr;
  FXival    data_capacity;
  FXival    data_size;
public:
  MemoryStream(FXival cap=32768);

  /// Append sz bytes.
  void append(const void * buf,FXival sz);

  /// Read sz bytes from buffer
  FXival read(void * buf,FXival sz);
  
  FXival copy(void * buf,FXival sz);

  /// Read sz bytes from buffer
  void read(FXival sz);

  void wrote(FXival sz);

  void padding(FXival sz);

  /// Clear Buffer
  void clear();

  /// Reserve sz bytes
  void reserve(FXival sz);

  /// Number of bytes in buffer
  FXival size() const { return data_size - (data_ptr-data_buffer); }

  /// Size of the total buffer
  FXival capacity() const { return data_capacity; }

  /// Space left in buffer
  FXival space() const { return data_capacity-data_size; }

  /// Write Ptr
  FXuchar * ptr() const { return data_buffer+data_size; }


  ~MemoryStream();
  };

}

#endif

