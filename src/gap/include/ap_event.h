#ifndef AP_EVENT_H
#define AP_EVENT_H

namespace ap {

enum EventType {
  AP_INVALID,
  AP_BOS,           // Event
  AP_EOS,
  AP_STATE_READY,
  AP_STATE_PLAYING,
  AP_STATE_PAUSING,
  AP_TIMESTAMP,     // TimeUpdate
  AP_ERROR,         // ErrorMessage
  AP_META_INFO,
  AP_LAST           // Reserved
  };

enum ReplayGainMode {
  ReplayGainOff     = 0,
  ReplayGainTrack   = 1,
  ReplayGainAlbum   = 2,
  };


class Event;

class GMAPI Event {
public:
  Event     * next;       /// used by pool and queue to   (8)
  FXuchar     type;       /// type of event               (1)
  FXuint      stream;     ///                             (4)
protected:
  Event();
protected:
  virtual ~Event();
public:
  Event(FXuchar t);
  virtual void unref();
  static void unref(Event*&);
  };

class GMAPI ErrorMessage : public Event {
public:
  FXString msg;
protected:
  virtual ~ErrorMessage();
public:
  ErrorMessage(const FXString & t=FXString::null);
  };

class GMAPI TimeUpdate : public Event {
public:
  FXuint position;
  FXuint length;
protected:
  virtual ~TimeUpdate();
public:
  TimeUpdate(FXuint p,FXuint l);
  };

class GMAPI MetaInfo : public Event {
public:
  FXString title;
  FXString artist;
  FXString album;
protected:
  virtual ~MetaInfo();
public:
  MetaInfo();
  };


}
#endif
