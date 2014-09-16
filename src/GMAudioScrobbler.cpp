/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2014 by Sander Jansen. All Rights Reserved      *
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
#include "gmdefs.h"
#include "gmutils.h"
#include "GMApp.h"
#include "GMTrack.h"
#include "GMAudioScrobbler.h"
#include "GMAudioPlayer.h"

#ifdef HAVE_GCRYPT
#include "gcrypt.h"
#else
#include "md5.h"
#endif


/******************************************************************************
 *
 * D E F I N E S
 *
 ******************************************************************************/

#define MAX_HANDSHAKE_TIMEOUT 7200
#define MIN_HANDSHAKE_TIMEOUT 60
#define DISABLE_HANDSHAKE_TIMEOUT 0


/// Old Style
#define CLIENT_ID "gmm"
#define CLIENT_VERSION "0.1"

/// New Style
#define CLIENT_KEY "76525254135cc544d13d381514222c56"
#define CLIENT_SECRET "09397d5d6a55858a6883735b7cb694f7"

#define SCROBBLER_CACHE_FILE PATHSEPSTRING "scrobbler.cache"

#define LASTFM_URL "http://ws.audioscrobbler.com:80/2.0/"
#define LASTFM_OLD_URL "http://post.audioscrobbler.com:80"
#define LIBREFM_URL "http://turtle.libre.fm:80"

/// Error codes returned by last.fm
enum  {
  LASTFM_ERROR_UNKNOWN            = 0,
  LASTFM_ERROR_TOKEN_UNAUTHORIZED = 14,
  LASTFM_ERROR_TOKEN_EXPIRED      = 15,
  LASTFM_ERROR_BADSESSION         = 9,
  LASTFM_ERROR_OFFLINE            = 11,
  LASTFM_ERROR_UNAVAILABLE        = 16,
  LASTFM_ERROR_KEY_INVALID        = 10,
  LASTFM_ERROR_KEY_SUSPENDED      = 26
  };

/*
1 : This error does not exist
2 : Invalid service -This service does not exist
3 : Invalid Method - No method with that name in this package
4 : Authentication Failed - You do not have permissions to access the service
5 : Invalid format - This service doesn't exist in that format
6 : Invalid parameters - Your request is missing a required parameter
7 : Invalid resource specified
8 : Operation failed - Most likely the backend service failed. Please try again.
9 : Invalid session key - Please re-authenticate
10 : Invalid API key - You must be granted a valid key by last.fm
11 : Service Offline - This service is temporarily offline. Try again later.
12 : Subscribers Only - This station is only available to paid last.fm subscribers
13 : Invalid method signature supplied
14 : Unauthorized Token - This token has not been authorized
15 : This item is not available for streaming.
16 : The service is temporarily unavailable, please try again.
17 : Login: User requires to be logged in
18 : Trial Expired - This user has no free radio plays left. Subscription required.
19 : This error does not exist
20 : Not Enough Content - There is not enough content to play this station
21 : Not Enough Members - This group does not have enough members for radio
22 : Not Enough Fans - This artist does not have enough fans for for radio
23 : Not Enough Neighbours - There are not enough neighbours for radio
24 : No Peak Radio - This user is not allowed to listen to radio during peak usage
25 : Radio Not Found - Radio station not found
26 : API Key Suspended - This application is not allowed to make requests to the web services
27 : Deprecated - This type of request is no longer supported
*/



/******************************************************************************
 *
 * H E L P E R  F U N C T I O N S
 *
 ******************************************************************************/

FXbool init_gcrypt() {
#ifdef HAVE_GCRYPT
  if (!gcry_check_version(GCRYPT_VERSION)) {
    fxwarning("libgcrypt version mismatch");
    return false;
    }
  gcry_control(GCRYCTL_DISABLE_SECMEM,0);
  gcry_control(GCRYCTL_INITIALIZATION_FINISHED,0);
#endif
  return true;
  }

static void checksum(FXString & io){
  if (io.empty()) return;
#ifdef HAVE_GCRYPT
  FXuchar digest[16];
  gcry_md_hash_buffer(GCRY_MD_MD5,(void*)digest,(const void*)io.text(),io.length());
#else
  md5_state_t pms;
  md5_byte_t digest[16];
  md5_init(&pms);
  md5_append(&pms,(const md5_byte_t*)io.text(),io.length());
  md5_finish(&pms,digest);
#endif

  io.length(32);
  for (FXint i=0,d=0;i<32;i+=2,d++) {
    io[i]=Ascii::toLower(FXString::value2Digit[(digest[d]/16)%16]);
    io[i+1]=Ascii::toLower(FXString::value2Digit[digest[d]%16]);
    }
  }


#define URL_UNSAFE   "#$-_.+!*'><()\\,%\""          // Always Encode
#define URL_RESERVED ";/?:@=&"              // Only encode if not used as reserved by scheme


/**********************************************************************************************************/


void GMAudioScrobblerTrack::clear() {
  artist.clear();
  album.clear();
  title.clear();
  duration=0;
  no=0;
  timestamp=0;
  loveban=0;
  }

void GMAudioScrobblerTrack::save(FXStream & store) const {
  store << artist;
  store << album;
  store << title;
  store << duration;
  store << no;
  store << timestamp;
  store << loveban;
  }

void GMAudioScrobblerTrack::load(FXStream & store) {
  store >> artist;
  store >> album;
  store >> title;
  store >> duration;
  store >> no;
  store >> timestamp;
  store >> loveban;
  }


