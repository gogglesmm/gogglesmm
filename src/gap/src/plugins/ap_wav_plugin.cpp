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

class WavReader : public ReaderPlugin {
protected:
  FXuint datasize;    // size of the data section
  FXlong input_start;
protected:
  ReadStatus parse();
public:
  WavReader(AudioEngine*);
  FXbool init();
  ReadStatus process(Packet*);

  FXuchar format() const { return Format::WAV; };

  FXbool can_seek() const;
  FXbool seek(FXdouble);
  virtual ~WavReader();
  };




enum {
  WAV_FORMAT_PCM = 1,
  WAV_FORMAT_EXTENSIBLE = 0xFFFE
  };

WavReader::WavReader(AudioEngine*e) : ReaderPlugin(e) {
  }

WavReader::~WavReader(){
  }

FXbool WavReader::init() {
  datasize=0;
  input_start=0;
  return true;
  }

FXbool WavReader::can_seek() const {
  return true;
  }

FXbool WavReader::seek(FXdouble pos){
//  if (af.codec==Codec::PCM) {
    FXlong b = (FXlong)(((FXdouble)datasize)*pos);
    FXlong offset=FXCLAMP(0,((b / af.framesize()) * af.framesize()),datasize);
    fxmessage("seek to %ld\n",offset);

    offset+=input_start;
    engine->input->position(offset,FXIO::Begin);
//    }
  return true;
  }

ReadStatus WavReader::process(Packet*packet) {

  if (!(flags&FLAG_PARSED)) {
    if (parse()!=ReadOk)
      return ReadError;
    }

  FXint nbytes = (packet->space() / af.framesize()) * af.framesize();
  FXint nread = engine->input->read(packet->data(),nbytes);
  if (nread<0) {
    packet->unref();
    return ReadError;
    }
  else if (nread==0){
    packet->unref();
    return ReadDone;
    }

  packet->af              = af;
  packet->wrote(nread);
  packet->stream_position = static_cast<FXint>( (engine->input->position()-input_start-nread) / af.framesize() );
  packet->stream_length   = stream_length;
  if (engine->input->eof())
    packet->flags=FLAG_EOS;
  else
    packet->flags=0;

  engine->decoder->post(packet);


  return ReadOk;
  }

ReadStatus WavReader::parse() {
  FXuchar chunkid[4];
  FXuint  chunksize;
  FXushort wconfig;
  FXushort channels;
  FXuint rate;
  FXuint byterate;
  FXushort samplesize;
  FXushort block;

  FXushort subconfig;
  FXushort validbitspersample;
  FXuint   channelmask;

  fxmessage("parsing wav header\n");

  if (engine->input->read(&chunkid,4)!=4 || chunkid[0]!='R' || chunkid[1]!='I' || chunkid[2]!='F' || chunkid[3]!='F'){
    fxmessage("no RIFF tag found\n");
    return ReadError;
    }

  if (engine->input->read(&chunksize,4)!=4){
    return ReadError;
    }

  if (engine->input->read(&chunkid,4)!=4 || chunkid[0]!='W' || chunkid[1]!='A' || chunkid[2]!='V' || chunkid[3]!='E'){
    fxmessage("no WAVE tag found\n");
    return ReadError;
    }

  if (engine->input->read(&chunkid,4)!=4 || chunkid[0]!='f' || chunkid[1]!='m' || chunkid[2]!='t' || chunkid[3]!=' '){
    fxmessage("no fmt tag found\n");
    return ReadError;
    }

  if (engine->input->read(&chunksize,4)!=4)
    return ReadError;

  fxmessage("chunksize=%d\n",chunksize);

  if (engine->input->read(&wconfig,2)!=2 || !(wconfig==WAV_FORMAT_PCM || wconfig==WAV_FORMAT_EXTENSIBLE) ) {
    fxmessage("WAV not in PCM config: %x\n",wconfig);
    return ReadError;
    }

  if (engine->input->read(&channels,2)!=2)
    return ReadError;

  if (engine->input->read(&rate,4)!=4)
    return ReadError;

  if (engine->input->read(&byterate,4)!=4)
    return ReadError;

  if (engine->input->read(&block,2)!=2)
    return ReadError;

  if (engine->input->read(&samplesize,2)!=2)
    return ReadError;


  chunksize-=16;

/*


12        4   Subchunk1ID      Contains the letters "fmt "
                               (0x666d7420 big-endian form).
16        4   Subchunk1Size    16 for PCM.  This is the size of the
                               rest of the Subchunk which follows this number.



20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
                               Values other than 1 indicate some
                               form of compression.
22        2   NumChannels      Mono = 1, Stereo = 2, etc.
24        4   SampleRate       8000, 44100, etc.
28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
32        2   BlockAlign       == NumChannels * BitsPerSample/8
                               The number of bytes for one sample including
                               all channels. I wonder what happens when
                               this number isn't an integer?
34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.


*/






  if (wconfig==WAV_FORMAT_EXTENSIBLE) {

    if (engine->input->read(&validbitspersample,2)!=2)
      return ReadError;

      fxmessage("subsize: %d\n",validbitspersample);


    if (engine->input->read(&validbitspersample,2)!=2)
      return ReadError;

    if (engine->input->read(&channelmask,4)!=4)
      return ReadError;

    if (engine->input->read(&subconfig,2)!=2)
      return ReadError;

    chunksize-=10;


    fxmessage("validbitspersample: %d\n",validbitspersample);
    fxmessage("channelmask: %x\n",channelmask);

    }

  fxmessage("chunksize left: %d\n",chunksize);
  engine->input->position(chunksize,FXIO::Current);

  if (engine->input->read(&chunkid,4)!=4 || chunkid[0]!='d' || chunkid[1]!='a' || chunkid[2]!='t' || chunkid[3]!='a'){
    fxmessage("data tag not found: %c%c%c%c\n",chunkid[0],chunkid[1],chunkid[2],chunkid[3]);
    return ReadError;
    }

  if (engine->input->read(&datasize,4)!=4)
    return ReadError;

  input_start = engine->input->position();

  if (wconfig==WAV_FORMAT_EXTENSIBLE) {
    af.set(Format::Signed|Format::Little,validbitspersample,samplesize>>3,rate,channels);
    }
  else {
    af.set(Format::Signed|Format::Little,samplesize,samplesize>>3,rate,channels);
    }

  af.debug();

  if (block!=af.framesize())
    fxwarning("warning: blockalign not the same as framesize\n");


  flags|=FLAG_PARSED;

  stream_length=-1;
  if (!engine->input->serial()) {
    stream_length = (engine->input->size() - input_start) / af.framesize();
    }

  engine->decoder->post(new ConfigureEvent(af,Codec::PCM));
  return ReadOk;
  }


ReaderPlugin * ap_wav_reader(AudioEngine * engine) {
  return new WavReader(engine);
  }
}
