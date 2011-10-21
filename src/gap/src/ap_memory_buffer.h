#ifndef AP_MEMORY_BUFFER_H
#define AP_MEMORY_BUFFER_H

namespace ap {


class MemoryBuffer {
protected:
  FXchar * buffer;
  FXival   buffersize;
  FXchar * rdptr;
  FXchar * wrptr;
public:
  // Constructor
  MemoryBuffer(FXival cap=4096);

  // Construct initialized from other buffer
  MemoryBuffer(const MemoryBuffer &);

  // Assignment operator
  MemoryBuffer& operator=(const MemoryBuffer&);

  // Append operator
  MemoryBuffer& operator+=(const MemoryBuffer&);

  // Adopt from buffer
  void adopt(MemoryBuffer &);

  // Make room for nbytes
  void reserve(FXival nbytes);

  // Clear buffer and reset to nbytes
  void reset(FXival nbytes=4096);

  // Clear buffer
  void clear();

  // Number of unread bytes
  FXival size() const { return (wrptr-rdptr); }

  // Number of bytes that can be written
  FXival space() const { return buffersize - (wrptr-buffer); }

  // Size of the buffer
  FXival capacity() const { return buffersize; }

  // Read nbytes
  FXival read(void * bytes,FXival nbytes);

  // Read nbytes without advancing the read ptr.
  FXival peek(void * bytes,FXival nbytes);

  // Append bytes of nbytes.
  void append(const void * bytes,FXival nbytes);

  // Append constant nbytes.
  void append(const FXchar c, FXival nbytes=1);

  // Wrote nbytes. Updates the wrptr
  void wroteBytes(FXival nbytes);

  // Read nbytes. Update the rdptr
  void readBytes(FXival nbytes);

  /// Trim nbytes at beginning
  void trimBegin(FXival nbytes);

  /// Trim nbytes at end
  void trimEnd(FXival nbytes);


  /// Index into array
  const FXchar& operator[](FXint i) const { return rdptr[i]; }

  /// Return write pointer
  FXuchar* ptr() { return (FXuchar*)wrptr; }
  const FXuchar* ptr() const { return (FXuchar*)wrptr; }

  FXfloat * flt() { return reinterpret_cast<FXfloat*>(wrptr); }
  FXchar  * s8()  { return reinterpret_cast<FXchar*>(wrptr); }
  FXshort * s16() { return reinterpret_cast<FXshort*>(wrptr); }
  FXint   * s32() { return reinterpret_cast<FXint*>(wrptr); }

  /// Return pointer to buffer
  FXuchar* data() { return (FXuchar*)rdptr; }
  const FXuchar* data() const { return (const FXuchar*)rdptr; }

  void setReadPosition(const FXuchar *p) { rdptr=(FXchar*)p; }

  // Destructor
  ~MemoryBuffer();
  };
#if 0




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
  FXfloat*  flt() const { return reinterpret_cast<FXfloat*>(data_buffer+data_size); }


  /// Wrote sz bytes in buffer
  void wrote(FXival sz);

  /// Read sz bytes from buffer
  void read(FXival sz);

  /// Destructor
  ~MemoryBuffer();
  };


class MemoryStream {
public:
  FXuchar * buffer;
  FXival    buffersize;
  FXuchar * sr;
  FXuchar * sw;
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
  FXival size() const { return sw-sr; }

  /// Size of the total buffer
  FXival capacity() const { return buffersize; }

  /// Space left in buffer
  FXival space() const { return buffersize-size(); }

  /// Write Ptr
  FXuchar * ptr() const { return sw; }

  ~MemoryStream();
  };

#endif
}

#endif

