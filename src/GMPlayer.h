/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2010 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMPLAYER
#define GMPLAYER

#ifdef HAVE_XINE_LIB

#ifndef SEL_PLAYER_OFFSET
#define SEL_PLAYER_OFFSET SEL_LAST
#endif


struct PlaybackTime {
  FXint position;
  FXint length;
  };


enum {
  SEL_PLAYER_EOS = SEL_PLAYER_OFFSET,
  SEL_PLAYER_BOS,
  SEL_PLAYER_TIME,
  SEL_PLAYER_STATE,
  SEL_PLAYER_VOLUME
  };

enum PlayerState {
  PLAYER_STOPPED,
  PLAYER_PLAYING,
  PLAYER_PAUSING
  };



class GMTrackDatabase;

enum {
  REPEAT_AB_OFF=0,
  REPEAT_AB_A,
  REPEAT_AB_B
  };


class GMEQBands {
public:
  FXdouble bands[10];
public:
  GMEQBands();
  GMEQBands(FXdouble e0,FXdouble e1,FXdouble e2,FXdouble e3,FXdouble e4,FXdouble e5,FXdouble e6,FXdouble e7,FXdouble e8,FXdouble e9);
  GMEQBands(const GMEQBands &);

  FXdouble& operator[](FXint i){ return bands[i]; }
  const FXdouble& operator[](FXint i) const { return bands[i]; }

  GMEQBands& operator=(const GMEQBands& src){
    for (FXint i=0;i<10;i++) bands[i]=src.bands[i];
    return *this;
    }

  FXbool operator==(const GMEQBands& v) const {
    for (FXint i=0;i<10;i++) if (v.bands[i]!=bands[i]) return false;
    return true;
    }

  FXbool operator!=(const GMEQBands& v) const {
    for (FXint i=0;i<10;i++) if (v.bands[i]!=bands[i]) return true;
    return false;
    }

  void unparse(FXString & preset) const;
  void parse(const FXString & preset) ;
  };

class GMEqualizer{
public:
  GMEQBands bands;
  FXdouble  preamp;
  FXbool    enabled;
public:
  GMEqualizer();
  GMEqualizer(const GMEQBands&);

  FXint to_xine(FXint i) const {
    /// Adapted from Amarok, kaffeine, xfmedia.
    /// Transform (-100 ... 100) -> (1 ... 200)
    /// The original code was without the ceilf which returned 0 for -100.
    /// return (FXint)ceilf(100.0f+((float)bands[i]*0.995f));

    /// I think we can go beyond the 200 limit....
    return (FXint)(pow(10.0,bands[i]/20.0)*100.0);
    }

  FXint to_xine_preamp() const {
    return (FXint)(pow(10.0,preamp/20.0)*100.0);
    }

  FXdouble preamp_scale() const {
    return (FXint)(pow(10.0,preamp/20.0)); //((FXdouble)(100+preamp))/100.0;
    }

  void load(FXSettings&);
  void save(FXSettings&) const;
  };


struct GMReplayGain {
  FXdouble gain;
  FXdouble peak;
  GMReplayGain() : gain(NAN),peak(NAN) {}
  };


typedef struct xine_s xine_t;
typedef struct xine_audio_port_s xine_audio_port_t;
typedef struct xine_stream_s xine_stream_t;
typedef struct xine_event_queue_s xine_event_queue_t;
typedef struct xine_post_s xine_post_t;

/// Interface to Xine
class GMPlayer : public FXObject {
FXDECLARE(GMPlayer)
private:
  FXObject        * target;
  FXSelector        message;
  FXMessageChannel* channel;
private:
  xine_t 			  			* xine;
  xine_audio_port_t 	* ao;
  xine_stream_t	  		* so;
  xine_event_queue_t	* queue;
  xine_post_t         * post_volume_normalize;
private:
  FXString          mrl;
  FXint position;		/// Position of Stream 0...65535
  FXint ctime;
  FXint ttime;
  FXint hours;		  /// Position -> hours
  FXint minutes;		/// Position -> minutes
  FXint seconds;		/// Position -> seconds
  FXint volume;     /// Volume Level
  FXint progress;   /// load progress
  FXint repeat_a;
  FXint repeat_b;
  FXString msg;		/// Message from Xine
  FXbool ignore_uimsg;
  FXbool debug;
  GMEqualizer  equalizer;
  GMReplayGain replaygain;
protected:
  FXbool setStringValue(const FXString & entry,const FXString & value);

