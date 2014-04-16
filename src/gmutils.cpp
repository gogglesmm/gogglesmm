/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2014 by Sander Jansen. All Rights Reserved      *
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

#include "FXPNGImage.h"
#include "FXBMPImage.h"
#include "FXJPGImage.h"
#include "FXGIFImage.h"

#include <errno.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>






void FXIntMap::save(FXStream & store) const {
  FXint u = used();
  FXint t = no();
  store << u;
  for (FXint i=0;i<t;i++){
    if (!empty(i)) {
      store << key(i);
      store << value(i);
      }
    }
  }

void FXIntMap::load(FXStream & store) {
  FXint key,value,n;
  store >> n;
  for (FXint i=0;i<n;i++){
    store >> key;
    store >> value;
    insert(key,value);
    }
  }

void FXIntMap::adopt(FXIntMap & other) {
  // Clear this map first
  clear();

  // Populate with new entries
  for (FXint i=0;i<other.no();i++){
    if (!other.empty(i)) {
      insert(other.key(i),other.value(i));
      }
    }

  // Leave other empty
  other.clear();
  }





/******************************************************************************/



/*
enum {
  DESKTOP_SESSION_X11         = 0,
  DESKTOP_SESSION_KDE_PLASMA  = 1,
  DESKTOP_SESSION_XFCE        = 2,
  DESKTOP_SESSION_GNOME       = 3,
  };
*/

FXuint gm_desktop_session() {
  FXString desktop = FXSystem::getEnvironment("DESKTOP_SESSION");
  if (comparecase(desktop,"kde-plasma")==0)
    return DESKTOP_SESSION_KDE_PLASMA;
  else if (comparecase(desktop,"gnome")==0)
    return DESKTOP_SESSION_GNOME;
  else if (comparecase(desktop,"xfce")==0)
    return DESKTOP_SESSION_XFCE;
  else if (comparecase(desktop,"lxde")==0)
    return DESKTOP_SESSION_LXDE;
  else
    return DESKTOP_SESSION_X11;
  }


/******************************************************************************/


#define URL_UNSAFE   "#$-_.+!*'><()\\,%\""          // Always Encode
#define URL_RESERVED ";/?:@=&"              // Only encode if not used as reserved by scheme


// Encode url string
FXString gm_url_encode(const FXString& url){
  register FXint p=0;
  register FXint c;
  FXString result;
  while(p<url.length()){
    c=url[p++];
//    if(!Ascii::isAlphaNumeric(c) && ((c<=' ' || c>='{') || strchr(URL_UNSAFE URL_RESERVED,c))){
    if (!Ascii::isAlphaNumeric(c)){
      result.append('%');
      result.append(FXString::value2Digit[(c>>4)&15]);
      result.append(FXString::value2Digit[c&15]);
      continue;
      }
    result.append(c);
    }
  return result;
  }


FXdouble gm_parse_number(const FXString & str) {
  if (str.empty())
    return NAN;

  /// FOX 1.7 has its own scanf and always uses C locale for number conversions.
  FXdouble value=NAN;
  if (str.scan("%lg",&value)==1)
    return value;
  else
    return NAN;
  }


FXbool gm_buffer_file(const FXString & filename,FXString & buffer) {
  FXFile file(filename,FXIO::Reading);
  if (file.isOpen()) {

    buffer.assign('\0',file.size());

    return (file.readBlock((void*)buffer.text(),buffer.length())==buffer.length());
    }
  return false;
  }

FXbool gm_dump_file(const FXString & filename,FXString & buffer) {
  FXFile file(filename,FXIO::Writing);
  if (file.isOpen()) {
    return (file.writeBlock((void*)buffer.text(),buffer.length())==buffer.length());
    }
  return false;
  }


void gm_focus_and_select(FXTextField * textfield) {
  FXASSERT(textfield->id());
  textfield->setFocus();
  if (!textfield->getText().empty())
    textfield->setSelection(0,textfield->getText().length());
  }

void gm_run_popup_menu(FXMenuPane*pane,FXint rx,FXint ry) {
  pane->create();
  pane->forceRefresh();
  pane->show();
  pane->grabKeyboard();
  pane->popup(NULL,rx,ry);
  FXApp::instance()->runPopup(pane);
  pane->ungrabKeyboard();
  }

