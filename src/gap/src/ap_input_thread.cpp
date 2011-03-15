#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_pipe.h"
#include "ap_event.h"
#include "ap_event_private.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_format.h"
#include "ap_memory_buffer.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_thread_queue.h"
#include "ap_input_thread.h"
#include "ap_input_plugin.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"


#ifndef WIN32
#include <errno.h>
#endif

InputThread::InputThread(AudioEngine*e) : EngineThread(e),
  io(NULL),
  plugin(NULL),
  streamid(0),
  use_mmap(true),
  state(StateIdle) {
  }


InputThread::~InputThread() {
  }

FXbool InputThread::init() {

  if (!EngineThread::init())
    return false;

  if (!packetpool.init(8192,40))
    return false;

  return true;
  }

void InputThread::free() {
  packetpool.free();
  EngineThread::free();
  }


Event * InputThread::wait_for_event() {
  Event * event = fifo.pop();
  if (event==NULL) {
    //fxmessage("wait %d\n",ap_wait(fifo.handle()));
    //ap_pipe_read_one(fifo.handle());
    //ap_pipe_clear(fifo.handle());
    ap_wait(fifo.handle());
    event = fifo.pop();
    }
  FXASSERT(event);
  return event;
  }

Event * InputThread::wait_for_packet() {
  Event * event=NULL;
  do {
    event=fifo.pop();
    if (event) return event;

    Packet * packet = packetpool.pop();
    if (packet) return packet;

    ap_wait(packetpool.handle(),fifo.handle());
    }
  while(1);
  return NULL;
  }

/*
   event=fifo.pop();
  if (event) return event;





  do {
    event = fifo.pop();
    if (event)



    if (event) {
      ap_pipe_read_one(fifo.handle());
      return event;
      }

    event = DecoderPacket::get();
    if (event) {
      ap_pipe_read_one(DecoderPacket::handle());
      return event;
      }

    ap_wait(DecoderPacket::handle(),fifo.handle());
    }
  while(1);
  return NULL;
  }
*/

Packet * InputThread::get_packet() {
  FXuchar type;
  do {
    type = fifo.peek();

    if (type!=Buffer && type!=AP_INVALID){
      return NULL;
      }

    Packet * packet = packetpool.pop();
    if (packet) return packet;

//    event = DecoderPacket::get();
//    if (event) return dynamic_cast<DecoderPacket*>(event);

    ap_wait(fifo.handle(),packetpool.handle());
    }
  while(1);
  return NULL;
  }


#if 0
Event* InputThread::get_packet() {
  Event * event=NULL;


  event = DecoderPacket::get();
  if (event) return event;



  if (fifo.peek()!=Invalid) return NULL;

  FXint result = ap_wait(DecoderPacket::handle(),fifo.handle());
  //fxmessage("get packet result==%d\n",result);
  if (result==1) {
    event=DecoderPacket::get();
    }
  return event;
  }



#endif







FXint InputThread::run(){
  Event * event;

  ap_set_thread_name("ap_input");

  for (;;) {
    if (plugin && io && state==StateProcessing)
      event = wait_for_packet();
    else
      event = wait_for_event();

    switch(event->type) {
      case Ctrl_Close     : fxmessage("Ctrl_Close\n");
                            ctrl_flush(true);
                            ctrl_close_input(true);
                            break;
      case Ctrl_Open      : fxmessage("Ctrl_Open\n");
                            ctrl_close_input();
                            ctrl_open_input(((ControlEvent*)event)->text);
                            break;
      case Ctrl_Open_Flush: fxmessage("Ctrl_Open_Flush\n");
                            ctrl_flush();
                            ctrl_close_input();
                            ctrl_open_input(((ControlEvent*)event)->text);
                            break;
      case Ctrl_Quit      : ctrl_close_input(true);
                            //AP_RELEASE_EVENT(event);
                            engine->decoder->post(event,EventQueue::Flush);
                            return 0;
                            break;
      case Ctrl_Seek      : ctrl_seek(((CtrlSeekEvent*)event)->pos);
                            break;

      case Ctrl_EOS       : if (event->stream==streamid) {fxmessage("closing %d\n",streamid); ctrl_eos(); }
                            break;
      case Buffer         :
        {
          Packet * packet = dynamic_cast<Packet*>(event);
          FXASSERT(plugin);
          FXASSERT(packet);
          packet->stream = streamid;
          FXuint status = plugin->process(packet);
          switch(status) {
            case InputError: fxmessage("[input] error\n");
                             ctrl_close_input();
                             break;
            case InputDone : fxmessage("[input] done\n");
                             set_state(StateIdle);
                             break;
            default        : break;
            }
          continue; /* packet already released */
          break;
        }
      }
    Event::unref(event);
    }
  return 0;
  }


