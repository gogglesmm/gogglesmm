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

#include <expat.h>


namespace ap {


class XMLStream {
protected:
  XML_Parser   parser;
  FXint        depth;
  FXint        skip;
private:
  static void xml_element_start(void*,const FXchar*,const FXchar**);
  static void xml_element_end(void*,const FXchar*);
  static void xml_element_data(void*,const FXchar*,FXint);
protected:
  virtual FXint begin(const FXchar *,const FXchar**) { return 1;}
  virtual void data(const FXchar *,FXint) {}
  virtual void end(const FXchar *){}
public:
  XMLStream();

  FXbool parse(const FXchar * buffer,FXint length);
  FXbool parse(const FXString & buffer);

  virtual ~XMLStream();
  };

XMLStream::XMLStream() : parser(NULL), depth(1),skip(0) {
  parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser,this);
  XML_SetElementHandler(parser,xml_element_start,xml_element_end);
  XML_SetCharacterDataHandler(parser,xml_element_data);
  }

XMLStream::~XMLStream() {
  XML_ParserFree(parser);
  }

FXbool XMLStream::parse(const FXchar * buffer,FXint length) {
  return XML_Parse(parser,buffer,length,1)!=0;
  }

FXbool XMLStream::parse(const FXString & buffer) {
  return XML_Parse(parser,buffer.text(),buffer.length(),1)!=0;
  }

void XMLStream::xml_element_start(void*ptr,const FXchar * element,const FXchar ** attributes) {
  XMLStream * stream = reinterpret_cast<XMLStream*>(ptr);
  if (!stream->skip) {
    if (!stream->begin(element,attributes)) {
      stream->skip=stream->depth;
      }
    }
  stream->depth++;
  }

void XMLStream::xml_element_end(void*ptr,const FXchar * element) {
  XMLStream * stream = reinterpret_cast<XMLStream*>(ptr);

  if (!stream->skip)
    stream->end(element);

  stream->depth--;

  // turn off skip
  if (stream->skip==stream->depth)
    stream->skip=0;
  }

void XMLStream::xml_element_data(void*ptr,const FXchar * data,FXint len) {
  XMLStream * stream = reinterpret_cast<XMLStream*>(ptr);
  stream->data(data,len);
  }


class XSPFParser : public XMLStream{
public:
  FXStringList files;
  FXString     title;
protected:
  FXint        elem;
protected:
  FXint begin(const FXchar *,const FXchar**);
  void data(const FXchar *,FXint len);
  void end(const FXchar *);
public:
  enum {
    Elem_None,
    Elem_Playlist,
    Elem_Playlist_Title,
    Elem_Playlist_TrackList,
    Elem_Playlist_TrackList_Track,
    Elem_Playlist_TrackList_Track_Location,    
    };
public:
  XSPFParser();
  ~XSPFParser();
  };

XSPFParser::XSPFParser() : elem(Elem_None) {
  }
  
XSPFParser::~XSPFParser(){
  }  

FXint XSPFParser::begin(const FXchar * element,const FXchar ** attributes){
  switch(elem) {
    case Elem_None:
      {
        if (compare(element,"playlist")==0) {
          elem=Elem_Playlist;
          return 1;
          }
      } break;
    case Elem_Playlist:
      {
        if (compare(element,"title")==0) {
          elem=Elem_Playlist_Title;
          return 1;
          }
        else if (compare(element,"trackList")==0) {
          elem=Elem_Playlist_TrackList;
          return 1;
          }
      } break;
    case Elem_Playlist_TrackList:
      {
        if (compare(element,"track")==0) {
          elem=Elem_Playlist_TrackList_Track;
          return 1;
          }
      } break;      
    case Elem_Playlist_TrackList_Track:
      {
        if (compare(element,"location")==0) {
          elem=Elem_Playlist_TrackList_Track_Location;
          return 1;    
          }
      } break;      
    default: return 0; // skip
    }
  return 0;
  }


void XSPFParser::data(const FXchar* data,FXint len){
  if (elem==Elem_Playlist_Title) {
    title.assign(data,len);
    }
  else if (elem==Elem_Playlist_TrackList_Track_Location) {
    FXString url(data,len);
    files.append(url);
    }
  }

void XSPFParser::end(const FXchar*) {
  switch(elem){
    case Elem_Playlist_TrackList_Track_Location: elem=Elem_Playlist_TrackList_Track; break;
    case Elem_Playlist_TrackList_Track         : elem=Elem_Playlist_TrackList; break;
    case Elem_Playlist_TrackList               : 
    case Elem_Playlist_Title                   : elem=Elem_Playlist; break;
    case Elem_Playlist                         : elem=Elem_None; break;
    }
  }

static void gm_parse_xspf(FXString & data,FXStringList & mrl) {
  XSPFParser xspf;
  if (xspf.parse(data)) {
    mrl=xspf.files;
    }
  }




class XSPFReader : public ReaderPlugin {
protected:
  FXStringList uri;
public:
  XSPFReader(AudioEngine*);
  FXbool init();
  ReadStatus process(Packet*);
  FXuchar format() const { return Format::M3U; };
  FXbool seek(FXdouble) { return false; }
  FXbool redirect(FXStringList & u) { u=uri; return true; }
  virtual ~XSPFReader();
  };



XSPFReader::XSPFReader(AudioEngine*e) : ReaderPlugin(e) {
  }

XSPFReader::~XSPFReader(){
  }

FXbool XSPFReader::init() {
  uri.clear();
  return true;
  }

ReadStatus XSPFReader::process(Packet*packet) {
  packet->unref();
  FXString buffer;
  FXint n,l=0;

  fxmessage("[xspf] starting read\n");
  if (engine->input->size()>0){
    if (engine->input->size()>0xFFFF) {
      fxmessage("[xspf] input too big %ld\n",engine->input->size());
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
        fxmessage("[xspf] input too big %d\n",l);
        return ReadError;
        }
      buffer.length(buffer.length()+4096);
      n=engine->input->read(&buffer[l],4096);
      if (n==-1) return ReadInterrupted;
      else { l+=n; }
      }
    buffer.trunc(l);
    }
  fxmessage("[xspf] read ok, parsing\n");
  gm_parse_xspf(buffer,uri);

  for (FXint i=0;i<uri.no();i++){
    fxmessage("url[%d]=%s\n",i,uri[i].text());
    }

  if (uri.no())
    return ReadRedirect;
  else
    return ReadDone;
  }

ReaderPlugin * ap_xspf_input(AudioEngine * engine) {
  return new XSPFReader(engine);
  }
}