void gm_set_window_cursor(FXWindow * window,FXCursor * cur) {
  window->setDefaultCursor(cur);
  window->setDragCursor(cur);
  FXWindow * child=window->getFirst();
  while(child) {
    child->setDefaultCursor(cur);
    child->setDragCursor(cur);
    child=child->getNext();
    }
  }



FXString gm_make_url(const FXString & in) {
  if (in[0]=='/')
    return FXURL::fileToURL(in);
  else
    return in;
  }

FXbool gm_is_local_file(const FXString & filename) {
  if (filename[0]=='/') return true;
  FXString scheme = FXURL::scheme(filename);
  if (scheme.empty() || (comparecase(scheme,"file")==0))
    return true;
  else
    return false;
  }



void gm_convert_filenames_to_uri(const FXStringList & filenames,FXString & uri){
  if (filenames.no()) {
    uri=FXURL::fileToURL(filenames[0]);
    for (FXint i=1;i<filenames.no();i++){
      uri+="\r\n";
      uri+=FXURL::fileToURL(filenames[i]);
      }
    }
  }


void gm_convert_uri_to_filenames(FXString & files,FXStringList & filelist){
  FXint begin,end;
  FXString file;
  for(begin=0;begin<files.length();begin=end+2){
    end=files.find_first_of("\r\n",begin);
    if (end<0) end = files.length();
    file = FXURL::decode(FXURL::fileFromURL(files.mid(begin,end-begin)));
    if (!file.empty()) filelist.append(file);
    }
  }


void gm_convert_filenames_to_gnomeclipboard(const FXStringList & filenames,FXString & uri){
  if (filenames.no()) {
    uri="copy\n" + FXURL::fileToURL(filenames[0]);
    for (FXint i=1;i<filenames.no();i++){
      uri+="\n";
      uri+=FXURL::fileToURL(filenames[i]);
      }
    }
  }


void gm_convert_gnomeclipboard_to_filenames(FXString & files,FXStringList & filelist){
  FXint begin,end;
  FXString file;
  for(begin=0;begin<files.length();begin=end+1){
    end=files.find_first_of("\r\n",begin);
    if (end<0) end = files.length();
    if (begin) {
      file = FXURL::decode(FXURL::fileFromURL(files.mid(begin,end-begin)));
      if (!file.empty()) filelist.append(file);
      }
    }
  }

void gm_make_absolute_path(const FXString & path,FXStringList & urls) {
#ifndef WIN32
  for (FXint i=0;i<urls.no();i++) {
    if (!urls[i].empty()) {
      if (urls[i][0]!='/') {
        FXString scheme = FXURL::scheme(urls[i]);
        if (comparecase(scheme,"file")==0) {
          urls[i]=FXURL::fileFromURL(urls[i]);
          if (urls[i][0]!='/')
            urls[i]=FXPath::absolute(path,urls[i]);
          }
        else if (!scheme.empty()) {
          urls[i].clear();
          }
        else {
          urls[i]=FXPath::absolute(path,urls[i]);
          }
        }
      }
    }
#else
#error "not yet implemented"
#endif
  }


/******************************************************************************/
static FXbool gm_launch_program(const FXchar * const * programs,const FXString & url) {
  FXString path = FXSystem::getExecPath();
  FXString exec;
  for (int i=0;programs[i]!=NULL;i++){
    exec = FXPath::search(path,programs[i]);
    if (!exec.empty()) {
      pid_t pid = fork();
      if (pid==0) {
        int i = sysconf(_SC_OPEN_MAX);
        while (--i >= 3) {
          close(i);
          }
        execl(exec.text(),programs[i],url.text(),NULL);
        exit(EXIT_FAILURE);
        }
      else if (pid==-1)
        return false;
      return true;
      }
    }
  return false;
  }

FXbool gm_open_browser(const FXString & url) {
  static const char * const programs[]={"xdg-open","chromium","firefox","konqueror","opera","netscape",NULL};
  return gm_launch_program(programs,url);
  }

