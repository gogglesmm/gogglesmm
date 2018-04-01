/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2018 by Sander Jansen. All Rights Reserved      *
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
#include <math.h>
#include <errno.h>

#include "gmdefs.h"
#include "gmutils.h"
#include "GMTrack.h"
#include "GMCover.h"
#include "GMTag.h"
#include "GMAudioPlayer.h"

/// TagLib


#include <fileref.h>
#include <tstring.h>
#include <id3v1tag.h>
#include <id3v1genres.h>
#include <id3v2tag.h>
#include <id3v2framefactory.h>
#include <mpegfile.h>
#include <vorbisfile.h>
#include <opusfile.h>
#include <tdebuglistener.h>
#include <flacfile.h>
#include <apetag.h>
#include <textidentificationframe.h>
#include <attachedpictureframe.h>
#include <unsynchronizedlyricsframe.h>
#include <vorbisproperties.h>
#include <flacproperties.h>
#include <mp4file.h>
#include <mp4tag.h>
#include <mp4coverart.h>
#include <mp4properties.h>


#define TAGLIB_VERSION ((TAGLIB_PATCH_VERSION) + (TAGLIB_MINOR_VERSION*1000) + (TAGLIB_MAJOR_VERSION*100000))
#define TAGVERSION(major,minor,release) ((release)+(minor*1000)+(major*100000))

#if TAGLIB_VERSION >= TAGVERSION(1,10,0)
#include <synchronizedlyricsframe.h>
#endif



#include "FXPNGImage.h"
#include "FXJPGImage.h"


static FXbool to_int(const FXString & str,FXint & val){
  char * endptr=nullptr;
  errno=0;
  val=strtol(str.text(),&endptr,10);
  if (errno==0) {
    if (endptr==str.text())
      return false;
    return true;
    }
  return false;
  }


/// FlacPictureBlock structure.
struct FlacPictureBlock{
  FXString    mimetype;
  FXString    description;
  FXuint      type;
  FXuint      width;
  FXuint      height;
  FXuint      bps;
  FXuint      ncolors;
  FXuint      data_size;
  FXuchar*    data;
  };

#if TAGLIB_VERSION < TAGVERSION(1,11,0)

static FXbool gm_uint32_be(const FXuchar * block,FXuint & v) {
#if FOX_BIGENDIAN == 0
  ((FXuchar*)&v)[3]=block[0];
  ((FXuchar*)&v)[2]=block[1];
  ((FXuchar*)&v)[1]=block[2];
  ((FXuchar*)&v)[0]=block[3];
#else
  ((FXuchar*)&v)[3]=block[3];
  ((FXuchar*)&v)[2]=block[2];
  ((FXuchar*)&v)[1]=block[1];
  ((FXuchar*)&v)[0]=block[0];
#endif
  return true;
  }


static FXbool xiph_decode_bytevector(const TagLib::ByteVector & bytevector,FXuchar *& buffer,FXint & length) {
  if (bytevector.size()) {
    if (length==0)
      length = bytevector.size();

    allocElms(buffer,length);
    memcpy(buffer,bytevector.data(),length);
    if (gm_decode_base64(buffer,length))
      return true;

    freeElms(buffer);
    }
  return false;
  }

static FXbool xiph_decode_picture(const FXuchar * buffer,FXint len,FlacPictureBlock & picture,FXbool full=true){
  const FXuchar * p = buffer;
  const FXuchar * end = buffer+len;
  FXuint sz;

  gm_uint32_be(p,picture.type);

  if (!full)
    return true;

  p+=4;

  gm_uint32_be(p,sz);
  if (sz) {
    if (p+4+sz>end)
      return false;
    picture.mimetype.length(sz);
    picture.mimetype.assign((const FXchar*)p+4,sz);
    }
  p+=(4+sz);

  gm_uint32_be(p,sz);
  if (sz) {
    if (p+4+sz>end)
      return false;
    picture.description.length(sz);
    picture.description.assign((const FXchar*)p+4,sz);
    }
  p+=(4+sz);

  gm_uint32_be(p+0,picture.width);
  gm_uint32_be(p+4,picture.height);
  gm_uint32_be(p+8,picture.bps);
  gm_uint32_be(p+12,picture.ncolors);
  gm_uint32_be(p+16,picture.data_size);

  if (picture.data_size>0 && (p+20+picture.data_size)==end) {
    picture.data = (FXuchar*) p+20;
    return true;
    }
  return false;
  }


static FXint xiph_check_cover(const TagLib::ByteVector & bytevector){
  FlacPictureBlock picture;
  FXint     covertype = -1;
  FXuchar * buffer = nullptr;
  FXint     length = 8; // decode only 8 bytes
  if (xiph_decode_bytevector(bytevector,buffer,length)) {
    if (xiph_decode_picture(buffer,length,picture,false)) {
      covertype = picture.type;
      }
    freeElms(buffer);
    }
  return covertype;
  }

static GMCover * xiph_load_cover(const TagLib::ByteVector & bytevector) {
  FlacPictureBlock picture;
  GMCover * cover  = nullptr;
  FXuchar * buffer = nullptr;
  FXint     length = 0;
  if (xiph_decode_bytevector(bytevector,buffer,length)){
    if (xiph_decode_picture(buffer,length,picture)) {
      cover = new GMCover(picture.data,picture.data_size,picture.type,picture.description);
      }
    freeElms(buffer);
    }
  return cover;
  }

#endif

static GMCover * id3v2_load_cover(TagLib::ID3v2::AttachedPictureFrame * frame) {
  FXString mime = frame->mimeType().toCString(true);
  /// Skip File Icon
  if (frame->picture().size()==0 ||
      frame->type()==TagLib::ID3v2::AttachedPictureFrame::FileIcon ||
      frame->type()==TagLib::ID3v2::AttachedPictureFrame::OtherFileIcon ||
      frame->type()==TagLib::ID3v2::AttachedPictureFrame::ColouredFish) {
    return nullptr;
    }
  return new GMCover(frame->picture().data(),frame->picture().size(),frame->type());
  }

static FXbool id3v2_is_front_cover(TagLib::ID3v2::AttachedPictureFrame * frame){
  if (frame->type()==TagLib::ID3v2::AttachedPictureFrame::FrontCover)
    return true;
  else
    return false;
  }


