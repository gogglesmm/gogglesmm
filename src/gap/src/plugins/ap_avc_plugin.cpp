#include "ap_defs.h"
#include "ap_config.h"
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
#include "ap_input_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_memory_buffer.h"
#include "ap_decoder_thread.h"
#include "ap_memory_buffer.h"
#include "ap_output_thread.h"
#include "ap_reader_plugin.h"


extern "C" {
#include <libavformat/avformat.h>

// Apparently we need this in order to use ffmpeg from c++.
// /usr/include/libavutil/common.h:168:47: error: ‘UINT64_C’ was not declared in this scope
#include <libavcodec/avcodec.h>
}

namespace ap {

static int gap_open(URLContext *,const char *,int){
  return 0;
  }

static int gap_read(URLContext *context, unsigned char *buf, int size){
  AudioEngine * engine = (AudioEngine*)context->priv_data;
  FXASSERT(engine);
  return engine->input->read(buf,size);
  }

static int gap_write(URLContext *h, const unsigned char*,int){
  return -1;
  }

static int64_t gap_seek(URLContext *h, int64_t pos, int whence){
  return -1;
#if 0
    stream_t *stream = (stream_t*)h->priv_data;

    mp_msg(MSGT_HEADER,MSGL_DBG2,"mp_seek(%p, %d, %d)\n", h, (int)pos, whence);
    if(whence == SEEK_CUR)
        pos +=stream_tell(stream);
    else if(whence == SEEK_END)
        pos += stream->end_pos;
    else if(whence != SEEK_SET)
        return -1;

    if(pos<stream->end_pos && stream->eof)
        stream_reset(stream);
    if(stream_seek(stream, pos)==0)
        return -1;

    return pos;
#endif
}




static int gap_close(URLContext *){
  return 0;
  }


static URLProtocol gap_reader_protocol = {
  "gap",
  gap_open,
  gap_read,
  gap_write,
  gap_seek,
  gap_close
  };


#if 0
class AVReader : public ReaderPlugin {
protected:
  AVInputFormat*   av_format;
  AVFormatContext* av_format_context;
  AVIOContext*     av_io_context;
public:
  AVReader(AudioEngine*);
  FXbool init();
  ReadStatus process(Packet*);

  FXuchar format() const { return Format::Unknown; };
  virtual ~AVReader();
  };

AVReader::AVReader(AudioEngine*e) : ReaderPlugin(e) {
  av_register_all();
  }

AVReader::~AVReader(){
  }

FXbool AVReader::init(){
  AVFormatParameters av_format_params;
  av_format=NULL;
  av_format_context=NULL;
  av_io_context=NULL;


  av_format=av_find_input_format("mp3");

  memset(&av_format_params,0,sizeof(AVFormatParameters));

  if (av_register_protocol2(&gap_reader_protocol,sizeof(gap_reader_protocol)))
    fxmessage("failed to register protocol\n");

  av_format_context=avformat_alloc_context();
  av_format_params.prealloced_context=1;

  if (avio_open(&av_io_context,"gap:foo.bar",URL_RDONLY)){
    fxmessage("failed to open io\n");
    return false;
    }

  ((URLContext*)(av_io_context->opaque))->priv_data=engine;

  if(av_open_input_stream(&av_format_context,av_io_context,"gap:foo.bar",av_format,&av_format_params)<0){
    fxmessage("failed to open input stream\n");
    return false;
    }
  return true;
  }


ReaderPlugin * ap_avf_reader(AudioEngine * engine) {
  return new AVReader(engine);
  }
#endif
#if 0
    AVFormatContext *avfc;
    AVOption *opt;
    lavf_priv_t *priv= demuxer->priv;
    int i,g;
    char mp_filename[256]="mp:";

    memset(&ap, 0, sizeof(AVFormatParameters));

    stream_seek(demuxer->stream, 0);

    register_protocol(&mp_protocol);

