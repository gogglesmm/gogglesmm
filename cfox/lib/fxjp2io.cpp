/********************************************************************************
*                                                                               *
*                    J P E G - 2 0 0 0   I n p u t / O u t p u t                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2009,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXElement.h"
#include "FXStream.h"
#ifdef HAVE_JP2_H
#include "openjpeg.h"
#endif


/*
  Notes:
  - Support for JPEG 2000 image file compression.
*/

// Contents of signature box
#define SIGNATURE       0x0d0a870a              // Value for signature box

// File level boxes
#define BOX_JP          0x6a502020              // File signature box
#define BOX_FTYP        0x66747970              // File type box
#define BOX_JP2H        0x6a703268              // JP2 Header box
#define BOX_MHDR        0x6D686472              // Compound image header box
#define BOX_DBTL        0x6474626c		// Data reference box
#define BOX_URL         0x75726c20		// URL box
#define BOX_PCOL        0x70636F6C              // Page collection box
#define BOX_LBL         0x6C626C20              // Label box
#define BOX_PAGT        0x70616774              // Page table box
#define BOX_SDAT        0x73646174              // Shared data box
#define BOX_SREF        0x73726566              // Shared reference box
#define BOX_PAGE        0x70616765              // Page box
#define BOX_PHDR        0x70686472              // Page header box
#define BOX_RES         0x72657320              // Resolution box
#define BOX_BCLR        0x62636C72		// Base color box
#define BOX_LOBJ        0x6C6F626A		// Layout object box
#define BOX_LHDR        0x6C686472		// Layout object header box
#define BOX_OBJC        0x6F626A63		// Object box
#define BOX_OHDR        0x6F686472		// Object header box
#define BOX_SCAL        0x7363616C		// Object scale box
#define BOX_FTBL        0x6672626C              // Fragment table box
#define BOX_FLST        0x666C7374              // Fragment list box
#define BOX_MDAT        0x6D646174              // Media data box
#define BOX_JP2C        0x6a703263		// Contiguous codestream box
#define BOX_RESC        0x72657363              // Capture resolution box
#define BOX_RESD        0x72657364              // Default display resolution box
#define BOX_IHDR        0x69686472		// Image Header box
#define BOX_PCLR        0x70636C72		// Palette box
#define BOX_COLR        0x636f6c72		// Colour specification box
#define BOX_CMAP        0x636D6170              // Component mapping box
#define BOX_FTBL        0x6672626C              // Fragment table box
#define BOX_FREE        0x66726565		// Free box
#define BOX_JP2         0x6a703220              // JP2 box
#define BOX_BPCC        0x62706363		// Bits per component box
#define BOX_XML         0x786d6c20              // XML box
#define BOX_RREQ        0x72726571              // RREQ box

using namespace FX;

/*******************************************************************************/

