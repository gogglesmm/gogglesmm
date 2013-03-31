/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2012 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#include "ap_defs.h"
#include "ap_config.h"
#include "ap_utils.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_event.h"
#include "ap_buffer.h"
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
#include <ioctl.h>
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








OSSOutput::OSSOutput() : OutputPlugin(NULL), handle(BadHandle) {
  }

OSSOutput::~OSSOutput() {
  close();
  }

FXbool OSSOutput::setOutputConfig(const OutputConfig &c) {
  config=c.oss;
  return true;
  }


FXbool OSSOutput::open() {
  if (handle==BadHandle) {
    handle = ::open(config.device.text(),O_WRONLY);
    if (handle==BadHandle) {
      GM_DEBUG_PRINT("[oss] Unable to open device %s.\nError:%s\n",config.device.text(),strerror(errno));
      return false;
      }

/// OSS 3.9
#ifdef SNDCTL_DSP_COOKEDMODE
    /// Turn off automatic resampling.
    FXint enabled=(config.flags&OSSConfig::DeviceNoResample) ? 0 : 1;
    if (ioctl(fd,SNDCTL_DSP_COOKEDMODE,&enabled)==-1)
      GM_DEBUG_PRINT("[oss] unable to set cooked mode\n");
#endif

    GM_DEBUG_PRINT("[oss] opened device \"%s\"\n",config.device.text());
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
  FXint value=0;
  if (__likely(handle!=BadHandle)) {

    /// Delay in bytes
    if (ioctl(handle,SNDCTL_DSP_GETODELAY,&value)==-1)
      return 0;

    if (value<0)
      return 0;

    /// Return delay in number of frames
    return (value / af.framesize());
    }
  else {
    return 0;
    }
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
  if (p) {
#if defined(SNDCTL_DSP_SILENCE)
    ioctl(handle,SNDCTL_DSP_SILENCE,NULL);
#else
  #ifndef SNDCTL_DSP_SKIP
    ioctl(handle,SNDCTL_DSP_RESET,NULL);
  #else
    ioctl(handle,SNDCTL_DSP_SKIP,NULL);
  #endif
#endif
    }
  else {
#if defined(SNDCTL_DSP_SILENCE) && defined(SNDCTL_DSP_SKIP)
    ioctl(handle,SNDCTL_DSP_SKIP,NULL);
#endif
    }
  }



FXbool OSSOutput::configure(const AudioFormat & fmt){
  FXint format;

  if (handle && fmt==af)
    return true;

  /// To change format, it's safer to close and reopen the device
  if (handle) {
    close();
    }

  if (!open())
    return false;

  FXint num_channels=fmt.channels;
  FXint sample_rate=fmt.rate;

  if (!to_oss_format(fmt,format))
    goto failed;

  if (ioctl(handle,SNDCTL_DSP_SETFMT,&format)==-1)
    goto failed;

  if (ioctl(handle,SNDCTL_DSP_CHANNELS,&num_channels)==-1)
    goto failed;

  if (ioctl(handle,SNDCTL_DSP_SPEED,&sample_rate)==-1)
    goto failed;

  af=fmt;
  if (!to_gap_format(format,af))
    return false;

  af.channels=num_channels;
  af.rate=sample_rate;
  return true;
failed:
  GM_DEBUG_PRINT("[oss] Unsupported configuration:\n");
  af.debug();
  return false;
  }


FXbool OSSOutput::write(const void * buffer,FXuint nframes){
  FXival nwritten;
  FXival nbytes = nframes*af.framesize();
  const FXchar * buf = (const FXchar*)buffer;

  if (__unlikely(handle==BadHandle)) {
    GM_DEBUG_PRINT("[oss] device not opened\n");
    return false;
    }

  while(nbytes>0) {
    nwritten = ::write(handle,buf,nbytes);
    if (nwritten==-1) {
      if (errno==EAGAIN || errno==EINTR)
        continue;

      GM_DEBUG_PRINT("[oss] %s\n",strerror(errno));
      return false;
      }
    buf+=nwritten;
    nbytes-=nwritten;
    }
  return true;
  }

}
