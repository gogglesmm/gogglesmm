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
#ifndef READER_PLUGIN_H
#define READER_PLUGIN_H

namespace ap {

class Packet;
class InputPlugin;

enum ReadStatus {
  ReadError,    /* Error Occurred */
  ReadOk,       
  ReadDone,
//  ReadInterrupted,
  ReadRedirect
  };

class ReaderPlugin {
public:
  AudioEngine * engine;
  AudioFormat   af;
  InputPlugin * input;
protected:
  FXuchar flags;
  FXlong  stream_length;      /// Length of stream in samples
protected:
  enum {
    FLAG_PARSED = 0x1,
    };
public:
  ReaderPlugin(AudioEngine*);

  /// Init plugin
  virtual FXbool init(InputPlugin*);

  /// Format type
  virtual FXuchar format() const=0;

  /// Return redirect lsit
  virtual FXbool redirect(FXStringList&) { return false; }

  /// Return whether plugin can seek
  virtual FXbool can_seek() const { return false; }

  /// Seek to sample
  virtual FXbool seek(FXlong) { return false; }

  // Get the seek offset for given percentage (0-1.0)
  FXlong seek_offset(FXdouble) const;

  /// Process Input
  virtual ReadStatus process(Packet*);

  /// Destructor
  virtual ~ReaderPlugin();

  /// Open plugin for given format
  static ReaderPlugin* open(AudioEngine * engine,FXuint format);
  };




class TextReader : public ReaderPlugin {
protected:
  FXString textbuffer;
public:
  TextReader(AudioEngine*);
  FXbool init(InputPlugin*);
  ReadStatus process(Packet*);
  virtual ~TextReader();
  };


}
#endif
