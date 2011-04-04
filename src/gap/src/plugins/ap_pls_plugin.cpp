#include "ap_defs.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_event.h"
#include "ap_event_private.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_memory_buffer.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_reader_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_memory_buffer.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"

namespace ap {

static void gm_parse_pls(FXString & data,FXStringList & mrl) {
  FXint start=0,end=0,pos,next;
  for (FXint i=0;i<data.length();i++) {
    if (data[i]=='\n') {
      end=i;
      next=i+1;

      /// Skip white space
      while(start<end && Ascii::isSpace(data[start])) start++;

      /// Skip white space
      while(end>start && Ascii::isSpace(data[end])) end--;

      /// Parse the actual line.
      if ((end-start)>6) {
        if (compare(&data[start],"File",4)==0) {
          pos = data.find('=',start+4);
          if (pos==-1) continue;
          pos++;
          if (end-pos>0) {
            mrl.append(data.mid(pos,1+end-pos));
            }
          }
        }
      start=next;
      }
    }
  }




class PLSReader : public ReaderPlugin {
protected:
  FXStringList uri;
public:
  PLSReader(AudioEngine*);
  FXbool init();
  ReadStatus process(Packet*);
  FXuchar format() const { return Format::M3U; };
  FXbool seek(FXdouble) { return false; }
  FXbool redirect(FXStringList & u) { u=uri; return true; }
  virtual ~PLSReader();
  };



PLSReader::PLSReader(AudioEngine*e) : ReaderPlugin(e) {
  }

PLSReader::~PLSReader(){
  }

FXbool PLSReader::init() {
  uri.clear();
  return true;
  }

ReadStatus PLSReader::process(Packet*packet) {
  packet->unref();
  FXString buffer;
  FXint n,l=0;

  fxmessage("[pls] starting read\n");
  if (engine->input->size()>0){
    if (engine->input->size()>0xFFFF) {
      fxmessage("[pls] input too big %ld\n",engine->input->size());
      return ReadError;
      }
    l=engine->input->size();
    buffer.length(l);
    n=engine->input->read(&buffer[0],l);
    if (n==-1) return ReadInterrupted;
    else if (n!=l) return ReadError;
    }
  else {
    while(!engine->input->eof()) {
      if ((l+4096)>0xFFFF) {
        fxmessage("[m3u] input too big %d\n",l);
        return ReadError;
        }
      buffer.length(buffer.length()+4096);
      n=engine->input->read(&buffer[l],4096);
      if (n==-1) return ReadInterrupted;
      else { l+=n; }
      }
    buffer.trunc(l);
    }
  fxmessage("[pls] read ok, parsing\n");
  gm_parse_pls(buffer,uri);

  for (FXint i=0;i<uri.no();i++){
    fxmessage("url[%d]=%s\n",i,uri[i].text());
    }

  if (uri.no())
    return ReadRedirect;
  else
    return ReadDone;
  }

ReaderPlugin * ap_pls_reader(AudioEngine * engine) {
  return new PLSReader(engine);
  }
}
