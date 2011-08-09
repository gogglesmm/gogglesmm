#ifndef AP_EVENT_PRIVATE_H
#define AP_EVENT_PRIVATE_H

namespace ap {

enum EventTypePrivate {

  /// These events abort reading
  Ctrl_Quit       = 0x81,
  Ctrl_Close      = 0x82,
  Ctrl_Open       = 0x83,
  Ctrl_Open_Flush = 0x84,



  Ctrl_Seek       = AP_LAST,
  Ctrl_Pause,
  Ctrl_Set_Output_Config,
  Ctrl_Get_Output_Config,
  Ctrl_Set_Replay_Gain,
  Ctrl_Get_Replay_Gain,
  Ctrl_Volume,
  
  
  Buffer,
  Configure,
  Flush,
  End,
  Input_Read,
  Meta = AP_META_INFO,
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


class SetReplayGain : public Event {
public:
  ReplayGainMode mode;
protected:
  virtual ~SetReplayGain() {}
public:
  SetReplayGain(ReplayGainMode m) : Event(Ctrl_Set_Replay_Gain), mode(m) {}
  };

class SetOutputConfig : public Event {
public:
  OutputConfig config;
protected:
  virtual ~SetOutputConfig() {}
public:
  SetOutputConfig(const OutputConfig & cfg) : Event(Ctrl_Set_Output_Config), config(cfg) {}
  };


/*
  SyncEvent events should be created on the stack.
  When unref'd they signal the condition of the waiting thread.
*/
class SyncEvent : public Event {
public:
  FXMutex     mutex;
  FXCondition condition;
public:

  //// Constructor
  SyncEvent(FXuchar t) : Event(t) {
    mutex.lock();
    }

  /// Destructor
  ~SyncEvent() {
    mutex.unlock();
    }

  /// Wait for unref
  FXbool waitForUnref() {
    return condition.wait(mutex);
    }

  /// Notify waiting thread we're done.
  void unref() {
    FXScopedMutex lock(mutex);
    condition.signal();
    }
  };


class GetOutputConfig : public SyncEvent {
public:
  OutputConfig config;
public:
  GetOutputConfig() : SyncEvent(Ctrl_Get_Output_Config) {}
  virtual ~GetOutputConfig() {}
  };


class GetReplayGain : public SyncEvent {
public:
  ReplayGainMode mode;
public:
  GetReplayGain() : SyncEvent(Ctrl_Get_Replay_Gain) {}
  virtual ~GetReplayGain() {}
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
  ReplayGain    replaygain;
protected:
  virtual ~ConfigureEvent();
public:
  ConfigureEvent(const AudioFormat&,FXuchar codec=Codec::Invalid,FXint f=-1);
  };

}
#endif
