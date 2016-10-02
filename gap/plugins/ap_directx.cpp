/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2016-2016 by Sander Jansen. All Rights Reserved      *
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
#define GAP_PLUGIN 1

#include "ap_defs.h"
#include "ap_output_plugin.h"

#include "dsound.h"

using namespace ap;

namespace ap {

class DirectSoundOutput : public OutputPlugin {
protected:
  LPDIRECTSOUND8       handle = nullptr;
  LPDIRECTSOUNDBUFFER8 buffer = nullptr;

  DWORD offset = 0;
  DWORD buffersize = 0;



  FXFile file;
  FXlong data_pos;
public:
  DirectSoundOutput(Output * output);

  FXchar type() const { return DeviceDirectSound; }

  FXbool configure(const AudioFormat &);

  FXbool write(const void*, FXuint);

  FXint delay();

  void drop();
  void drain();
  void pause(FXbool) {}

  void close();

  virtual ~DirectSoundOutput();
  };

DirectSoundOutput::DirectSoundOutput(Output * out) : OutputPlugin(out){
  }

DirectSoundOutput::~DirectSoundOutput() {
  close();
  }


///FIXME perhaps support extensible wav format
FXbool DirectSoundOutput::configure(const AudioFormat & fmt) {
  
  if (handle==nullptr) {
    if (DirectSoundCreate8(NULL, &handle, nullptr) != DS_OK) {
      GM_DEBUG_PRINT("directx: failed to create directsound\n");
      return false;
      }

    if (handle->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY) != DS_OK) {
      GM_DEBUG_PRINT("directx: failed to set cooperativelevel\n");
      handle->Release();
      return false;
      }
    }

  DSBUFFERDESC buffer_description;
  WAVEFORMATEX waveformat;
  GM_DEBUG_PRINT("FRAMESIZE: %d",fmt.bps());

  buffersize = 2 * fmt.framesize() * fmt.rate;
  offset = 0;

  memset(&waveformat, 0, sizeof(WAVEFORMATEX)); 
  waveformat.nChannels = fmt.channels;
  waveformat.nSamplesPerSec = fmt.rate;
  waveformat.nBlockAlign = fmt.framesize();
  waveformat.wFormatTag = WAVE_FORMAT_PCM;
  waveformat.wBitsPerSample = fmt.bps();
  waveformat.nAvgBytesPerSec = waveformat.nBlockAlign * waveformat.nSamplesPerSec;

  memset(&buffer_description, 0, sizeof(DSBUFFERDESC)); 
  buffer_description.dwSize = sizeof(DSBUFFERDESC);
  buffer_description.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLFREQUENCY;
  buffer_description.dwReserved = 0;
  buffer_description.dwBufferBytes = buffersize;
  buffer_description.lpwfxFormat = &waveformat;
  buffer_description.guid3DAlgorithm = DS3DALG_DEFAULT;

  LPDIRECTSOUNDBUFFER sb;
  if (handle->CreateSoundBuffer(&buffer_description, &sb, nullptr) != DS_OK) {
    GM_DEBUG_PRINT("directx: failed to create buffer\n");
    handle->Release();
    handle = BadHandle;
    return false;
    }
  if (sb->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&buffer) != DS_OK) {
    sb->Release();
    handle->Release();
    return false;
    }
  sb->Release();

  af = fmt;

  return true;
  }


FXint DirectSoundOutput::delay() {
  if (buffer) {
    DWORD position = 0;
    buffer->GetCurrentPosition(&position, nullptr);

    if (offset < position) {
      return (offset + (buffersize - position)) / af.framesize();
      }
    else {
      return (offset - position) / af.framesize();
      }
    }
  return 0;
  }

void DirectSoundOutput::drop() {
  if (buffer) {
    buffer->Stop();
    buffer->SetCurrentPosition(0);
    offset = 0;
    }
  }

void DirectSoundOutput::drain() {
  if (buffer) {
    DWORD position = 0;
    buffer->GetCurrentPosition(&position, nullptr);

    FXlong left = 0;

    if (offset < position)
      left = offset + (buffersize - position);
    else
      left = offset - position;

    GM_DEBUG_PRINT("LEFT %ld\n",left);
    FXTime wait = ((left/af.framesize()) * 1000000000) / af.rate;
    FXThread::sleep(wait);


    buffer->Stop();
    buffer->SetCurrentPosition(0);
    offset = 0;
    }
  }