/*
  A blocking read that processes events while waiting...
*/
FXival InputThread::read(void * data,FXival count) {
#ifndef WIN32
  FXival nread;
  FXival ncount=count;
  FXchar * buffer = (FXchar *)data;
  while(ncount>0) {
    nread=io->readBlock(buffer,ncount);
    if (__likely(nread>0)) {
      buffer+=nread;
      ncount-=nread;
      }
    else if (nread==0) {
      return count-ncount;
      }
    else {
      if (errno==EINTR) continue;
      else if (errno==EAGAIN || errno==EWOULDBLOCK){
        ap_wait(io->handle(),fifo.handle());
        }
      else {
        return -1;
        }
      }
    }
  return count;
#else
  /// FIXME, implement ASYNC io on Windows.
  return -1;
#endif
  }





/// Postion Input
FXlong InputThread::position(FXlong offset,FXuint from){
  FXASSERT(io);
  return io->position(offset,from);
  }

FXlong InputThread::position() const{
  FXASSERT(io);
  return io->position();
  }

  /// Postion Input
FXbool InputThread::serial() const{
  FXASSERT(io);
  return io->isSerial();
  }

  /// Postion Input
FXbool InputThread::eof() const{
  FXASSERT(io);
  return io->eof();
  }
  /// Postion Input
FXlong InputThread::size() const {
  FXASSERT(io);
  return io->size();
  }


void InputThread::ctrl_eos() {
  fxmessage("end of stream reached\n");
  if (state==StateIdle) {
    ctrl_close_input(true);
    }
  }

void InputThread::ctrl_seek(FXdouble pos) {
  if (plugin && !serial() && plugin->can_seek()) {
    ctrl_flush();
    plugin->seek(pos);
    }
  }


FXIO * InputThread::open_url(const FXString & url_in) {
  if (use_mmap) {
    fxmessage("[input] open using memmap\n");  
    FXMemMap * map = new FXMemMap;
    if (!map->openMap(url_in)) {
      delete map;
      map=NULL;
      }
    else {
      url=url_in;
      }
    return map;
    }
  else {
    FXFile * file = new FXFile;
    if (!file->open(url_in)) {
      delete file;
      file=NULL;
      }
    else {
      url=url_in;
      }
    return file;
    }
  }


void InputThread::ctrl_open_input(const FXString & url_in) {
  fxmessage("ctrl_open_input %s\n",url_in.text());

  /// close current io
  if (io) {
    delete io;
    io=NULL;
    url.clear();
    }

  FXASSERT(io==NULL);
  FXASSERT(plugin==NULL);

  if (url_in.empty())
    goto failed;

  plugin = InputPlugin::open(engine,FXPath::extension(url_in));
  if (plugin==NULL) {
    engine->post(new ErrorMessage(FXString::value("No input plugin available for %s",FXPath::extension(url_in).text())));
    goto failed;
    }

  if (!plugin->init()) {
    engine->post(new ErrorMessage(FXString::value("Failed to initialize plugin")));
    goto failed;
    }

  io = open_url(url_in);
  if (io==NULL) {
    engine->post(new ErrorMessage(FXString::value("Unable to open %s",url_in.text())));
    goto failed;
    }

  streamid++;

  set_state(StateProcessing,true);
  return;

failed:
  if (io) {
    delete io;
    io=NULL;
    }
  if (plugin) {
    delete plugin;
    plugin=NULL;
    }
  url.clear();
  set_state(StateIdle,true);
  }



void InputThread::set_state(FXuchar s,FXbool notify) {
  if (state!=s) {
    state=s;
    }

  /// Tell front end about the state.
  if (notify) {
    switch(state) {
      case StateIdle      : engine->post(new Event(AP_STATE_READY));   break;
      case StateProcessing: engine->post(new Event(AP_STATE_PLAYING)); break;
      default       : break;
      }
    }
  }


void InputThread::ctrl_close_input(FXbool notify) {
  fxmessage("ctrl_close_input %d\n",notify);
  if (io) {
    delete io;
    io=NULL;
    url.clear();
    }
  if (plugin) {
    delete plugin;
    plugin=NULL;
    }
  set_state(StateIdle,notify);
  }

void InputThread::ctrl_flush(FXbool close){
  engine->decoder->post(new FlushEvent(close),EventQueue::Flush);
  }