FXbool gm_open_folder(const FXString & folder) {
  static const char * const programs[]={"xdg-open","thunar","dolphin","konqueror","nautilus",NULL};
  return gm_launch_program(programs,folder);
  }

FXbool gm_image_search(const FXString & s) {
  #define ENCODE_THESE "<>#%{}|^~[]`\"?$&'*,;="   // Encode these for pathnames
  FXString search = FXURL::encode(s,ENCODE_THESE);
  FXString query = "http://www.google.com/search?&tbm=isch&as_epq=" + search;
  return gm_open_browser(query);
  }


/******************************************************************************/

static void gm_scale_crop(FXImage * image,FXint scale,FXint crop){
  FXint ww=image->getWidth();
  FXint hh=image->getHeight();
  if (crop) {
    if (ww>hh) {
      FXfloat aspect = (float)ww/(float)hh;
      if (aspect<=(4.f/3.f)) {
        image->crop(((ww-hh)/2),0,hh,hh);
        ww=image->getWidth();
        }
      }
    else if (hh>ww){
      FXfloat aspect = (float)hh/(float)ww;
      if (aspect<=(4.f/3.f)) {
        image->crop(0,((hh-ww)/2),ww,ww);
        hh=image->getHeight();
        }
      }
    }
  if (scale && ((ww>scale) || (hh>scale))) {
    if (ww>hh)
      image->scale(scale,(scale*hh)/ww,FOX_SCALE_BEST);
    else
      image->scale((scale*ww)/hh,scale,FOX_SCALE_BEST);
    }
  }

static FXImage * gm_load_pixels(FXStream & store,FXint scale,FXint crop){
  FXImage * image = NULL;
  FXColor * data = NULL;
  FXint     width,height,extra;

  if (fxcheckJPG(store)) {
    if (fxloadJPG(store,data,width,height,extra)) {
      image = new FXImage(FXApp::instance(),data,IMAGE_OWNED|IMAGE_SHMI|IMAGE_SHMP,width,height);
      }
    }
  else if (fxcheckPNG(store)) {
    if (fxloadPNG(store,data,width,height)) {
      image =new FXImage(FXApp::instance(),data,IMAGE_OWNED|IMAGE_SHMI|IMAGE_SHMP,width,height);
      }
    }
  else if (fxcheckBMP(store)) {
    if (fxloadBMP(store,data,width,height)) {
      image = new FXImage(FXApp::instance(),data,IMAGE_OWNED|IMAGE_SHMI|IMAGE_SHMP,width,height);
      }
    }
  else if (fxcheckGIF(store)) {
    if (fxloadGIF(store,data,width,height)) {
      image = new FXImage(FXApp::instance(),data,IMAGE_OWNED|IMAGE_SHMI|IMAGE_SHMP,width,height);
      }
    }

  if (image) {
    gm_scale_crop(image,scale,crop);
    }
  return image;
  }




FXImage * gm_load_image_from_file(const FXString & filename,FXint scale,FXint crop) {
  FXFileStream store;
  if (store.open(filename,FXStreamLoad)) {
    return gm_load_pixels(store,scale,crop);
    }
  return NULL;
  }


FXImage * gm_load_image_from_data(const void * data,FXuval size,FXint scale,FXint crop) {
  FXMemoryStream store;
  if (store.open(FXStreamLoad,(FXuchar*)data,size)) {
    return gm_load_pixels(store,scale,crop);
    }
  return NULL;
  }



FXbool gm_decode_base64(FXuchar * buffer,FXint & len){
  static const FXuchar base64[256]={
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x3e,0x80,0x80,0x80,0x3f,
    0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,
    0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x80,0x80,0x80,0x80,0x80,
    0x80,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
    0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80};

  FXuint  pos=0;
  FXuchar v;
  for (FXint i=0,b=0;i<len;i++) {
    v=base64[buffer[i]];
    if (v!=0x80) {
      switch(b) {
        case 0: buffer[pos]=(v<<2);
                b++;
                break;
        case 1: buffer[pos++]|=(v>>4);
                buffer[pos]=(v<<4);
                b++;
                break;
        case 2: buffer[pos++]|=(v>>2);
                buffer[pos]=(v<<6);
                b++;
                break;
        case 3: buffer[pos++]|=v;
                b=0;
                break;
        }
      }
    else {
      if (buffer[i]=='=' && b>1) {
        len=pos;
        return true;
        }
      else {
        return false;
        }
      }
    }
  len=pos;
  return true;
  }


