#include "ap_defs.h"
#include "ap_config.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_event.h"
#include "ap_event_private.h"
#include "ap_memory_buffer.h"
#include "ap_packet.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_engine.h"
#include "ap_input_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_memory_buffer.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"


#include <mad.h>

namespace ap {

class XingHeader;
class VBRIHeader;
class LameHeader;
struct mpeg_frame;

class MadInput : public InputPlugin {
protected:
  FXuchar      buffer[4];
  FXbool       sync;
  FXint        bitrate;
protected:
  FXlong       input_start;
  FXlong       input_end;
  FXlong       stream_position;
protected:
  XingHeader * xing;
  VBRIHeader * vbri;
  LameHeader * lame;
  InputStatus parse(Packet*);
protected:
  FXbool readFrame(Packet*,const mpeg_frame&);
  void parseFrame(Packet*,const mpeg_frame&);
  FXbool parse_id3v1();
  FXbool parse_ape();
public:
  MadInput(AudioEngine*);
  FXbool init();
  FXbool can_seek() const;
  FXbool seek(FXdouble);
  InputStatus process(Packet*);
  virtual ~MadInput();
  };


class MadDecoder : public DecoderPlugin {
protected:
  MemoryBuffer buffer;
  Packet * out;
protected:
  mad_synth synth;
  mad_stream stream;
  mad_frame frame;
protected:
  FXuchar flags;
  FXlong  stream_position;
  FXshort stream_offset_start;
  FXshort stream_offset_end;
  FXint        frame_counter;

protected:
  enum {
    FLAG_INIT = 0x1,
    };
public:
  MadDecoder(AudioEngine*);
  FXuchar codec() const { return Codec::MPEG; }
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  FXbool flush();
  virtual ~MadDecoder();
  };

















static const FXint bitrates[]={
  0,32000,64000,96000, 128000,160000,192000,224000, 256000,288000,320000,352000, 384000,416000,448000,-1, /// v1,l1
  0,32000,48000,56000,  64000, 80000, 96000,112000, 128000,160000,192000,224000, 256000,320000,384000,-1, /// v1,l2
  0,32000,40000,48000,	56000, 64000, 80000, 96000, 112000,128000,160000,192000, 224000,256000,320000,-1, /// v1,l3
  0,32000,48000,56000,  64000, 80000, 96000,112000, 128000,144000,160000,176000, 192000,224000,256000,-1, /// v2,l1
  0, 8000,16000,24000,  32000, 40000, 48000, 56000,  64000, 80000, 96000,112000, 128000,144000,160000,-1  /// v2,l2,l3
  };

static const FXint samplerates[]={
  44100,48000,32000,-1, /// v1
  22050,24000,16000,-1, /// v2
  11025,12000, 8000,-1  /// v2.5
  };

static const FXchar * const channels[]={
  "Stereo",
  "Joint",
  "Dual",
  "Single"
  };


#define SYNCSAFE_INT32(b0,b1,b2,b3) (((b0&0x7f)<<21) | ((b1&0x7f)<<14) | ((b2&0x7F)<<7) | (b3&0x7F))





struct mpeg_frame {
private:
  FXuint header;
public:
  enum {
    Invalid = -1,
    V1			=  0,
    V2			=  1,
    V25		  =  2,
    };

  enum {
    Layer_1 = 0,
    Layer_2 = 1,
    Layer_3 = 2,
    };

  enum {
    Stereo = 0,
    Joint	 = 1,
    Dual   = 2,
    Single = 3
    };
protected:
public:
  mpeg_frame() : header(0) {}


#define PRINT_YES_NO(x) (x ? "yes" : "no")

  void debug() {
    fxmessage("   has_sync: %s\n",PRINT_YES_NO(sync()));
    fxmessage("    version: %d\n",version());
    fxmessage("      layer: %d\n",layer());
    fxmessage("    bitrate: %d\n",bitrate());
    fxmessage(" samplerate: %d\n",samplerate());
    fxmessage("   channels: %d\n",channel());
    fxmessage("    samples: %d\n",nsamples());
    fxmessage("       size: %d\n",size());
    }


  FXbool validate(const FXuchar * buffer) {
    header=INT32_BE(buffer);
    if ((!sync()) ||
        (version()==Invalid) ||
        (layer()==Invalid) ||
        (samplerate()==-1)
        ) {
      return false;
      }
    return true;
    }