GMCover* flac_load_cover_from_taglib(const TagLib::FLAC::Picture * picture) {
  if (picture && picture->data().size()>0) {
    if (picture->type()==TagLib::FLAC::Picture::FileIcon ||
        picture->type()==TagLib::FLAC::Picture::OtherFileIcon ||
        picture->type()==TagLib::FLAC::Picture::ColouredFish) {
        return nullptr;
        }
    return new GMCover(picture->data().data(),picture->data().size(),picture->type(),picture->description().toCString(true));
    }
  return nullptr;
  }

GMCover* flac_load_frontcover_from_taglib(const TagLib::FLAC::Picture * picture) {
  if (picture && picture->type()==TagLib::FLAC::Picture::FrontCover && picture->data().size()>0) {
    return new GMCover(picture->data().data(),picture->data().size(),picture->type(),picture->description().toCString(true));
    }
  return nullptr;
  }



// For conversion from UTF16 to UTF32
static const FXint SURROGATE_OFFSET=0x10000-(0xD800<<10)-0xDC00;


static FXbool gm_taglib_string(const TagLib::String & src,FXString & dst) {
  // Method 1 - Uses 1 extra buffer
  // const TagLib::ByteVector b = src.data(TagLib::String::UTF8);
  // dst.assign(b.data(),b.size());
  // dst.trim();

  // Method 2 - Use 2 extra buffers (ByteVector -> std::string -> c_str)
  // dst = src.toCString(true);
  // dst.trim();

  // Method 3 - Use FOX's utf16->utf8 and copy directly into FXString
  const int slen = src.size();

  /* Determine Number of Bytes Needed */
  FXint p=0;
  FXint q=0;
  FXwchar w;

  while(q<slen){
    w=src[q++];
    if(__likely(w<0x80)){ p+=1; continue; }
    if(__likely(w<0x800)){ p+=2; continue; }
    if(__likely(!FXISSEQUTF16(w))){ p+=3; continue; }
    if(__likely(FXISLEADUTF16(w) && q<slen && FXISFOLLOWUTF16(src[q++]))){ p+=4; continue; }
    break;
    }

  dst.length(p);
  const FXint dlen = dst.length();

  /* Copy Bytes */
  p=q=0;
  while(q<slen){
    w=src[q++];
    if(__likely(w<0x80)){
      if(__unlikely(p>=dlen)) break;
      dst[p++]=w;
      continue;
      }
    if(__likely(w<0x800)){
      if(__unlikely(p+1>=dlen)) break;
      dst[p++]=(w>>6)|0xC0;
      dst[p++]=(w&0x3F)|0x80;
      continue;
      }
    if(__likely(!FXISSEQUTF16(w))){
      if(__unlikely(p+2>=dlen)) break;
      dst[p++]=(w>>12)|0xE0;
      dst[p++]=((w>>6)&0x3F)|0x80;
      dst[p++]=(w&0x3F)|0x80;
      continue;
      }
    if(__likely(FXISLEADUTF16(w) && q<slen && FXISFOLLOWUTF16(src[q]))){
      if(__unlikely(p+3>=dlen)) break;
      w=SURROGATE_OFFSET+(w<<10)+src[q++];
      dst[p++]=(w>>18)|0xF0;
      dst[p++]=((w>>12)&0x3F)|0x80;
      dst[p++]=((w>>6)&0x3F)|0x80;
      dst[p++]=(w&0x3F)|0x80;
      continue;
      }
    break;
    }
  dst.trim();

  return !dst.empty();
  }





 #if 0

static void gm_strip_tags(TagLib::File * file,FXuint opts) {
  TagLib::MPEG::File * mpeg = dynamic_cast<TagLib::MPEG::File*>(file);
  if (mpeg) {
    int tags = TagLib::MPEG::NoTags;

    if (opts&TAG_STRIP_ID3v1)
      tags|=TagLib::MPEG::ID3v1;
    if (opts&TAG_STRIP_ID3v2)
      tags|=TagLib::MPEG::ID3v2;
    if (opts&TAG_STRIP_APE)
      tags|=TagLib::MPEG::APE;

    if (tags)
      mpeg->strip(tags);
    }
  }
#endif


/******************************************************************************/

GMFileTag::GMFileTag() :
    file(nullptr),
    tag(nullptr),
    mp4(nullptr),
    xiph(nullptr),
    id3v2(nullptr),
    ape(nullptr) {
  /// TODO
  }

GMFileTag::~GMFileTag() {
  if (file) delete file;
  }


FXbool GMFileTag::open(const FXString & filename,FXuint opts) {

  file = TagLib::FileRef::create(filename.text(),(opts&FILETAG_AUDIOPROPERTIES));
  if (file==nullptr || !file->isValid() || file->tag()==nullptr) {
    if (file) {
      delete file;
      file=nullptr;
      }
    return false;
    }

  TagLib::MPEG::File        * mpgfile   = nullptr;
  TagLib::Ogg::Vorbis::File * oggfile   = nullptr;
  TagLib::Ogg::Opus::File   * opusfile  = nullptr;
  TagLib::FLAC::File        * flacfile  = nullptr;
  TagLib::MP4::File         * mp4file   = nullptr;

  tag = file->tag();

  if ((oggfile = dynamic_cast<TagLib::Ogg::Vorbis::File*>(file))!=nullptr) {
    xiph=oggfile->tag();
    }
  else if ((flacfile = dynamic_cast<TagLib::FLAC::File*>(file))!=nullptr){
    xiph=flacfile->xiphComment();
    id3v2=flacfile->ID3v2Tag();
    }
  else if ((mpgfile = dynamic_cast<TagLib::MPEG::File*>(file))!=nullptr){
    id3v2=mpgfile->ID3v2Tag();
    ape=mpgfile->APETag();
    }
  else if ((mp4file = dynamic_cast<TagLib::MP4::File*>(file))!=nullptr){
    mp4=mp4file->tag();
    }
  else if ((opusfile = dynamic_cast<TagLib::Ogg::Opus::File*>(file))!=nullptr){
    xiph=opusfile->tag();
    }
  return true;
  }


FXbool GMFileTag::save() {
  return file->save();
  }


