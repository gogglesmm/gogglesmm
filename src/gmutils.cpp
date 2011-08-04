/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2010 by Sander Jansen. All Rights Reserved      *
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

/******************************************************************************/





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
      uri+="\r\n";
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
    if (!exec.empty()) break;
    }

  if (exec.empty()) return false;

  exec += " " + FXPath::enquote(url);

  pid_t pid = fork();
  if (pid==-1){ /// Failure delivered to Parent Process
      return false;
      }
  else if (pid==0) { /// Child Process
      int i = sysconf(_SC_OPEN_MAX);
      while (--i >= 3) {
        close(i);
        }
      execlp("/bin/sh", "sh", "-c",exec.text(),(char *)0);
      exit(EXIT_FAILURE);
      }
  else { /// Parent Process
    return true;
      }
  return true;
  }

FXbool gm_open_browser(const FXString & url) {
  static const char * const programs[]={"xdg-open","chromium","firefox","konqueror","opera","netscape",NULL};
  return gm_launch_program(programs,url);
  }

FXbool gm_open_folder(const FXString & folder) {
  static const char * const programs[]={"xdg-open","thunar","dolphin","konqueror","nautilus",NULL};
  return gm_launch_program(programs,folder);
  }


void gm_copy_hash(FXHash & from,FXHash & to) {
  for (FXuint i=0;i<from.size();i++){
    if (!from.empty(i)) {
      to.insert(from.key(i),from.value(i));
      }
    }
  }


/******************************************************************************/


FXImage * gm_create_image(const FXString & mime) {
  FXImage * image=NULL;
  if ((comparecase(mime,"image/jpg")==0) || (comparecase(mime,"image/jpeg")==0) || (comparecase(mime,"JPG")==0)) {
    image=new FXJPGImage(FXApp::instance(),NULL,IMAGE_OWNED|IMAGE_SHMI|IMAGE_SHMP);
    }
  else if (comparecase(mime,FXPNGImage::mimeType)==0 || (comparecase(mime,"png")==0)) {
    image=new FXPNGImage(FXApp::instance(),NULL,IMAGE_OWNED|IMAGE_SHMI|IMAGE_SHMP);
    }
  else if ((comparecase(mime,"image/bmp")==0) || (comparecase(mime,"image/x-bmp")==0) || (comparecase(mime,"bmp")==0)) {
    image=new FXBMPImage(FXApp::instance(),NULL,IMAGE_OWNED|IMAGE_SHMI|IMAGE_SHMP);
    }
  else if ((comparecase(mime,FXGIFImage::mimeType)==0) || (comparecase(mime,"gif")==0)) {
    image=new FXGIFImage(FXApp::instance(),NULL,IMAGE_OWNED|IMAGE_SHMI|IMAGE_SHMP);
    }
  return image;
  }

FXbool gm_load_pixels(FXStream & store,FXImage *image,FXint scale,FXint crop) {
  if (image->loadPixels(store)){
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
        image->scale(scale,(scale*hh)/ww,1);
      else
        image->scale((scale*ww)/hh,scale,1);
      }
    return true;
    }
  return false;
  }


FXImage * gm_load_image_from_file(const FXString & filename,FXint scale,FXint crop) {
  FXImage * image=gm_create_image(FXPath::extension(filename));
  if (image) {
    FXFileStream store;
    if (store.open(filename,FXStreamLoad)) {
      if (gm_load_pixels(store,image,scale,crop))
        return image;
      }
    delete image;
    }
  return NULL;
  }


FXImage * gm_load_image_from_data(const void * data,FXuval size,const FXString & mime,FXint scale,FXint crop) {
  FXImage * image=gm_create_image(mime);
  if (image) {
    FXMemoryStream store;
    store.open(FXStreamLoad,(FXuchar*)data,size);
    if (gm_load_pixels(store,image,scale,crop))
      return image;

    delete image;
    }
  return NULL;
  }

FXbool gm_decode_base64(FXuchar * buffer,FXint & len){
  static const char base64[256]={
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