class ServiceResponse : public XmlParser {
protected:
  FXbool       status;
  FXint        code;
  FXString     message;
  FXString     key;
  FXString     token;
protected:
  FXint begin(const FXchar *,const FXchar**);
  void data(const FXchar *,FXint len);
  void end(const FXchar *);
  void parse_lfm_atts(const FXchar **);
  void parse_lfm_error_atts(const FXchar **);
public:
  enum {
    Elem_LastFM = Elem_Last,
    Elem_LastFM_Error,
    Elem_LastFM_Token,
    Elem_LastFM_Session,
    Elem_LastFM_Session_Key,
    };
public:
  ServiceResponse();
  ~ServiceResponse();

  FXbool getStatus() const { return status; }
  FXint  getErrorCode() const { return code; }
  const FXString & getErrorMessage() const { return message; }
  const FXString & getSessionKey() const { return key; }
  const FXString & getToken() const { return token; }
  };


ServiceResponse::ServiceResponse() : status(false), code(LASTFM_ERROR_UNKNOWN) {
   }

ServiceResponse::~ServiceResponse() {
  }


void ServiceResponse::parse_lfm_atts(const FXchar ** attributes) {
  status=false;
  for (FXint i=0;attributes[i];i+=2){
    if (compare(attributes[i],"status")==0) {
      if (comparecase(attributes[i+1],"ok")==0){
        status=true;
        }
      }
    }
  }


void ServiceResponse::parse_lfm_error_atts(const FXchar ** attributes) {
  FXString cd;
  for (FXint i=0;attributes[i];i+=2){
    if (compare(attributes[i],"code")==0) {
      cd=attributes[i+1];
      code=cd.toInt();
      }
    }
  }


FXint ServiceResponse::begin(const FXchar * element,const FXchar ** attributes){
  switch(node()) {
    case Elem_None:
      {
        if (compare(element,"lfm")==0) {
          parse_lfm_atts(attributes);
          return Elem_LastFM;
          }
      } break;
    case Elem_LastFM:
      {
        if (compare(element,"error")==0) {
          parse_lfm_error_atts(attributes);
          return Elem_LastFM_Error;
          }
        else if (compare(element,"session")==0) {
          return Elem_LastFM_Session;
          }
        else if (compare(element,"token")==0) {
          return Elem_LastFM_Token;
          }
      } break;
    case Elem_LastFM_Session:
      {
        if (compare(element,"key")==0) {
          return Elem_LastFM_Session_Key;
          }
      } break;
    default: return 0; // skip
    }
  return 0;
  }


void ServiceResponse::data(const FXchar* d,FXint len){
  switch(node()) {
    case Elem_LastFM_Error: message.assign(d,len); break;
    case Elem_LastFM_Session_Key: key.assign(d,len); break;
    case Elem_LastFM_Token: token.assign(d,len); break;
    }
  }

void ServiceResponse::end(const FXchar*) {
  }



FXbool GMAudioScrobbler::post_request(const FXString & url,const FXString & msg,FXString & output) {
  FXTRACE((60,"post_request %s - %s\n",url.text(),msg.text()));
  HttpClient client;
  if (client.basic("POST",url,
                   "Content-Type: application/x-www-form-urlencoded\r\n"
                   "Connection: close\r\n",
                   msg)) {
    output = client.textBody();
    return true;
    }
  else {
    FXTRACE((60,"post_request failed\n"));
    set_timeout();
    return false;
    }
  }

FXbool GMAudioScrobbler::get_request(const FXString & url,FXString & output) {
  FXTRACE((60,"get_request\n"));
  HttpClient client;
  if (client.basic("GET",url,"Connection: close\r\n")) {
    output = client.textBody();
    return true;
    }
  else {
    FXTRACE((60,"get_request failed\n"));
    set_timeout();
    return false;
    }
  }




/******************************************************************************
 *
 * C O N S T R U C T O R
 *
 ******************************************************************************/


GMAudioScrobbler::GMAudioScrobbler(FXObject* tgt,FXSelector msg) :
  flags(FLAG_NONE),
  feedback(FXApp::instance()),
  target(tgt),
  message(msg),
  started(false),
  mode(SERVICE_LASTFM),
  timeout(DISABLE_HANDSHAKE_TIMEOUT),
  nsubmitted(0),
  nfailed(0) {

  username=FXApp::instance()->reg().readStringEntry("LastFM","username",NULL);
  password=FXApp::instance()->reg().readStringEntry("LastFM","password",NULL);

  FXString uri = FXApp::instance()->reg().readStringEntry("LastFM","handshake-url",LASTFM_URL);
  if (uri==LASTFM_OLD_URL) {
    uri=LASTFM_URL;
    username.clear();
    password.clear();
    FXApp::instance()->reg().writeBoolEntry("LastFM","client-banned",false);
    }

  handshake_url = uri;
  mode=getService();

  /// Get last session
  if (mode==SERVICE_LASTFM) {
    session=FXApp::instance()->reg().readStringEntry("LastFM","session",NULL);
    if (!session.empty()) {
      FXTRACE((60,"GMAudioScrobbler::GMAudioScrobbler - Session: %s\n",session.text()));
      nowplaying_url=handshake_url;
      submit_url=handshake_url;
      }
    }

  /// Check if we're banned
  if (compare(FXApp::instance()->reg().readStringEntry("LastFM","client-id",CLIENT_ID),CLIENT_ID)==0 &&
      compare(FXApp::instance()->reg().readStringEntry("LastFM","client-version",CLIENT_VERSION),CLIENT_VERSION)==0){
    if (FXApp::instance()->reg().readBoolEntry("LastFM","client-banned",false))
      flags|=FLAG_BANNED;
    }
  else { /// using different client, reset banned flag
    FXApp::instance()->reg().writeBoolEntry("LastFM","client-banned",false);
    }

  /// Check if we should scrobble tracks or not.
  if (!FXApp::instance()->reg().readBoolEntry("LastFM","scrobble-tracks",false))
    flags|=FLAG_DISABLED;

  FXApp::instance()->reg().writeStringEntry("LastFM","client-id",CLIENT_ID);
  FXApp::instance()->reg().writeStringEntry("LastFM","client-version",CLIENT_VERSION);
  load_queue();


  }