    avfc = av_alloc_format_context();
    ap.prealloced_context = 1;
    if(opt_probesize) {
        double d = (double) opt_probesize;
        opt = av_set_double(avfc, "probesize", opt_probesize);
        if(!opt) mp_msg(MSGT_HEADER,MSGL_ERR, "demux_lavf, couldn't set option probesize to %.3f\r\n", d);
    }

    if(demuxer->stream->url)
        strncpy(mp_filename + 3, demuxer->stream->url, sizeof(mp_filename)-3);
    else
        strncpy(mp_filename + 3, "foobar.dummy", sizeof(mp_filename)-3);

    url_fopen(&priv->pb, mp_filename, URL_RDONLY);

    ((URLContext*)(priv->pb.opaque))->priv_data= demuxer->stream;

    if(av_open_input_stream(&avfc, &priv->pb, mp_filename, priv->avif, &ap)<0){
        mp_msg(MSGT_HEADER,MSGL_ERR,"LAVF_header: av_open_input_stream() failed\n");
        return NULL;
    }

    priv->avfc= avfc;

    if(av_find_stream_info(avfc) < 0){
        mp_msg(MSGT_HEADER,MSGL_ERR,"LAVF_header: av_find_stream_info() failed\n");
        return NULL;
    }

    if(avfc->title    [0]) demux_info_add(demuxer, "name"     , avfc->title    );
    if(avfc->author   [0]) demux_info_add(demuxer, "author"   , avfc->author   );
    if(avfc->copyright[0]) demux_info_add(demuxer, "copyright", avfc->copyright);
    if(avfc->comment  [0]) demux_info_add(demuxer, "comments" , avfc->comment  );
    if(avfc->album    [0]) demux_info_add(demuxer, "album"    , avfc->album    );
//    if(avfc->year        ) demux_info_add(demuxer, "year"     , avfc->year     );
//    if(avfc->track       ) demux_info_add(demuxer, "track"    , avfc->track    );
    if(avfc->genre    [0]) demux_info_add(demuxer, "genre"    , avfc->genre    );

