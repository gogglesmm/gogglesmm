#ifndef AP_EVENT_PRIVATE_H
#define AP_EVENT_PRIVATE_H


enum EventTypePrivate {
  Ctrl_Shutdown = AP_LAST,
  Ctrl_Close,
  Ctrl_Open,
  Ctrl_Open_Flush,
  Ctrl_Play,
  Ctrl_Pause,
  Ctrl_Stop,
  Ctrl_Quit,
  Ctrl_Seek,
  Ctrl_EOS,
  Ctrl_Change_Driver,
  Ctrl_Volume,
  Packet_Configure,
  Packet_Data,
  Packet_Available,
  Buffer,
  Configure,
  Flush,
  Input_Read,
  };


class CtrlSeekEvent : public Event {
public:
  FXdouble pos;
protected:
  virtual ~CtrlSeekEvent();
public:
  CtrlSeekEvent(FXdouble);
  };

class CtrlVolumeEvent : public Event {
public:
  FXfloat vol;
protected:
  virtual ~CtrlVolumeEvent();
public:
  CtrlVolumeEvent(FXfloat);
  };

class FlushEvent : public Event {
public:
  FXbool close;
public:
  FlushEvent(FXbool c=false);
  };

class ControlEvent : public Event {
public:
  FXString text;
protected:
  virtual ~ControlEvent();
public:
  ControlEvent(FXuchar type,const FXString & t=FXString::null);
  ControlEvent(FXuchar type,FXuint id);
  };


#endif
