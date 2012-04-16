/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2010-2010 by Sander Jansen. All Rights Reserved      *
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
#include "GMTrack.h"
#include "GMCover.h"
#include "GMTag.h"







/******************************************************************************/
/*            FLAC PICTURE LOADING                                            */
/******************************************************************************/



#if TAGLIB_VERSION < MKVERSION(1,7,0)

#if FOX_BIGENDIAN == 0
#define FLAC_LAST_BLOCK   0x80
#define FLAC_BLOCK_TYPE_MASK 0x7f
#define FLAC_BLOCK_TYPE(h) (h&0x7f)
#define FLAC_BLOCK_SIZE(h) ( ((h&0xFF00)<<8) | ((h&0xFF0000)>>8) | ((h&0xFF000000)>>24) )
#define FLAC_BLOCK_SET_TYPE(h,type) (h|=(type&FLAC_BLOCK_TYPE_MASK))
#define FLAC_BLOCK_SET_SIZE(h,size) (h|=(((size&0xFF)<<24) | ((size&0xFF0000)>>16) | ((size&0xFF00)<<8)))
#else
#define FLAC_LAST_BLOCK      0x80000000
#define FLAC_BLOCK_TYPE_MASK 0x7F000000
#define FLAC_BLOCK_TYPE(h)   ((h&0x7F000000)>>24)
#define FLAC_BLOCK_SIZE(h)   (h&0xFFFFFF)
#define FLAC_BLOCK_SET_TYPE(h,type) (h|=((type<<24)&FLAC_BLOCK_TYPE_MASK))
#define FLAC_BLOCK_SET_SIZE(h,size) (h|=(size&0xFFFFFF))
#endif


enum {
  FLAC_BLOCK_STREAMINFO     = 0,
  FLAC_BLOCK_PADDING        = 1,
  FLAC_BLOCK_VORBIS_COMMENT = 4,
  FLAC_BLOCK_PICTURE        = 6
  };


static FXbool gm_read_uint32_be(FXIO & io,FXuint & v) {
#if FOX_BIGENDIAN == 0
  FXuchar block[4];
  if (io.readBlock(block,4)!=4) return false;
  ((FXuchar*)&v)[3]=block[0];
  ((FXuchar*)&v)[2]=block[1];
  ((FXuchar*)&v)[1]=block[2];
  ((FXuchar*)&v)[0]=block[3];
#else
  if (io.readBlock(&v,4)!=4) return false;
#endif
  return true;
  }


static FXbool gm_read_string_be(FXIO & io,FXString & v) {
  FXuint len=0;
  gm_read_uint32_be(io,len);
  if (len>0) {
    v.length(len);
    if (io.readBlock(&v[0],len)!=len)
      return false;
    }
  return true;
  }

FXbool gm_flac_is_front_cover(FXIO & io) {
  FlacPictureBlock picture;
  gm_read_uint32_be(io,picture.type);
  if (picture.type==GMCover::FrontCover)
    return true;
  else
    return false;
  }

GMCover * gm_flac_parse_block_picture(FXIO & io,FXint scale,FXint crop) {
  GMCover*  cover=NULL;
  FlacPictureBlock picture;

  gm_read_uint32_be(io,picture.type);

  /// Skip useless icons
  if (picture.type==GMCover::FileIcon || picture.type==GMCover::OtherFileIcon ||
      picture.type==GMCover::Fish) {
    return NULL;
    }

  gm_read_string_be(io,picture.mimetype);
  gm_read_string_be(io,picture.description);
  gm_read_uint32_be(io,picture.width);
  gm_read_uint32_be(io,picture.height);
  gm_read_uint32_be(io,picture.bps);
  gm_read_uint32_be(io,picture.ncolors);
  gm_read_uint32_be(io,picture.data_size);

  if (picture.data_size>0) {
    allocElms(picture.data,picture.data_size);
    if (io.readBlock(picture.data,picture.data_size)==picture.data_size) {
      FXImage * image = gm_load_image_from_data(picture.data,picture.data_size,picture.mimetype,scale,crop);
      if (image) {
        cover = new GMCover(image,picture.type,picture.description);
        }
      }
    freeElms(picture.data);
    }
  return cover;
  }


