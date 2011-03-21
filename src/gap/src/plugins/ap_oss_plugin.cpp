#include "ap_defs.h"
#include "ap_config.h"
#include "ap_utils.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_event.h"
#include "ap_memory_buffer.h"
#include "ap_packet.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_input_plugin.h"
#include "ap_output_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_decoder_thread.h"

#include "ap_oss_plugin.h"
#include "ap_oss_defs.h"

/// For Open
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stropts.h>
#include <unistd.h>

using namespace ap;


extern "C" GMAPI OutputPlugin * ap_load_plugin() {
  return new OSSOutput();
  }

extern "C" GMAPI void ap_free_plugin(OutputPlugin* plugin) {
  delete plugin;
  }

namespace ap {


static FXbool to_gap_format(const FXint oss,AudioFormat & af) {
  switch(oss){
    case AFMT_S8          : af.format=AP_FORMAT_S8;     break;
    case AFMT_S16_LE      : af.format=AP_FORMAT_S16_LE; break;
    case AFMT_FLOAT       : af.format=AP_FORMAT_FLOAT;  break;
    case AFMT_S24_PACKED  : af.format=AP_FORMAT_S24_3;  break;
    case AFMT_S24_LE      : af.format=AP_FORMAT_S24_LE; break;
    default               : return false; break;
    }
  return true;
  }

static FXbool to_oss_format(const AudioFormat & af,FXint & oss_format){
  switch(af.format) {
    case AP_FORMAT_S8       : oss_format=AFMT_S8;     break;
    case AP_FORMAT_S16_LE   : oss_format=AFMT_S16_LE; break;
    case AP_FORMAT_S16_BE   : oss_format=AFMT_S16_BE; break;
    case AP_FORMAT_FLOAT_LE : oss_format=AFMT_FLOAT;  break;
    case AP_FORMAT_FLOAT_BE : oss_format=AFMT_FLOAT;  break;
    default                 : return false; break;
    }
  if (oss_format<0)
    return false;
  else
    return true;
  }








OSSOutput::OSSOutput() : OutputPlugin(), handle(BadHandle),
  device("/dev/dsp1"),  // "plug:front";
//  device("front"),
  use_hw_samplerate(false),
  use_mmap(true) {
  }

OSSOutput::~OSSOutput() {
  close();
  }


FXbool OSSOutput::open() {
  if (handle==BadHandle) {

    ap_get_device(device);

    handle = ::open(device.text(),O_WRONLY);
    if (handle==BadHandle) {
      fxmessage("Unable to open device %s.\nError:%s\n",device.text(),strerror(errno));
      return false;
      }
    }
  return true;
  }

void OSSOutput::close() {
  if (handle!=BadHandle) {
    drop();
    ::close(handle);
    handle=BadHandle;
    }
  af.reset();
  }

FXint OSSOutput::delay() {
  return 0;
  }


void OSSOutput::drop() {
  if (__likely(handle!=BadHandle)) {
#ifndef SNDCTL_DSP_SKIP
    ioctl(handle,SNDCTL_DSP_RESET,NULL);
#else
    ioctl(handle,SNDCTL_DSP_SKIP,NULL);
#endif
    }
  }

void OSSOutput::drain() {
  if (__likely(handle)) {
    ioctl(handle,SNDCTL_DSP_SYNC,NULL);
    }
  }

void OSSOutput::pause(FXbool p) {
  if (p) drain();
  }



FXbool OSSOutput::configure(const AudioFormat & fmt){
  FXint tmp,format;

  if (__unlikely(handle==BadHandle) && !open()) {
    return false;
    }

  if (handle && fmt==af)
    return true;

  FXint num_channels=fmt.channels;
  FXint sample_rate=fmt.rate;

  if (!to_oss_format(fmt,format))
    goto failed;

  if (ioctl(handle,SNDCTL_DSP_SETFMT,&format)==-1)
    goto failed;

  if (ioctl(handle,SNDCTL_DSP_CHANNELS,&num_channels)==-1)
    goto failed;

  if (ioctl(handle, SNDCTL_DSP_SPEED, &tmp)==-1)
    goto failed;

  af=fmt;
  if (!to_gap_format(format,af))
    return false;

  af.channels=num_channels;
  af.rate=sample_rate;
  return true;
failed:
  fxmessage("Unsupported oss configuration:\n");
  af.debug();
  return false;
  }


FXbool OSSOutput::write(const void * buffer,FXuint nframes){
  FXival nwritten;
  FXival nbytes = nframes*af.framesize();
  int result;
  const FXchar * buf = (const FXchar*)buffer;

  if (__unlikely(handle==BadHandle)) {
    fxmessage("device not opened\n");
    return false;
    }

  while(nbytes>0) {
    nwritten = ::write(handle,buf,nbytes);
    if (nwritten==-1) {
      if (errno==EAGAIN || errno==EINTR)
        continue;

      fxmessage("oss: %s\n",strerror(errno));
      return false;
      }
    buf+=nwritten;
    nbytes-=nwritten;
    }
  return true;
  }

}
