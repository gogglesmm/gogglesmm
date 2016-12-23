/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2017 by Sander Jansen. All Rights Reserved      *
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
#ifndef AP_EVENT_H
#define AP_EVENT_H

namespace ap {

enum EventType {
  AP_INVALID,
  AP_BOS,           // Event
  AP_EOS,
  AP_STATE_READY,
  AP_STATE_PLAYING,
  AP_STATE_PAUSING,
  AP_TIMESTAMP,     // TimeUpdate
  AP_ERROR,         // ErrorMessage
  AP_META_INFO,
  AP_VOLUME_NOTIFY,
  AP_LAST           // Reserved
  };

enum ReplayGainMode {
  ReplayGainOff     = 0,
  ReplayGainTrack   = 1,
  ReplayGainAlbum   = 2,
  };

class GMAPI Volume {
public:
  FXfloat value;
  FXuchar enabled;
public:
  Volume() : value(0.0f), enabled(false) {}
  Volume(FXfloat v) : value(v), enabled(true) {}
  };



class Event;

class GMAPI Event {
public:
  Event     * next;       /// used by pool and queue to   (8)
  FXuchar     type;       /// type of event               (1)
  FXuint      stream;     ///                             (4)
protected:
  Event();
protected:
  virtual ~Event();
public:
  Event(FXuchar t);
  virtual void unref();
  static void unref(Event*&);
  };

class GMAPI ErrorMessage : public Event {
public:
  FXString msg;
protected:
  virtual ~ErrorMessage();
public:
  ErrorMessage(const FXString & t=FXString::null);
  };

class GMAPI TimeUpdate : public Event {
public:
  FXuint position;
  FXuint length;
protected:
  virtual ~TimeUpdate();
public:
  TimeUpdate(FXuint p,FXuint l);
  };

class GMAPI MetaInfo : public Event {
public:
  FXString title;
  FXString artist;
  FXString album;
protected:
  virtual ~MetaInfo();
public:
  MetaInfo();
  };

class GMAPI VolumeNotify : public Event{
public:
  Volume volume;
protected:
  virtual ~VolumeNotify();
public:
  VolumeNotify();
  VolumeNotify(FXfloat v);
  };

}
#endif
