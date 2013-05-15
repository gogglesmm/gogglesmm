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
#ifndef INPUT_H
#define INPUT_H

namespace ap {

class AudioEngine;
class ReaderPlugin;
class InputPlugin;



class InputThread : public EngineThread {
protected:
  InputPlugin*  input;
  ReaderPlugin* reader;
  FXString      url;
  FXuint        streamid;
  PacketPool    packetpool;
  FXbool        use_mmap;
protected:
//  FXIO * open_io(const FXString & uri);
//  FXIO * open_file(const FXString & uri);
//#ifdef HAVE_CDDA_PLUGIN
//  FXIO * open_cdda(const FXString & uri);
//#endif
  InputPlugin  * open_input(const FXString & uri);
  ReaderPlugin * open_reader();
protected:
  enum {
    StateIdle       = 0,
    StateProcessing = 1,
    StateError      = 2
    };
  FXuchar state;
  void set_state(FXuchar s,FXbool notify=false);
public:
  /// Constructor
  InputThread(AudioEngine*);
  virtual FXint run();
  virtual FXbool init();
  virtual void free();

  Event * wait_for_event();
  Event * wait_for_packet();
  Packet * get_packet();

  FXbool aborted();

  void ctrl_open_inputs(const FXStringList & url);
  void ctrl_open_input(const FXString & url);
  void ctrl_close_input(FXbool notify=false);
  void ctrl_flush(FXbool close=false);
  void ctrl_seek(FXdouble pos);
  void ctrl_eos();


  /// Destructor
  virtual ~InputThread();
  };
}
#endif