void gm_print_time(FXint time,FXString & result){
  FXint	days = (FXint) floor((double)time/86400.0);
  time -= (86400*days);
  FXint	hours = (FXint) floor((double)time/3600.0);
  time -= (3600*hours);
  FXint	minutes = (FXint) floor((double)time/60.0);
  result.format("%d days %d hours %d minutes",days,hours,minutes);
  }


void gm_bgra_to_rgba(FXColor * inbuf,FXColor * outbuf, FXint len) {
   FXuchar * in  = reinterpret_cast<FXuchar*>(inbuf);
   FXuchar * out = reinterpret_cast<FXuchar*>(outbuf);
   for (FXint i=0;i<(len*4);i+=4) {
      out[i+0]=in[i+2]; // r
      out[i+1]=in[i+1]; // g
      out[i+2]=in[i+0]; // b
      out[i+3]=in[i+3]; // a
      }
  }




/*
  The following function tries to parse datetime strings for the following
  standards:

    RFC 822 (2 digit years)
    RFC 1123 (updated 822 with 4 digit years)
    RFC 2822:

        Sun, 06 Nov 1994 08:49:37 GMT

    RFC 850, obsoleted by RFC 1036:

        Sunday, 06-Nov-94 08:49:37 GMT

    asctime:

        Sun Nov  6 08:49:37 1994

    It doesn't handle any header unfolding, but does skip over any comments which may appear anywhere
    between fields as described in RFC822 / RFC 2822.

    It also handles missing or optional week days for all formats
*/


#define ONE_DIGIT_VALUE(d1) (FXString::digit2Value[(const FXuchar)d1])
#define TWO_DIGIT_VALUE(d1,d2) (FXString::digit2Value[(const FXuchar)d1]*10) + (FXString::digit2Value[(const FXuchar)d2])
#define THREE_DIGIT_VALUE(d1,d2,d3) (FXString::digit2Value[(const FXuchar)d1]*100) +\
                                    (FXString::digit2Value[(const FXuchar)d2]*10) +\
                                    (FXString::digit2Value[(const FXuchar)d3])

#define FOUR_DIGIT_VALUE(d1,d2,d3,d4) (FXString::digit2Value[(const FXuchar)d1]*1000) + \
                                      (FXString::digit2Value[(const FXuchar)d2]*100) + \
                                      (FXString::digit2Value[(const FXuchar)d3]*10) +\
                                      (FXString::digit2Value[(const FXuchar)d4])

