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
#include "ap_defs.h"
#include "ap_input_plugin.h"
#include "ap_id3v2.h"

#include <FXUTF8Codec.h>
#include <FXUTF16Codec.h>
#include <FX88591Codec.h>

namespace ap {


ID3V2 * ID3V2::parse(InputPlugin * input,const FXuchar * id) {
  FXuchar info[6];

  if (input->read(info,6)!=6)
    return nullptr;

  const FXuchar & id3v2_flags = info[1];
  FXint tagsize = ID3_SYNCSAFE_INT32(info+2);

  tagsize+=10;
  if (id3v2_flags&ID3V2::HAS_FOOTER) {
    tagsize+=10;
    }

  FXuchar * tagbuffer=nullptr;
  allocElms(tagbuffer,tagsize);

  if (input->read(tagbuffer+10,tagsize-10)!=tagsize-10){
    freeElms(tagbuffer);
    return nullptr;
    }

  memcpy(tagbuffer,id,4);
  memcpy(tagbuffer+4,info,6);

  ID3V2 * id3v2 = new ID3V2(tagbuffer,tagsize);

  freeElms(tagbuffer);

  return id3v2;
  }


FXbool ID3V2::skip(InputPlugin * input,const FXuchar * /*id*/) {
  FXuchar info[6];

  if (input->read(info,6)!=6)
    return false;

  const FXuchar & id3v2_flags = info[1];
  FXint tagsize = ID3_SYNCSAFE_INT32(info+2);

  tagsize+=10;
  if (id3v2_flags&ID3V2::HAS_FOOTER) {
    tagsize+=10;
    }

  // fast forward past id3v2 tag
  input->position(tagsize-10,FXIO::Current);

  return true;
  }


ID3V2::ID3V2(FXuchar *b,FXint len) : buffer(b),size(len),p(0),padstart(0),padend(0),length(-1) {
  const FXchar & flags = buffer[5];

  version = buffer[3];
  GM_DEBUG_PRINT("[id3v2] version %d\n",version);

  buffer+=10;
  size-=10;

  /// we can skip the footer
  if (version>=4 && flags&HAS_FOOTER)
    size-=10;

  /// Apply unsync, according to spec 2.3, the extended header also needs unsyncing
  if (flags&HAS_UNSYNC) {
    unsync(buffer,size);
    }

  /// skip the extended header
  if (version>=3 && flags&HAS_EXTENDED_HEADER) {
    FXint header_size;
    if (version==3)
      header_size = ID3_INT32(buffer);
    else
      header_size = ID3_SYNCSAFE_INT32(buffer);
    buffer+=header_size;
    }

  /// Parse
  p=0;
  while(p<size)
    parse_frame();
  }

ID3V2::~ID3V2() {
  }

void ID3V2::unsync(FXuchar * src,FXint & len) {
  FXint k=0;
  for (FXint i=0;i<len;i++) {
    src[k++]=src[i];
    if (src[i]==0xff && (i+1)<len && src[i+1]==0x00)
      i++;
    }
  len=k;
  }

void ID3V2::parse_priv_frame(FXint /*framesize*/) {
  if (buffer[p]!='\0'){
    GM_DEBUG_PRINT("[id3v2] priv text %s\n",buffer+p);
    }
  }

void ID3V2::parse_rva2_frame(FXint framesize) {
  if (framesize>6) {
    FXbool is_track_gain=false;
    if (comparecase((const FXchar*)buffer+p,"track\0",6)==0) {
      is_track_gain=true;
      }
    else if (comparecase((const FXchar*)buffer+p,"album\0",6)) {
      return;
      }

    p+=6;
    framesize-=6;

    while(framesize) {
      const FXuchar & type = buffer[p];
      FXdouble        gain = (double)(((signed char)(buffer[p+1]) << 8) | buffer[p+2]) / 512.0;
      const FXuchar & bits = buffer[p+3];
      FXdouble        peak = 0.0;
      FXint nbytes         = (bits+7) / 8;
      if (type==1) {
        if (bits>0)  {
          peak+=buffer[p+4];
          if (bits>8) {
            peak+=buffer[p+5] / 256.0;
            if (bits>16) {
              peak+=buffer[p+6] / 65536.0;
              }
            }
          peak = peak / (double)(1<<((bits-1)&7));
          }
        if (is_track_gain) {
          replaygain.track = gain;
          }
        else {
          replaygain.album = gain;
          }
        break;
        }
      p+=4+nbytes;
      framesize-=4+nbytes;
      }
    }
  }


static FXint strwlen(const FXchar * str,FXint len) {
  const FXnchar * wstr = (const FXnchar*)str;

  for (FXint i=0;i<(len/2);i++){
    if (wstr[i]==0) return i*2;
    }
  return len;
  }


FXbool ID3V2::parse_text(FXint encoding,const FXchar * buffer,FXint length,FXString & text){
  switch(encoding) {

    case ISO_8859_1 :
      {
        FX88591Codec codec;
        FXint n = codec.mb2utflen(buffer,length);
        if (n>0) {
          text.length(n);
          codec.mb2utf(text.text(),text.length(),buffer,length);
          }
      } break;

    case UTF16_BOM  :
      {
        FXUTF16Codec codec;
        FXint n = codec.mb2utflen(buffer,length);
        if (n>0) {
          text.length(n);
          codec.mb2utf(text.text(),text.length(),buffer,length);
          }
      } break;

    case UTF16      :
      {
        FXUTF16BECodec codec;
        FXint n = codec.mb2utflen(buffer,length);
        if (n>0) {
          text.length(n);
          codec.mb2utf(text.text(),text.length(),buffer,length);
          }

      } break;

    case UTF8      :
      {
        FXUTF8Codec codec;
        FXint n = codec.mb2utflen(buffer,length);
        if (n>0) {
          text.length(n);
          codec.mb2utf(text.text(),text.length(),buffer,length);
          }

      } break;
    default: return false;
    }
  return true;
  }





void ID3V2::parse_comment_frame(FXint framesize) {
  FXString key,field;

  const FXuchar & encoding = buffer[p];
  const FXchar* textstart  = (const FXchar*)(buffer+p+4);
  const FXint   textlength = framesize - 4;

  if (encoding==UTF16_BOM || encoding==UTF16) {
    FXint ksize = strwlen(textstart,textlength);
    FXint vsize = strwlen(textstart+ksize+2,textlength-ksize-2);
    parse_text(encoding,textstart,ksize,key);
    parse_text(encoding,textstart+ksize+2,vsize,field);
    }
  else {
    FXint ksize = strnlen(textstart,textlength);
    FXint vsize = strnlen(textstart+ksize+1,textlength-ksize-1);
    parse_text(encoding,textstart,ksize,key);
    parse_text(encoding,textstart+ksize+1,vsize,field);
    }

  FXString comment = key + " " + field;
  if (comment.find("iTunSMPB")>=0) {
    comment.simplify().scan("iTunSMPB %*x %hx %hx %lx",&padstart,&padend,&length);
    GM_DEBUG_PRINT("[id3v2] found iTunSMPB (padding %d %d, length %ld)\n",padstart,padend,length);
    }
  }


void ID3V2::parse_text_frame(FXuint frameid,FXint framesize) {
  FXString text;
  const FXuchar & encoding = buffer[p];

  parse_text(encoding,(const FXchar*)buffer+p+1,framesize-1,text);

  GM_DEBUG_PRINT("[id3v2] text: \"%s\"\n",text.text());

  switch(frameid) {
    case TP1  :
    case TPE1 : artist.adopt(text); break;
    case TAL  :
    case TALB : album.adopt(text); break;
    case TT2  :
    case TIT2 : title.adopt(text); break;
    default   : break;
    }
  }

void ID3V2::parse_frame() {
  FXuint frameid;
  FXint  framesize;
  FXbool skip=false;

  if (version==2)
    frameid = DEFINE_FRAME_V2(buffer[p+0],buffer[p+1],buffer[p+2]);
  else
    frameid = DEFINE_FRAME(buffer[p+0],buffer[p+1],buffer[p+2],buffer[p+3]);

  switch(version){
    case 0  :
    case 1  :
    case 2  : framesize = (buffer[p+3]<<16) | (buffer[p+4]<<8) | (buffer[p+5]); break;
    case 3  : framesize = ID3_INT32(buffer+p+4); break;
    case 4  : framesize = ID3_SYNCSAFE_INT32(buffer+p+4); break;
    default : FXASSERT(0); p=size; return; break;
    };

  if (version==2)
    GM_DEBUG_PRINT("[id3v2] frame %c%c%c\n",buffer[p+0],buffer[p+1],buffer[p+2]);
  else
    GM_DEBUG_PRINT("[id3v2] frame %c%c%c%c\n",buffer[p+0],buffer[p+1],buffer[p+2],buffer[p+3]);


  if (version==2) {
    p+=6;
    }
  else {
    FXint extra=0;
    if (buffer[9]&FRAME_COMPRESSED) {
      extra+=4;
      skip=true;
      }
    if (buffer[9]&FRAME_ENCRYPTED) {
      extra+=1;
      skip=true;
      }
    if (buffer[9]&FRAME_GROUP) {
      extra+=1;
      }
    p+=10+extra;
    }

  if (!skip) {
    switch(frameid) {
      case TP1  :
      case TPE1 :
      case TAL  :
      case TALB :
      case TT2  :
      case TIT2 : parse_text_frame(frameid,framesize); break;
      case RVA2 : parse_rva2_frame(framesize); break;
      case PRIV : parse_priv_frame(framesize); break;
      case COMM : parse_comment_frame(framesize); break;
      case 0    : p=size; return; break;
      default   : break;
      };
    }
  p+=framesize;
  }


FXbool ID3V2::empty() const {
  return (artist.empty() && album.empty() && title.empty());
  }

}












