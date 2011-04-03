#include "ap_config.h"
#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_pipe.h"
#include "ap_event.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_input_plugin.h"
#include "ap_event_private.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_memory_buffer.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_thread_queue.h"
#include "ap_input_thread.h"
#include "ap_reader_plugin.h"
#include "ap_decoder_thread.h"

#include "plugins/ap_file_plugin.h"
#include "plugins/ap_http_plugin.h"
#include "plugins/ap_cdda_plugin.h"


#ifndef WIN32
#include <errno.h>
#endif

namespace ap {

InputThread::InputThread(AudioEngine*e) : EngineThread(e),
  input(NULL),
  reader(NULL),
  streamid(0),
  use_mmap(false),
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

/*
Event * InputThread::wait_for_io() {
  Event * event=NULL;
  do {
    event=fifo.pop();
    if (event) return event;

    ap_wait(io.handle(),fifo.handle());
    }
  while(1);
  return NULL;
  }
*/





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
    if (reader && state==StateProcessing)
      event = wait_for_packet();
    else
      event = wait_for_event();

    switch(event->type) {
      case Ctrl_Close     : fxmessage("Ctrl_Close\n");
                            ctrl_flush(true);
                            ctrl_close_input(true);
                            break;
      case Ctrl_Open      : fxmessage("Ctrl_Open\n");
                            //ctrl_close_input();
                            ctrl_open_input(((ControlEvent*)event)->text);
                            break;
      case Ctrl_Open_Flush: fxmessage("Ctrl_Open_Flush\n");
                            ctrl_flush();
                            //ctrl_close_input();
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
          FXASSERT(reader);
          FXASSERT(packet);
          packet->stream = streamid;
          FXuint status = reader->process(packet);
          switch(status) {
            case ReadError    : fxmessage("[input] error\n");
                                ctrl_close_input();
                                break;
            case ReadDone     : fxmessage("[input] done\n");
                                set_state(StateIdle);
                                break;
            case ReadRedirect : {fxmessage("[input] redirect\n");
                                FXStringList list;
                                reader->redirect(list);
                                ctrl_open_inputs(list);
                                }
                                break;
            default         : break;
            }
          continue; /* packet already released */
          break;
        }
      }
    Event::unref(event);
    }
  return 0;
  }


FXival InputThread::read(void * data,FXival count) {
  FXASSERT(input);
  return input->read(data,count);
  }

FXlong InputThread::position(FXlong offset,FXuint from){
  FXASSERT(input);
  return input->position(offset,from);
  }

FXlong InputThread::position() const{
  FXASSERT(input);
  return input->position();
  }

  /// Postion Input
FXbool InputThread::serial() const{
  FXASSERT(input);
  return input->serial();
  }

  /// Postion Input
FXbool InputThread::eof() const{
  FXASSERT(input);
  return input->eof();
  }
  /// Postion Input
FXlong InputThread::size() const {
  FXASSERT(input);
  return input->size();
  }


void InputThread::ctrl_eos() {
  fxmessage("end of stream reached\n");
  if (state==StateIdle) {
    ctrl_close_input(true);
    }
  }

void InputThread::ctrl_seek(FXdouble pos) {
  if (reader && !serial() && reader->can_seek()) {
    ctrl_flush();
    reader->seek(pos);
    }
  }


#if 0
FXIO * InputThread::open_cdda(const FXString & uri) {
  AudioCD * cdda = NULL;
  if (io)
    cdda = dynamic_cast<AudioCD*>(io);

  if (cdda==NULL) {
    if (io) ctrl_close_input();
    cdda = new AudioCD;
    if (!cdda->open(FXString::null)) {
      delete cdda;
      cdda=NULL;
      }
    }

  if (cdda) {
    url=uri;
    FXint t = FXURL::path(url).after('/').toInt();
    fxmessage("t=%d\n",t);
    cdda->setTrack(t);
    }
  return cdda;
  }
#endif


InputPlugin* InputThread::open_input(const FXString & uri) {
  FXString scheme = FXURL::scheme(uri);

  if (input) {
    delete input;
    input=NULL;
    }

  if (scheme=="file" || scheme.empty()) {
    FileInput * file = new FileInput(fifo.handle());
    if (!file->open(uri)){
      delete file;
      return NULL;
      }
    url=uri;
    return file;
    }
  else if (scheme=="http") {
    HttpInput * http = new HttpInput(fifo.handle());
    if (!http->open(uri)){
      delete http;
      return NULL;
      }
    url=uri;
    return http;
    }
#ifdef HAVE_CDDA_PLUGIN
  else if (scheme=="cdda") {
    CDDAInput * cdda = new CDDAInput(fifo.handle());
    if (cdda->open(uri)) {
      delete cdda;
      return NULL;
      }
    url=uri;
    return cdda;
    }
#endif
  else {
    return NULL;
    }
  }

ReaderPlugin* InputThread::open_reader() {
  /// FIXME try to reuse existing plugin
  if (reader) {
    delete reader;
    reader=NULL;
    }
  return ReaderPlugin::open(engine,input->plugin());
  }


void InputThread::ctrl_open_inputs(const FXStringList & url){
  for (FXint i=0;i<url.no();i++){
    if (url[i].empty()) continue;

    /// Open Input
    input=open_input(url[i]);
    if (input==NULL) continue;

    /// Open Reader
    reader = open_reader();
    if (reader==NULL) continue;

    if (!reader->init())
      continue;

    streamid++;
    set_state(StateProcessing,true);
    return;
    }
  ctrl_close_input();
  set_state(StateIdle,true);
  }

void InputThread::ctrl_open_input(const FXString & uri) {
  fxmessage("ctrl_open_input %s\n",uri.text());

  if (uri.empty()) {
    goto failed;
    }

  /// Open Input
  input=open_input(uri);
  if (input==NULL) {
    engine->post(new ErrorMessage(FXString::value("Unable to open %s",uri.text())));
    goto failed;
    }

  reader = open_reader();
  if (reader==NULL) {
    engine->post(new ErrorMessage(FXString::value("No input plugin available for %s",uri.text())));
    goto failed;
    }

  if (!reader->init()) {
    engine->post(new ErrorMessage(FXString::value("Failed to initialize plugin")));
    goto failed;
    }

  streamid++;
  set_state(StateProcessing,true);
  return;
failed:
  ctrl_close_input();
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
  if (input) {
    delete input;
    input=NULL;
    url.clear();
    }
  if (reader) {
    delete reader;
    reader=NULL;
    }
  set_state(StateIdle,notify);
  }

void InputThread::ctrl_flush(FXbool close){
  engine->decoder->post(new FlushEvent(close),EventQueue::Flush);
  }

}