    for(i=0; i<avfc->nb_streams; i++){
        AVStream *st= avfc->streams[i];
        AVCodecContext *codec= st->codec;

        switch(codec->codec_type){
        case CODEC_TYPE_AUDIO:{
            WAVEFORMATEX *wf= calloc(sizeof(WAVEFORMATEX) + codec->extradata_size, 1);
            sh_audio_t* sh_audio;
            if(priv->audio_streams >= MAX_A_STREAMS)
                break;
            sh_audio=new_sh_audio(demuxer, i);
            if(!sh_audio)
                break;
            priv->astreams[priv->audio_streams] = i;
            priv->audio_streams++;
            if(!codec->codec_tag)
                codec->codec_tag= codec_get_wav_tag(codec->codec_id);
            wf->wFormatTag= codec->codec_tag;
            wf->nChannels= codec->channels;
            wf->nSamplesPerSec= codec->sample_rate;
            wf->nAvgBytesPerSec= codec->bit_rate/8;
            wf->nBlockAlign= codec->block_align;
            wf->wBitsPerSample= codec->bits_per_sample;
            wf->cbSize= codec->extradata_size;
            if(codec->extradata_size){
                memcpy(
                    wf + 1,
                    codec->extradata,
                    codec->extradata_size);
            }
            sh_audio->wf= wf;
            sh_audio->audio.dwSampleSize= codec->block_align;
            if(codec->frame_size && codec->sample_rate){
                sh_audio->audio.dwScale=codec->frame_size;
                sh_audio->audio.dwRate= codec->sample_rate;
            }else{
                sh_audio->audio.dwScale= codec->block_align ? codec->block_align*8 : 8;
                sh_audio->audio.dwRate = codec->bit_rate;
            }
            g= ff_gcd(sh_audio->audio.dwScale, sh_audio->audio.dwRate);
            sh_audio->audio.dwScale /= g;
            sh_audio->audio.dwRate  /= g;
//            printf("sca:%d rat:%d fs:%d sr:%d ba:%d\n", sh_audio->audio.dwScale, sh_audio->audio.dwRate, codec->frame_size, codec->sample_rate, codec->block_align);
            sh_audio->ds= demuxer->audio;
            sh_audio->format= codec->codec_tag;
            sh_audio->channels= codec->channels;
            sh_audio->samplerate= codec->sample_rate;
            sh_audio->i_bps= codec->bit_rate/8;
            switch (codec->codec_id) {
              case CODEC_ID_PCM_S8:
              case CODEC_ID_PCM_U8:
                sh_audio->samplesize = 1;
                break;
              case CODEC_ID_PCM_S16LE:
              case CODEC_ID_PCM_S16BE:
              case CODEC_ID_PCM_U16LE:
              case CODEC_ID_PCM_U16BE:
                sh_audio->samplesize = 2;
                break;
              case CODEC_ID_PCM_ALAW:
                sh_audio->format = 0x6;
                break;
              case CODEC_ID_PCM_MULAW:
                sh_audio->format = 0x7;
                break;
            }
            if( mp_msg_test(MSGT_HEADER,MSGL_V) ) print_wave_header(sh_audio->wf, MSGL_V);
            if((audio_lang && st->language[0] && !strncmp(audio_lang, st->language, 3))
                || (demuxer->audio->id == i || demuxer->audio->id == -1)
            ) {
                demuxer->audio->id = i;
                demuxer->audio->sh= demuxer->a_streams[i];
            }
            else
                st->discard= AVDISCARD_ALL;
            break;}
        case CODEC_TYPE_VIDEO:{
            BITMAPINFOHEADER *bih=calloc(sizeof(BITMAPINFOHEADER) + codec->extradata_size,1);
            sh_video_t* sh_video=new_sh_video(demuxer, i);

            priv->video_streams++;
            if(!codec->codec_tag)
                codec->codec_tag= codec_get_bmp_tag(codec->codec_id);
            bih->biSize= sizeof(BITMAPINFOHEADER) + codec->extradata_size;
            bih->biWidth= codec->width;
            bih->biHeight= codec->height;
            bih->biBitCount= codec->bits_per_sample;
            bih->biSizeImage = bih->biWidth * bih->biHeight * bih->biBitCount/8;
            bih->biCompression= codec->codec_tag;
            sh_video->bih= bih;
            sh_video->disp_w= codec->width;
            sh_video->disp_h= codec->height;
            if (st->time_base.den) { /* if container has time_base, use that */
                sh_video->video.dwRate= st->time_base.den;
                sh_video->video.dwScale= st->time_base.num;
            } else {
            sh_video->video.dwRate= codec->time_base.den;
            sh_video->video.dwScale= codec->time_base.num;
            }
            sh_video->fps=av_q2d(st->r_frame_rate);
            sh_video->frametime=1/av_q2d(st->r_frame_rate);
            sh_video->format = bih->biCompression;
            sh_video->aspect=   codec->width * codec->sample_aspect_ratio.num
                              / (float)(codec->height * codec->sample_aspect_ratio.den);
            sh_video->i_bps= codec->bit_rate/8;
            mp_msg(MSGT_DEMUX,MSGL_DBG2,"aspect= %d*%d/(%d*%d)\n",
                codec->width, codec->sample_aspect_ratio.num,
                codec->height, codec->sample_aspect_ratio.den);

            sh_video->ds= demuxer->video;
            if(codec->extradata_size)
                memcpy(sh_video->bih + 1, codec->extradata, codec->extradata_size);
            if( mp_msg_test(MSGT_HEADER,MSGL_V) ) print_video_header(sh_video->bih, MSGL_V);
/*    short     biPlanes;
    int         biXPelsPerMeter;
    int         biYPelsPerMeter;
    int         biClrUsed;
    int         biClrImportant;*/
            if(demuxer->video->id != i && demuxer->video->id != -1)
                st->discard= AVDISCARD_ALL;
            else{
                demuxer->video->id = i;
                demuxer->video->sh= demuxer->v_streams[i];
            }
            break;}
        default:
            st->discard= AVDISCARD_ALL;
        }
    }