FXuchar GMFileTag::getFileType() const {
  TagLib::MP4::File       * mp4file    = nullptr;
  TagLib::AudioProperties * properties = file->audioProperties();
  if (properties) {
    if ((dynamic_cast<TagLib::Ogg::File*>(file))) {
      if (dynamic_cast<TagLib::Vorbis::Properties*>(properties))
        return FILETYPE_OGG_VORBIS;
      else if (dynamic_cast<TagLib::Ogg::Opus::Properties*>(properties))
        return FILETYPE_OGG_OPUS;
      }
    else if (dynamic_cast<TagLib::FLAC::File*>(file)){
      if (dynamic_cast<TagLib::FLAC::Properties*>(properties))
        return FILETYPE_FLAC;
      }
    else if (dynamic_cast<TagLib::MPEG::File*>(file)){
      return FILETYPE_MP3;
      }
    else if ((mp4file = dynamic_cast<TagLib::MP4::File*>(file))){
      TagLib::MP4::Properties * mp4prop = dynamic_cast<TagLib::MP4::Properties*>(properties);
      if (mp4prop) {
        switch(mp4prop->codec()) {
          case TagLib::MP4::Properties::AAC : return FILETYPE_MP4_AAC; break;
          case TagLib::MP4::Properties::ALAC: return FILETYPE_MP4_ALAC; break;
          default: break;
          }
        }
      }
    }
  return FILETYPE_UNKNOWN;
  }


FXbool GMFileTag::trimList(FXStringList & list) const {
  for (FXint i=list.no()-1;i>=0;i--){
    list[i].trim();
    if (list[i].empty())
      list.erase(i);
    }
  return list.no()>0;
  }


void GMFileTag::xiph_add_field(const FXchar * field,const FXString & value) {
  FXASSERT(field);
  FXASSERT(xiph);
  if (!value.empty())
    xiph->addField(field,TagLib::String(value.text(),TagLib::String::UTF8),false);
  }

void GMFileTag::xiph_update_field(const FXchar * field,const FXString & value) {
  FXASSERT(field);
  FXASSERT(xiph);
  if (!value.empty())
    xiph->addField(field,TagLib::String(value.text(),TagLib::String::UTF8),true);
  else
#if TAGLIB_VERSION >= TAGVERSION(1,11,0)
    xiph->removeFields(field);
#else
    xiph->removeField(field); // deprecated
#endif
  }


void GMFileTag::xiph_update_field(const FXchar * field,const FXStringList & list) {
  FXASSERT(field);
  FXASSERT(xiph);
#if TAGLIB_VERSION >= TAGVERSION(1,11,0)
  xiph->removeFields(field);
#else
  xiph->removeField(field); // deprecated
#endif
  for (FXint i=0;i<list.no();i++) {
    xiph->addField(field,TagLib::String(list[i].text(),TagLib::String::UTF8),false);
    }
  }


FXbool GMFileTag::xiph_get_field(const FXchar * field,FXString & value) const {
  FXASSERT(field);
  FXASSERT(xiph);
  if (xiph->contains(field)) {
    return gm_taglib_string(xiph->fieldListMap()[field].front(),value);
    }
  else {
    value.clear();
    return false;
    }
  }

FXbool GMFileTag::xiph_get_field(const FXchar * field,FXStringList & list)  const{
  FXASSERT(field);
  FXASSERT(xiph);
  if (xiph->contains(field)) {
    const TagLib::StringList & fieldlist = xiph->fieldListMap()[field];
    list.no(fieldlist.size());
    FXint item=0;
    for(TagLib::StringList::ConstIterator it = fieldlist.begin(); it != fieldlist.end(); it++) {
      gm_taglib_string(*it,list[item++]);
      }
    return trimList(list);
    }
  else {
    list.clear();
    return false;
    }
  }

FXbool GMFileTag::ape_get_field(const FXchar * field,FXString & value)  const{
  FXASSERT(field);
  FXASSERT(ape);
  if (ape->itemListMap().contains(field) && !ape->itemListMap()[field].isEmpty()){
    return gm_taglib_string(ape->itemListMap()[field].toString(),value);
    }
  else {
    value.clear();
    return false;
    }
  }

FXbool GMFileTag::ape_get_field(const FXchar * field,FXStringList & list)  const{
  FXASSERT(field);
  FXASSERT(ape);
  if (ape->itemListMap().contains(field)) {
    const TagLib::StringList fieldlist = ape->itemListMap()[field].toStringList();
    list.no(fieldlist.size());
    FXint item=0;
    for(TagLib::StringList::ConstIterator it = fieldlist.begin(); it != fieldlist.end(); it++) {
      gm_taglib_string(*it,list[item++]);
      }
    return trimList(list);
    }
  else {
    list.clear();
    return false;
    }
  }

void GMFileTag::ape_update_field(const FXchar * field,const FXString & value) {
  FXASSERT(field);
  FXASSERT(ape);
  if (!value.empty())
    ape->addValue(field,TagLib::String(value.text(),TagLib::String::UTF8),true);
  else
    ape->removeItem(field);
  }


void GMFileTag::ape_update_field(const FXchar * field,const FXStringList & list) {
  FXASSERT(field);
  FXASSERT(ape);
  ape->removeItem(field);

  TagLib::StringList values;
  for (FXint i=0;i<list.no();i++) {
    values.append(TagLib::String(list[i].text(),TagLib::String::UTF8));
    }

  ape->setItem(field,TagLib::APE::Item(field,values));
  }


void GMFileTag::id3v2_update_field(const FXchar * field,const FXString & value) {
  FXASSERT(field);
  FXASSERT(id3v2);
  if (value.empty()) {
    id3v2->removeFrames(field);
    }
  else if (id3v2->frameListMap().contains(field) && !id3v2->frameListMap()[field].isEmpty()) {
    id3v2->frameListMap()[field].front()->setText(TagLib::String(value.text(),TagLib::String::UTF8));
    }
  else {
    TagLib::ID3v2::TextIdentificationFrame *frame = new TagLib::ID3v2::TextIdentificationFrame(field,TagLib::ID3v2::FrameFactory::instance()->defaultTextEncoding());
    frame->setText(TagLib::String(value.text(),TagLib::String::UTF8) );
    id3v2->addFrame(frame);
    }
  }

