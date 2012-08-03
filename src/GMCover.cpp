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





#if FOX_BIGENDIAN == 0
#define MSB_UINT(x) ((x)[3]) | ((x)[2]<<8) | ((x)[1]<<16) | ((x)[0]<<24)
#define MSB_SHORT(x) ((x)[0]<<8) | ((x)[1])
#else
#define MSB_UINT(data) (data[0]) | (data[1]<<8) | (data[2]<<16) | (data[3]<<24)
#define MSB_SHORT(data) (data[1]<<8) | (data[0])
#endif


FXbool gm_meta_png(const FXuchar * data,FXival size,GMImageInfo & info) {

  enum {
    PNG_TYPE_GRAYSCALE            = 0,
    PNG_TYPE_TRUECOLOR            = 2,
    PNG_TYPE_PALETTE              = 3,
    PNG_TYPE_GRAYSCALE_WITH_ALPHA = 4,
    PNG_TYPE_TRUECOLOR_WITH_ALPHA = 6,
    };

  info.width   = 0;
  info.height  = 0;
  info.bps     = 0;
  info.colors  = 0;

  // Make sure it's a PNG file with a IHDR as first chunk
  if (data[ 0]==137 && data[ 1]==80  && data[ 2]==78  && data[ 3]==71  &&
      data[ 4]==13  && data[ 5]==10  && data[ 6]==26  && data[ 7]==10  &&
      data[12]=='I' && data[13]=='H' && data[14]=='D' && data[15]=='R') {

    FXival nbytes  = size-8;
    const FXuchar * chunk = data + 8;

    while(nbytes>=12) {
      FXuint chunk_length = MSB_UINT(chunk);

      // IHDR chunk
      if (compare((const FXchar*)chunk,"IHDR",4)==0) {
        if (chunk_length!=13)
          return false;

        info.width     = MSB_UINT(chunk+16);
        info.height    = MSB_UINT(chunk+20);
        FXuchar depth  = chunk[24];
        FXuchar color  = chunk[24];

        switch(color) {
          case PNG_TYPE_GRAYSCALE           : info.bps = depth;   break;
          case PNG_TYPE_TRUECOLOR           : info.bps = depth*3; break;
          case PNG_TYPE_PALETTE             : info.bps = 24;      break;
          case PNG_TYPE_GRAYSCALE_WITH_ALPHA: info.bps = depth*2; break;
          case PNG_TYPE_TRUECOLOR_WITH_ALPHA: info.bps = depth*4; break;
          default                           : return false;       break;
          }

        if (color!=PNG_TYPE_PALETTE)
          return true;

        }
      else if (compare((const FXchar*)chunk,"PLTE",4)==0) {
        info.colors = chunk_length / 3; /// 3 bytes for each palette entry
        return true;
        }

      // next chunk
      chunk  += chunk_length + 12;
      nbytes -= chunk_length + 12;
      }
    }
  return false;
  }


FXbool gm_meta_jpeg(const FXuchar * data,FXival size,GMImageInfo & info) {
  const FXuchar * chunk  = data + 2;
  FXival   nbytes = size - 2;
  FXuchar  marker;

  if (nbytes>2 && data[0]==0xFF && data[1]==0xD8){

    while(nbytes) {

      // Find Marker
      while(nbytes && *chunk!=0xFF){
        chunk++;
        nbytes--;
        }

      // End of data
      if (nbytes==0)
        return false;

      // Skip padding
      while(nbytes && *chunk==0xFF){
        chunk++;
        nbytes--;
        }

      // End of data
      if (nbytes<2)
        return false;

      // Read marker
      marker = *chunk;
      chunk++;
      nbytes--;

      switch(marker) {

        // Start of Frame
        case 0xC0:
        case 0xC1:
        case 0xC2:
        case 0xC3:
        case 0xC5:
        case 0xC6:
        case 0xC7:
        case 0xC9:
        case 0xCA:
        case 0xCB:
        case 0xCD:
        case 0xCE:
        case 0xCF:
          {
            FXuint length = MSB_SHORT(chunk);

            if (length<8 || nbytes<8)
              return false;

            info.height      = MSB_SHORT(chunk+3);
            info.width       = MSB_SHORT(chunk+5);
            info.bps         = chunk[7]*chunk[2];
            info.colors      = 0;

            return true;
          }


        // Bail out
        case 0xDA:                // Beginning of compressed data
        case 0xD9:  return false; // End of datastream


        /* Skip unknown chunks */
        default:
          {
            FXuint length = MSB_SHORT(chunk);
            if (length<2)
              return false;

            chunk  += length;
            nbytes -= length;
          }
        }
      }
    }
  return false;
  }


