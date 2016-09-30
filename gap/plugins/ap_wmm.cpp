/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2016 by Sander Jansen. All Rights Reserved      *
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
#include "ap_output_plugin.h"


#include <windows.h>

//extern GMAPI FXString FX::FXSystem::getHostName();

using namespace ap;

namespace ap {


	struct Buffer {
		WAVEHDR header;
		HANDLE  event;
		FXuchar data[8192];
		FXuint  len;
		FXbool  playing;

		Buffer() : len(0),playing(false) {
			event = CreateEvent(NULL, false, false, NULL);
			header.lpData = (LPSTR) data;
			header.dwBufferLength = 0;
			header.dwUser = (DWORD_PTR)this;
		}

		void setData(const void * srcdata, FXuint length) {
			const FXuchar * src = reinterpret_cast<const FXuchar*>(srcdata);
			memcpy(data, src, length);
			header.dwBufferLength = length;
			header.lpData = (LPSTR)data;
			header.dwLoops = 0;
			header.dwFlags = 0;
			header.lpNext = NULL;
			header.dwBytesRecorded = 0;
			header.reserved = NULL;

		}
	};


	class WindowsMultimediaOutput : public OutputPlugin {
	protected:
		HWAVEOUT handle;
		Buffer * buffers;
		FXint    nbuffers;
		FXint    index;
		FXuint   nsamples;
		
		FXFile file;
		FXlong data_pos;

	protected:
		Buffer * next() {
			//GM_DEBUG_PRINT("Next Buffer %d\n", index);
			Buffer * buffer = &buffers[index];

			if (buffer->playing == false) {
				index = (index + 1) % nbuffers;
				return buffer;
			    }

			WaitForSingleObject(buffer->event, INFINITE);

			waveOutUnprepareHeader(handle,&buffer->header,sizeof(WAVEHDR));

			buffer->playing = false;
			index = (index + 1) % nbuffers;
			return buffer;
			}


	public:
		WindowsMultimediaOutput(Output * output);

		FXchar type() const { return DeviceWindowsMultimedia; }

		FXbool configure(const AudioFormat &);

		FXbool write(const void*, FXuint);

		void drop() {
			waveOutReset(handle);
			nsamples = 0;
		
		}
		void drain() {
			FXint idx = index;
			do {
				next();
			} while (idx != index);
			waveOutReset(handle);
			nsamples = 0;
		}

		FXint delay() {
			if (handle) {
				MMTIME time;
				time.wType = TIME_SAMPLES;
				if (waveOutGetPosition(handle, &time, sizeof(MMTIME)) == MMSYSERR_NOERROR) {
					if (time.wType == TIME_SAMPLES) {
						return FXMAX(0,nsamples - time.u.sample);
					}
					else
						return 0;
				}
			}
			return 0;
		}

		void pause(FXbool) {}

		void close();

		virtual ~WindowsMultimediaOutput();
	};

	WindowsMultimediaOutput::WindowsMultimediaOutput(Output * out) : OutputPlugin(out),handle(BadHandle), data_pos(0) {
		buffers = new Buffer[20];
		nbuffers = 20;
		index = 0;
		nsamples = 0;
	}

	WindowsMultimediaOutput::~WindowsMultimediaOutput() {
		close();
	}


	enum {
		WAV_FORMAT_PCM = 0x0001,
		WAV_FORMAT_FLOAT = 0x0003,
		//WAV_FORMAT_EXTENSIBLE = 0xFFFE
	};


	static void CALLBACK CallBack(HWAVEOUT handle, UINT message, DWORD data, DWORD p1, DWORD p2) {
			if (message == WOM_DONE) {
				//GM_DEBUG_PRINT("done with buffer\n");
				WAVEHDR * header = (WAVEHDR*)p1;
				Buffer * buffer = (Buffer*)header->dwUser;
				SetEvent(buffer->event);
			}
		}






	///FIXME perhaps support extensible wav format
	FXbool WindowsMultimediaOutput::configure(const AudioFormat & fmt) {

		
		WAVEFORMATEX waveformat;

		fmt.debug();

		waveformat.wFormatTag = WAVE_FORMAT_PCM;
		waveformat.nChannels = fmt.channels;
		waveformat.nSamplesPerSec = fmt.rate;
		waveformat.wBitsPerSample = fmt.bps();
		waveformat.nBlockAlign = 4;// fmt.framesize() / 8;
		waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec * waveformat.nBlockAlign;
		waveformat.cbSize = 0;// sizeof(WAVEFORMATEX);


		MMRESULT result;
		if ((result=waveOutOpen(&handle, WAVE_MAPPER, &waveformat,(DWORD)CallBack, NULL, CALLBACK_FUNCTION))==MMSYSERR_NOERROR) {
			GM_DEBUG_PRINT("Opened WAVE_MAPPER");
			af = fmt;
				return true;

		}
		else {
			GM_DEBUG_PRINT("Failed to open wmm device %d", fmt.framesize() / 8);

			switch(result){
			case MMSYSERR_ALLOCATED: GM_DEBUG_PRINT("error ALLOCATED\n"); break;
			case MMSYSERR_BADDEVICEID: GM_DEBUG_PRINT("error BADDEV\n"); break;
			case MMSYSERR_NODRIVER: GM_DEBUG_PRINT("error NODRIVER\n"); break;
			case MMSYSERR_NOMEM: GM_DEBUG_PRINT("error NOMEM\n"); break;
			case WAVERR_BADFORMAT: GM_DEBUG_PRINT("error BADFORMAT\n"); break;
			case WAVERR_SYNC: GM_DEBUG_PRINT("error BADSYNC\n"); break;
			}

				
				





		}



		return false;
	}


	FXbool WindowsMultimediaOutput::write(const void * data, FXuint nframes) {
		if (handle!=BadHandle) {

			Buffer * buffer = next();

			buffer->setData(data, af.framesize()*nframes);

			if (waveOutPrepareHeader(handle, &buffer->header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
				buffer->playing = true;
				//GM_DEBUG_PRINT("wmm: prepare header success\n");
				if (waveOutWrite(handle, &buffer->header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
					//GM_DEBUG_PRINT("wmm: write success\n");
					
					nsamples += nframes;
					return true;
				}
			}





		}


		return false;
	}

	void WindowsMultimediaOutput::close() {
		if (handle!=BadHandle)
			waveOutClose(handle);
		af.reset();
	}

}


AP_IMPLEMENT_PLUGIN(WindowsMultimediaOutput);