    mp_msg(MSGT_HEADER,MSGL_V,"LAVF: %d audio and %d video streams found\n",priv->audio_streams,priv->video_streams);
    mp_msg(MSGT_HEADER,MSGL_V,"LAVF: build %d\n", LIBAVFORMAT_BUILD);
    if(!priv->audio_streams) demuxer->audio->id=-2;  // nosound
//    else if(best_audio > 0 && demuxer->audio->id == -1) demuxer->audio->id=best_audio;
    if(!priv->video_streams){
        if(!priv->audio_streams){
            mp_msg(MSGT_HEADER,MSGL_ERR,"LAVF: no audio or video headers found - broken file?\n");
            return NULL;
        }
        demuxer->video->id=-2; // audio-only
    } //else if (best_video > 0 && demuxer->video->id == -1) demuxer->video->id = best_video;

    return demuxer;
#endif




















class AVReader : public ReaderPlugin {
protected:
  AVInputFormat*   av_input_format;
  AVFormatContext* av_format_context;
  AVIOContext*     av_io_context;
  AVCodec*         av_codec;
  AVCodecContext*  av_codec_context;
public:
  AVReader(AudioEngine*);
  FXbool init();
  ReadStatus process(Packet*);

  FXuchar format() const { return Format::Unknown; };
  virtual ~AVReader();
  };

AVReader::AVReader(AudioEngine*e) : ReaderPlugin(e) {
  av_register_all();
  avcodec_init();
  avcodec_register_all();
  }

AVReader::~AVReader(){
  }

FXbool AVReader::init(){
  AVFormatParameters av_format_params;
  av_input_format=NULL;
  av_format_context=NULL;
  av_io_context=NULL;

  av_input_format=av_find_input_format("mp3");

  memset(&av_format_params,0,sizeof(AVFormatParameters));

  if (av_register_protocol2(&gap_reader_protocol,sizeof(gap_reader_protocol)))
    fxmessage("failed to register protocol\n");

  av_format_context=avformat_alloc_context();
  av_format_params.prealloced_context=1;

  if (avio_open(&av_io_context,"gap:foo.bar",URL_RDONLY)){
    fxmessage("failed to open io\n");
    return false;
    }

  ((URLContext*)(av_io_context->opaque))->priv_data=engine;

  if(av_open_input_stream(&av_format_context,av_io_context,"gap:foo.bar",av_input_format,&av_format_params)<0){
    fxmessage("failed to open input stream\n");
    return false;
    }

  av_codec = avcodec_find_decoder(CODEC_ID_MP3);
  av_codec_context = avcodec_alloc_context();
  if (avcodec_open(av_codec_context,av_codec)<0)
    return false;

  return true;
  }


ReadStatus AVReader::process(Packet* packet){
#if 0
  AVPacket av_packet;

  int16_t * out_samples = reinterpret_cast<int16_t*>(packet.data());    /// output buffer
  int       out_size    = packet.capacity();                            /// output buffer size

  while(av_read_frame(av_format_context,&av_packet)>=0) {

    // Is this a packet from the video stream?
    if(packet.stream_index==audioStream)
    {

      int result = avcodec_decode_audio3(av_codec_context,out_samples,&out_size,&avp);
      if (out_size) {




        }


        // Decode video frame
        avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,
            packet.data, packet.size);

        // Did we get a video frame?
        if(frameFinished)
        {
            // Convert the image from its native format to RGB
            img_convert((AVPicture *)pFrameRGB, PIX_FMT_RGB24,
                (AVPicture*)pFrame, pCodecCtx->pix_fmt, pCodecCtx->width,
                pCodecCtx->height);

            // Process the video frame (save to disk etc.)
            DoSomethingWithTheImage(pFrameRGB);
        }
    }

    // Free the packet that was allocated by av_read_frame
    av_free_packet(&packet);
}

#endif
   return ReadOk;
  }