static FXbool gm_flac_parse_header(FXIO & io,FXuint & header) {
  FXchar  marker[4];

  if (io.readBlock(marker,4)!=4 || compare(marker,"fLaC",4))
    return false;

  if (io.readBlock(&header,4)!=4 || FLAC_BLOCK_TYPE(header)!=FLAC_BLOCK_STREAMINFO || FLAC_BLOCK_SIZE(header)!=34  || (header&FLAC_LAST_BLOCK))
    return false;

  /// Go to beginning of meta data
  io.position(34,FXIO::Current);
  return true;
  }

static FXbool gm_flac_next_block(FXIO & io,FXuint & header) {
  if (!(header&FLAC_LAST_BLOCK) && (io.readBlock(&header,4)==4))
    return true;
  else
    return false;
  }

static FXint flac_load_covers(const FXString & mrl,GMCoverList & covers,FXint scale,FXint crop) {
  FXuint  header;
  FXFile io;

  if (io.open(mrl,FXIO::Reading)) {

    if (!gm_flac_parse_header(io,header))
      return 0;

    while(gm_flac_next_block(io,header)) {
      if (FLAC_BLOCK_TYPE(header)==FLAC_BLOCK_PICTURE) {
        GMCover * cover = gm_flac_parse_block_picture(io,scale,crop);
        if (cover) covers.append(cover);
        }
      else if (!(header&FLAC_LAST_BLOCK)){
        io.position(FLAC_BLOCK_SIZE(header),FXIO::Current);
        }
      }
    }
  return covers.no();
  }

static GMCover * flac_load_front_cover(const FXString & mrl,FXint scale,FXint crop) {
  FXuint  header;
  FXlong  pos;
  FXFile io;

  if (io.open(mrl,FXIO::Reading)) {

    if (!gm_flac_parse_header(io,header))
      return 0;

    while(gm_flac_next_block(io,header)) {
      if (FLAC_BLOCK_TYPE(header)==FLAC_BLOCK_PICTURE) {
        pos=io.position();
        if (gm_flac_is_front_cover(io)) {
          io.position(pos,FXIO::Begin);
          GMCover * cover = gm_flac_parse_block_picture(io,scale,crop);
          if (cover) return cover;
          }
        }
      else if (!(header&FLAC_LAST_BLOCK)){
        io.position(FLAC_BLOCK_SIZE(header),FXIO::Current);
        }
      }
    }
  return NULL;
  }

#endif




GMCover::GMCover() : data(NULL),len(0), type(0) {
  }

GMCover::GMCover(const void * ptr,FXuval size,FXuint /*t*/,const FXString & /*label*/,FXbool owned) : data(NULL), len(size) {
  if (owned==false) {
    allocElms(data,len);
    memcpy(data,(const FXuchar*)ptr,len);
    }
  else {
    data=(FXuchar*)ptr;
    }
  }

GMCover::~GMCover() {
  freeElms(data);
  }


FXString GMCover::fileExtension() const{
  if (     data[0]==137 &&
           data[1]==80  &&
           data[2]==78  &&
           data[3]==71  &&
           data[4]==13  &&
           data[5]==10  &&
           data[6]==26  &&
           data[7]==10) {

    return ".png";
    }
  else if (data[0]==0xFF &&
           data[1]==0xD8){
    return ".jpg";
    }
  else if (data[0]=='B' &&
           data[1]=='M'){
    return ".bmp";
    }
  else if (data[0]==0x47 &&
           data[1]==0x49 &&
           data[2]==0x46){
    return ".gif";
    }
  return FXString::null;
  }


FXbool GMCover::save(const FXString & filename) {
  FXString path = FXPath::directory(filename);
  if (FXStat::exists(path) || FXDir::createDirectories(path)) {
    FXFile file (filename,FXIO::Writing);
    file.writeBlock(data,len);
    file.close();
    return true;
    }
  return false;
  }