FXbool gm_parse_datetime(const FXString & str,FXTime & timestamp) {

  // Fields
  enum {
    Done = 0,
    WeekDay,
    WeekDaySep,
    Day,
    DaySep,
    Month,
    MonthSep,
    Year,
    Hour,
    HourSep,
    Minute,
    MinuteSep,
    Seconds,
    Zone
    };

  /// 1 second expresed in nanoseconds
  const FXlong seconds = 1000000000;

  FXbool is_week_day = false;
  FXbool ctime       = false;
  FXuint parse       = WeekDay;
  FXint i=0;
  FXint tzoffset = 0;
  FXint day      = 0;
  FXint month    = 0;
  FXint year     = 0;
  FXint hour     = 0;
  FXint minute   = 0;
  FXint second   = 0;

  while(parse) {

    // skip comments and white space
    do {
      while(Ascii::isSpace(str[i])) i++;
      if (str[i]=='('){
        FXint c=1;
        do {
          i++;
          switch(str[i]) {
            case '\\': i++;           break;
            case  '(': c++;           break;
            case  ')': c--;           break;
            case '\0': return false;  break;
            default  : break;
            }
          }
        while(c);
        i++;
        continue;
        }
      break;
      }
    while(1);

    // Check end of input
    if (i>=str.length())
      break;

parsefield:

    switch(parse) {

      /// Skip optional weekday. If we don't encounter a letter, we skip to the day parsing.
      case WeekDay    :

        if (Ascii::isLetter(str[i])) {

          /* does it look like a day */
          switch(str[i]){
            case 'T':
            case 'W': is_week_day=true;
                      break;

            case 'M': if (str[i+1]=='o')
                        is_week_day=true;
                      break;

            case 'S': if (str[i+1]=='u' || str[i+1]=='a')
                        is_week_day=true;
                      break;

            case 'F': if (str[i+1]=='r')
                        is_week_day=true;
            default : break;
            }

          // Input doesn't look like a weekday, assume it's ctime and look for month.
          if (!is_week_day) {
            ctime=true;
            parse=Month;
            goto parsefield;
            }

          // skip weekday and look for separator
          parse=WeekDaySep;
          while(Ascii::isLetter(str[i])) i++;
          continue;
          }

        // Not a letter, so it must be starting with a Day.
        parse=Day;
        goto parsefield;
        break;



      /// Skip weekday separator. If there isn't any, it means we're parsing the asctime format
      case WeekDaySep :

        // If there's a separator, it must be followed by a day
        if (str[i]==',') {
          i++;
          parse=Day;
          continue;
          }

        /// No Separator, so this must be a ctime format and next up is month
        parse=Month;
        ctime=true;
        goto parsefield;
        break;

      /// Parse day, handles both 1 or 2 digit cases
      case Day        :
        if (Ascii::isDigit(str[i])) {
          if (Ascii::isDigit(str[i+1])) {
            day = TWO_DIGIT_VALUE(str[i+0],str[i+1]);
            i+=2;
            }
          else {
            day = ONE_DIGIT_VALUE(str[i+0]);
            i+=1;
            }
          parse=(ctime) ? Hour : DaySep;
          continue;
          }
        break;

      // Optional separator between day and month in case we're dealing with rfc 850
      case DaySep:
        parse=Month;
        if (str[i]=='-') {
          i++;
          continue;
          }
        // not a separator, so go directly to month parsing
        goto parsefield;
        break;


      /// Parse abbreviated month. In ctime the month is followed by the day.
      case Month      :
        if (Ascii::isLetter(str[i]) && Ascii::isLetter(str[i+1]) && Ascii::isLetter(str[i+2])) {
          switch(str[i]) {
            case 'A': month=(str[i+1]=='p') ? 4 : 8; break;                         // Apr or Aug
            case 'D': month=12; break;                                              // Dec
            case 'F': month=2; break;                                               // Feb
            case 'J': month=(str[i+1]=='a') ? 1 : (str[i+2]=='n') ? 6 : 7 ; break;  // Jan or Jun or Jul
            case 'M': month=(str[i+2]=='r') ? 3 : 5; break;                         // Mar or May
            case 'N': month=11; break;                                              // Nov
            case 'O': month=10; break;                                              // Oct
            case 'S': month=9; break;                                               // Sep
            default : return false; break;
            };
          parse=(ctime) ? Day : MonthSep;
          i+=3;
          continue;
          }
        break;

      // Optional separator between month and year in case we're dealing with rfc 850
      case MonthSep    :
        parse=Year;
        if (str[i]=='-') {
          i++;
          continue;
          }
        // not a separator, so go directly to year parsing
        goto parsefield;
        break;

      /*
         Parse the year, we liberally accept either 2, 3 or 4 digits and interpret them according RFC2822:

         If a two digit year is encountered whose
         value is between 00 and 49, the year is interpreted by adding 2000,
         ending up with a value between 2000 and 2049.  If a two digit year is
         encountered with a value between 50 and 99, or any three digit year
         is encountered, the year is interpreted by adding 1900.
      */
      case Year       :
        if (Ascii::isDigit(str[i+0]) && Ascii::isDigit(str[i+1])) {
          if (Ascii::isDigit(str[i+2])) {
            if (Ascii::isDigit(str[i+3])) { /// 4 digit year
              year = FOUR_DIGIT_VALUE(str[i+0],str[i+1],str[i+2],str[i+3]);
              i+=4;
              }
            else { /// 3 digit year
              year = 1900 + THREE_DIGIT_VALUE(str[i+0],str[i+1],str[i+2]);
              i+=3;
              }
            }
          else { // 2 digit year
            year = TWO_DIGIT_VALUE(str[i+0],str[i+1]);
            i+=2;
            if (year<50)
              year+=2000;
            else
              year+=1900;
            }
          parse=(ctime) ? Done : Hour;
          continue;
          }
        break;


      /// Parse hour, handles both 1 and 2 digit hours just in case
      case Hour       :
        if (Ascii::isDigit(str[i])) {
          if (Ascii::isDigit(str[i+1])) {
            hour = TWO_DIGIT_VALUE(str[i+0],str[i+1]);
            i+=2;
            }
          else {
            hour = ONE_DIGIT_VALUE(str[i+0]);
            i+=1;
            }
          parse=HourSep;
          continue;
          }
        break;

      /// Skip separator between hours and minutes
      case HourSep    :
        if (str[i]==':') {
          parse=Minute;
          i++;
          continue;
          }
        break;

      /// Minutes should be 2 digits
      case Minute     :
        if (Ascii::isDigit(str[i]) && Ascii::isDigit(str[i+1])) {
          minute=TWO_DIGIT_VALUE(str[i+0],str[i+1]);
          parse=MinuteSep;
          i+=2;
          continue;
          }
        break;

      /// If we find a separator, it means the minutes are followed by seconds. If not,
      /// we skip to the next field (year or zone).
      case MinuteSep  :
        if (str[i]==':') {
          parse=Seconds;
          i++;
          continue;
          }
        parse=(ctime) ? Year : Zone;
        goto parsefield;
        break;

      /// Seconds should be 2 digits. Seconds are followed by Year or Zone
      case Seconds    :
        if (Ascii::isDigit(str[i]) && Ascii::isDigit(str[i+1])) {
          second=TWO_DIGIT_VALUE(str[i+0],str[i+1]);
          parse=(ctime) ? Year : Zone;
          i+=2;
          continue;
          }
        break;

      /// Time Zone
      case Zone       :
        if ((str[i]=='+' || str[i]=='-') && ( Ascii::isDigit(str[i+1]) && Ascii::isDigit(str[i+2]) && Ascii::isDigit(str[i+3]) && Ascii::isDigit(str[i+4]))) {
          FXint hh = TWO_DIGIT_VALUE(str[i+1],str[i+2]);
          FXint mm = TWO_DIGIT_VALUE(str[i+3],str[i+4]);
          tzoffset = (hh*3600+mm*60);
          if (str[i]=='-' ) tzoffset=-tzoffset;
          }
        else {
          if ((str[i+0]=='G' && str[i+1]=='M' && str[i+2]=='T') || (str[i+0]=='U' && str[i+1]=='T'))
            tzoffset = 0;
          else if (str[i+0]=='E' && str[i+1]=='D' && str[i+2]=='T')
            tzoffset=-(4*3600);
          else if (str[i+0]=='E' && str[i+1]=='S' && str[i+2]=='T')
            tzoffset=-(5*3600);
          else if (str[i+0]=='C' && str[i+1]=='D' && str[i+2]=='T')
            tzoffset=-(5*3600);
          else if (str[i+0]=='C' && str[i+1]=='S' && str[i+2]=='T')
            tzoffset=-(6*3600);
          else if (str[i+0]=='M' && str[i+1]=='D' && str[i+2]=='T')
            tzoffset=-(6*3600);
          else if (str[i+0]=='M' && str[i+1]=='S' && str[i+2]=='T')
            tzoffset=-(7*3600);
          else if (str[i+0]=='P' && str[i+1]=='D' && str[i+2]=='T')
            tzoffset=-(7*3600);
          else if (str[i+0]=='P' && str[i+1]=='S' && str[i+2]=='T')
            tzoffset=-(8*3600);
          else
            tzoffset=-0;
          }
        parse=Done;
        continue;
        break;

      default: break;
      }
    return false;
    }

  timestamp  = FXDate(year,month,day).getTime();
  timestamp += seconds * ((hour*3600)+(minute*60)+(second)-(tzoffset));
  return true;
  }