ReaderPlugin * ap_avf_reader(AudioEngine * engine) {
  return new AVReader(engine);
  }























class OutputPacket;

class AVDecoder : public DecoderPlugin {
protected:
  AVCodecContext * ctx;
protected:
  MemoryStream     buffer;
  MemoryBuffer     outbuf;
protected:
  Packet * out;
public:
  AVDecoder(AudioEngine*);
  FXuchar codec() const { return Codec::PCM; }
  FXbool flush();
  FXbool init(ConfigureEvent*);
  DecoderStatus process(Packet*);
  virtual ~AVDecoder();
  };





AVDecoder::AVDecoder(AudioEngine * e) : DecoderPlugin(e), ctx(NULL),outbuf(AVCODEC_MAX_AUDIO_FRAME_SIZE),out(NULL) {

  avcodec_init();
  avcodec_register_all();

  AVCodec * codec = avcodec_find_decoder(CODEC_ID_MP3);
  FXASSERT(codec);

  ctx = avcodec_alloc_context();
  if (avcodec_open(ctx,codec)<0)
    fxerror("error opening codec\n");


  }

AVDecoder::~AVDecoder() {
  flush();
  if (ctx) avcodec_close(ctx);
  }

FXbool AVDecoder::init(ConfigureEvent*event) {
  switch(ctx->sample_fmt) {
    case SAMPLE_FMT_U8    : event->af.format = AP_FORMAT_U8;     break;
    case SAMPLE_FMT_S16   : event->af.format = AP_FORMAT_S16;    break;
    case SAMPLE_FMT_S32   : event->af.format = AP_FORMAT_S32;    break;
    case SAMPLE_FMT_FLT   : event->af.format = AP_FORMAT_FLOAT;  break;
    default               : return false;                         break;
    }
  af=event->af;
  return true;
  }

FXbool AVDecoder::flush() {
  if (out) {
    out->unref();
    out=NULL;
    }
  return true;
  }

DecoderStatus AVDecoder::process(Packet*in) {
  AVPacket avp;

  fxmessage("decode packet %ld\n",in->size());

  FXASSERT(in);
  buffer.append(in->data(),in->size());
  buffer.padding(FF_INPUT_BUFFER_PADDING_SIZE+1);
  in->unref();


  av_init_packet(&avp);

  avp.data = buffer.data_ptr;
  avp.size = buffer.size()-(FF_INPUT_BUFFER_PADDING_SIZE+1);

  fxmessage("buffer %ld\n",buffer.size());


  int16_t * out_samples = reinterpret_cast<int16_t*>(outbuf.data());    /// output buffer
  int       out_size    = outbuf.capacity();                            /// output buffer size

  int result = avcodec_decode_audio3(ctx,out_samples,&out_size,&avp);
  fxmessage("decode_audio3: %d\n",result);

  if (result<0)
    return DecoderError;

  buffer.read(result+(FF_INPUT_BUFFER_PADDING_SIZE+1));

  if (out_size) {
    /// Get new buffer
    if (out==NULL) {
      out = engine->decoder->get_output_packet();
      if (out==NULL) return DecoderInterrupted; // FIXME
      out->af=af;
      }

//    fxmessage("got %d bytes %d %d\n",out_size,ctx->sample_rate,ctx->channels);
    out->append(outbuf.data(),out_size);
    if (out->availableFrames()==0) {
      engine->output->post(out);
      out=NULL;
      }
    }
  fxmessage("success\n");
  return DecoderOk;
  }


//InputPlugin * ap_aac_input(AudioEngine * engine) {
//  return new AacInput(engine);
//  }

DecoderPlugin * ap_avc_decoder(AudioEngine * engine) {
  return new AVDecoder(engine);
  }


}