GMAudioScrobbler::~GMAudioScrobbler(){
  save_queue();
  }


/******************************************************************************
 *
 *  P U B L I C  A P I
 *
 ******************************************************************************/

FXuint GMAudioScrobbler::getService() {
  FXString host = FXURL::host(handshake_url);
  if (host=="ws.audioscrobbler.com")
    return SERVICE_LASTFM;
  else if (host=="turtle.libre.fm")
    return SERVICE_LIBREFM;
  else
    return SERVICE_CUSTOM;
  }

void GMAudioScrobbler::service(FXuint s) {
  FXTRACE((60,"GMAudioScrobbler::service\n"));
  if (s==SERVICE_LASTFM || s==SERVICE_LIBREFM) {

    mutex_data.lock();

    if (s==SERVICE_LASTFM)
      FXApp::instance()->reg().writeStringEntry("LastFM","handshake-url",LASTFM_URL);
    else
      FXApp::instance()->reg().writeStringEntry("LastFM","handshake-url",LIBREFM_URL);

    handshake_url=FXApp::instance()->reg().readStringEntry("LastFM","handshake-url",LASTFM_URL);
    nowplaying_url.clear();
    submit_url.clear();
    session.clear();
    token.clear();

    flags|=FLAG_SERVICE;
    flags&=~FLAG_BADAUTH|FLAG_TIMEOUT|FLAG_BANNED|FLAG_BADTIME;

    username.clear();
    password.clear();
    FXApp::instance()->reg().writeStringEntry("LastFM","username",username.text());
    FXApp::instance()->reg().writeStringEntry("LastFM","password",password.text());
    FXApp::instance()->reg().writeStringEntry("LastFM","session","");
    FXApp::instance()->reg().writeBoolEntry("LastFM","client-banned",false);

    mode=s;

    reset_timeout();
    mutex_data.unlock();

    wakeup();
    }
  }




void GMAudioScrobbler::login(const FXString & user,const FXString & pass) {
  FXTRACE((60,"GMAudioScrobbler::login\n"));
  mutex_data.lock();
  flags&=~FLAG_DISABLED;
  if ( (flags&FLAG_BANNED) || (flags&FLAG_BADTIME) ) {
    mutex_data.unlock();
    shutdown();
    }
  else {
    if (mode==SERVICE_LIBREFM) {
      FXString newpass=pass;
      checksum(newpass);
      if (user!=username || newpass!=password) {
        username=user;
        password=newpass;
        FXApp::instance()->reg().writeStringEntry("LastFM","username",username.text());
        FXApp::instance()->reg().writeStringEntry("LastFM","password",password.text());
        flags|=FLAG_LOGIN_CHANGED;
        flags&=~FLAG_BADAUTH;
        mutex_data.unlock();
        if (username.empty() || password.empty())
          shutdown();
        else
          runTask();
        }
      else {
        mutex_data.unlock();
        }
      }
    else {
      flags|=FLAG_LOGIN_CHANGED;
      flags&=~FLAG_BADAUTH;
      mutex_data.unlock();
      runTask();
      }
    }
  }

void GMAudioScrobbler::nudge(){
  FXTRACE((60,"GMAudioScrobbler::nudge\n"));
  if (started) {

    mutex_data.lock();
    flags|=FLAG_NETWORK;
    mutex_data.unlock();

    wakeup();
    }
  }


FXbool GMAudioScrobbler::can_submit() {
  if (getService()==SERVICE_LASTFM)
    return !( (flags&FLAG_DISABLED) || (flags&FLAG_BANNED) || (flags&FLAG_BADAUTH) || (flags&FLAG_BADTIME));
  else
    return !( (flags&FLAG_DISABLED) || (flags&FLAG_BANNED) || (flags&FLAG_BADAUTH) || (flags&FLAG_BADTIME) || username.empty() || password.empty() );
  }

void GMAudioScrobbler::nowplaying(GMTrack & info){
  mutex_data.lock();
  if (!can_submit()) {
    mutex_data.unlock();
    shutdown();
    }
  else {
    nowplayingtrack=GMAudioScrobblerTrack(1,info,0);
    mutex_data.unlock();
    runTask();
    }
  }

void GMAudioScrobbler::submit(FXlong timestamp,GMTrack & info){
  if (info.time<30)
    return;

  mutex_data.lock();
  if (!can_submit()) {
    mutex_data.unlock();
    shutdown();
    }
  else {
    submitqueue.append(GMAudioScrobblerTrack(timestamp,info,0));
    mutex_data.unlock();
    runTask();
    }
  }

void GMAudioScrobbler::loveban(GMTrack & /*info*/, FXint /*loveban*/){
#if 0
  mutex_data.lock();
  if (!can_submit()) {
    mutex_data.unlock();
    shutdown();
    }
  else {
    submitqueue.append(GMAudioScrobblerTrack(0,info,loveban));
    mutex_data.unlock();
    runTask();
    }
#endif
  return;
}

