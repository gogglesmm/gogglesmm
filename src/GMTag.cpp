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
#include <math.h>
#include <errno.h>

#include "gmdefs.h"
#include "gmutils.h"
#include "GMTrack.h"
#include "GMCover.h"
#include "GMTag.h"

/// TagLib
#include <fileref.h>
#include <tstring.h>
#include <id3v1genres.h>
#include <id3v2tag.h>
#include <id3v2framefactory.h>
#include <mpegfile.h>
#include <vorbisfile.h>
#include <flacfile.h>
#include <apetag.h>
#include <textidentificationframe.h>
#include <attachedpictureframe.h>
#ifdef TAGLIB_HAVE_MP4
#include "mp4file.h"
#include "mp4tag.h"
#include "mp4coverart.h"
#endif

#include "FXPNGImage.h"
#include "FXJPGImage.h"



class Base64Encoder {
private:
  static const FXchar base64[];
private:
  FXString out;
  FXuchar  buffer[3];
  FXint    nbuffer;
  FXint    index;
protected:
  void encodeChunks(const FXuchar * in,FXint len) {

    // resize buffer if needed
    FXint needed = 4*(len/3);
    if (index+needed>=out.length()) {
      out.length(out.length()+needed-(out.length()-index));
      }

    for (int i=0;i<len;i+=3) {
      out[index++]=base64[(in[i]>>2)];
      out[index++]=base64[((in[i]&0x3)<<4)|(in[i+1]>>4)];
      out[index++]=base64[((in[i+1]&0xf)<<2)|(in[i+2]>>6)];
      out[index++]=base64[(in[i+2]&0x3f)];
      }
    }
public:
  Base64Encoder(FXint source_length=0) : nbuffer(0), index(0){
    if (source_length)
      out.length(4*(source_length/3));
    }

  void encode(FXuint value) {
    encode((const FXuchar*)&value,4);
    }


  void encode(const FXuchar * in,FXint len) {
    FXint rindex=0;

    if (nbuffer) {
      for (rindex=0;(nbuffer<3)&&(rindex<len);rindex++)
        buffer[nbuffer++]=in[rindex];

      if (nbuffer<3)
        return;

      encodeChunks(buffer,3);
      len-=rindex;
      nbuffer=0;
      }

    FXint r = len % 3;
    FXint n = len - r;
    if (n) encodeChunks(in+rindex,n);

    for (int i=0;i<r;i++)
      buffer[i]=in[rindex+n+i];
    }

  void finish() {
    if (nbuffer) {
      if (index+4>=out.length()) {
        out.length(out.length()+4-(out.length()-index));
        }
      out[index++]=base64[(buffer[0]>>2)];
      if (nbuffer>1) {
        out[index++]=base64[((buffer[0]&0x3)<<4)|(buffer[1]>>4)];
        out[index++]=base64[((buffer[1]&0xf)<<2)|buffer[2]>>6];
        out[index++]='=';
        }
      else {
        out[index++]=base64[((buffer[0]&0x3)<<4)];
        out[index++]='=';
        out[index++]='=';
        }
      }
    }

  FXString & getOutput() { return out; }
  };

