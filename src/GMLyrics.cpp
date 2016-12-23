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
#include "gmutils.h"
#include "ap.h"
#include "GMTrack.h"
#include "GMLyrics.h"

#if FOXVERSION < FXVERSION(1,7,55)

// From FOX-1.7.55
// Copyright (C) 2016 by Jeroen van der Zijp.

enum {
  CRLF = 0x0001,      /// CRLF, LFCR, CR, LF map to LF
  REFS = 0x0002,      /// Character references processed
  };


// Decode escaped special characters from XML stream
static FXbool xml_decode(FXString& dst,const FXString& src,FXuint flags=CRLF|REFS){
  register FXival p,q;
  register FXwchar wc;

  // Measure the resulting string first
  p=q=0;
  while(q<src.length()){
    wc=src[q++];
    if(wc=='\r' && (flags&CRLF)){               // CR, CRLF -> LF
      if(src[q]=='\n'){ q++; }
      p++;
      continue;
      }
    if(wc=='\n' && (flags&CRLF)){               // LF, LFCR -> LF
      if(src[q]=='\r'){ q++; }
      p++;
      continue;
      }
    if(wc=='&' && (flags&REFS)){
      if(src[q]=='#'){
        if(src[q+1]=='x'){                      // &#xXXXX;
          q+=2;
          if(!Ascii::isHexDigit(src[q])) return false;  // Expected at least one hex digit
          wc=FXString::digit2Value[(FXuchar)src[q++]];
          while(Ascii::isHexDigit(src[q])){
            wc=wc*16+FXString::digit2Value[(FXuchar)src[q++]];
            }
          if(src[q++]!=';') return false;       // Expected semicolon
          }
        else{                                   // &#DDDD;
          q+=1;
          if(!Ascii::isDigit(src[q])) return false;     // Expected at least one digit
          wc=src[q++]-'0';
          while(Ascii::isDigit(src[q])){
            wc=wc*10+(src[q++]-'0');
            }
          if(src[q++]!=';') return false;       // Expected semicolon
          }
        p+=wc2utf(wc);
        continue;
        }
      if(src[q]=='q' && src[q+1]=='u' && src[q+2]=='o' && src[q+3]=='t' && src[q+4]==';'){      // &quot;
        q+=5;
        p++;
        continue;
        }
      if(src[q]=='a' && src[q+1]=='p' && src[q+2]=='o' && src[q+3]=='s' && src[q+4]==';'){      // &apos;
        q+=5;
        p++;
        continue;
        }
      if(src[q]=='a' && src[q+1]=='m' && src[q+2]=='p' && src[q+3]==';'){       // &amp;
        q+=4;
        p++;
        continue;
        }
      if(src[q]=='l' && src[q+1]=='t' && src[q+2]==';'){        // &lt;
        q+=3;
        p++;
        continue;
        }
      if(src[q]=='g' && src[q+1]=='t' && src[q+2]==';'){        // &gt;
        q+=3;
        p++;
        continue;
        }
      return false;                             // Unknown reference
      }
    p++;
    }

  // Now allocate space
  dst.length(p);

  // Now produce the result string
  p=q=0;
  while(q<src.length()){
    wc=src[q++];
    if(wc=='\r' && (flags&CRLF)){               // CR, CRLF -> LF
      if(src[q]=='\n'){ q++; }
      dst[p++]='\n';
      continue;
      }
    if(wc=='\n' && (flags&CRLF)){               // LF, LFCR -> LF
      if(src[q]=='\r'){ q++; }
      dst[p++]='\n';
      continue;
      }
    if(wc=='&' && (flags&REFS)){
      if(src[q]=='#'){
        if(src[q+1]=='x'){                      // &#xXXXX;
          q+=2;
          FXASSERT(Ascii::isHexDigit(src[q]));  // Expected at least one hex digit
          wc=FXString::digit2Value[(FXuchar)src[q++]];
          while(Ascii::isHexDigit(src[q])){
            wc=wc*16+FXString::digit2Value[(FXuchar)src[q++]];
            }
          FXASSERT(src[q]==';');                // Expected semicolon
          q++;
          }
        else{                                   // &#DDDD;
          q+=1;
          FXASSERT(Ascii::isDigit(src[q]));     // Expected at least one digit
          wc=src[q++]-'0';
          while(Ascii::isDigit(src[q])){
            wc=wc*10+(src[q++]-'0');
            }
          FXASSERT(src[q]==';');                // Expected semicolon
          q++;
          }
        p+=wc2utf(&dst[p],wc);
        continue;
        }
      if(src[q]=='q' && src[q+1]=='u' && src[q+2]=='o' && src[q+3]=='t' && src[q+4]==';'){      // &quot;
        q+=5;
        dst[p++]='\"';
        continue;
        }
      if(src[q]=='a' && src[q+1]=='p' && src[q+2]=='o' && src[q+3]=='s' && src[q+4]==';'){      // &apos;
        q+=5;
        dst[p++]='\'';
        continue;
        }
      if(src[q]=='a' && src[q+1]=='m' && src[q+2]=='p' && src[q+3]==';'){       // &amp;
        q+=4;
        dst[p++]='&';
        continue;
        }
      if(src[q]=='l' && src[q+1]=='t' && src[q+2]==';'){        // &lt;
        q+=3;
        dst[p++]='<';
        continue;
        }
      if(src[q]=='g' && src[q+1]=='t' && src[q+2]==';'){        // &gt;
        q+=3;
        dst[p++]='>';
        continue;
        }
      }
    dst[p++]=wc;
    }
  FXASSERT(p<=dst.length());
  return true;
  }