void GMAudioScrobbler::wakeup(){
  mutex_task.lock();
  condition_task.signal();
  mutex_task.unlock();
  }

void GMAudioScrobbler::runTask() {
  if (!started) {
    start();
    started=true;
    }
  else {
    wakeup();
    }
  }


void GMAudioScrobbler::shutdown(){
  if (started) {
    mutex_data.lock();
    flags|=FLAG_SHUTDOWN;
    mutex_data.unlock();
    wakeup();
    join();
    started=false;
    }

  /// Save Session
  FXApp::instance()->reg().writeStringEntry("LastFM","session",session.text());
  FXApp::instance()->reg().writeBoolEntry("LastFM","client-banned",(flags&FLAG_BANNED));

  }

void GMAudioScrobbler::disable(){
  if (started) {
    mutex_data.lock();
    flags|=FLAG_SHUTDOWN|FLAG_DISABLED;
    mutex_data.unlock();
    wakeup();
    join();
    started=false;
    }
  FXApp::instance()->reg().writeBoolEntry("LastFM","scrobble-tracks",false);
  }

void GMAudioScrobbler::enable() {
  mutex_data.lock();
  flags&=~FLAG_DISABLED;
  flags&=~FLAG_SHUTDOWN;
  mutex_data.unlock();
  FXApp::instance()->reg().writeBoolEntry("LastFM","scrobble-tracks",true);
  }


FXString GMAudioScrobbler::getUsername() {
  FXMutexLock lock(mutex_data);
  return username;
  }

FXbool GMAudioScrobbler::hasPassword(){
  FXMutexLock lock(mutex_data);
  return !password.empty();
  }


FXbool GMAudioScrobbler::isBanned(){
  FXMutexLock lock(mutex_data);
  return (flags&FLAG_BANNED);
  }

FXbool GMAudioScrobbler::isEnabled(){
  FXMutexLock lock(mutex_data);
  return !(flags&FLAG_DISABLED);
  }

/******************************************************************************
 *
 *  P R O T E C T E D  A P I
 *
 ******************************************************************************/


void GMAudioScrobbler::load_queue(){
  FXTRACE((60,"GMAudioScrobbler::load_queue\n"));
  FXuint version,size;
  FXString filename = GMApp::getCacheDirectory() + SCROBBLER_CACHE_FILE;
  FXFileStream store;
  if (store.open(filename,FXStreamLoad)){
    store >> version;
    if (version==20080501) {
      store >> size;
      submitqueue.no(size);
      for (FXint i=0;i<submitqueue.no();i++){
        submitqueue[i].load(store);
        }
      }
    store.close();
    }
  FXFile::remove(filename);
  }


void GMAudioScrobbler::save_queue(){
  FXTRACE((60,"GMAudioScrobbler::save_queue => %ld entries\n",submitqueue.no()));
  FXuint version=20080501,size;
  if (submitqueue.no()) {
    FXString filename = GMApp::getCacheDirectory(true) + SCROBBLER_CACHE_FILE;
    FXFileStream store;
    if (store.open(filename,FXStreamSave)){
      store << version;
      size = submitqueue.no();
      store << size;
      for (FXint i=0;i<submitqueue.no();i++)
        submitqueue[i].save(store);
      }
    }
  else {
    FXString filename = GMApp::getCacheDirectory() + SCROBBLER_CACHE_FILE;
    FXFile::remove(filename);
    }
  }

FXbool GMAudioScrobbler::waitForTask() {
  FXTRACE((60,"GMAudioScrobbler::waitForTask\n"));
  if (timeout>0) {
    FXlong wakeuptime = ((FXlong)timeout*1000000000LL); // relative time
    FXlong starttime  = FXThread::time();
    while(1) {
      FXTRACE((60,"GMAudioScrobbler::waitForTask => %lld\n",wakeuptime/1000000000LL));
      mutex_task.lock();
      if (!condition_task.wait(mutex_task,wakeuptime)){
        mutex_task.unlock();
        return true;
        }
      else  {
        mutex_task.unlock();
        FXMutexLock lock(mutex_data);

        if (flags&FLAG_SHUTDOWN)
          return false;

        if (flags&FLAG_NETWORK) {
          flags&=~FLAG_NETWORK;
          return true;
          }

        FXlong now  = FXThread::time();
        wakeuptime -= (now - starttime);
        starttime   = now;

        /// Done waiting
        if (wakeuptime<=0)
          return true;
        FXTRACE((60,"GMAudioScrobbler::waitForTask => reset\n"));
        }
      }
    }
  else {
    mutex_task.lock();
    condition_task.wait(mutex_task);
    mutex_task.unlock();
    }
  return true;
  }

FXuchar GMAudioScrobbler::getNextTask() {
  FXTRACE((60,"GMAudioScrobbler::getNextTask\n"));
  FXMutexLock lock(mutex_data);

  if (flags&FLAG_SHUTDOWN){
    flags&=~FLAG_SHUTDOWN;
    return TASK_SHUTDOWN;
    }

  if (flags&FLAG_SERVICE) {
    session.clear();
    flags&=~FLAG_SERVICE;
    return TASK_NONE;
    }

  if (flags&FLAG_BADAUTH)
    return TASK_NONE;

  if (flags&FLAG_TIMEOUT) {
    flags&=~FLAG_TIMEOUT;
    return TASK_NONE;
    }

  if (flags&FLAG_LOGIN_CHANGED) {
    session.clear();
    if (getService()==SERVICE_LASTFM) {
      if (!token.empty())
        return TASK_LOGIN;
      else
        return TASK_AUTHENTICATE;
      }
    else {
      return TASK_LOGIN;
      }
    }

  if (submitqueue.no() || nowplayingtrack.timestamp) {
    if (session.empty()) {
      if (getService()==SERVICE_LASTFM) {
        if (!token.empty())
          return TASK_LOGIN;
        else
          return TASK_AUTHENTICATE;
        }
      else {
        return TASK_LOGIN;
        }
      }
    else if (submitqueue.no()) return TASK_SUBMIT;
    else return TASK_NOWPLAYING;
    }

  return TASK_NONE;
  }