  FXbool init_internal();
  void set_preamp();
  void check_xine_error();
protected:
  GMPlayer();
public:
  enum {
    ID_VOLUME,
    ID_MUTE,
    ID_UNMUTE,
    ID_TOGGLE_MUTE,
    ID_PREAMP,
    ID_EQ_30HZ,
    ID_EQ_60HZ,
    ID_EQ_125HZ,
    ID_EQ_250HZ,
    ID_EQ_500HZ,
    ID_EQ_1000HZ,
    ID_EQ_2000HZ,
    ID_EQ_4000HZ,
    ID_EQ_8000HZ,
    ID_EQ_16000HZ,
    };
public:
  long onCmdVolume(FXObject*,FXSelector,void*);
  long onUpdVolume(FXObject*,FXSelector,void*);
  long onCmdMute(FXObject*,FXSelector,void*);
  long onUpdMute(FXObject*,FXSelector,void*);
  long onCmdUnMute(FXObject*,FXSelector,void*);
  long onUpdUnMute(FXObject*,FXSelector,void*);
  long onCmdToggleMute(FXObject*,FXSelector,void*);
  long onUpdToggleMute(FXObject*,FXSelector,void*);
  long onCmdEqualizer(FXObject*,FXSelector,void*);
  long onUpdEqualizer(FXObject*,FXSelector,void*);
  long onCmdPreamp(FXObject*,FXSelector,void*);
  long onUpdPreamp(FXObject*,FXSelector,void*);
public:
  GMPlayer(FXApp * app,int argc,char** argv,FXObject * tgt=NULL,FXSelector sel=0);

  FXbool init();

  void exit();


  /// TNG: open
  void open(const FXString & mrl,FXbool flush);

  /// TNG: seek
  void seek(FXdouble pos);
  
  
  


  FXbool opened() const;


  /// Open a new stream
  FXbool open(const FXString & mrl);

  /// Play the current stream from position
  FXbool play(FXint pos=0);

  /// Seek to pos.
  //FXbool seek(FXint pos=0);

  FXbool seekable() const;

  void setRepeatAB();

  FXuint getRepeatAB() const;

  /// Close the current stream

  void close_device();


  FXbool changeDriver(const FXString & driver);
  FXint  getAvailableDrivers(FXString & drivers);
  void getCurrentDriver(FXString & driver);


  /// Stop the current stream
  void stop();

  /// Pause the current stream
  void pause();

  /// Continue playback
  void unpause();

  /// Are we currently pausing?
  FXbool pausing();

  /// Set the playback speed
  void setSpeed(FXint level);

  /// Get the playback speed
  FXint getSpeed() const;

  void incSpeed();

  void decSpeed();

  FXint remaining() const;

  /// Are we currently playing
  FXbool playing() const;

  /// Get the current position
  FXint getHours() 	const {return hours;}
  FXint getMinutes() 	const {return minutes;}
  FXint getSeconds() 	const {return seconds;}
  FXint getPosition() const {return position;}
  FXint getPositionMS() const { return ctime; }


  FXint getCurrentTime() const { return ctime; }
  FXint getTotalTime() const { return ttime; }
  FXint getRemainingTime() const { return ttime-ctime; }

  /// Set Audio Volume (0...100)
  void setVolume(FXint level);

  /// Get Audio Volume (0...100)
  FXint getVolume() const;

  /// Mute the Audio
  void mute();

  /// Unmute the Audiu
  void unmute();

  /// mute or not
  FXbool isMute() const;

  FXbool checkInitialized();

  FXbool hasVolumeNormalization() const;

  FXbool hasGapless() const;

  void setupGapless();

  void setVolumeNormalization(FXbool enable);

  FXbool getVolumeNormalization() { return post_volume_normalize!=NULL; }

  const char * getVersion() const;

  /// Update the player state.
  void handle_async_events();

  void getTrackInformation(GMTrack &);

  void getErrorMessage(FXString & errormsg);

  void disableEqualizer();

  void setEqualizer(const GMEqualizer &);

  void getEqualizer(GMEqualizer&);

  void setReplayGain(FXdouble gain,FXdouble peak);

  ~GMPlayer();
  };
#endif
#endif