void GMFileTag::id3v2_update_field(const FXchar * field,const FXStringList & list) {
  FXASSERT(field);
  FXASSERT(id3v2);
  if (list.no()==0) {
    id3v2->removeFrames(field);
    }
  else {
    TagLib::ID3v2::TextIdentificationFrame * frame = nullptr;
    if (id3v2->frameListMap().contains(field) && !id3v2->frameListMap()[field].isEmpty()) {
      frame = dynamic_cast<TagLib::ID3v2::TextIdentificationFrame*>(id3v2->frameListMap()[field].front());
      }
    else {
      frame = new TagLib::ID3v2::TextIdentificationFrame(field,TagLib::ID3v2::FrameFactory::instance()->defaultTextEncoding());
      id3v2->addFrame(frame);
      }
    FXASSERT(frame);

    TagLib::StringList values;
    for (FXint i=0;i<list.no();i++) {
      values.append(TagLib::String(list[i].text(),TagLib::String::UTF8));
      }
    frame->setText(values);
    }
  }

FXbool  GMFileTag::id3v2_get_field(const FXchar * field,FXString & value) const{
  FXASSERT(field);
  FXASSERT(id3v2);
  if (id3v2->frameListMap().contains(field) && !id3v2->frameListMap()[field].isEmpty() ){
    return gm_taglib_string(id3v2->frameListMap()[field].front()->toString(),value);
    }
  else {
    value.clear();
    return false;
    }
  }

FXbool  GMFileTag::id3v2_get_field(const FXchar * field,FXStringList & list) const {
  FXASSERT(field);
  FXASSERT(id3v2);
  if (id3v2->frameListMap().contains(field) && !id3v2->frameListMap()[field].isEmpty() ) {
    TagLib::ID3v2::TextIdentificationFrame * frame = dynamic_cast<TagLib::ID3v2::TextIdentificationFrame*>(id3v2->frameListMap()[field].front());
    const TagLib::StringList fieldlist = frame->fieldList();
    list.no(fieldlist.size());
    FXint item=0;
    for(TagLib::StringList::ConstIterator it = fieldlist.begin(); it != fieldlist.end(); it++) {
      gm_taglib_string(*it,list[item++]);
      }
    return trimList(list);
    }
  else {
    list.clear();
    return false;
    }
  }


void GMFileTag::mp4_update_field(const FXchar * field,const FXString & value) {
  FXASSERT(field);
  FXASSERT(mp4);
  if (!value.empty())
    mp4->itemListMap().insert(field,TagLib::StringList(TagLib::String(value.text(),TagLib::String::UTF8)));
  else
    mp4->itemListMap().erase(field);
  }


void GMFileTag::mp4_update_field(const FXchar * field,const FXStringList & list) {
  FXASSERT(field);
  FXASSERT(mp4);
  if (list.no()==0) {
    mp4->itemListMap().erase(field);
    }
  else {
    TagLib::StringList values;
    for (FXint i=0;i<list.no();i++) {
      values.append(TagLib::String(list[i].text(),TagLib::String::UTF8));
      }
    mp4->itemListMap().insert(field,values);
    }
  }


FXbool GMFileTag::mp4_get_field(const FXchar * field,FXString & value) const {
  FXASSERT(field);
  FXASSERT(mp4);
  if (mp4->itemListMap().contains(field)) {
    value=mp4->itemListMap()[field].toStringList().toString(", ").toCString(true);
    value.trim();
    return !value.empty();
    }
  else {
    value.clear();
    return false;
    }
  }


FXbool GMFileTag::mp4_get_field(const FXchar * field,FXStringList & list) const{
  FXASSERT(field);
  FXASSERT(mp4);
  if (mp4->itemListMap().contains(field)) {
    const TagLib::StringList fieldlist = mp4->itemListMap()[field].toStringList();
    list.no(fieldlist.size());
    FXint item=0;
    for(TagLib::StringList::ConstIterator it = fieldlist.begin(); it != fieldlist.end(); it++) {
      gm_taglib_string(*it,list[item++]);
      }
    return trimList(list);
    }
  else {
    list.clear();
    return false;
    }
  }



/******************************************************************************/

void GMFileTag::setComposer(const FXString & composer) {
  if (xiph)
    xiph_update_field("COMPOSER",composer);
  if (id3v2)
    id3v2_update_field("TCOM",composer);
  if (mp4)
    mp4_update_field("\251wrt",composer);
  if (ape)
    ape_update_field("Composer",composer);
  }

void GMFileTag::getComposer(FXString & composer) const{
  if (xiph && xiph_get_field("COMPOSER",composer))
    return;
  else if (id3v2 && id3v2_get_field("TCOM",composer))
    return;
  else if (mp4 && mp4_get_field("\251wrt",composer))
    return;
  else if (ape && ape_get_field("Composer",composer))
    return;
  else
    composer.clear();
  }

void GMFileTag::setConductor(const FXString & conductor) {
  if (xiph)
    xiph_update_field("CONDUCTOR",conductor);
  if (id3v2)
    id3v2_update_field("TPE3",conductor);
  if (mp4)
    mp4_update_field("----:com.apple.iTunes:CONDUCTOR",conductor);
  if (ape)
    ape_update_field("Conductor",conductor);
  }

void GMFileTag::getConductor(FXString & conductor) const{
  if (xiph && xiph_get_field("CONDUCTOR",conductor))
    return;
  else if (id3v2 && id3v2_get_field("TPE3",conductor))
    return;
  else if (mp4 && mp4_get_field("----:com.apple.iTunes:CONDUCTOR",conductor))
    return;
  else if (ape && ape_get_field("Conductor",conductor))
    return;
  else
    conductor.clear();
  }


void GMFileTag::setAlbumArtist(const FXString & albumartist) {
  if (xiph)
    xiph_update_field("ALBUMARTIST",albumartist);
  if (id3v2)
    id3v2_update_field("TPE2",albumartist);
  if (mp4)
    mp4_update_field("aART",albumartist);
  //FIXME ape?
  }