namespace FX {


#ifndef FXLOADJP2
extern FXAPI FXbool fxcheckJP2(FXStream& store);
extern FXAPI FXbool fxloadJP2(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& quality);
extern FXAPI FXbool fxsaveJP2(FXStream& store,const FXColor* data,FXint width,FXint height,FXint quality);
#endif


#undef HAVE_JP2_H
#ifdef HAVE_JP2_H

/*******************************************************************************/


// Report error
void j2k_error_callback(const char *msg, void *client_data){
  FXTRACE((100,"fxjp2io: error: %s.\n",msg));
  }


// Report warning
void j2k_warning_callback(const char *msg, void *client_data){
  FXTRACE((100,"fxjp2io: warning: %s.\n",msg));
  }


// Report info
void j2k_info_callback(const char *msg, void *client_data){
  FXTRACE((100,"fxjp2io: info: %s.\n",msg));
  }


// Check if stream contains a JPG
FXbool fxcheckJP2(FXStream& store){
  FXuchar ss[12];
  store.load(ss,12);
  store.position(-12,FXFromCurrent);
  return ss[0]==0 && ss[1]==0 && ss[2]==0 && ss[3]==12 && ss[4]=='j' && ss[5]=='P' && ss[6]==' ' && ss[7]==' ' && ss[8]==0x0D && ss[9]==0x0A && ss[10]==0x87 && ss[11]==0x0A;
  }


// Load a JPEG image
FXbool fxloadJP2(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint&){
  FXint x,y,cw,rsh,gsh,bsh,ash,rof,gof,bof,aof;
  FXuchar r,g,b,a;
  FXbool swap=store.swapBytes();
  FXlong pos=store.position();
  FXbool result=false;
  FXuint box[4];
  FXlong boxsize;
  FXuint size;
  FXuchar *ptr;

  // Null out
  data=nullptr;
  width=0;
  height=0;

  // Switch big-endian to grab header
  store.setBigEndian(true);

  // Grab signature
  store.load(box,3);

  // Check signature, bail quickly if no match
  if(box[0]==12 && box[1]==BOX_JP && box[2]==SIGNATURE){

    // Figure size
    store.position(0,FXFromEnd);
    size=store.position()-pos;
    store.position(pos);

    FXTRACE((100,"fxloadJP2: file size=%d\n",size));

    // Allocate chunk for file data
    if(allocElms(ptr,size)){

      // Load entire file
      store.load(ptr,size);

      // Create decompressor
      opj_dinfo_t *decompressor=opj_create_decompress(CODEC_JP2);
      if(decompressor){
        opj_dparameters_t     parameters;
        opj_event_mgr_t       event_mgr;
        opj_cio_t            *cio=nullptr;
        opj_image_t          *image=nullptr;

        // Set up callbacks
        event_mgr.error_handler=j2k_error_callback;
        event_mgr.warning_handler=j2k_warning_callback;
        event_mgr.info_handler=j2k_info_callback;

        // Set event manager
        opj_set_event_mgr((opj_common_ptr)decompressor,&event_mgr,nullptr);

        // Initialize decompression parameters
        opj_set_default_decoder_parameters(&parameters);

        // Setup the decoder decoding parameters using user parameters
        opj_setup_decoder(decompressor,&parameters);

        // Open a byte stream */
        cio=opj_cio_open((opj_common_ptr)decompressor,ptr,size);
        if(cio){

          // Decode the stream and fill the image structure
          image=opj_decode(decompressor,cio);
          if(image){

            // Image size
            width=image->x1-image->x0;
            height=image->y1-image->y0;

            FXTRACE((100,"fxloadJP2: width=%d height=%d numcomps=%d color_space=%d\n",width,height,image->numcomps,image->color_space));

            // Only support GREY, RGB, and RGBA
            if(((image->numcomps==1) && (image->color_space==CLRSPC_GRAY)) || ((image->numcomps==3 || image->numcomps==4) && (image->color_space==CLRSPC_SRGB))){

              // Allocate image data
              if(allocElms(data,width*height)){
                rof=gof=bof=aof=rsh=gsh=bsh=ash=0;
                switch(image->numcomps){
                  case 1:
                    if(image->comps[0].sgnd) gof=1<<(image->comps[0].prec-1);
                    gsh=image->comps[0].prec-8;
                    cw=image->comps[0].w;
                    for(y=0; y<height; ++y){
                      for(x=0; x<width; ++x){
                        g=(image->comps[0].data[y*cw+x]+gof)>>gsh;
                        data[y*width+x]=FXRGB(g,g,g);
                        }
                      }
                    break;
                  case 3:
                    if(image->comps[0].sgnd) rof=1<<(image->comps[0].prec-1);
                    if(image->comps[1].sgnd) gof=1<<(image->comps[1].prec-1);
                    if(image->comps[2].sgnd) bof=1<<(image->comps[2].prec-1);
                    rsh=image->comps[0].prec-8;
                    gsh=image->comps[1].prec-8;
                    bsh=image->comps[2].prec-8;
                    cw=image->comps[0].w;
                    for(y=0; y<height; ++y){
                      for(x=0; x<width; ++x){
                        r=(image->comps[0].data[y*cw+x]+rof)>>rsh;
                        g=(image->comps[1].data[y*cw+x]+gof)>>gsh;
                        b=(image->comps[2].data[y*cw+x]+bof)>>bsh;
                        data[y*width+x]=FXRGB(r,g,b);
                        }
                      }
                    break;
                  default:
                    if(image->comps[0].sgnd) rof=1<<(image->comps[0].prec-1);
                    if(image->comps[1].sgnd) gof=1<<(image->comps[1].prec-1);
                    if(image->comps[2].sgnd) bof=1<<(image->comps[2].prec-1);
                    if(image->comps[3].sgnd) aof=1<<(image->comps[3].prec-1);
                    rsh=image->comps[0].prec-8;
                    gsh=image->comps[1].prec-8;
                    bsh=image->comps[2].prec-8;
                    ash=image->comps[3].prec-8;
                    cw=image->comps[0].w;
                    for(y=0; y<height; ++y){
                      for(x=0; x<width; ++x){
                        r=(image->comps[0].data[y*cw+x]+rof)>>rsh;
                        g=(image->comps[1].data[y*cw+x]+gof)>>gsh;
                        b=(image->comps[2].data[y*cw+x]+bof)>>bsh;
                        a=(image->comps[3].data[y*cw+x]+aof)>>ash;
                        data[y*width+x]=FXRGBA(r,g,b,a);
                        }
                      }
                    break;
                  }
                result=true;
                }
              }
            opj_image_destroy(image);
            }
          opj_cio_close(cio);
          }
        opj_destroy_decompress(decompressor);
        }
      freeElms(ptr);
      }
    }
  store.swapBytes(swap);
  return result;
  }


/*******************************************************************************/


// Save a JPEG image
FXbool fxsaveJP2(FXStream& store,const FXColor* data,FXint width,FXint height,FXint quality){
  FXint x,y,c,p;
  FXbool result=false;

  // Must make sense
  if(data && 0<width && 0<height){
    opj_cinfo_t* compressor=opj_create_compress(CODEC_JP2);
    if(compressor){
      opj_event_mgr_t       event_mgr;
      opj_cparameters_t     parameters;
      opj_cio_t            *cio=nullptr;
      opj_image_t          *image=nullptr;
      opj_image_cmptparm_t  components[3];
      FXColor               color;

      // Set up callbacks
      event_mgr.error_handler=j2k_error_callback;
      event_mgr.warning_handler=j2k_warning_callback;
      event_mgr.info_handler=j2k_info_callback;

      // Set event manager
      opj_set_event_mgr((opj_common_ptr)compressor,&event_mgr,nullptr);

      // Set encoding parameters to default values
      opj_set_default_encoder_parameters(&parameters);

//      parameters.tcp_rates[0]=((100-quality)/90.0f*99.0f)+1;
      parameters.tcp_rates[0]=16;
      parameters.tcp_numlayers=1;
      parameters.cp_disto_alloc=1;

      // Set up parameters
      for(c=0; c<3; c++){
        components[c].dx=parameters.subsampling_dx;
        components[c].dy=parameters.subsampling_dy;
        components[c].w=width;
        components[c].h=height;
        components[c].x0=0;
        components[c].y0=0;
        components[c].prec=8;
        components[c].bpp=8;
        components[c].sgnd=0;
        }

      // Create image
      image=opj_image_create(3,components,CLRSPC_SRGB);
      if(image){

	/* set image offset and reference grid */
	image->x0=parameters.image_offset_x0;
	image->y0=parameters.image_offset_y0;
	image->x1=parameters.image_offset_x0+(width-1)*parameters.subsampling_dx+1;
	image->y1=parameters.image_offset_y0+(height-1)*parameters.subsampling_dy+1;

        // Setup encoder for image
        opj_setup_encoder(compressor,&parameters,image);

        // Fill image buffers
	for(y=p=0; y<height; ++y){
          for(x=0; x<width; ++x){
            color=data[y*width+x];
            image->comps[0].data[p]=FXREDVAL(color);
            image->comps[1].data[p]=FXGREENVAL(color);
            image->comps[2].data[p]=FXBLUEVAL(color);
            p++;
            }
          }

        // Open code stream
        cio=opj_cio_open((opj_common_ptr)compressor,nullptr,0);
        if(cio){

          // Encode the image
          result=opj_encode(compressor,cio,image,nullptr);

          // Encoded properly
          if(result){

            // Write to store
            store.save(cio->buffer,cio_tell(cio));

            // Check if write was successful
            if(store.status()!=FXStreamOK) result=false;
            }

          // Close stream
          opj_cio_close(cio);
          }

        // Destroy image
        opj_image_destroy(image);
        }
      opj_destroy_compress(compressor);
      }
    }
  return result;
  }

/*******************************************************************************/

#else

// Check if stream contains a JPG
FXbool fxcheckJP2(FXStream&){
  return false;
  }


// Stub routine
FXbool fxloadJP2(FXStream&,FXColor*& data,FXint& width,FXint& height,FXint& quality){
  static const FXColor color[2]={FXRGB(0,0,0),FXRGB(255,255,255)};
  static const FXuchar image_bits[]={
   0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x80, 0xfd, 0xff, 0xff, 0xbf,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0xc5, 0xe7, 0xc3, 0xa1,
   0x45, 0x44, 0x24, 0xa2, 0x05, 0x44, 0x04, 0xa2, 0x05, 0x44, 0x04, 0xa2,
   0x05, 0x44, 0x04, 0xa1, 0x05, 0xc4, 0xc3, 0xa0, 0x05, 0x44, 0x20, 0xa0,
   0x45, 0x44, 0x20, 0xa2, 0x85, 0xe3, 0xe0, 0xa3, 0x05, 0x00, 0x00, 0xa0,
   0x05, 0x00, 0x00, 0xa0, 0x05, 0x00, 0x00, 0xa0, 0xfd, 0xff, 0xff, 0xbf,
   0x01, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff};
  allocElms(data,32*32);
  for(FXint p=0; p<32*32; p++){
    data[p]=color[(image_bits[p>>3]>>(p&7))&1];
    }
  width=32;
  height=32;
  quality=75;
  return true;
  }


// Stub routine
FXbool fxsaveJP2(FXStream&,const FXColor*,FXint,FXint,FXint){
  return false;
  }

#endif

}
