/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2026 by Sander Jansen. All Rights Reserved      *
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
*                               ---                                            *
* SPDX-License-Identifier: GPL-3.0-or-later                                    *
********************************************************************************/
#include "ap_defs.h"
#include "ap_input_plugin.h"
#include "ap_id3v2.h"

#include <FXUTF8Codec.h>
#include <FXUTF16Codec.h>
#include <FX88591Codec.h>

namespace ap {

#define ID3_SYNCSAFE_UINT32(b) ((static_cast<FXuint>(*(b+0)&0x7f)<<21) | \
                                (static_cast<FXuint>(*(b+1)&0x7f)<<14) | \
                                (static_cast<FXuint>(*(b+2)&0x7f)<<7) | \
                                (static_cast<FXuint>(*(b+3)&0x7f)))

#define ID3_UINT32(b) ((static_cast<FXuint>(*(b + 0)) << 24) | \
                       (static_cast<FXuint>(*(b + 1)) << 16) | \
                       (static_cast<FXuint>(*(b + 2)) << 8) | \
                       (static_cast<FXuint>(*(b + 3)) << 0))


constexpr FXuint ID3V2_MAX_SIZE = 64 * 1024 * 1024;   /// 64MB seems reasonable

enum TextEncoding {
  ISO_8859_1 = 0,
  UTF16_BOM  = 1,
  UTF16      = 2,
  UTF8       = 3
  };


static FXint strwlen(const FXchar * str, FXint len) {
  auto* wstr = reinterpret_cast<const FXnchar*>(str);
  for (FXint i=0, c=0; i < len; i+=2, c++){
    if (wstr[c]==0) return i;
    }
  return len;
  }


void id3v2_unsync(FXuchar * src,FXuint & len) {
  FXuint k=0;
  for (FXuint i=0;i<len;i++) {
    src[k++]=src[i];
    if (src[i]==0xff && (i+1)<len && src[i+1]==0x00)
      i++;
    }
  len=k;
  }