void GMFileTag::getAlbumArtist(FXString & albumartist) const{
  if (xiph && xiph_get_field("ALBUMARTIST",albumartist))
    return;
  else if (id3v2 && id3v2_get_field("TPE2",albumartist))
    return;
  else if (mp4 && mp4_get_field("aART",albumartist))
    return;
  else
    albumartist.clear();
  }


void GMFileTag::setLyrics(const FXString & lyrics) {
  if (xiph) {
    xiph_update_field("LYRICS",lyrics);
    }
  else if (id3v2) {

    // Always store as Unsynchronized Lyrics Frame
    TagLib::ID3v2::FrameList framelist = id3v2->frameListMap()["USLT"];
    if (!framelist.isEmpty()) {
      framelist.front()->setText(TagLib::String(lyrics.text(),TagLib::String::UTF8));
      }
    else {
      TagLib::ID3v2::UnsynchronizedLyricsFrame *frame = new TagLib::ID3v2::UnsynchronizedLyricsFrame(TagLib::ID3v2::FrameFactory::instance()->defaultTextEncoding());
      frame->setText(TagLib::String(lyrics.text(),TagLib::String::UTF8));
      id3v2->addFrame(frame);
      }
    }
  else if (mp4) {
    mp4_update_field("\251lyr",lyrics);
    }
  }


void GMFileTag::getLyrics(FXString & lyrics) const{
  if (xiph && xiph_get_field("LYRICS",lyrics))
    return;
  else if (id3v2) {

    {
      const TagLib::ID3v2::FrameList framelist = id3v2->frameListMap()["USLT"];
      for(auto it = framelist.begin(); it != framelist.end(); it++) {
        TagLib::ID3v2::UnsynchronizedLyricsFrame * frame = dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame*>(*it);
        gm_taglib_string(frame->text(),lyrics);
        if (!lyrics.empty()) return;
        }
    }
#if TAGLIB_VERSION >= TAGVERSION(1,10,0)
    {
      const TagLib::ID3v2::FrameList framelist = id3v2->frameListMap()["SYLT"];
      for(auto it = framelist.begin(); it != framelist.end(); it++) {
        TagLib::ID3v2::SynchronizedLyricsFrame * frame = dynamic_cast<TagLib::ID3v2::SynchronizedLyricsFrame*>(*it);
        if (frame->type()==TagLib::ID3v2::SynchronizedLyricsFrame::Lyrics) {
          auto textlist = frame->synchedText();
          for(auto it = textlist.begin(); it != textlist.end(); it++) {
            FXString line;
            gm_taglib_string((*it).text,line);
            lyrics.append(line);
            }
          if (!lyrics.empty()) return;
          }
        }
    }
#endif

    }
  else if (mp4 && mp4_get_field("\251lyr",lyrics)) {
    return;
    }
  else
    lyrics.clear();
  }



void GMFileTag::setTags(const FXStringList & tags){
  if (xiph)
    xiph_update_field("GENRE",tags);
  if (id3v2)
    id3v2_update_field("TCON",tags);
  if (mp4)
    mp4_update_field("\251gen",tags);
  if (ape)
    ape_update_field("GENRE",tags);

  if (xiph==nullptr && id3v2==nullptr && mp4==nullptr && ape==nullptr) {
    if (tags.no())
      tag->setGenre(TagLib::String(tags[0].text(),TagLib::String::UTF8));
    else
      tag->setGenre(TagLib::String("",TagLib::String::UTF8));
    }
  }

void GMFileTag::getTags(FXStringList & tags) const {
  tags.clear();
  if (xiph && xiph_get_field("GENRE",tags))
    return;
  else if (id3v2 && id3v2_get_field("TCON",tags)) {
    parse_tagids(tags);
    }
  else if (mp4 && mp4_get_field("\251gen",tags))
    return;
  else if (ape && ape_get_field("GENRE",tags))
    return;
  else {
    FXString genre = tag->genre().toCString(true);
    genre.trim();
    if (!genre.empty()) tags.append(genre);
    }
  }

void GMFileTag::setComment(const FXString & value) {
  tag->setComment(TagLib::String(value.text(),TagLib::String::UTF8));
  }

void GMFileTag::getComment(FXString & value) const {
  gm_taglib_string(tag->comment(),value);
  }

void GMFileTag::setArtist(const FXString & value){
  tag->setArtist(TagLib::String(value.text(),TagLib::String::UTF8));
  }

void GMFileTag::getArtist(FXString& value) const{
  gm_taglib_string(tag->artist(),value);
  }

void GMFileTag::setAlbum(const FXString & value){
  tag->setAlbum(TagLib::String(value.text(),TagLib::String::UTF8));
  }

void GMFileTag::getAlbum(FXString& value) const{
  gm_taglib_string(tag->album(),value);
  }

void GMFileTag::setTitle(const FXString & value){
  tag->setTitle(TagLib::String(value.text(),TagLib::String::UTF8));
  }

void GMFileTag::getTitle(FXString& value) const {
  FXStringList vals;
  if (xiph && xiph_get_field("TITLE",vals)) {
    value.clear();
    if (vals.no()) {
      value=vals[0];
      for (FXint i=1;i<vals.no();i++) {
        value+=" - ";
        value+=vals[i];
        }
      }
    }
  else {
    gm_taglib_string(tag->title(),value);
    }
  }

void GMFileTag::parse_tagids(FXStringList & tags)const{
  FXint id;
  for (FXint i=tags.no()-1;i>=0;i--){
    if (to_int(tags[i],id)) {
      tags[i]=TagLib::ID3v1::genre(id).toCString(true);
      }
    }
  trimList(tags);
  }


void GMFileTag::setDiscNumber(FXushort disc) {
  if (xiph) {
    if (disc>0)
      xiph_update_field("DISCNUMBER",FXString::value("%d",disc));
    else
      xiph_update_field("DISCNUMBER",FXString::null);
    }
  if (id3v2) {
    if (disc>0)
      id3v2_update_field("TPOS",FXString::value("%d",disc));
    else
      id3v2_update_field("TPOS",FXString::null);
    }
  if (mp4) {
    if (disc>0)
      mp4->itemListMap().insert("disk",TagLib::MP4::Item(disc,0));
    else
      mp4->itemListMap().erase("disk");
    }
  }