#define BIH_RGB         0       // biCompression values
#define BIH_RLE8        1
#define BIH_RLE4        2
#define BIH_BITFIELDS   3

#define OS2_OLD         12      // biSize values
#define WIN_NEW         40
#define OS2_NEW         64



FXbool gm_meta_bmp(const FXuchar * data,FXival size,GMImageInfo & info) {
  FXMemoryStream store(FXStreamLoad,(FXuchar*)data,size);

  FXint    bfSize;
  FXint    bfOffBits;
  FXushort bfType;
  FXushort bfReserved;
  FXushort biBitCount;
  FXushort biPlanes;

  FXint    biWidth;
  FXint    biHeight;
  FXint    biSizeImage;
  FXint    biSize;
  FXint    biCompression;
  FXint    biXPelsPerMeter;
  FXint    biYPelsPerMeter;
  FXint    biClrUsed;
  FXint    biClrImportant;

  store.setBigEndian(false);

  // Get size and offset
  store >> bfType;
  store >> bfSize;
  store >> bfReserved;
  store >> bfReserved;
  store >> bfOffBits;

  // Check signature
  if(bfType!=0x4d42)
    return false;

  store >> biSize;
  if(biSize==OS2_OLD){                  // Old format
    store >> bfReserved; biWidth=bfReserved;
    store >> bfReserved; biHeight=bfReserved;
    store >> biPlanes;
    store >> biBitCount;

    info.width  = biWidth;
    info.height = biHeight;
    info.bps    = biBitCount;

    if (biBitCount<=8)
      info.colors = 1<<biBitCount;
    else
      info.colors = 0;
    }
  else {
    store >> biWidth;
    store >> biHeight;
    store >> biPlanes;
    store >> biBitCount;
    store >> biCompression;
    store >> biSizeImage;
    store >> biXPelsPerMeter;
    store >> biYPelsPerMeter;
    store >> biClrUsed;
    store >> biClrImportant;

    info.width  = biWidth;
    info.height = biHeight;
    info.bps    = biBitCount;
    if (biBitCount<=8)
      info.colors = biClrUsed ? biClrUsed : 1<<biBitCount;
    else
      info.colors = 0;
    }
  return true;
  }


// Codes found in the GIF specification
const FXuchar TAG_EXTENSION   = 0x21;   // Extension block
const FXuchar TAG_IMAGE       = 0x2c;   // Image separator


FXbool gm_meta_gif(const FXuchar * data,FXival size,GMImageInfo & info) {
  FXMemoryStream store(FXStreamLoad,(FXuchar*)data,size);

  FXuchar c1,c2,c3,flagbits,background,sbsize;
  FXint ncolors;

  // Load signature
  store >> c1;
  store >> c2;
  store >> c3;

  // Check signature
  if(c1!=0x47 || c2!=0x49 || c3!=0x46) return false;

  // Load version
  store >> c1;
  store >> c2;
  store >> c3;

  // Check version
  if(c1!=0x38 || (c2!=0x37 && c2!=0x39) || c3!=0x61) return false;

  // Get screen descriptor
  store >> c1 >> c2;    // Skip screen width
  store >> c1 >> c2;    // Skip screen height
  store >> flagbits;    // Get flag bits
  store >> background;  // Background
  store >> c2;          // Skip aspect ratio

  // Determine number of colors
  ncolors=2<<(flagbits&7);

  info.colors = ncolors;
  info.bps    = 3 * (1+((flagbits&0x70)>>4));

  // Skip Global Colormap
  if(flagbits&0x80){
    store.position(ncolors*3);
    }

  while(1){
    store >> c1;
    if(c1==TAG_EXTENSION){
      do{
        store >> sbsize;
        store.position(sbsize,FXFromCurrent);
        }
      while(sbsize>0 && !store.eof());    // FIXME this logic still flawed
      continue;
      }
    else if (c1==TAG_IMAGE) {
      store >> c1 >> c2;
      store >> c1 >> c2;

      // Get image width
      store >> c1 >> c2;
      info.width=(c2<<8)+c1;

      // Get image height
      store >> c1 >> c2;
      info.height=(c2<<8)+c1;

      // Read local map if there is one
      if(flagbits&0x80){
        ncolors=2<<(flagbits&7);
        info.colors = ncolors;
        }
      return true;
      }
    break;
    }
  return false;
  }




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




