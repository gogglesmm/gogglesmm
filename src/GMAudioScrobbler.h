/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*         Copyright (C) 2008-2015 by Sander Jansen. All Rights Reserved        *
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
#ifndef GMAUDIOSCROBBLER_H
#define GMAUDIOSCROBBLER_H

struct GMAudioScrobblerTrack {
  FXString artist;
  FXString album;
  FXString title;
  FXuint   duration = 0;
  FXint    no = 0;
  FXlong   timestamp = 0;
  FXint    loveban = 0;
  GMAudioScrobblerTrack(){}
  GMAudioScrobblerTrack(FXlong time,GMTrack & t,FXint lb) : artist(t.artist),album(t.album),title(t.title),duration(t.time),no(t.no),timestamp(time)/* LastFM */,loveban(lb)/* LastFM End */{}

  void load(FXStream & store);
  void save(FXStream & store) const;

  FXuint getTimeStamp() const { return (FXuint)(timestamp/1000000000); }

  void clear();
  };

typedef FXArray<GMAudioScrobblerTrack> GMAudioScrobblerTrackList;

enum {
  SERVICE_LASTFM,
  SERVICE_LIBREFM,
  SERVICE_CUSTOM
  };

class GMAudioScrobbler : public FXThread {
private:
  FXMutex           mutex_task;
  FXMutex           mutex_data;
  FXCondition       condition_task;
  FXushort          flags = FLAG_NONE;
  FXMessageChannel  feedback;
  FXObject*         target = nullptr;
  FXSelector        message = 0;
  FXbool            started = false;
private:
  FXuint            mode = SERVICE_LASTFM;
  FXString          handshake_url;
  FXString          nowplaying_url;
  FXString          submit_url;
  FXString          username;
  FXString          password;
  FXString          session;
  FXString          token;
protected:
  FXlong            timeout = 0;
private:
  enum {
    TASK_NONE         = 0x0,
    TASK_LOGIN 	      = 0x1,
    TASK_NOWPLAYING   = 0x2,
    TASK_SUBMIT 	    = 0x4,
    TASK_SHUTDOWN     = 0x8,
    TASK_AUTHENTICATE = 0x10,
    };
private:
  enum {
    FLAG_NONE          = 0,
    FLAG_LOGIN_CHANGED = 0x1,
    FLAG_BANNED        = 0x2,
    FLAG_BADAUTH       = 0x4,
    FLAG_BADTIME	   = 0x8,
    FLAG_SHUTDOWN      = 0x10,
    FLAG_TIMEOUT       = 0x20,
    FLAG_NETWORK       = 0x40,
    FLAG_DISABLED      = 0x80,
    FLAG_SERVICE       = 0x100
    };
protected:
  FXint run();
protected:
  FXbool post_request(const FXString & url,const FXString & request,FXString & response);
  FXbool get_request(const FXString & url,FXString & response);
protected:
  FXuchar getNextTask();
  FXbool  waitForTask();
  void    runTask();
  void    wakeup();
protected:
  GMAudioScrobblerTrack     nowplayingtrack;
  GMAudioScrobblerTrackList submitqueue;
  FXint nsubmitted;
  FXint nfailed;
protected:
  void authenticate();
  void handshake();
  void submit();
  void nowplaying();
  void loveban();
  void create_loveban_request(FXString &);
  void process_loveban_response(const FXString&);
  void create_token_request(FXString &);
  void process_token_response(const FXString&);
  FXuint create_handshake_request(FXString &);
  void process_handshake_response(const FXString&);
  void create_nowplaying_request(FXString &);
  void process_nowplaying_response(const FXString&);
  void create_submit_request(FXString &);
  void process_submit_response(const FXString&);
  void set_timeout();
  void reset_timeout();
  void set_submit_failed();
  void load_queue();
  void save_queue();
  FXbool can_submit();
public:
  GMAudioScrobbler(FXObject* tgt,FXSelector msg);
  FXString getUsername();
  FXbool hasPassword();
  FXbool isBanned();
  FXbool isEnabled();
  FXuint getService();

  void nowplaying(GMTrack & info);
  void loveban(GMTrack & info, FXint loveban);

  void service(FXuint s);
  void submit(FXlong timestamp,GMTrack & info);
  void login(const FXString & user,const FXString & pass);
  void shutdown();
  void nudge();
  void disable();
  void enable();

  virtual ~GMAudioScrobbler();
  };

extern FXbool init_gcrypt();


#endif