static FXushort string_to_disc_number(const FXString & disc) {
  if (disc.empty())
    return 0;
  return FXMIN(disc.before('/').toUInt(),0xFFFF);
  }

FXushort GMFileTag::getDiscNumber() const{
  FXString disc;
  if (xiph && xiph_get_field("DISCNUMBER",disc)) {
    return string_to_disc_number(disc);
    }
  else if (id3v2 && id3v2_get_field("TPOS",disc)) {
    return string_to_disc_number(disc);
    }
  else if (mp4 && mp4->itemListMap().contains("disk")) {
    return FXMIN(mp4->itemListMap()["disk"].toIntPair().first,0xFFFF);
    }
  return 0;
  }

FXint GMFileTag::getTime() const{
  FXASSERT(file);
  TagLib::AudioProperties * properties = file->audioProperties();
  if (properties)
    return properties->length();
  else
    return 0;
  }

FXint GMFileTag::getBitRate() const{
  FXASSERT(file);
  TagLib::AudioProperties * properties = file->audioProperties();
  if (properties)
    return properties->bitrate();
  else
    return 0;
  }

FXint GMFileTag::getSampleRate() const{
  FXASSERT(file);
  TagLib::AudioProperties * properties = file->audioProperties();
  if (properties)
    return properties->sampleRate();
  else
    return 0;
  }


FXint GMFileTag::getChannels() const{
  FXASSERT(file);
  TagLib::AudioProperties * properties = file->audioProperties();
  if (properties)
    return properties->channels();
  else
    return 0;
  }


FXint GMFileTag::getSampleSize() const{
  FXASSERT(file);
  TagLib::FLAC::File * flacfile = dynamic_cast<TagLib::FLAC::File*>(file);
  if (flacfile && flacfile->audioProperties()) {
    return flacfile->audioProperties()->sampleWidth();
    }
  else
    return 0;
  }



void GMFileTag::setTrackNumber(FXushort track) {
  tag->setTrack(track);
  }

FXushort GMFileTag::getTrackNumber() const{
  return FXMIN(tag->track(),0xFFFF);
  }

void GMFileTag::setYear(FXint year) {
  tag->setYear(year);
  }

FXint GMFileTag::getYear()const {
  return tag->year();
  }

GMCover * GMFileTag::getFrontCover() const {
  TagLib::FLAC::File * flacfile = dynamic_cast<TagLib::FLAC::File*>(file);
  if (flacfile) {
    const TagLib::List<TagLib::FLAC::Picture*> picturelist = flacfile->pictureList();
    for(TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = picturelist.begin(); it != picturelist.end(); it++) {
      GMCover * cover = flac_load_frontcover_from_taglib((*it));
      if (cover) return cover;
      }
    }
  else if (id3v2) {
    TagLib::ID3v2::FrameList framelist = id3v2->frameListMap()["APIC"];
    if(!framelist.isEmpty()){
      /// First Try Front Cover
      for(TagLib::ID3v2::FrameList::Iterator it = framelist.begin(); it != framelist.end(); it++) {
        TagLib::ID3v2::AttachedPictureFrame * frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(*it);
        FXASSERT(frame);
        if (id3v2_is_front_cover(frame)) {
          GMCover * cover = id3v2_load_cover(frame);
          if (cover) return cover;
          }
        }
      for(TagLib::ID3v2::FrameList::Iterator it = framelist.begin(); it != framelist.end(); it++) {
        TagLib::ID3v2::AttachedPictureFrame * frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(*it);
        FXASSERT(frame);
        GMCover * cover = id3v2_load_cover(frame);
        if (cover) return cover;
        }
      }
    }
  else if (xiph) {
#if TAGLIB_VERSION >= TAGVERSION(1,11,0)
    const TagLib::List<TagLib::FLAC::Picture*> picturelist = xiph->pictureList();
    for(TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = picturelist.begin(); it != picturelist.end(); it++) {
      GMCover * cover = flac_load_frontcover_from_taglib((*it));
      if (cover) return cover;
      }
#else
    if (xiph->contains("METADATA_BLOCK_PICTURE")) {
      const TagLib::StringList & coverlist = xiph->fieldListMap()["METADATA_BLOCK_PICTURE"];
      for(TagLib::StringList::ConstIterator it = coverlist.begin(); it != coverlist.end(); it++) {
        const TagLib::ByteVector & bytevector = (*it).data(TagLib::String::Latin1);
        if (xiph_check_cover(bytevector)==GMCover::FrontCover){
          GMCover* cover = xiph_load_cover(bytevector);
          if (cover) return cover;
         }
        }
      }
#endif
    }
  else if (mp4) { /// MP4
    if (mp4->itemListMap().contains("covr")) {
      TagLib::MP4::CoverArtList coverlist = mp4->itemListMap()["covr"].toCoverArtList();
      for(TagLib::MP4::CoverArtList::Iterator it = coverlist.begin(); it != coverlist.end(); it++) {
        if (it->data().size())
          return new GMCover(it->data().data(),it->data().size());
        }
      }
    }
  return nullptr;
  }