FXint GMAudioScrobbler::run() {
  FXTRACE((60,"GMAudioScrobbler::run\n"));

  ap_set_thread_name("gm_scrobbler");

  FXuchar next=TASK_NONE;
  do {
    while((next=getNextTask())!=TASK_NONE) {
      switch(next){
        case TASK_AUTHENTICATE : authenticate(); break;
        case TASK_LOGIN        : handshake();    break;
        case TASK_SUBMIT       : submit();       break;
        case TASK_NOWPLAYING   : nowplaying();   break;
        case TASK_SHUTDOWN     : goto done;      break;
        }
      }
    }	while(waitForTask());
done:

  mutex_data.lock();
  flags&=~FLAG_SHUTDOWN;
  flags&=~FLAG_LOGIN_CHANGED;
  flags&=~FLAG_TIMEOUT;
  mutex_data.unlock();

  FXTRACE((60,"GMAudioScrobbler::run -> shutdown\n"));
  return 0;
  }



void GMAudioScrobbler::set_timeout(){
  flags|=FLAG_TIMEOUT;
  if (timeout==0)
    timeout=MIN_HANDSHAKE_TIMEOUT;
  else
    timeout=FXMIN(MAX_HANDSHAKE_TIMEOUT,timeout<<1);
  }

void GMAudioScrobbler::reset_timeout(){
  timeout=DISABLE_HANDSHAKE_TIMEOUT;
  }


void GMAudioScrobbler::set_submit_failed() {
  FXTRACE((60,"GMAudioScrobbler::set_failed\n"));
  nfailed++;
  if (nfailed==3) {
    session.clear();
    nfailed=0;
    }
  }





void GMAudioScrobbler::create_token_request(FXString & request) {
  FXMutexLock lock(mutex_data);
  FXTRACE((60,"GMAudioScrobbler::create_token_request\n"));
  FXString signature="api_key" CLIENT_KEY "methodauth.getToken" CLIENT_SECRET;
  checksum(signature);
  request=FXString::value("method=auth.getToken&api_key=" CLIENT_KEY "&api_sig=%s",signature.text());
  flags&=~(FLAG_LOGIN_CHANGED);
  }


void GMAudioScrobbler::process_token_response(const FXString & response){
  FXMutexLock lock(mutex_data);
  if (flags&FLAG_LOGIN_CHANGED) return;
  ServiceResponse sr;
  FXTRACE((60,"GMAudioScrobbler::process_token_response\n%s\n",response.text()));
  if (sr.parse(response) && sr.getStatus()) {
    token=sr.getToken();
    FXTRACE((60,"GMAudioScrobbler::process_token_response => token=%s\n",token.text()));
    FXString url="http://www.last.fm/api/auth/?api_key=" CLIENT_KEY "&token="+token;
    feedback.message(target,FXSEL(SEL_OPENED,message),url.text(),url.length());
    reset_timeout(); /// Reset timer
    set_timeout();   /// Let's wait at least 60s
    }
  else {
    FXTRACE((60,"last.fm service failed with code %d: %s\n",sr.getErrorCode(),sr.getErrorMessage().text()));
    flags|=FLAG_BADAUTH;
    }
  }



FXuint GMAudioScrobbler::create_handshake_request(FXString & request) {
  FXMutexLock lock(mutex_data);
  FXTRACE((60,"GMAudioScrobbler::create_handshake_request\n"));
  if (mode==SERVICE_LASTFM) {
    FXString signature=FXString::value("api_key%smethodauth.getSessiontoken%s%s",CLIENT_KEY,token.text(),CLIENT_SECRET);
    checksum(signature);
    request=FXString::value("method=auth.getSession&api_key=%s&api_sig=%s&token=%s",CLIENT_KEY,signature.text(),token.text());
    }
  else {
    FXlong timestamp = FXThread::time()/1000000000;
    FXString timestamp_text = FXString::value(timestamp);
    FXString tk = password + timestamp_text;
    checksum(tk);
    request=FXString::value("/?hs=true&p=1.2&c=" CLIENT_ID "&v=" CLIENT_VERSION "&u=%s&t=%s&a=%s",username.text(),timestamp_text.text(),tk.text());
    }
  flags&=~(FLAG_LOGIN_CHANGED);
  return mode;
  }