FXint GMCover::fromTag(const FXString & mrl,GMCoverList & covers) {
  GM_TICKS_START();
  FXString extension = FXPath::extension(mrl);

#if TAGLIB_VERSION < MKVERSION(1,7,0)
  if (comparecase(extension,"flac")==0){
    flac_load_covers(mrl,covers,scale,crop);
    if (covers.no()) return (covers.no());
    }
#endif

  GMFileTag tags;
  if (!tags.open(mrl,FILETAG_TAGS)) {
    GM_TICKS_END();
    return 0;
    }
  tags.getCovers(covers);
  GM_TICKS_END();
  return covers.no();
  }

FXint GMCover::fromPath(const FXString & path,GMCoverList & list) {
  FXString * files=NULL;
  FXImage * image;
  FXint nfiles = FXDir::listFiles(files,path,"*.(png,jpg,jpeg,bmp,gif)",FXDir::NoDirs|FXDir::NoParent|FXDir::CaseFold|FXDir::HiddenFiles);
  if (nfiles) {
    for (FXint i=0;i<nfiles;i++) {
      image = gm_load_image_from_file(path+PATHSEPSTRING+files[i],1);
      if (image)
        list.append(new GMCover(image,0));
      }
    delete [] files;
    }
  return list.no();
  }




GMCover * GMCover::fromTag(const FXString & mrl) {
  FXString extension = FXPath::extension(mrl);
#if TAGLIB_VERSION < MKVERSION(1,7,0)
  if (comparecase(extension,"flac")==0){
    GMCover * cover = flac_load_front_cover(mrl,scale,crop);
    if (cover) { return cover; }
    }
#endif
  GMFileTag tags;
  if (!tags.open(mrl,FILETAG_TAGS)) {
    return NULL;
    }
  return tags.getFrontCover();
  }

GMCover * GMCover::fromFile(const FXString & filename) {
  FXFile file(filename,FXIO::Reading);
  FXuval size = file.size();
  if (file.isOpen() && size) {
    FXchar * data=NULL;
    allocElms(data,size);
    if (file.readBlock(data,size)==(FXival)size) {
      return new GMCover(data,size);
      }
    freeElms(data);
    }
  return NULL;
  }

GMCover * GMCover::fromPath(const FXString & path) {
  static const FXchar * const covernames[]={"cover","album","front","albumart",".folder","folder",NULL};
  FXString * files=NULL;
  FXString * names=NULL;
  GMCover  * cover=NULL;

  FXint nfiles = FXDir::listFiles(files,path,"*.(png,jpg,jpeg,bmp,gif)",FXDir::NoDirs|FXDir::NoParent|FXDir::CaseFold|FXDir::HiddenFiles);
  if (nfiles) {
    names = new FXString[nfiles];
    for (FXint i=0;i<nfiles;i++)
      names[i]=FXPath::title(files[i]);

    for (FXint c=0;covernames[c]!=NULL;c++) {
      for (FXint i=0;i<nfiles;i++){
        if (comparecase(names[i],covernames[c])==0) {
          cover = GMCover::fromFile(path+PATHSEPSTRING+files[i]);
          if (cover) {
            delete [] names;
            delete [] files;
            return cover;
            }
          }
        }
      }

    delete [] names;
    /// No matching cover name found. Just load the first file.
    cover = GMCover::fromFile(path+PATHSEPSTRING+files[0]);
    delete [] files;
    }
  return cover;
  }


FXImage * GMCover::copyToImage(GMCover * cover,FXint scale/*=0*/,FXint crop/*=0*/) {
  if (cover) {
    return gm_load_image_from_data(cover->data,cover->len,scale,crop);
    }
  return NULL;
  }

FXImage * GMCover::toImage(GMCover * cover,FXint scale/*=0*/,FXint crop/*=0*/) {
  if (cover) {
    FXImage * image = gm_load_image_from_data(cover->data,cover->len,scale,crop);
    delete cover;
    return image;
    }
  return NULL;
  }