FXbool DirectSoundOutput::write(const void * data, FXuint nframes) {
  const FXuchar * src = reinterpret_cast<const FXuchar*>(data);
  if (buffer) {
    do {
      DWORD position = 0;
      FXuint space;
      FXuint len = nframes * af.framesize();

      buffer->GetCurrentPosition(&position, nullptr);
      //GM_DEBUG_PRINT("Position %d\n", position);

      if (offset >= position)
        space = (buffersize - offset) + position;
      else
        space = position - offset;

      if (space < len) {


        //GM_DEBUG_PRINT("wait for %ld ms\n", ((len - space) * 1000000) / af.rate);
        FXTime wait = ((len/af.framesize()) * 1000000000) / af.rate;
        FXThread::sleep(wait);
        continue;
        }

      FXuchar * wptr[2];
      DWORD     size[2];

      HRESULT hresult = buffer->Lock(offset, len, (LPVOID*)&wptr[0], &size[0], (LPVOID*)&wptr[1], &size[1],0);
      if (hresult == DSERR_BUFFERLOST) {

        if (buffer->Restore() != DS_OK)
          return false;
        
        continue;
        }

      FXASSERT(size[0] + size[1] >= len);

      size[0] = FXMIN(size[0], len);
      memcpy(wptr[0], src, size[0]);
      len -= size[0];
      src += size[0];
      offset += size[0];

      if (len && size[1]) {
        size[1] = FXMIN(size[1], len);
        memcpy(wptr[1], src, size[1]);
        len -= size[1];
        src += size[1];
        offset += size[1];
        }
      
      offset %= buffersize;

      buffer->Unlock(wptr[0], size[0], wptr[1], size[1]);
      FXASSERT(len == 0);

        if (buffer->Play(0, 0, DSBPLAY_LOOPING) != DS_OK)
          GM_DEBUG_PRINT("directx: failed to play\n");


      return true;
      }
    while (1);
    }
  return false;
  }



#if 0


  if (buffer) {
    FXuint len = nframes * af.framesize();

    if (offset + len >= buffersize) {
      if (buffer->Play(0, 0, DSBPLAY_LOOPING) != DS_OK)
        GM_DEBUG_PRINT("directx: failed to play\n");
      }


    while (len) {
      DWORD status;
      /*
      if (buffer->GetStatus(&status) == DS_OK && (status&DSBSTATUS_PLAYING)) {
        HANDLE h[3];

        h[0] = notify[0].hEventNotify;
        h[1] = notify[1].hEventNotify;
        h[2] = notify[2].hEventNotify;

        DWORD n = WaitForMultipleObjects(3, h, FALSE, INFINITE);
        GM_DEBUG_PRINT("directx: wait %d\n", n - WAIT_OBJECT_0);
        }
        */
      FXuchar * wrptr1, *wrptr2;
      DWORD wrsize1, wrsize2, nwritten1 = 0, nwritten2 = 0;
      HRESULT hresult;

      GM_DEBUG_PRINT("offset %d\n", offset);


      hresult = buffer->Lock(offset, len, (LPVOID*)&wrptr1, &wrsize1, (LPVOID*)&wrptr2, &wrsize2,0);
      if (hresult == DSERR_BUFFERLOST) {
        GM_DEBUG_PRINT("directx: buffer was lost\n");
        if (buffer->Restore() != DS_OK) {
          GM_DEBUG_PRINT("directx: failed to restore buffer\n");
          return false;
          }            
        hresult = buffer->Lock(offset, len, (LPVOID*)&wrptr1, &wrsize1, (LPVOID*)&wrptr2, &wrsize2, 0);
        }
      if (hresult != DS_OK) {
        GM_DEBUG_PRINT("directx: failed to lock buffer\n");
        return false;
        }
      GM_DEBUG_PRINT("buffer lock %p %d %p %d\n", wrptr1,wrsize1,wrptr2,wrsize2);
      nwritten1 = FXMIN(wrsize1, len);
      const FXuchar * ptr = (const FXuchar*)data;
      if (nwritten1) {
        memcpy(wrptr1, ptr, nwritten1);
        len -= nwritten1;
        ptr += nwritten1;
        offset = (offset + nwritten1) % buffersize;
        }
      if (len && wrptr2) {
        nwritten2 = FXMIN(wrsize2, len);
        memcpy(wrptr2, ptr, nwritten2);
        offset = (offset + nwritten2) % buffersize;
        len -= nwritten2;
        ptr += nwritten2;
        }
      GM_DEBUG_PRINT("buffer unlock %d %d\n", nwritten1, nwritten2);
      if (buffer->Unlock(wrptr1, nwritten1, wrptr2, nwritten2) != DS_OK) {
        GM_DEBUG_PRINT("directx: failed to unlock buffer\n");
        return false;
        }



      if (len) {
        GM_DEBUG_PRINT("directx: bytes left %d\n", len);


        HANDLE h[3];

        h[0] = notify[0].hEventNotify;
        h[1] = notify[1].hEventNotify;
        h[2] = notify[2].hEventNotify;

        DWORD n = WaitForMultipleObjects(2, h, FALSE, INFINITE);
        GM_DEBUG_PRINT("directx: wait %d\n", n - WAIT_OBJECT_0);
        }


      }

    }

    
  //GM_DEBUG_PRINT("Write Succeeded\n");
  return true;
  }

#endif
void DirectSoundOutput::close() {
  if (buffer) {
    buffer->Stop();
    buffer->Release();
    buffer = nullptr;
    }
  if (handle) {
    handle->Release();
    handle = nullptr;
    }
  af.reset();
  buffersize = offset = 0;
  }

}


AP_IMPLEMENT_PLUGIN(DirectSoundOutput);