  FXbool sync() const {
    return (header>>21)==0x7ff;
    }

  FXchar version() const {
    const FXuchar v = ((header>>19)&0x3);
    switch(v) {
      case 0 : return V25; 		break;
      case 2 : return V2; 			break;
      case 3 : return V1; 			break;
      default: break;
      }
    //fxmessage("Invalid version %u\n",v);
    return Invalid;
    }

  FXchar layer() const {
    const FXuchar v = ((header>>17)&0x3);
    switch(v) {
      case 1 : return Layer_3; break;
      case 2 : return Layer_2; break;
      case 3 : return Layer_1; break;
      default: break;
      }
    //fxmessage("Invalid layer %u\n",v);
    return Invalid;
    }

  FXbool crc() const {
    return ((header>>16)&0x1)==0;
    }

  FXint bitrate() const {
    const FXuint b = ((header>>12)&0xf);
    if (version()==V1)
      return bitrates[b+(layer()<<4)];
    else if (version()==V2 || version()==V25) {
      if (layer()==Layer_1)
        return bitrates[48+b];
      else
        return bitrates[64+b];
      }
    return 0;
    }

  FXint samplerate() const {
    const FXuchar s = ((header>>10)&0x3);
    if (version()!=Invalid) {
      return samplerates[s+(version()<<2)];
      }
    return -1;
    }

  FXint padding() const {
    if ((header>>9)&0x1)
      return (layer()==Layer_1) ? 4 : 1;
    else
      return 0;
    }

  FXuchar channel() const {
    return ((header>>6)&0x3);
    }

  FXint nsamples() const {
    if (layer()==Layer_1)
      return 384;
    else if (layer()==Layer_2 || version()==V1)
      return 1152;
    else  /// layer_3 && (V2 || V2_5)
      return 576;
    }

  FXint size() const {
    FXint s = nsamples() * (bitrate() / 8);
    s /= samplerate();
    s += padding();
    return s;
    }

  FXint xing_offset() const {
    if (version()==V1) {
      if (channel()!=3)
        return 32+4;
      else
        return 17+4;
      }
    else {
      if (channel()!=3)
        return 17+4;
      else
        return 9+4;
      }
    }

  FXint vbri_offset() const {
    return 32+4;
    }

  FXint lame_offset()  const {
    return xing_offset() + 120;
    }


  };



class VBRIHeader {
public:
  FXushort version;
  FXushort delay;
  FXushort quality;
  FXuint   nbytes;
  FXuint   nframes;
  FXushort ntoc;
  FXushort toc_scale;
  FXushort toc_entry_nbytes;
  FXushort toc_entry_nframes;
  FXuint*  toc;
public:
  VBRIHeader();
  void parse(const FXuchar * buffer,FXival nbytes);
  ~VBRIHeader();
  };

VBRIHeader::VBRIHeader() : toc(NULL) {
  }

VBRIHeader::~VBRIHeader(){
  if (toc) freeElms(toc);
  }

void VBRIHeader::parse(const FXuchar * buffer,FXival nbytes) {
  version           = INT16_BE(buffer+4);
  delay             = INT16_BE(buffer+6);
  quality           = INT16_BE(buffer+8);
  nbytes            = INT32_BE(buffer+10);
  nframes           = INT32_BE(buffer+14);
  ntoc              = INT16_BE(buffer+18);
  toc_scale         = INT16_BE(buffer+20);
  toc_entry_nbytes  = INT16_BE(buffer+22);
  toc_entry_nframes = INT16_BE(buffer+24);

  if (toc_entry_nbytes>=1 && toc_entry_nbytes <=4) {
    callocElms(toc,ntoc);
    for (FXint i=0;i<ntoc;i++) {
#if FOX_BIGENDIAN == 0
      for (FXint j=0;j<toc_entry_nbytes;j++) {
        toc[i]=(toc[i]<<8) | *(buffer+26+j+(i*toc_entry_nbytes));
        }
#else
#error fixme
#endif
      }
    }

  fxmessage("VBRI:\n");
  fxmessage("version  :%d\n",version);
  fxmessage("delay   :%d\n",delay);
  fxmessage("quality:%d\n",quality);
  fxmessage("nbytes   :%ld\n",nbytes);
  fxmessage("nframes:%d\n",nframes);
  fxmessage("ntoc   :%d\n",ntoc);
  fxmessage("toc_scale:%d\n",toc_scale);
  fxmessage("toc_entry_nbytes   :%d\n",toc_entry_nbytes);
  fxmessage("toc_entry_nframes:%d\n",toc_entry_nframes);

  for (FXint i=0;i<ntoc;i++) {
    fxmessage("toc[%d]=%d\n",i,toc[i]);
    }
  }

#define XING_HEADER_SIZE 120

class XingHeader {
public:
  FXuint    flags;
  FXint     nframes;
  FXint     nbytes;
  FXint     vbr_scale;
  FXuchar   toc[100];
public:
  enum {
    HAS_FRAMES    = 0x1,
    HAS_BYTES     = 0x2,
    HAS_TOC       = 0x4,
    HAS_VBR_SCALE = 0x8
    };
public:
  XingHeader();
  void parse(const FXuchar * buffer,FXival nbytes);