void GMAudioScrobbler::process_handshake_response(const FXString & response){
  FXMutexLock lock(mutex_data);
  if (flags&FLAG_LOGIN_CHANGED) return;
  FXTRACE((60,"GMAudioScrobbler::process_handshake_response\n%s\n",response.text()));
  if (mode==SERVICE_LASTFM) {
    ServiceResponse sr;
    if (!sr.parse(response) || !sr.getStatus()){
      FXTRACE((60,"last.fm service failed with code %d: %s\n",sr.getErrorCode(),sr.getErrorMessage().text()));
      switch(sr.getErrorCode()) {
        case LASTFM_ERROR_TOKEN_EXPIRED     : token.clear();        break;
        case LASTFM_ERROR_TOKEN_UNAUTHORIZED:
        case LASTFM_ERROR_OFFLINE           :
        case LASTFM_ERROR_UNAVAILABLE       : set_timeout();        break;
        default                             : flags|=FLAG_BADAUTH;  break;
        }
      }
    else {
      session=sr.getSessionKey();
      nowplaying_url=handshake_url;
      submit_url=handshake_url;
      flags&=~FLAG_BADAUTH;
      reset_timeout();
      }
    }
  else {
    FXString code;
    code=response.section('\n',0);
    if (compare(code,"OK",2)==0) {
      session = response.section('\n',1);
      nowplaying_url=response.section('\n',2);
      submit_url=response.section('\n',3);
      flags&=~FLAG_BADAUTH;
      reset_timeout();
      }
    else if (compare(code,"BANNED",6)==0){
      FXTRACE((60,"\t=> BANNED\n"));
      const FXchar msg[] = "This version of Goggles Music Manager is not supported\nby scrobbler service. Please upgrade to a newer version of GMM.";
      feedback.message(target,FXSEL(SEL_COMMAND,message),msg,ARRAYNUMBER(msg));
      flags|=FLAG_BANNED;
      //FXApp::instance()->reg().writeBoolEntry("LastFM","client-banned",true);
      }
    else if (compare(code,"BADTIME",7)==0){
      FXTRACE((60,"\t=> BADTIME\n"));
      const FXchar msg[] = "Unable submit tracks scrobbler service. The system time doesn't match\n"
                           "the scrobbler server time. Please adjust your system time\n"
                           "and restart GMM to start scrobbling.";
      feedback.message(target,FXSEL(SEL_COMMAND,message),msg,ARRAYNUMBER(msg));
      flags|=FLAG_BADTIME;
      }
    else if (compare(code,"BADAUTH",7)==0){
      FXTRACE((60,"\t=> BADAUTH\n"));
      const FXchar msg[] = "Unable to login to scrobbler service.\nUsername and password do not match.";
      feedback.message(target,FXSEL(SEL_COMMAND,message),msg,ARRAYNUMBER(msg));
      flags|=FLAG_BADAUTH;
      }
    else if (compare(code,"FAILED",6)==0){
      FXTRACE((60,"\t=> FAILED\n"));
      set_timeout();
      }
    else {
      FXTRACE((60,"\t=> Unknown\n"));
      FXTRACE((60,"%s\n",response.text()));
      set_timeout();
      }
    }
  }



void GMAudioScrobbler::authenticate() {
  FXString request;
  FXString response;
  create_token_request(request);
  if (post_request(handshake_url,request,response))
    process_token_response(response);
  }

void GMAudioScrobbler::handshake() {
  FXString request;
  FXString response;
  FXbool result;

  if (create_handshake_request(request)==SERVICE_LASTFM)
    result = post_request(handshake_url,request,response);
  else
    result = get_request(handshake_url+request,response);

  if (result)
    process_handshake_response(response);
  }

void GMAudioScrobbler::nowplaying() {
  FXString request;
  FXString response;
  create_nowplaying_request(request);
  if (post_request(nowplaying_url,request,response))
    process_nowplaying_response(response);
  }

void GMAudioScrobbler::submit() {
  FXString request;
  FXString response;
  create_submit_request(request);
  if (post_request(submit_url,request,response))
    process_submit_response(response);
  }

void GMAudioScrobbler::loveban() {
  FXString request;
  FXString response;
  create_loveban_request(request);
  if (post_request(submit_url,request,response))
    process_loveban_response(response);
  }







void GMAudioScrobbler::create_nowplaying_request(FXString & request) {
  FXMutexLock lock(mutex_data);
  FXTRACE((60,"GMAudioScrobbler::create_nowplaying_request\n"));
  if (mode==SERVICE_LASTFM) {
    FXString signature=FXString::value("album%s"
                                      "api_key" CLIENT_KEY
                                      "artist%s"
                                      "duration%d"
                                      "methodtrack.updateNowPlaying"
                                      "sk%s"
                                      "track%s"
                                      "trackNumber%d"
                                      CLIENT_SECRET,
                                      nowplayingtrack.album.text(),
                                      nowplayingtrack.artist.text(),
                                      nowplayingtrack.duration,
                                      session.text(),
                                      nowplayingtrack.title.text(),
                                      nowplayingtrack.no);

    checksum(signature);

    request=FXString::value("method=track.updateNowPlaying"
                           "&track=%s"
                           "&artist=%s"
                           "&album=%s"
                           "&trackNumber=%d"
                           "&duration=%d"
                           "&api_key=" CLIENT_KEY
                           "&api_sig=%s"
                           "&sk=%s",
                           gm_url_encode(nowplayingtrack.title).text(),
                           gm_url_encode(nowplayingtrack.artist).text(),
                           gm_url_encode(nowplayingtrack.album).text(),
                           nowplayingtrack.no,
                           nowplayingtrack.duration,
                           signature.text(),
                           session.text());
    }
  else {
    request=FXString::value("s=%s&a=%s&t=%s&b=%s&l=%d&n=%d&m",gm_url_encode(session).text(),
                                                             gm_url_encode(nowplayingtrack.artist).text(),
                                                             gm_url_encode(nowplayingtrack.title).text(),
                                                             gm_url_encode(nowplayingtrack.album).text(),
                                                             nowplayingtrack.duration,
                                                             nowplayingtrack.no);
    }
  nowplayingtrack.clear();
  }


