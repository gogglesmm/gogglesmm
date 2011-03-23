#ifndef AP_EVENT_PRIVATE_H
#define AP_EVENT_PRIVATE_H

namespace ap {

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
  Ctrl_Output_Config,
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


class OutputConfigEvent : public Event {
public:
  OutputConfig config;
protected:
  virtual ~OutputConfigEvent() {}
public:
  OutputConfigEvent(const OutputConfig & cfg) : Event(Ctrl_Output_Config), config(cfg) {}
  };



/*
class StreamInfo {
  FXlong  length;       /// Length in samples
  FXlong  position;     /// Position in samples
  FXshort padstart;     /// Start offset in samples
  FXshort padend;       /// End offset in samples
  };
*/

class ConfigureEvent : public Event {
public:
  AudioFormat   af;
  FXuchar       codec;
  FXint         stream_length;
  void*         data;
  FXshort       stream_offset_start;
  FXshort       stream_offset_end;
protected:
  virtual ~ConfigureEvent();
public:
  ConfigureEvent(const AudioFormat&,FXuchar codec=Codec::Invalid,FXint f=-1);
  };

}
#endif