  FXlong seek(FXdouble pos,FXlong length);
  };


XingHeader::XingHeader() : flags(0),nframes(0),nbytes(0),vbr_scale(0) {
  }

void XingHeader::parse(const FXuchar * buffer,FXival nbytes) {

  flags = INT32_BE(buffer+4);
  if (flags&HAS_FRAMES)
    nframes = INT32_BE(buffer+8);
  if (flags&HAS_BYTES)
    nbytes  = INT32_BE(buffer+12);
  if (flags&HAS_TOC)
    memcpy(toc,buffer+16,100);
  if (flags&HAS_VBR_SCALE)
    vbr_scale =  INT32_BE(buffer+16+100);

  fxmessage("Xing:\n");
  fxmessage("nframes  :%d\n",nframes);
  fxmessage("nbytes   :%ld\n",nbytes);
  fxmessage("vbr_scale:%d\n",vbr_scale);
  }

FXlong XingHeader::seek(FXdouble pos,FXlong length) {
  if (flags&HAS_TOC) {
    FXdouble fa,fb,fx;

    FXdouble percent = FXCLAMP(0.0,(pos * 100.0),100.0);

//    fxmessage("percent: %g\n",percent);
    FXint a = FXMIN(99,((FXint)percent));

 //   fxmessage("a: %d\n",a);
    fa = toc[a];
 //   fxmessage("fa: %g\n",fa);

    if (a<99)
      fb = toc[a+1];
    else
      fb = 256.0;

//    fxmessage("fb: %g\n",fb);

    fx=FXLERP(fa,fb,(percent-a));
    return (FXlong) ((1.0/256.0)*fx*length);
    }
  else {
    return -1;
    }
  }



class LameHeader {
public:
  FXushort padstart;
  FXushort padend;
  FXuint   length;
public:
  LameHeader();
  void parse(const FXuchar * buffer,FXival nbytes);
  };

LameHeader::LameHeader() : padstart(0), padend(0), length(0) {
  }

void LameHeader::parse(const FXuchar * buffer,FXival nbytes) {
   fxmessage("Lame:\n");
   FXuchar revision = (*(buffer+9))>>4;
   FXuchar vbr_methed = (*(buffer+9))&0xf;
   fxmessage("revision: %d\n",revision);
   fxmessage("vbr_methed: %d\n",vbr_methed);

   FXint lowpass = (*(buffer+10)) * 100;
   fxmessage("lowpass: %d\n",lowpass);

   FXfloat gain_peak = INT32_BE(buffer+11);
   fxmessage("gain_peak: %f\n",gain_peak);

   FXushort album_gain = INT16_BE(buffer+15);
   FXushort track_gain = INT16_BE(buffer+17);

   FXuchar encoding_flags = (*(buffer+19))>>4;
   FXuchar lame_type = (*(buffer+19))&0xf;
   fxmessage("encoding_flags: %x\n",encoding_flags);
   fxmessage("lame_type: %d\n",lame_type);

   FXuchar bitrate = (*(buffer+21));

   padstart = ((FXuint)*(buffer+22))<<4 | (((FXuint)*(buffer+23))>>4);
   padend   = ((FXuint)*(buffer+23)&0xf)<<8 | ((FXuint)*(buffer+24));
   fxmessage("padding: %d %d\n",padstart,padend);

   FXuchar misc = (*(buffer+25));
   fxmessage("misc: %x\n",misc);

   FXuchar mp3gain = (*(buffer+25));
   FXushort surround = INT16_BE(buffer+26);
   length = INT32_BE(buffer+28);
   fxmessage("length: %d\n",length);
  }



MadInput::MadInput(AudioEngine*e) : InputPlugin(e),
  sync(false),
  xing(NULL),
  vbri(NULL),
  lame(NULL) {
  }

MadInput::~MadInput() {
  delete xing;
  delete vbri;
  delete lame;
  }

FXbool MadInput::init(){
  buffer[0]=buffer[1]=buffer[2]=buffer[3]=0;
  if (xing) {
    delete xing;
    xing=NULL;
    }
  if (vbri) {
    delete vbri;
    vbri=NULL;
    }
  if (lame) {
    delete lame;
    lame=NULL;
    }
  flags&=~FLAG_PARSED;
  sync=false;

  input_start=0;
  input_end=0;
  return true;
  }

FXbool MadInput::can_seek() const{
  return true;
  }

FXbool MadInput::seek(FXdouble pos) {
  FXlong offset = 0;
  if (xing) {
    offset = xing->seek(pos,(input_end - input_start));
    fxmessage("offset: %ld\n",offset);
    if (offset==-1) return false;
    stream_position = stream_length * pos;
    }
  else if (vbri) {
    }
  else {
    stream_position = stream_length * pos;
    offset = (input_end - input_start) * pos;
    }
  engine->input->position(input_start+offset,FXIO::Begin);
  return true;
  }


FXbool MadInput::readFrame(Packet * packet,const mpeg_frame & frame) {
  memcpy(packet->ptr(),buffer,4);
  FXint nread = engine->input->read(packet->ptr()+4,frame.size()-4);
  if (nread!=(frame.size()-4)) {
    packet->unref();
    return false;
    }
  packet->wrote(frame.size());
  return true;
  }


void MadInput::parseFrame(Packet * packet,const mpeg_frame & frame) {
  if (compare((const FXchar*)(packet->data()+frame.xing_offset()),"Xing",4)==0 ||
      compare((const FXchar*)(packet->data()+frame.xing_offset()),"Info",4)==0 ) {

    xing = new XingHeader;
    xing->parse(packet->data()+frame.xing_offset(),packet->size()-frame.xing_offset());

    const FXint lame_offset = frame.xing_offset()+XING_HEADER_SIZE;
    if (compare((const FXchar*)(packet->data()+lame_offset),"LAME",4)==0) {
      lame = new LameHeader;
      lame->parse(packet->data()+lame_offset,packet->size()-lame_offset);
      }
    }

  if (compare((const FXchar*)(packet->data()+frame.vbri_offset()),"VBRI",4)==0) {
    vbri = new VBRIHeader;
    vbri->parse(packet->data()+frame.vbri_offset(),packet->size()-frame.vbri_offset());
    }

  if (xing || vbri || lame) {
    input_start = input_start + frame.size(); // start at next frame.
    if (xing) {
      stream_length = xing->nframes * frame.nsamples();
      if (input_end==-1 || (xing->nbytes>0 && input_start+xing->nbytes<=input_end))
        input_end = input_start+xing->nbytes;
      }
    else if (vbri) {
      stream_length = vbri->nframes * frame.nsamples();
      if (input_end==-1 || (vbri->nbytes>0 && input_start+vbri->nbytes<=input_end))
        input_end = input_start+vbri->nbytes;
      }
    else {
      fxmessage("mad_input: only lame header found\n");
      }

    if (lame) {
      fxmessage("mad_input: lame adjusting stream length by -%d frames \n",(lame->padstart + lame->padend));
      stream_length -= (lame->padstart + lame->padend);
      }

    }
  else {
    bitrate = frame.bitrate();
    if (bitrate>0)
      stream_length =  (FXlong)frame.samplerate() * ((input_end-input_start) / (bitrate / 8) );
    }
  }

FXbool MadInput::parse_id3v1() {
  engine->input->position(input_end-128,FXIO::Begin);

  if (engine->input->read(buffer,3)!=3)
    return false;

  if (compare((FXchar*)buffer,"TAG",3)==0) {
    fxmessage("mad_input: found id3v1 tag\n");
    input_end -= 128;
    }
  return true;
  }

FXbool MadInput::parse_ape() {
  FXchar apebuffer[32];
  FXchar * ape = apebuffer;

  engine->input->position(input_end-32,FXIO::Begin);

  if (engine->input->read(ape,32)!=32)
    return false;

  if (compare(ape,"APETAGEX",8)==0) {

    fxmessage("mad_input: found ape tag");
    FXint ape_version  = INT32_LE(ape+8);
    FXint ape_size     = INT32_LE(ape+12);
    FXint ape_count    = INT32_LE(ape+16);
    FXint ape_flags    = INT32_LE(ape+20);
    FXint ape_reserved = INT32_LE(ape+24);

    enum {
      APE_HEADER          = (1<<29),
      APE_CONTAINS_FOOTER = (1<<30),
      APE_CONTAINS_HEADER = (1<<31)
      };

    if (ape_version==2000) {
      if (ape_flags&APE_HEADER) {
        /// We expect a footer
        fxmessage("mad_input: found ape tag header but expected a footer?\n");
        }
      else {
        if (ape_flags&APE_CONTAINS_HEADER)
          input_end = input_end - ape_size - 32;
        else
          input_end = input_end - ape_size;
        }
      }
    else if (ape_version==1000) {
      input_end = input_end - ape_size;
      }
    else {
      fxmessage("mad_input: unknown ape version %d\n",ape_version);
      }
    }
  return true;
  }


InputStatus MadInput::parse(Packet * packet) {
  mpeg_frame frame;

  stream_position=0;
  stream_length=-1;

  while(1) {

    /* update read buffer */
    if (sync) {
      buffer[0]=buffer[1];
      buffer[1]=buffer[2];
      buffer[2]=buffer[3];
      if (engine->input->read(&buffer[3],1)!=1)
        return InputError;
      }
    else {
      if (engine->input->read(buffer,4)!=4)
        return InputError;
      }

    /// Is this a frame header
    if (frame.validate(buffer)) {

      /// Mark this frame as start of our file.
      input_start = engine->input->position() - 4;
      if (!engine->input->serial())
        input_end = engine->input->size();
      else
        input_end = -1;

      readFrame(packet,frame);

      /// Check for end tags
      if (!engine->input->serial()) {

        if (!parse_id3v1() || !parse_ape())
          return InputError;

        engine->input->position(input_start,FXIO::Begin);
        }

      /// Check for frame headers
      parseFrame(packet,frame);

      /// We found a frame
      af.set(AP_FORMAT_S16,frame.samplerate(),(frame.channel()==mpeg_frame::Single) ? 1 : 2);
      ConfigureEvent * cfg = new ConfigureEvent(af,Codec::MPEG);
      if (lame) {
        cfg->stream_offset_start=lame->padstart;
        cfg->stream_offset_end  =lame->padend;
        }
      engine->decoder->post(cfg);

      if (xing||vbri||lame){
        packet->unref();
        packet=NULL;
        }
      else {
        packet->af              = af;
        packet->stream_position = stream_position;
        packet->stream_length   = stream_length;
        stream_position        += frame.nsamples();
        engine->decoder->post(packet);
        }

      flags|=FLAG_PARSED;
      memset(buffer,0,4);
      sync=false;
      return InputOk;
      }

    /// Check for ID3 tag
    if (buffer[0]=='I' && buffer[1]=='D' && buffer[2]=='3') {
      fxmessage("mad_input: found id3 tag\n");

      FXint   tagsize=0;
      FXuchar id3buf[6];
      if (engine->input->read(id3buf,6)!=6)
        return InputError;

      /* check for footer */
      if (id3buf[1]&0x10)
        tagsize+=10;

      tagsize = SYNCSAFE_INT32(id3buf[2],id3buf[3],id3buf[4],id3buf[5]);

      engine->input->position(tagsize,FXIO::Current);
      sync=false;
      continue;
      }

    if (buffer[0]==0 && buffer[1]==0 && buffer[2]==0 && buffer[3]==0)
      sync=false;
    else
      sync=true;
    }
  return InputError;
  }

InputStatus MadInput::process(Packet*packet) {
  packet->af              = af;
  packet->stream_position = stream_position;
  packet->stream_length   = stream_length;

  if (!(flags&FLAG_PARSED)) {
    return parse(packet);
    }

  mpeg_frame frame;

  InputStatus status=InputOk;
  FXint nread;
  FXbool lostsync=false;

  while(1) {

    if (frame.validate(buffer)) {

      lostsync=false;


      if (frame.size()>packet->space())
        goto done;

      memcpy(packet->ptr(),buffer,4);
      nread = engine->input->read(packet->ptr()+4,frame.size()-4);
      if (nread!=(frame.size()-4)) {
        fxmessage("mad_input: truncated frame at end of input.");
        packet->flags|=FLAG_EOS;
        status=InputDone;
        goto done;
        }
      packet->wrote(frame.size());
      stream_position+=frame.nsamples();
      sync=false;

      if (engine->input->read(buffer,4)!=4){
        packet->flags|=FLAG_EOS;
        status=InputDone;
        goto done;
        }

      continue;
      }
    else {

      if (lostsync==false) {
        lostsync=true;
        fxmessage("mad_input: lost frame sync\n");
        }

      if (buffer[0]==0 && buffer[1]==0 && buffer[2]==0 && buffer[3]==0) {
        if (engine->input->read(buffer,4)!=4){
          packet->flags|=FLAG_EOS;
          status=InputDone;
          goto done;
          }
        }
      else {
        buffer[0]=buffer[1];
        buffer[1]=buffer[2];
        buffer[2]=buffer[3];
        if (engine->input->read(&buffer[3],1)!=1){
          packet->flags|=FLAG_EOS;
          status=InputDone;
          goto done;
          }
        }
      }
    }
done:
  if (packet->size() || packet->flags&FLAG_EOS)
    engine->decoder->post(packet);
  else {
    packet->unref();
    packet=NULL;
    }
  return status;
  }
























MadDecoder::MadDecoder(AudioEngine *e) : DecoderPlugin(e), buffer(MAD_BUFFER_MDLEN),flags(0) {
  out=NULL;
  }

MadDecoder::~MadDecoder(){
  if (out) {
    out->unref();
    out=NULL;
    }
  if (flags&FLAG_INIT) {
    mad_synth_finish(&synth);
    mad_stream_finish(&stream);
    mad_frame_finish(&frame);
    }
  }




FXbool MadDecoder::init(ConfigureEvent* event){
  FXASSERT(out==NULL);
  af=event->af;
  stream_offset_start=event->stream_offset_start;
  stream_offset_end=event->stream_offset_end;
  buffer.clear();

  if (out) {
    out->unref();
    out=NULL;
    }

  if (flags&FLAG_INIT) {
    mad_synth_mute(&synth);
    mad_frame_mute(&frame);

    mad_synth_finish(&synth);
    mad_frame_finish(&frame);
    mad_stream_finish(&stream);
    }

  mad_stream_init(&stream);
  mad_frame_init(&frame);
  mad_synth_init(&synth);

  flags|=FLAG_INIT;


  frame_counter=0;
  return true;
  }

#include <limits.h>

static FXshort madfixed_to_s16(mad_fixed_t Fixed)
{
  /* A fixed point number is formed of the following bit pattern:
   *
   * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
   * MSB                          LSB
   * S ==> Sign (0 is positive, 1 is negative)
   * W ==> Whole part bits
   * F ==> Fractional part bits
   *
   * This pattern contains MAD_F_FRACBITS fractional bits, one
   * should alway use this macro when working on the bits of a fixed
   * point number. It is not guaranteed to be constant over the
   * different platforms supported by libmad.
   *
   * The signed short value is formed, after clipping, by the least
   * significant whole part bit, followed by the 15 most significant
   * fractional part bits. Warning: this is a quick and dirty way to
   * compute the 16-bit number, madplay includes much better
   * algorithms.
   */

  /* round */
  Fixed += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (Fixed >= MAD_F_ONE)
    Fixed = MAD_F_ONE - 1;
  else if (Fixed < -MAD_F_ONE)
    Fixed = -MAD_F_ONE;

  /* quantize */
  return Fixed >> (MAD_F_FRACBITS + 1 - 16);

#if 0
  /* Clipping */
  if(Fixed>=MAD_F_ONE)
    return(SHRT_MAX);
  if(Fixed<=-MAD_F_ONE)
    return(-SHRT_MAX);

  /* Conversion. */
  Fixed=Fixed>>(MAD_F_FRACBITS-15);
  return((FXshort)Fixed);
#endif
}





DecoderStatus MadDecoder::process(Packet*in){
  FXASSERT(in);

  FXint p,s;
  FXuint streamid=in->stream;
  FXlong stream_length=in->stream_length;
  FXbool eos=(in->flags&FLAG_EOS);

  if (in->size() || eos){
    if (in->size()) {
      if (buffer.size()) {
        if (stream.next_frame!=NULL)
          buffer.trimBefore(stream.next_frame);
        }
      else {
        stream_position=in->stream_position;
        }
      buffer.append(in->data(),in->size());
      }
    if (eos) buffer.appendZero(MAD_BUFFER_GUARD);
    mad_stream_buffer(&stream,buffer.data(),buffer.size());
    }

  stream.error=MAD_ERROR_NONE;

  in->unref();

  while(1) {

    /// Decode a frame
    if (mad_frame_decode(&frame,&stream)) {
      if (MAD_RECOVERABLE(stream.error)) {
        continue;
        }
      else if(stream.error==MAD_ERROR_BUFLEN) {
        if (eos) {
          fxmessage("mad_decoder: post end of stream %d\n",streamid);
          if (out && out->numFrames()) {
             if (stream_offset_end)
              out->trimFrames(stream_offset_end);
             engine->output->post(out);
             out=NULL;
             }
          engine->output->post(new ControlEvent(Ctrl_EOS,streamid));
          engine->post(new Event(AP_EOS));
          }
        return DecoderOk;
        }
      else {
        fxmessage("mad_decoder: %s\n",mad_stream_errorstr(&stream));
        return DecoderError;
        }
      }

    mad_synth_frame(&synth,&frame);

    frame_counter++;


    if (frame.header.samplerate!=af.rate) {
      fxmessage("mad_decoder: sample rate changed: %d->%d \n",af.rate,frame.header.samplerate);
      }

    FXint nframes=synth.pcm.length;

    if (nframes==0)
      continue;

    mad_fixed_t * left  = synth.pcm.samples[0];
    mad_fixed_t * right = synth.pcm.samples[1];

    if (stream_position==0) {
      left+=stream_offset_start;
      right+=stream_offset_start;
      nframes-=stream_offset_start;
      fxmessage("mad_decoder: skipping %d frames\n",stream_offset_start);
      }

    if (out) {
      FXASSERT(stream_position);
      if (out->availableFrames()<nframes) {
        engine->output->post(out);
        out=NULL;
        }
      }

    // Get new buffer
    if (out==NULL) {
      out = engine->decoder->get_output_packet();
      if (out==NULL) return DecoderInterrupted; // FIXME
      out->af=af;
      out->stream_position=stream_position;
      out->stream_length=stream_length;
      }

    FXshort * buf16 = reinterpret_cast<FXshort*>(out->ptr());
    if (synth.pcm.channels==2) {
      for (p=0,s=0;s<nframes;s++) {
        buf16[p++] = madfixed_to_s16(*left++);
        buf16[p++] = madfixed_to_s16(*right++);
        }
      }
    else {
      for (p=0,s=0;s<nframes;s++) {
        buf16[p++] = madfixed_to_s16(*left++);
        }
      }
    out->wroteFrames(nframes);
    stream_position+=nframes;
    }
  return DecoderOk;
  }

FXbool MadDecoder::flush(){
  if (out) {
    out->unref();
    out=NULL;
    }
  buffer.clear();
  if (flags&FLAG_INIT) {
    mad_synth_finish(&synth);
    mad_stream_finish(&stream);
    mad_frame_finish(&frame);
    }
  mad_synth_init(&synth);
  mad_stream_init(&stream);
  mad_frame_init(&frame);
  flags|=FLAG_INIT;
  return true;
  }



InputPlugin * ap_mad_input(AudioEngine * engine) {
  return new MadInput(engine);
  }

DecoderPlugin * ap_mad_decoder(AudioEngine * engine) {
  return new MadDecoder(engine);
  }



}