#endif




class LyricsSource {
public:
  virtual FXbool fetch(GMTrack & track) = 0;
  virtual ~LyricsSource() {};
  };



class LrcSource : public LyricsSource {
protected:
  FXRex wordtags;
public:
  LrcSource() {
    wordtags.parse("<\\d\\d:\\d\\d.\\d\\d>",FXRex::Normal);
    }

  virtual FXbool fetch(GMTrack & track) override {

    FXString filename = track.url;
    if (!FXPath::isAbsolute(track.url)) {
      filename = FXURL::fileFromURL(track.url);
      if (filename.empty()) return false;
      }
    filename = FXPath::stripExtension(filename) + ".lrc";
    if (!FXStat::exists(filename)) return false;
    FXString data;
    FXString lyrics;
    if (!gm_buffer_file(filename,data)) return false;


    // Parse each line
    FXint start=0,end=0,next=0;
    for (FXint i=0;i<data.length();i++) {
      if (data[i]=='\n') {
        end=i;
        next=i+1;

        /// Skip white space
        while(start<end && Ascii::isSpace(data[start])) start++;

        /// Skip white space
        while(end>start && Ascii::isSpace(data[end])) end--;

        /// Skip
        if (data[start]=='[') {
          while(start<end && data[start]!=']') start++;
          start+=1;
          if ((end-start>2) && (data[start]=='D' || data[start]=='M' || data[start]=='F') && data[start+1]==':')
            start+=2;
          }

        /// New Line
        if (!lyrics.empty()) lyrics+='\n';

        /// Parse the actual line.
        if ((end-start)) {
          lyrics+=data.mid(start,1+end-start);
          }
        start=next;
        }
      }

    // Remove any wordtags
    FXint bb[2],ee[2],ff=0;
    while(wordtags.search(lyrics,ff,lyrics.length()-1,FXRex::Normal,bb,ee,1)>=0) {
      GM_DEBUG_PRINT("[lyrics] found <tags>: %s\n",lyrics.mid(bb[0],ee[0]-bb[0]).text());
      lyrics.erase(bb[0],ee[0]-bb[0]);
      ff=bb[0];
      }

    // Finalize
    lyrics.trim();
    if (!lyrics.empty()) {
      track.lyrics.adopt(lyrics);
      }
    return true;
    }
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

#if FOXVERSION < FXVERSION(1,7,55)
    xml_decode(result,src);
#else
    FXXML::decode(result,src);
#endif

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
  source[0] = new LrcSource;
  source[1] = new LyricsWikiaSource;
  }

Lyrics::~Lyrics() {
  delete source[0];
  delete source[1];
  }

FXbool Lyrics::fetch(GMTrack & track) const {
  for (FXint i=0;i<2;i++) {
    if (source[i]->fetch(track)) {
      GM_DEBUG_PRINT("Found lyrics for %s from src %d\n",track.url.text(),i);
      return true;
      }
    }
  return false;
  }





 