GMCover::GMCover() : data(NULL),size(0),type(0) {
  }

GMCover::GMCover(const void * ptr,FXuint len,FXuint t,const FXString & label,FXbool owned) : 
  data(NULL), 
  size(len),
  description(label),
  type(t) {

  if (ptr && size) {
    if (owned==false) {
      allocElms(data,size);
      memcpy(data,(const FXuchar*)ptr,size);
      }
    else {
      data=(FXuchar*)ptr;
      }
    }
  }

GMCover::~GMCover() {
  freeElms(data);
  }


FXbool GMCover::getImageInfo(GMImageInfo & ii) {
  if (info.width==0 && info.height==0) {
    FXbool success = false;
    switch(fileType()) {    
      case FILETYPE_PNG: success = gm_meta_png(data,size,info); break;
      case FILETYPE_JPG: success = gm_meta_jpeg(data,size,info); break;
      case FILETYPE_BMP: success = gm_meta_bmp(data,size,info); break;
      case FILETYPE_GIF: success = gm_meta_gif(data,size,info); break;
      default          : break;
      }
    if (!success) return false;
    }

  if (info.width>0 && info.height>0) {
    info=ii;
    return true;    
    }

  return false;  
  }


FXuint GMCover::fileType() const {
  if (     data[0]==137 &&
           data[1]==80  &&
           data[2]==78  &&
           data[3]==71  &&
           data[4]==13  &&
           data[5]==10  &&
           data[6]==26  &&
           data[7]==10) {

    return FILETYPE_PNG;
    }
  else if (data[0]==0xFF &&
           data[1]==0xD8){
    return FILETYPE_JPG;
    }
  else if (data[0]=='B' &&
           data[1]=='M'){
    return FILETYPE_BMP;
    }
  else if (data[0]==0x47 &&
           data[1]==0x49 &&
           data[2]==0x46){
    return FILETYPE_GIF;
    }
  else
    return FILETYPE_UNKNOWN;
  }


FXString GMCover::fileExtension() const{
  static const FXchar * const filetype_extension[]={"",".png",".jpg",".bmp",".gif"};
  return filetype_extension[fileType()];
  }

FXString GMCover::mimeType() const{
  static const FXchar * const mimetypes[]={"","image/png","image/jpeg","image/x-bmp","image/gif"};
  return mimetypes[fileType()];
  }




FXbool GMCover::save(const FXString & filename) {
  FXString path = FXPath::directory(filename);
  if (FXStat::exists(path) || FXDir::createDirectories(path)) {
    FXFile file (filename,FXIO::Writing);
    file.writeBlock(data,size);
    file.close();
    return true;
    }
  return false;
  }

#if 0
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
#endif

#if 0 // FIXME
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
#endif



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
    return gm_load_image_from_data(cover->data,cover->size,scale,crop);
    }
  return NULL;
  }

FXImage * GMCover::toImage(GMCover * cover,FXint scale/*=0*/,FXint crop/*=0*/) {
  if (cover) {
    FXImage * image = gm_load_image_from_data(cover->data,cover->size,scale,crop);
    delete cover;
    return image;
    }
  return NULL;
  }
