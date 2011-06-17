#include "ap_defs.h"
#include "ap_common.h"
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
#include "ap_xml_parser.h"

namespace ap {


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

void ap_parse_xspf(const FXString & data,FXStringList & mrl,FXString & title) {
  XSPFParser xspf;
  if (xspf.parse(data)) {
    mrl=xspf.files;
    title=xspf.title;
    }
  }


class XSPFReader : public TextReader {
protected:
  FXStringList uri;
public:
  XSPFReader(AudioEngine*);
  ReadStatus process(Packet*);
  FXbool init();
  FXuchar format() const { return Format::XSPF; };
  FXbool redirect(FXStringList & u) { u=uri; return true; }
  virtual ~XSPFReader();
  };


XSPFReader::XSPFReader(AudioEngine*e) : TextReader(e) {
  }

XSPFReader::~XSPFReader(){
  }

FXbool XSPFReader::init() {
  TextReader::init();
  uri.clear();
  return true;
  }

ReadStatus XSPFReader::process(Packet*packet) {
  if (TextReader::process(packet)==ReadDone) {
    FXString title;
    ap_parse_xspf(textbuffer,uri,title);
    if (uri.no())
      return ReadRedirect;
    else
      return ReadDone;
    }
  return ReadError;
  }

ReaderPlugin * ap_xspf_reader(AudioEngine * engine) {
  return new XSPFReader(engine);
  }


}