FXint GMFileTag::getCovers(GMCoverList & covers) const {
  TagLib::FLAC::File * flacfile = dynamic_cast<TagLib::FLAC::File*>(file);
  if (flacfile) {
    const TagLib::List<TagLib::FLAC::Picture*> picturelist = flacfile->pictureList();
    for(TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = picturelist.begin(); it != picturelist.end(); it++) {
      GMCover * cover = flac_load_cover_from_taglib((*it));
      if (cover) covers.append(cover);
      }
    if (covers.no()) return covers.no();
    }
  else if (xiph) {
#if TAGLIB_VERSION >= TAGVERSION(1,11,0)
    const TagLib::List<TagLib::FLAC::Picture*> picturelist = xiph->pictureList();
    for(TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = picturelist.begin(); it != picturelist.end(); it++) {
      GMCover * cover = flac_load_cover_from_taglib((*it));
      if (cover) covers.append(cover);
      }
#else
    if (xiph->contains("METADATA_BLOCK_PICTURE")) {
      const TagLib::StringList & coverlist = xiph->fieldListMap()["METADATA_BLOCK_PICTURE"];
      for(TagLib::StringList::ConstIterator it = coverlist.begin(); it != coverlist.end(); it++) {
        const TagLib::ByteVector & bytevector = (*it).data(TagLib::String::Latin1);
        FXint type = xiph_check_cover(bytevector);
        if (type>=0 && type!=GMCover::FileIcon && type!=GMCover::OtherFileIcon && type!=GMCover::Fish){
          GMCover * cover = xiph_load_cover((*it).data(TagLib::String::Latin1));
          if (cover) covers.append(cover);
          }
        }
      }
#endif
    }
  else if (id3v2) {
    TagLib::ID3v2::FrameList framelist = id3v2->frameListMap()["APIC"];
    if(!framelist.isEmpty()){
      for(TagLib::ID3v2::FrameList::Iterator it = framelist.begin(); it != framelist.end(); it++) {
        TagLib::ID3v2::AttachedPictureFrame * frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(*it);
        GMCover * cover = id3v2_load_cover(frame);
        if (cover) covers.append(cover);
        }
      }
    }
  else if (mp4) {
    if (mp4->itemListMap().contains("covr")) {
      TagLib::MP4::CoverArtList coverlist = mp4->itemListMap()["covr"].toCoverArtList();
      for(TagLib::MP4::CoverArtList::Iterator it = coverlist.begin(); it != coverlist.end(); it++) {
        if (it->data().size())
          covers.append(new GMCover(it->data().data(),it->data().size(),0));
        }
      }
    }
  return covers.no();
  }


void GMFileTag::replaceCover(GMCover*cover,FXuint mode){
  TagLib::FLAC::File * flacfile = dynamic_cast<TagLib::FLAC::File*>(file);
  if (mode==COVER_REPLACE_TYPE) {
    if (flacfile) {
      const TagLib::List<TagLib::FLAC::Picture*> picturelist = flacfile->pictureList();
      for(TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = picturelist.begin(); it != picturelist.end();it++){
        if (cover->type==(*it)->type()) {
          flacfile->removePicture((*it));
          }
        }
      }
    else if (id3v2) {
      TagLib::ID3v2::FrameList framelist = id3v2->frameListMap()["APIC"];
      if(!framelist.isEmpty()){
        for(TagLib::ID3v2::FrameList::Iterator it = framelist.begin(); it != framelist.end(); it++) {
          TagLib::ID3v2::AttachedPictureFrame * frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(*it);
          if (frame->type()==cover->type) {
            id3v2->removeFrame(frame);
            }
          }
        }
      }
    else if (xiph) {
#if TAGLIB_VERSION >= TAGVERSION(1,11,0)
      const TagLib::List<TagLib::FLAC::Picture*> picturelist = xiph->pictureList();
      for(TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = picturelist.begin(); it != picturelist.end();it++){
        if (cover->type==(*it)->type()) {
          xiph->removePicture((*it));
          }
        }
#else
      TagLib::StringList & coverlist = const_cast<TagLib::StringList&>(xiph->fieldListMap()["METADATA_BLOCK_PICTURE"]);
      TagLib::StringList::Iterator it = coverlist.begin();
      while(it!=coverlist.end()){
        const TagLib::ByteVector & bytevector = (*it).data(TagLib::String::Latin1);
        FXint type = xiph_check_cover(bytevector);
        if (type==cover->type)
          it=coverlist.erase(it);
        else
          it++;
        }
#endif
      }
    else if (mp4) {
      // mp4 has no type information so we erase all
      mp4->itemListMap().erase("covr");
      }
    }
  else { // COVER_REPLACE_ALL
    clearCovers();
    }
  appendCover(cover);
  }

void GMFileTag::clearCovers() {
  TagLib::FLAC::File * flacfile = dynamic_cast<TagLib::FLAC::File*>(file);
  if (flacfile) {
    flacfile->removePictures();
    }
  else if (id3v2) {
    id3v2->removeFrames("APIC");
    }
  else if (xiph) {
#if TAGLIB_VERSION >= TAGVERSION(1,11,0)
    xiph->removeAllPictures();
#else
    xiph->removeField("METADATA_BLOCK_PICTURE");
#endif
    }
  else if (mp4) {
    mp4->itemListMap().erase("covr");
    }
  }


