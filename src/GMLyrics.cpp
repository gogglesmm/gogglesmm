/*******************************************************************************
*                         Goggles Music Manager                                *
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
#include "gmdefs.h"
#include "ap.h"
#include "GMTrack.h"
#include "GMLyrics.h"

class LyricsSource {
public:
  virtual FXbool fetch(GMTrack & track) = 0;
  virtual ~LyricsSource() {};
  };

class HtmlSource : public LyricsSource {
protected:
  FXRex linebreaks;
  FXRex sup;
  FXRex tags;
public:

  HtmlSource() {
    linebreaks.parse("(</?div[^>*]>|<\\s*br\\s*/?>)",FXRex::IgnoreCase|FXRex::Normal);
    sup.parse("<sup\\s*>.*</sup\\s*>",FXRex::IgnoreCase|FXRex::Normal);
    tags.parse("</?[^>]*/?>",FXRex::Normal);
    }

  void clearMarkup(FXString & src) {
    FXint bb[2],ee[2],ff=0;

    // Turn <br> and <div> into newlines
    ff=0;
    while(linebreaks.search(src,ff,src.length()-1,FXRex::Normal,bb,ee,1)>=0) {
      src.replace(bb[0],ee[0]-bb[0],"\n");
      ff=bb[0]+1;
      }

    // Remove <sup>...</sup>
    ff=0;
    while(sup.search(src,ff,src.length()-1,FXRex::Normal,bb,ee,1)>=0) {
      GM_DEBUG_PRINT("[lyrics] found <sup>: %s\n",src.mid(bb[0],ee[0]-bb[0]).text());
      src.erase(bb[0],ee[0]-bb[0]);
      ff=bb[0];
      }

    // Remove any tags
    ff=0;
    while(tags.search(src,ff,src.length()-1,FXRex::Normal,bb,ee,1)>=0) {
      GM_DEBUG_PRINT("[lyrics] found <tags>: %s\n",src.mid(bb[0],ee[0]-bb[0]).text());
      src.erase(bb[0],ee[0]-bb[0]);
      ff=bb[0];
      }


    FXString result;
    FXXML::decode(result,src);
    src = result.trim();
    }
  };


class LyricsWikiaSource : public HtmlSource {
protected:
  FXRex lyricbox;

public:
  LyricsWikiaSource() {
    lyricbox.parse("<div class=\'lyricbox\'>(.*)<div class=\'lyricsbreak\'>",FXRex::IgnoreCase|FXRex::Capture);
    }

  FXbool fetch(GMTrack & track) override {
    HttpClient http;

    FXString artist = track.artist;
    FXString title  = track.title;

    FXString url = FXString::value("http://lyrics.wikia.com/%s:%s",FXURL::encode(artist.substitute(' ','_')).text(),
                                                                   FXURL::encode(title.substitute(' ','_')).text());
    if (http.basic("GET",url)) {
      FXString html = http.textBody();
      FXint b[2],e[2];
      if (lyricbox.search(html,0,html.length()-1,FXRex::Normal,b,e,2)>=0){
        FXString content = html.mid(b[1],e[1]-b[1]);
        clearMarkup(content);
        track.lyrics = content;
        return true;
        }
      }
    return false;
    }

  };





Lyrics::Lyrics() {
  source = new LyricsWikiaSource;
  }

Lyrics::~Lyrics() {
  delete source;
  }

FXbool Lyrics::fetch(GMTrack & track) const {
  if (source->fetch(track)) {
    GM_DEBUG_PRINT("Found lyrics for %s\n",track.url.text());
    return true;
    }
  return false;
  }





 