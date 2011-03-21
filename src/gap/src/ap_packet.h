#ifndef MEM_PACKET_H
#define MEM_PACKET_H

namespace ap {

class Packet;

/*
class Stream {
  FXint  id;
  FXlong length;
  FXlong position;
  };
*/

class PacketPool {
protected:
  EventPipe ppool;
  Event*    list;
protected:
  void fetchEvents();
public:
  /// Constructor
  PacketPool();

  /// Initialize pool
  FXbool init(FXival sz,FXival n);

  /// free pool
  void free();

  /// Get packet out of pool [if any]
  Packet * pop();

  /// Put event back into pool
  void push(Packet*);

  /// Get a handle to the pool
  FXInputHandle handle() const;

  /// Destructor
  ~PacketPool();
  };


class Packet : public Event, public MemoryBuffer {
  friend class PacketPool;
protected:
  PacketPool*   pool;
public:
  AudioFormat   af;
  FXuchar       flags;
  FXlong        stream_position;
  FXlong        stream_length;
protected:
  Packet(PacketPool*,FXival sz);
  virtual ~Packet();
public:
  virtual void unref();

  void clear();

  FXbool full() const { return (af.framesize() > space()); }

  FXint numFrames() const { return size() / af.framesize(); }

  FXint availableFrames() const { return space() / af.framesize(); }

  void wroteFrames(FXint nframes) { wrote(nframes*af.framesize()); }

  void appendFrames(const FXuchar * buf,FXival nframes) { append(buf,af.framesize()*nframes); }

  void trimFrames(FXint nframes) { trimEnd(nframes*af.framesize()); }
  };

}

#endif