void GMAudioScrobbler::process_nowplaying_response(const FXString & response){
  FXMutexLock lock(mutex_data);
  if (flags&FLAG_LOGIN_CHANGED) return;
  FXTRACE((60,"GMAudioScrobbler::process_nowplaying_response:\n%s\n",response.text()));
  if (mode==SERVICE_LASTFM) {
    ServiceResponse sr;
    if (!sr.parse(response) || !sr.getStatus()) {
      FXASSERT(0);
      FXTRACE((60,"last.fm service failed with code %d: %s\n",sr.getErrorCode(),sr.getErrorMessage().text()));
      switch(sr.getErrorCode()) {
        case LASTFM_ERROR_UNKNOWN      :
        case LASTFM_ERROR_OFFLINE      :
        case LASTFM_ERROR_UNAVAILABLE  : set_timeout();           break;
        case LASTFM_ERROR_BADSESSION   : session.clear();         break;
        case LASTFM_ERROR_KEY_INVALID  :
        case LASTFM_ERROR_KEY_SUSPENDED: flags|=FLAG_BANNED;      break;
        default                        : flags|=FLAG_BADAUTH;     break;
        }
      }
    }
  else {
    FXString code=response.section('\n',0);
    FXTRACE((70,"Now Playing Response: %s\n\n",response.text()));
    if (compare(code,"BADSESSION",10)==0) {
      session.clear();
      }
    }
  }


void GMAudioScrobbler::create_submit_request(FXString & request) {
  FXMutexLock lock(mutex_data);
  FXTRACE((60,"GMAudioScrobbler::create_submit_request\n"));
  FXint i,s;
  FXint ntracks = FXMIN(50,submitqueue.no());
  FXString signature;

  if (mode==SERVICE_LASTFM) {
    if (ntracks==1) {
      signature=FXString::value("album[0]%s"
                               "api_key" CLIENT_KEY
                               "artist[0]%s"
                               "duration[0]%d"
                               "methodtrack.scrobble"
                               "sk%s"
                               "timestamp[0]%u"
                               "trackNumber[0]%d"
                               "track[0]%s"
                               CLIENT_SECRET,
                               submitqueue[0].album.text(),
                               submitqueue[0].artist.text(),
                               submitqueue[0].duration,
                               session.text(),
                               submitqueue[0].getTimeStamp(),
                               submitqueue[0].no,
                               submitqueue[0].title.text());
      }
    else if (ntracks<10) {

      for (i=0;i<ntracks;i++)
        signature+=FXString::value("album[%d]%s",i,submitqueue[i].album.text());

      signature+="api_key" CLIENT_KEY;

      for (i=0;i<ntracks;i++)
        signature+=FXString::value("artist[%d]%s",i,submitqueue[i].artist.text());

      for (i=0;i<ntracks;i++)
        signature+=FXString::value("duration[%d]%d",i,submitqueue[i].duration);

      signature+="methodtrack.scrobble";
      signature+="sk"+session;

      for (i=0;i<ntracks;i++)
        signature+=FXString::value("timestamp[%d]%u",i,submitqueue[i].getTimeStamp());

      for (i=0;i<ntracks;i++)
        signature+=FXString::value("trackNumber[%d]%d",i,submitqueue[i].no);

      for (i=0;i<ntracks;i++)
        signature+=FXString::value("track[%d]%s",i,submitqueue[i].title.text());

      signature+=CLIENT_SECRET;
      }
    else {
      /// albums
      for (s=0,i=10;i<ntracks;i++){
        if ((i%10)==0) {
          signature+=FXString::value("album[%d]%s",s,submitqueue[s].album.text());
          s++;
          }
        signature+=FXString::value("album[%d]%s",i,submitqueue[i].album.text());
        }
      for (;s<10;s++)
        signature+=FXString::value("album[%d]%s",s,submitqueue[s].album.text());

      signature+="api_key" CLIENT_KEY;

      /// artists
      for (s=0,i=10;i<ntracks;i++){
        if ((i%10)==0) {
          signature+=FXString::value("artist[%d]%s",s,submitqueue[s].artist.text());
          s++;
          }
        signature+=FXString::value("artist[%d]%s",i,submitqueue[i].artist.text());
        }
      for (;s<10;s++)
        signature+=FXString::value("artist[%d]%s",s,submitqueue[s].artist.text());

      /// duration
      for (s=0,i=10;i<ntracks;i++){
        if ((i%10)==0) {
          signature+=FXString::value("duration[%d]%d",s,submitqueue[s].duration);
          s++;
          }
        signature+=FXString::value("duration[%d]%d",i,submitqueue[i].duration);
        }
      for (;s<10;s++)
        signature+=FXString::value("duration[%d]%d",s,submitqueue[s].duration);

      signature+="methodtrack.scrobble";
      signature+="sk"+session;

      /// Timestamp
      for (s=0,i=10;i<ntracks;i++){
        if ((i%10)==0) {
          signature+=FXString::value("timestamp[%d]%u",s,submitqueue[s].getTimeStamp());
          s++;
          }
        signature+=FXString::value("timestamp[%d]%u",i,submitqueue[i].getTimeStamp());
        }
      for (;s<10;s++)
        signature+=FXString::value("timestamp[%d]%u",s,submitqueue[s].getTimeStamp());

      /// track number
      for (s=0,i=10;i<ntracks;i++){
        if ((i%10)==0) {
          signature+=FXString::value("trackNumber[%d]%d",s,submitqueue[s].no);
          s++;
          }
        signature+=FXString::value("trackNumber[%d]%d",i,submitqueue[i].no);
        }
      for (;s<10;s++)
        signature+=FXString::value("trackNumber[%d]%d",s,submitqueue[s].no);


      /// track
      for (s=0,i=10;i<ntracks;i++){
        if ((i%10)==0) {
          signature+=FXString::value("track[%d]%s",s,submitqueue[s].title.text());
          s++;
          }
        signature+=FXString::value("track[%d]%s",i,submitqueue[i].title.text());
        }
      for (;s<10;s++)
        signature+=FXString::value("track[%d]%s",s,submitqueue[s].title.text());


      signature+=CLIENT_SECRET;
      }

    checksum(signature);

    request="method=track.scrobble";
    for (i=0;i<ntracks;i++) {
      request+=FXString::value("&track[%d]=%s"
                              "&timestamp[%d]=%u"
                              "&artist[%d]=%s"
                              "&album[%d]=%s"
                              "&trackNumber[%d]=%d"
                              "&duration[%d]=%d",
                              i,gm_url_encode(submitqueue[i].title).text(),
                              i,submitqueue[i].getTimeStamp(),
                              i,gm_url_encode(submitqueue[i].artist).text(),
                              i,gm_url_encode(submitqueue[i].album).text(),
                              i,submitqueue[i].no,
                              i,submitqueue[i].duration);
      }
    request+=FXString::value("&api_key=" CLIENT_KEY "&api_sig=%s&sk=%s",signature.text(),session.text());
    }
  else {
    request+="s=";
    request+=gm_url_encode(session);
    for (i=0;i<ntracks;i++) {
      request+=FXString::value("&a[%d]=%s"
                              "&t[%d]=%s"
                              "&i[%d]=%u"
                              "&o[%d]=P"
                              "&r[%d]"
                              "&l[%d]=%d"
                              "&b[%d]=%s"
                              "&n[%d]=%d"
                              "&m[%d]",
                              i,gm_url_encode(submitqueue[i].artist).text(),
                              i,gm_url_encode(submitqueue[i].title).text(),
                              i,submitqueue[i].getTimeStamp(),
                              i,i,
                              i,submitqueue[i].duration,
                              i,gm_url_encode(submitqueue[i].album).text(),
                              i,submitqueue[i].no,
                              i);
        }
    }
  nsubmitted=ntracks;
  }