FXbool id3v2_parse_text(FXint encoding,const FXchar * buffer,FXint length,FXString & text){
  switch(encoding) {

    case TextEncoding::ISO_8859_1 :
    {
      FX88591Codec codec;
      FXint n = codec.mb2utflen(buffer,length);
      if (n>0) {
        text.length(n);
        codec.mb2utf(text.text(),text.length(),buffer,length);
      }
    } break;

    case TextEncoding::UTF16_BOM  :
    {
      FXUTF16Codec codec;
      FXint n = codec.mb2utflen(buffer,length);
      if (n>0) {
        text.length(n);
        codec.mb2utf(text.text(),text.length(),buffer,length);
      }
    } break;

    case TextEncoding::UTF16      :
    {
      FXUTF16BECodec codec;
      FXint n = codec.mb2utflen(buffer,length);
      if (n>0) {
        text.length(n);
        codec.mb2utf(text.text(),text.length(),buffer,length);
      }

    } break;

    case TextEncoding::UTF8      :
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

void id3v2_parse_text_frame(const FXuchar * frame, FXint size, FXString & dst) {
  if (size > 2) {
    const FXuchar & text_encoding = frame[0]; // first byte indicates text encoding
    const FXchar*  text_start     = reinterpret_cast<const FXchar*>(frame + 1);
    id3v2_parse_text(text_encoding, text_start,size - 1,dst);
    GM_DEBUG_PRINT("[id3v2] text: \"%s\"\n", dst.text());
    }
  }

// void id3v2_parse_priv_frame(const FXuchar * frame, FXint size) {
//   FXString value;
//   const FXchar*  text_start = reinterpret_cast<const FXchar*>(frame);
//   FXint ksize = strnlen(text_start, size);
//   id3v2_parse_text(TextEncoding::UTF8, text_start,ksize,value);
//   GM_DEBUG_PRINT("[id3v2] PRIV (%ld): %s\n", size - ksize, value.text());
// }
//

void id3v2_parse_comment_frame(const FXuchar * frame, FXint size, FXString & key, FXString & field) {
  if (size > 4) {
    const FXuchar & encoding = frame[0];
    const FXchar* textstart  = reinterpret_cast<const FXchar*>(frame + 4);
    const FXint   textlength = size - 4;
    /* A Comment consists of a "short content description" followed by a null character,
     * followed by the full text string */

    if (encoding==TextEncoding::UTF16_BOM || encoding==TextEncoding::UTF16) {
      FXint ksize = strwlen(textstart, textlength);
      FXint vsize = strwlen(textstart + ksize + 2, textlength - ksize - 2);
      if (ksize)
        id3v2_parse_text(encoding,textstart,ksize,key);
      if (vsize)
        id3v2_parse_text(encoding,textstart + ksize + 2,vsize,field);
      }
    else {
      FXint ksize = strnlen(textstart, textlength);
      FXint vsize = strnlen(textstart + ksize + 1,textlength - ksize - 1);
      if (ksize)
        id3v2_parse_text(encoding,textstart,ksize,key);
      if (vsize)
        id3v2_parse_text(encoding,textstart + ksize + 1,vsize,field);
      }
    }
  }

void id3v2_rva2_frame(const FXuchar * frame, FXint framesize, ReplayGain & replaygain) {
  if (framesize > 6) {
    FXbool is_track_gain;

    auto* identifier = reinterpret_cast<const FXchar*>(frame);

    if (FXString::comparecase(identifier,"track\0",6)==0) {
      is_track_gain = true;
      }
    else if (FXString::comparecase(identifier,"album\0",6)) {
      is_track_gain = false;
      }
    else {
      /// nothing to do
      return;
    }

    frame += 6;
    framesize -= 6;

    while(framesize > 5) {
      const FXuchar & type = frame[0];
      FXdouble        gain = static_cast<FXdouble>(static_cast<FXshort>(frame[1]<<8 | frame[2])) / 512.0;
      const FXuchar & bits = frame[3];
      FXdouble        peak = 0.0;
      const FXint   nbytes = (bits+7) / 8;
      if (type==1) { /// 1 - Master Volume
        // if (bits>0)  {
        //   peak += static_cast<FXdouble>(frame[4]);
        //   if (bits>8) {
        //     peak += static_cast<FXdouble>(frame[5]) / 256.0;
        //     if (bits>16) {
        //       peak += static_cast<FXdouble>(frame[6]) / 65536.0;
        //     }
        //   }
        //   peak = peak / (double)(1<<((bits-1)&7));
        // }
        if (is_track_gain) {
          replaygain.track = gain;
        }
        else {
          replaygain.album = gain;
        }
        return;
      }
      framesize -= (4 + nbytes);
      frame += (4 + nbytes);
    }
  }
}


FXuint ID3V2::parse_frame(const FXuchar * buffer, const FXint size, const FXint version) {
  //GM_DEBUG_PRINT("parse_frame %ld %ld\n", size, version);
  FXuint frameid;
  FXuint framesize;
  FXint  headersize;
  FXint  extrasize = 0;
  FXbool skip=false;

  if (version <= 2) {

    if (size < 6)
      return size;

    frameid    = DEFINE_FRAME_V2(buffer[0],buffer[1],buffer[2]);
    framesize  = (buffer[3]<<16) | (buffer[4]<<8) | (buffer[5]);
    headersize = 6;

    if (framesize)
      GM_DEBUG_PRINT("[id3v2] frame %lu %c%c%c\n", framesize, static_cast<FXchar>(buffer[0]),
                                                              static_cast<FXchar>(buffer[1]),
                                                              static_cast<FXchar>(buffer[2]));
  }
  else {

    if (size < 10)
      return size;

    frameid = DEFINE_FRAME(buffer[0],buffer[1],buffer[2],buffer[3]);
    switch (version) {
      case 3  : framesize = ID3_UINT32(buffer+4); break;
      case 4  : framesize = ID3_SYNCSAFE_UINT32(buffer+4); break;
      default : return size; break;
    }
    headersize = 10;
    const FXuchar flags = buffer[9];

    if (framesize)
      GM_DEBUG_PRINT("[id3v2] frame %lu %c%c%c%c\n", framesize, static_cast<FXchar>(buffer[0]),
                                                              static_cast<FXchar>(buffer[1]),
                                                              static_cast<FXchar>(buffer[2]),
                                                              static_cast<FXchar>(buffer[3]));
    if (flags&FRAME_COMPRESSED) {
      extrasize += 4;
      skip=true;
    }
    if (flags&FRAME_ENCRYPTED) {
      extrasize += 1;
      skip=true;
    }
    if (flags&FRAME_GROUP) {
      extrasize += 1;
    }
  }


  // Skip bullshit
  if (framesize > ID3V2_MAX_SIZE)
    return headersize + framesize;

  // Check for corrupted data
  if (size - headersize - extrasize < static_cast<FXint>(framesize))
    return size;

  if (!skip && framesize) {
    const FXuchar * datastart = buffer + headersize + extrasize;

    switch(frameid) {
      case TP1  :
      case TPE1 : id3v2_parse_text_frame(datastart, static_cast<FXint>(framesize), artist);
                  break;

      case TAL  :
      case TALB : id3v2_parse_text_frame(datastart, static_cast<FXint>(framesize), album);
                  break;

      case TT2  :
      case TIT2 : id3v2_parse_text_frame(datastart, static_cast<FXint>(framesize), title);
                  break;

      case RVA2 : id3v2_rva2_frame(datastart, static_cast<FXint>(framesize), replaygain);
                  break;

      // case PRIV: id3v2_parse_priv_frame(datastart, static_cast<FXint>(framesize));
      //            break;

      case COMM :
        {
          FXString key, value;
          id3v2_parse_comment_frame(datastart, static_cast<FXint>(framesize), key, value);
          GM_DEBUG_PRINT("[id3v2] \"%s\" / \"%s\"\n", key.text(), value.text());
          if (key.length() + value.length() > 0) {
            FXString comment = key + " " + value;
            if (comment.find("iTunSMPB") >=0 ) {
              FXushort pstart, pend;
              FXlong plength;
              if (comment.simplify().scan("iTunSMPB %*x %hx %hx %lx",&pstart,&pend,&plength) == 3) {
                padstart = pstart;
                padend = pend;
                length = plength;;
                GM_DEBUG_PRINT("[id3v2] found iTunSMPB (padding %d %d, length %ld)\n",padstart,padend,length);
                }
              }
          }
        } break;
      default   : break;
    };
  }
  return headersize + framesize;
}


ID3V2 * ID3V2::parse(InputPlugin * input,const FXuchar * id, FXbool skip) {
  FXuchar info[6];

  // Read remaining header bytes
  if (input->read(info, 6) != 6)
    return nullptr;

  // id3v2 major version
  const FXuchar& version = id[3];
  const FXuchar  flags   = info[1];
  FXuint         size    = ID3_SYNCSAFE_UINT32(info+2);

  if (skip) {
    if (flags & ID3V2::HAS_FOOTER)
      input->position(size + 10,FXIO::Current);
    else
      input->position(size,FXIO::Current);
    return nullptr;
  }

  // don't bother with bad versions
  if (version > 4)
    return nullptr;

  FXuint minsize = 10;  // size of frame header
  if (flags & ID3V2::HAS_EXTENDED_HEADER)
    minsize += 4;

  // disallow bullshit
  if (size > ID3V2_MAX_SIZE || size < minsize)
    return nullptr;

  // Allocate Buffer
  FXuchar * buffer = nullptr;
  FXuint    offset = 0;
  if (!allocElms(buffer, size))
    return nullptr;

  // Read Data
  if (input->read(buffer, size) != size) {
    freeElms(buffer);
    return nullptr;
    }

  // Skip past optional footer
  if (flags & ID3V2::HAS_FOOTER) {
    input->position(10,FXIO::Current);
    }

  // Apply unsync to the whole buffer
  if (flags & ID3V2::HAS_UNSYNC)
    id3v2_unsync(buffer, size);

  // Ignore Extended Header if needed
  if (version >= 3 && flags&HAS_EXTENDED_HEADER) {

    if (size < 4) {
      freeElms(buffer);
      return nullptr;
    }

    FXuint extended_header_size;
    if (version==3)
      extended_header_size = ID3_UINT32(buffer);
    else
      extended_header_size = ID3_SYNCSAFE_UINT32(buffer);

    if (size < extended_header_size) {
      freeElms(buffer);
      return nullptr;
    }

    offset += extended_header_size;
    }

  auto* id3v2 = new ID3V2();
  while (offset < size && size - offset > 6) {
    offset += id3v2->parse_frame(buffer + offset, static_cast<FXint>(size - offset), version);
    }

  freeElms(buffer);
  return id3v2;
  }


FXbool ID3V2::empty() const {
  return (artist.empty() && album.empty() && title.empty());
  }

}