void GMFileTag::appendCover(GMCover* cover){
  GMImageInfo info;

  TagLib::FLAC::File * flacfile = dynamic_cast<TagLib::FLAC::File*>(file);
  if (flacfile) {
    if (cover->getImageInfo(info)) {
      TagLib::FLAC::Picture * picture = new TagLib::FLAC::Picture();
      picture->setWidth(info.width);
      picture->setHeight(info.height);
      picture->setColorDepth(info.bps);
      picture->setNumColors(info.colors);
      picture->setMimeType(TagLib::String(cover->mimeType().text(),TagLib::String::UTF8));
      picture->setDescription(TagLib::String(cover->description.text(),TagLib::String::UTF8));
      picture->setType(static_cast<TagLib::FLAC::Picture::Type>(cover->type));
      picture->setData(TagLib::ByteVector((const FXchar*)cover->data,cover->size));
      flacfile->addPicture(picture);
      }
    }
  else if (xiph) {
    if (cover->getImageInfo(info)) {
#if TAGLIB_VERSION >= TAGVERSION(1,11,0)
      TagLib::FLAC::Picture * picture = new TagLib::FLAC::Picture();
      picture->setWidth(info.width);
      picture->setHeight(info.height);
      picture->setColorDepth(info.bps);
      picture->setNumColors(info.colors);
      picture->setMimeType(TagLib::String(cover->mimeType().text(),TagLib::String::UTF8));
      picture->setDescription(TagLib::String(cover->description.text(),TagLib::String::UTF8));
      picture->setType(static_cast<TagLib::FLAC::Picture::Type>(cover->type));
      picture->setData(TagLib::ByteVector((const FXchar*)cover->data,cover->size));
      xiph->addPicture(picture);
#else
      FXString mimetype = cover->mimeType();
      FXint nbytes = 32 + cover->description.length() + mimetype.length() + cover->size;
      Base64Encoder base64(nbytes);
#if FOX_BIGENDIAN == 0
      base64.encode(swap32(cover->type));
      base64.encode(swap32(mimetype.length()));
      base64.encode((const FXuchar*)mimetype.text(),mimetype.length());
      base64.encode(swap32(cover->description.length()));
      base64.encode((const FXuchar*)cover->description.text(),cover->description.length());
      base64.encode(swap32(info.width));
      base64.encode(swap32(info.height));
      base64.encode(swap32(info.bps));
      base64.encode(swap32(info.colors));
      base64.encode(swap32(cover->size));
#else
      base64.encode(cover->type);
      base64.encode(mimetype.length());
      base64.encode((const FXuchar*)mimetype.text(),mimetype.length());
      base64.encode(cover->description.length());
      base64.encode(cover->description.text(),cover->description.length());
      base64.encode(info.width);
      base64.encode(info.height);
      base64.encode(info.bps);
      base64.encode(info.colors);
      base64.encode(cover->size);
#endif
      base64.encode(cover->data,cover->size);
      base64.finish();
      xiph_add_field("METADATA_BLOCK_PICTURE",base64.getOutput());
#endif
      }
    }
  else if (id3v2) {
    TagLib::ID3v2::AttachedPictureFrame * frame = new TagLib::ID3v2::AttachedPictureFrame();
    frame->setPicture(TagLib::ByteVector((const FXchar*)cover->data,cover->size));
    frame->setType(static_cast<TagLib::ID3v2::AttachedPictureFrame::Type>(cover->type));
    frame->setMimeType(TagLib::String(cover->mimeType().text(),TagLib::String::UTF8));
    frame->setDescription(TagLib::String(cover->description.text(),TagLib::String::UTF8));
    frame->setTextEncoding(TagLib::ID3v2::FrameFactory::instance()->defaultTextEncoding());
    id3v2->addFrame(frame);
    }
  else if (mp4) {
    TagLib::MP4::CoverArt::Format format;
    switch(cover->fileType()){
      case FILETYPE_PNG: format = TagLib::MP4::CoverArt::PNG; break;
      case FILETYPE_JPG: format = TagLib::MP4::CoverArt::JPEG; break;
      case FILETYPE_BMP: format = TagLib::MP4::CoverArt::BMP; break;
      case FILETYPE_GIF: format = TagLib::MP4::CoverArt::GIF; break;
      default: return; break;
      }
    if (!mp4->itemListMap().contains("covr")) {
      TagLib::MP4::CoverArtList list;
      list.append(TagLib::MP4::CoverArt(format,TagLib::ByteVector((const FXchar*)cover->data,cover->size)));
      mp4->itemListMap().insert("covr",list);
      }
    else {
      TagLib::MP4::CoverArtList list = mp4->itemListMap()["covr"].toCoverArtList();
      list.append(TagLib::MP4::CoverArt(format,TagLib::ByteVector((const FXchar*)cover->data,cover->size)));
      mp4->itemListMap().insert("covr",list);
      }
    }
}







GMAudioProperties::GMAudioProperties() :
  bitrate(0),
  samplerate(0),
  channels(0),
  samplesize(0),
  filetype(FILETYPE_UNKNOWN) {
  }


FXbool GMAudioProperties::load(const FXString & filename){
  GMFileTag tags;
  if (tags.open(filename,FILETAG_AUDIOPROPERTIES)){
    bitrate    = tags.getBitRate();
    samplerate = tags.getSampleRate();
    channels   = tags.getChannels();
    samplesize = tags.getSampleSize();
    filetype   = tags.getFileType();
    return true;
    }
  return false;
  }


class GMTagLibDebugListener : public TagLib::DebugListener {
private:
  GMTagLibDebugListener(const GMTagLibDebugListener &);
  GMTagLibDebugListener &operator=(const GMTagLibDebugListener &);
public:
  GMTagLibDebugListener(){}

#ifdef DEBUG
  virtual void printMessage(const TagLib::String &msg){
    fxmessage("%s\n",msg.toCString(true));
    }
#else
  virtual void printMessage(const TagLib::String&){}
#endif

  };


class GMStringHandler : public TagLib::ID3v1::StringHandler {
  public:
    static GMStringHandler * instance;
  protected:
    const FXTextCodec * codec;
  public:
    GMStringHandler(const FXTextCodec *c) : codec(c) {
      FXASSERT(codec!=nullptr);
      FXASSERT(instance==nullptr);
      GM_DEBUG_PRINT("[tag] id3v1 string handler: %s\n",codec->name());
      instance=this;
      }

      /*!
       * Decode a string from \a data.  The default implementation assumes that
       * \a data is an ISO-8859-1 (Latin1) character array.
       */
    virtual TagLib::String parse(const TagLib::ByteVector &in) const {
       TagLib::ByteVector utf;
       FXint n = codec->mb2utflen(in.data(),in.size());
       utf.resize(n);
       codec->mb2utf(utf.data(),utf.size(),in.data(),in.size());
       return TagLib::String(utf,TagLib::String::UTF8).stripWhiteSpace();
      }

      /*!
       * Encode a ByteVector with the data from \a s.  The default implementation
       * assumes that \a s is an ISO-8859-1 (Latin1) string.  If the string is
       * does not conform to ISO-8859-1, no value is written.
       *
       * \warning It is recommended that you <b>not</b> override this method, but
       * instead do not write an ID3v1 tag in the case that the data is not
       * ISO-8859-1.
       */
      //virtual ByteVector render(const String &s) const;

    virtual ~GMStringHandler() {
      instance=nullptr;
      }
    };



static GMTagLibDebugListener debuglistener;
GMStringHandler* GMStringHandler::instance = nullptr;

namespace GMTag {

void init(){
  TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(TagLib::String::UTF16);
  TagLib::setDebugListener(&debuglistener);
  }

void setID3v1Encoding(const FXTextCodec * codec){
  if (codec) {
    FXASSERT(GMStringHandler::instance==nullptr);
    TagLib::ID3v1::Tag::setStringHandler(new GMStringHandler(codec));
    }
  else {
    TagLib::ID3v1::Tag::setStringHandler(nullptr);
    if (GMStringHandler::instance) {
      delete GMStringHandler::instance;
      }
    }
  }


FXbool length(GMTrack & info) {
  GMFileTag tags;
  if (!tags.open(info.url,FILETAG_AUDIOPROPERTIES))
    return false;
  info.time = tags.getTime();
  return true;
  }




}