void GMAudioScrobbler::process_submit_response(const FXString & response){
  FXMutexLock lock(mutex_data);
  FXTRACE((60,"GMAudioScrobbler::process_submit_response\n%s\n",response.text()));
  if (mode==SERVICE_LASTFM) {
    ServiceResponse sr;
    if (!sr.parse(response) || !sr.getStatus()) {
      FXASSERT(0);
      FXTRACE((60,"last.fm service failed with code %d: %s\n",sr.getErrorCode(),sr.getErrorMessage().text()));
      switch(sr.getErrorCode()) {
        case LASTFM_ERROR_UNKNOWN      :
        case LASTFM_ERROR_OFFLINE      :
        case LASTFM_ERROR_UNAVAILABLE  : set_timeout();           break;
        case LASTFM_ERROR_BADSESSION   : session.clear();         break;
        case LASTFM_ERROR_KEY_INVALID  :
        case LASTFM_ERROR_KEY_SUSPENDED: flags|=FLAG_BANNED;      break;
        default                        : flags|=FLAG_BADAUTH;     break;
        }
      }
    else {
      FXTRACE((60,"last.fm service submit success\n"));
      submitqueue.erase(0,nsubmitted);
      nsubmitted=0;
      }
    }
  else {
    FXString code=response.section('\n',0);
    FXTRACE((70,"Submit Response: %s\n\n",response.text()));
    if (compare(code,"OK",2)==0) {
      if (submitqueue[0].loveban!=0) {
        submitqueue[0].timestamp=0;
        loveban();
        }
      else {
        submitqueue.erase(0,nsubmitted);
        }
      nsubmitted=0;
      }
    else if (compare(code,"BADSESSION",10)==0) {
      session.clear();
      nsubmitted=0;
      }
    else if (compare(code,"FAILED",6)==0) {
      session.clear();
      nsubmitted=0;
      }
    else {
      set_submit_failed();
      nsubmitted=0;
      }
    }
  }


/* LastFM */
void GMAudioScrobbler::create_loveban_request(FXString & request){
  FXMutexLock lock(mutex_data);
  FXTRACE((60,"GMAudioScrobbler::create_loveban_request\n"));
  FXASSERT(mode==SERVICE_LASTFM);

  FXString signature=FXString::value("api_key" CLIENT_KEY
                           "artist%s"
                           "methodtrack.love" //methodtrack.ban
                           "sk%s"
                           "track%s"
                           CLIENT_SECRET,
                           submitqueue[0].artist.text(),
                           session.text(),
                           submitqueue[0].title.text());

  checksum(signature);

  request=FXString::value("method=track.love" ////method=track.ban
                         "&track=%s"
                         "&artist=%s"
                         "&api_key=" CLIENT_KEY
                         "&api_sig=%s"
                         "&sk=%s",
                         gm_url_encode(submitqueue[0].title).text(),
                         gm_url_encode(submitqueue[0].artist).text(),
                         signature.text(),
                         session.text());
  }

void GMAudioScrobbler::process_loveban_response(const FXString & response){
  FXMutexLock lock(mutex_data);
  FXTRACE((60,"GMAudioScrobbler::process_loveban_response\n%s\n",response.text()));
  ServiceResponse sr;
  if (!sr.parse(response) || !sr.getStatus()) {
    session.clear();
    FXTRACE((60,"last.fm service failed with code %d: %s\n",sr.getErrorCode(),sr.getErrorMessage().text()));
    }
  }