const FXchar Base64Encoder::base64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static FXbool to_int(const FXString & str,FXint & val){
  char * endptr=NULL;
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

//// Parse FLAC picture block from buffer
static GMCover * xiph_parse_flac_picture_block(const FXuchar * buffer,FXint len) {
  FlacPictureBlock picture;
  const FXuchar * p = buffer;
  FXuint sz;
  gm_uint32_be(p,picture.type);

  /// Skip useless icons
  if (picture.type==GMCover::FileIcon || picture.type==GMCover::OtherFileIcon ||
      picture.type==GMCover::Fish) {
    return NULL;
    }
  p+=4;

  gm_uint32_be(p,sz);
  picture.mimetype.length(sz);
  picture.mimetype.assign((const FXchar*)p+4,sz);

  p+=(4+sz);

  gm_uint32_be(p,sz);
  picture.description.length(sz);
  picture.description.assign((const FXchar*)p+4,sz);

  p+=(4+sz);

  gm_uint32_be(p+0,picture.width);
  gm_uint32_be(p+4,picture.height);
  gm_uint32_be(p+8,picture.bps);
  gm_uint32_be(p+12,picture.ncolors);
  gm_uint32_be(p+16,picture.data_size);

  if (picture.data_size>0) {
    picture.data = (FXuchar*) p+20;
    if (picture.data+picture.data_size>buffer+len)
      return NULL;
    return new GMCover(picture.data,picture.data_size,picture.type,picture.description);
    }
  return NULL;
  }


/// Load xiph cover
static GMCover * xiph_load_cover(const TagLib::ByteVector & tbuf) {
  GMCover * cover = NULL;
  if (tbuf.size()) {
    FXuchar * buffer=NULL;
    FXint   len=tbuf.size();

    allocElms(buffer,len);
    memcpy(buffer,tbuf.data(),len);
    if (gm_decode_base64(buffer,len)) {
      cover = xiph_parse_flac_picture_block(buffer,len);
      }
    freeElms(buffer);
    }
  return cover;
  }

static GMCover * id3v2_load_cover(TagLib::ID3v2::AttachedPictureFrame * frame) {
  FXString mime = frame->mimeType().toCString(true);
  /// Skip File Icon
  if (frame->type()==TagLib::ID3v2::AttachedPictureFrame::FileIcon ||
      frame->type()==TagLib::ID3v2::AttachedPictureFrame::OtherFileIcon ||
      frame->type()==TagLib::ID3v2::AttachedPictureFrame::ColouredFish) {
    return NULL;
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
  if (picture) {
    if (picture->type()==TagLib::FLAC::Picture::FileIcon ||
        picture->type()==TagLib::FLAC::Picture::OtherFileIcon ||
        picture->type()==TagLib::FLAC::Picture::ColouredFish) {
        return NULL;
        }
    return new GMCover(picture->data().data(),picture->data().size(),picture->type(),picture->description().toCString(true));
    }
  return NULL;
  }

GMCover* flac_load_frontcover_from_taglib(const TagLib::FLAC::Picture * picture) {
  if (picture && picture->type()==TagLib::FLAC::Picture::FrontCover) {
    return new GMCover(picture->data().data(),picture->data().size(),picture->type(),picture->description().toCString(true));
    }
  return NULL;
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
    file(NULL),
    tag(NULL),
    mp4(NULL),
    xiph(NULL),
    id3v2(NULL),
    ape(NULL) {
  /// TODO
  }

GMFileTag::~GMFileTag() {
  if (file) delete file;
  }


FXbool GMFileTag::open(const FXString & filename,FXuint opts) {

  file = TagLib::FileRef::create(filename.text(),(opts&FILETAG_AUDIOPROPERTIES));
  if (file==NULL || !file->isValid() || file->tag()==NULL) {
    if (file) {
      delete file;
      file=NULL;
      }
    return false;
    }

  TagLib::MPEG::File        * mpgfile   = NULL;
  TagLib::Ogg::Vorbis::File * oggfile   = NULL;
  TagLib::FLAC::File        * flacfile  = NULL;
#ifdef TAGLIB_HAVE_MP4
  TagLib::MP4::File         * mp4file   = NULL;
#endif
  tag = file->tag();

  if ((oggfile = dynamic_cast<TagLib::Ogg::Vorbis::File*>(file))) {
    xiph=oggfile->tag();
    }
  else if ((flacfile = dynamic_cast<TagLib::FLAC::File*>(file))){
    xiph=flacfile->xiphComment();
    id3v2=flacfile->ID3v2Tag();
    }
  else if ((mpgfile = dynamic_cast<TagLib::MPEG::File*>(file))){
    id3v2=mpgfile->ID3v2Tag();
    ape=mpgfile->APETag();
    }
#ifdef TAGLIB_HAVE_MP4
  else if ((mp4file = dynamic_cast<TagLib::MP4::File*>(file))){
    mp4=mp4file->tag();
    }
#endif
  return true;
  }

FXbool GMFileTag::save() {
  return file->save();
  }

void GMFileTag::xiph_update_field(const FXchar * field,const FXString & value) {
  FXASSERT(field);
  FXASSERT(xiph);
  if (!value.empty())
    xiph->addField(field,TagLib::String(value.text(),TagLib::String::UTF8),true);
  else
    xiph->removeField(field);
  }


void GMFileTag::xiph_update_field(const FXchar * field,const FXStringList & list) {
  FXASSERT(field);
  FXASSERT(xiph);
  xiph->removeField(field);
  for (FXint i=0;i<list.no();i++) {
    xiph->addField(field,TagLib::String(list[i].text(),TagLib::String::UTF8),false);
    }
  }


void  GMFileTag::xiph_get_field(const FXchar * field,FXString & value) const {
  FXASSERT(field);
  FXASSERT(xiph);
  if (xiph->contains(field))
    value=xiph->fieldListMap()[field].front().toCString(true);
  else
    value.clear();
  }

void GMFileTag::xiph_get_field(const FXchar * field,FXStringList & list)  const{
  FXASSERT(field);
  FXASSERT(xiph);
  if (xiph->contains(field)) {
    const TagLib::StringList & fieldlist = xiph->fieldListMap()[field];
    for(TagLib::StringList::ConstIterator it = fieldlist.begin(); it != fieldlist.end(); it++) {
      list.append(it->toCString(true));
      }
    }
  else {
    list.clear();
    }
  }

void GMFileTag::ape_get_field(const FXchar * field,FXString & value)  const{
  FXASSERT(field);
  FXASSERT(ape);
  if (ape->itemListMap().contains(field) && !ape->itemListMap()[field].isEmpty())
    value=ape->itemListMap()[field].toString().toCString(true);
  else
    value.clear();
  }

void GMFileTag::ape_get_field(const FXchar * field,FXStringList & list)  const{
  FXASSERT(field);
  FXASSERT(ape);
  if (ape->itemListMap().contains(field)) {
    TagLib::StringList fieldlist = ape->itemListMap()[field].toStringList();
    for(TagLib::StringList::ConstIterator it = fieldlist.begin(); it != fieldlist.end(); it++) {
      list.append(it->toCString(true));
      }
    }
  else {
    list.clear();
    }
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
    TagLib::ID3v2::TextIdentificationFrame * frame = NULL;
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

void  GMFileTag::id3v2_get_field(const FXchar * field,FXString & value) const{
  FXASSERT(field);
  FXASSERT(id3v2);
  if (id3v2->frameListMap().contains(field) && !id3v2->frameListMap()[field].isEmpty() )
    value=id3v2->frameListMap()[field].front()->toString().toCString(true);
  else
    value.clear();
  }

void  GMFileTag::id3v2_get_field(const FXchar * field,FXStringList & list) const {
  FXASSERT(field);
  FXASSERT(id3v2);
  if (id3v2->frameListMap().contains(field) && !id3v2->frameListMap()[field].isEmpty() ) {
    TagLib::ID3v2::TextIdentificationFrame * frame = dynamic_cast<TagLib::ID3v2::TextIdentificationFrame*>(id3v2->frameListMap()[field].front());
    TagLib::StringList fieldlist = frame->fieldList();
    for(TagLib::StringList::ConstIterator it = fieldlist.begin(); it != fieldlist.end(); it++) {
      list.append(it->toCString(true));
      }
    }
  else {
    list.clear();
    }
  }


void GMFileTag::mp4_update_field(const FXchar * field,const FXString & value) {
#ifdef TAGLIB_HAVE_MP4
  FXASSERT(field);
  FXASSERT(mp4);
  if (!value.empty())
    mp4->itemListMap()[field] = TagLib::StringList(TagLib::String(value.text(),TagLib::String::UTF8));
  else
    mp4->itemListMap().erase(field);
#endif
  }


void GMFileTag::mp4_update_field(const FXchar * field,const FXStringList & list) {
#ifdef TAGLIB_HAVE_MP4
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
    mp4->itemListMap()[field]=values;
    }
#endif
  }


void GMFileTag::mp4_get_field(const FXchar * field,FXString & value) const {
#ifdef TAGLIB_HAVE_MP4
  FXASSERT(field);
  FXASSERT(mp4);
  if (mp4->itemListMap().contains(field) && !mp4->itemListMap().isEmpty())
    value=mp4->itemListMap()[field].toStringList().toString(", ").toCString(true);
  else
    value.clear();
#else
  value.clear();
#endif
  }


void GMFileTag::mp4_get_field(const FXchar * field,FXStringList & list) const{
#ifdef TAGLIB_HAVE_MP4
  FXASSERT(field);
  FXASSERT(mp4);
  if (mp4->itemListMap().contains(field) && !mp4->itemListMap().isEmpty()) {
    TagLib::StringList fieldlist = mp4->itemListMap()[field].toStringList();
    for(TagLib::StringList::ConstIterator it = fieldlist.begin(); it != fieldlist.end(); it++) {
      list.append(it->toCString(true));
      }
    }
  else
    list.clear();
#else
  list.clear();
#endif
  }



/******************************************************************************/

void GMFileTag::setComposer(const FXString & composer) {
  if (xiph)
    xiph_update_field("COMPOSER",composer);
  else if (id3v2)
    id3v2_update_field("TCOM",composer);
  else if (mp4)
    mp4_update_field("\251wrt",composer);
  }

void GMFileTag::getComposer(FXString & composer) const{
  if (xiph)
    xiph_get_field("COMPOSER",composer);
  else if (id3v2)
    id3v2_get_field("TCOM",composer);
  else if (mp4)
    mp4_get_field("\251wrt",composer);
  else
    composer.clear();
  }

void GMFileTag::setConductor(const FXString & conductor) {
  if (xiph)
    xiph_update_field("CONDUCTOR",conductor);
  else if (id3v2)
    id3v2_update_field("TPE3",conductor);
  else if (mp4)
    mp4_update_field("----:com.apple.iTunes:CONDUCTOR",conductor);
  }

void GMFileTag::getConductor(FXString & conductor) const{
  if (xiph)
    xiph_get_field("CONDUCTOR",conductor);
  else if (id3v2)
    id3v2_get_field("TPE3",conductor);
  else if (mp4)
    mp4_get_field("----:com.apple.iTunes:CONDUCTOR",conductor);
  else
    conductor.clear();
  }


void GMFileTag::setAlbumArtist(const FXString & albumartist) {
  if (xiph)
    xiph_update_field("ALBUMARTIST",albumartist);
  else if (id3v2)
    id3v2_update_field("TPE2",albumartist);
  else if (mp4)
    mp4_update_field("aART",albumartist);
  }


void GMFileTag::getAlbumArtist(FXString & albumartist) const{
  if (xiph)
    xiph_get_field("ALBUMARTIST",albumartist);
  else if (id3v2)
    id3v2_get_field("TPE2",albumartist);
  else if (mp4)
    mp4_get_field("aART",albumartist);
  else
    albumartist.clear();

  if (albumartist.empty())
    getArtist(albumartist);
  }

void GMFileTag::setTags(const FXStringList & tags){
  if (xiph)
    xiph_update_field("GENRE",tags);
  else if (id3v2)
    id3v2_update_field("TCON",tags);
  else if (mp4)
    mp4_update_field("\251gen",tags);
  else if (ape)
    ape_update_field("GENRE",tags);
  else {
    if (tags.no())
      tag->setGenre(TagLib::String(tags[0].text(),TagLib::String::UTF8));
    else
      tag->setGenre(TagLib::String("",TagLib::String::UTF8));
    }
  }

void GMFileTag::getTags(FXStringList & tags) const {
  if (xiph)
    xiph_get_field("GENRE",tags);
  else if (id3v2) {
    id3v2_get_field("TCON",tags);
    parse_tagids(tags);
    }
  else if (mp4)
    mp4_get_field("\251gen",tags);
  else if (ape)
    ape_get_field("GENRE",tags);
  else
    tags.append(FXString(tag->genre().toCString(true)));
  }


void GMFileTag::setArtist(const FXString & value){
  tag->setArtist(TagLib::String(value.text(),TagLib::String::UTF8));
  }

void GMFileTag::getArtist(FXString& value) const{
  value=tag->artist().toCString(true);
  value.trim();
  }


void GMFileTag::setAlbum(const FXString & value){
  tag->setAlbum(TagLib::String(value.text(),TagLib::String::UTF8));
  }
void GMFileTag::getAlbum(FXString& value) const{
  value=tag->album().toCString(true);
  value.trim();
  }

void GMFileTag::setTitle(const FXString & value){
  tag->setTitle(TagLib::String(value.text(),TagLib::String::UTF8));
  }

void GMFileTag::getTitle(FXString& value) const {
  if (xiph) {
    FXStringList vals;
    xiph_get_field("TITLE",vals);
    value.clear();
    if (vals.no()) {
      for (FXint i=0;i<vals.no();i++) {
        vals[i].trim();
        if (!value.empty()) value+=" - ";
        value+=vals[i];
        }
      }
    }
  else {
    value=tag->title().toCString(true);
    value.trim();
    }
  }

void GMFileTag::parse_tagids(FXStringList & tags)const{
  FXint id;
  for (FXint i=tags.no()-1;i>=0;i--){
    if (to_int(tags[i],id)) {
      tags[i]=TagLib::ID3v1::genre(id).toCString(true);
      }
    }
  }


void GMFileTag::setDiscNumber(FXushort disc) {
  if (xiph) {
    if (disc>0)
      xiph_update_field("DISCNUMBER",FXString::value("%d",disc));
    else
      xiph_update_field("DISCNUMBER",FXString::null);
    }
  else if (id3v2) {
    if (disc>0)
      id3v2_update_field("TPOS",FXString::value("%d",disc));
    else
      id3v2_update_field("TPOS",FXString::null);
    }
#ifdef TAGLIB_HAVE_MP4
  else if (mp4) {
    if (disc>0)
      mp4->itemListMap()["disk"] = TagLib::MP4::Item(disc,0);
    else
      mp4->itemListMap().erase("disk");
    }
#endif
  }


static FXushort string_to_disc_number(const FXString & disc) {
  if (disc.empty())
    return 0;
  return FXMIN(disc.before('/').toUInt(),0xFFFF);
  }

FXushort GMFileTag::getDiscNumber() const{
  FXString disc;
  if (xiph) {
    xiph_get_field("DISCNUMBER",disc);
    return string_to_disc_number(disc);
    }
  else if (id3v2) {
    id3v2_get_field("TPOS",disc);
    return string_to_disc_number(disc);
    }
#ifdef TAGLIB_HAVE_MP4
  else if (mp4) {
    if (mp4->itemListMap().contains("disk"))
      return FXMIN(mp4->itemListMap()["disk"].toIntPair().first,0xFFFF);
    }
#endif
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
    if (xiph->contains("METADATA_BLOCK_PICTURE")) {
      const TagLib::StringList & coverlist = xiph->fieldListMap()["METADATA_BLOCK_PICTURE"];
      for(TagLib::StringList::ConstIterator it = coverlist.begin(); it != coverlist.end(); it++) {
        GMCover * cover = xiph_load_cover((*it).data(TagLib::String::UTF8));
        if (cover) return cover;
        }
      }
    }
#ifdef TAGLIB_HAVE_MP4
  else if (mp4) { /// MP4
    if (mp4->itemListMap().contains("covr")) {
      TagLib::MP4::CoverArtList coverlist = mp4->itemListMap()["covr"].toCoverArtList();
      for(TagLib::MP4::CoverArtList::Iterator it = coverlist.begin(); it != coverlist.end(); it++) {
        return new GMCover(it->data().data(),it->data().size());
        }
      }
    }
#endif
  return NULL;
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
    if (xiph->contains("METADATA_BLOCK_PICTURE")) {
      const TagLib::StringList & coverlist = xiph->fieldListMap()["METADATA_BLOCK_PICTURE"];
      for(TagLib::StringList::ConstIterator it = coverlist.begin(); it != coverlist.end(); it++) {
        GMCover * cover = xiph_load_cover((*it).data(TagLib::String::UTF8));
        if (cover) covers.append(cover);
        }
      }
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
#ifdef TAGLIB_HAVE_MP4
  else if (mp4) {
    if (mp4->itemListMap().contains("covr")) {
      TagLib::MP4::CoverArtList coverlist = mp4->itemListMap()["covr"].toCoverArtList();
      for(TagLib::MP4::CoverArtList::Iterator it = coverlist.begin(); it != coverlist.end(); it++) {
        covers.append(new GMCover(it->data().data(),it->data().size(),0));
        }
      }
    }
#endif
  return covers.no();
  }

void GMFileTag::replaceCover(GMCover*cover){
  TagLib::FLAC::File * flacfile = dynamic_cast<TagLib::FLAC::File*>(file);
  if (flacfile) {
    flacfile->removePictures();
    }
  else if (id3v2) {
    id3v2->removeFrames("APIC");
    }      
  else if (xiph) {
    xiph->removeField("METADATA_BLOCK_PICTURE");
    }
  appendCover(cover);
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
      xiph_update_field("METADATA_BLOCK_PICTURE",base64.getOutput());
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
  }












GMAudioProperties::GMAudioProperties() :
  bitrate(0),
  samplerate(0),
  channels(0) {
  }

FXbool GMAudioProperties::load(const FXString & filename){
  GMFileTag tags;
  if (tags.open(filename,FILETAG_AUDIOPROPERTIES)){
    bitrate    = tags.getBitRate();
    samplerate = tags.getSampleRate();
    channels   = tags.getChannels();
    return true;
    }
  return false;
  }


namespace GMTag {

void init(){
  TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(TagLib::String::UTF16);
  }


FXbool length(GMTrack & info) {
  GMFileTag tags;
  if (!tags.open(info.mrl,FILETAG_AUDIOPROPERTIES))
    return false;
  info.time = tags.getTime();
  return true;
  }




}








