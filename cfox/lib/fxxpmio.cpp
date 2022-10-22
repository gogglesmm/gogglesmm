/********************************************************************************
*                                                                               *
*                          X P M   I n p u t / O u t p u t                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxascii.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXElement.h"
#include "FXStream.h"
#include "FXColors.h"


/*
  Notes:
  - The transparent color hopefully does not occur in the image.
  - If the image is rendered opaque, the transparent is close to white.
  - References: http://www-sop.inria.fr/koala/lehors/xpm.html
  - XPM reader/writer is tweaked so that an XPM written to FXStream
    can be read back in and read exactly as many bytes as were written.
  - There may be other comment blocks in the file
*/

#define MAXPRINTABLE    92
#define MAXVALUE        96
#define HASH1(x,n)      (((unsigned int)(x)*13)%(n))            // Number [0..n-1]
#define HASH2(x,n)      (1|(((unsigned int)(x)*17)%((n)-1)))    // Number [1..n-2]

using namespace FX;


/*******************************************************************************/

namespace FX {


extern FXbool fxfsquantize(FXuchar* dst,const FXColor* src,FXColor* colormap,FXint& actualcolors,FXint w,FXint h,FXint maxcolors);
extern FXbool fxezquantize(FXuchar* dst,const FXColor* src,FXColor* colormap,FXint& actualcolors,FXint w,FXint h,FXint maxcolors);
extern FXbool fxwuquantize(FXuchar* dst,const FXColor* src,FXColor* colormap,FXint& actualcolors,FXint w,FXint h,FXint maxcolors);


// Declarations
#ifndef FXLOADXPM
extern FXAPI FXbool fxcheckXPM(FXStream& store);
extern FXAPI FXbool fxloadXPM(const FXchar **pix,FXColor*& data,FXint& width,FXint& height);
extern FXAPI FXbool fxloadXPM(FXStream& store,FXColor*& data,FXint& width,FXint& height);
extern FXAPI FXbool fxsaveXPM(FXStream& store,const FXColor *data,FXint width,FXint height,FXbool fast=true);
#endif

// Furnish our own version
extern FXAPI FXint __sscanf(const FXchar* string,const FXchar* format,...);
extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);


// Read till end of line
static void readline(FXStream& store,FXchar* buffer,FXuint size){
  FXuint i=0;
  while(!store.eof() && i<size){
    store >> buffer[i];
    if(buffer[i]=='\r') continue;
    if(buffer[i]=='\n') break;
    i++;
    }
  buffer[i]=0;
  }


// Read quoted text
static void readtext(FXStream& store,FXchar* buffer,FXuint size){
  FXuint i=0;
  FXchar ch;
  store >> ch;
  while(!store.eof() && ch!='"') store >> ch;
  while(!store.eof() && i<size){
    store >> ch;
    if(ch=='"') break;
    buffer[i++]=ch;
    }
  buffer[i]=0;
  }


// Parse next word
static FXint nextword(const FXchar*& src,FXchar* dst){
  FXchar *ptr=dst;
  while(*src && Ascii::isSpace(*src)) src++;
  while(*src && !Ascii::isSpace(*src)) *ptr++=*src++;
  *ptr=0;
  return (FXint)(ptr-dst);
  }


// Is key
static FXbool iskey(const FXchar *str){
  return ((str[0]=='c' || str[0]=='s' || str[0]=='m' || str[0]=='g') && str[1]==0) || (str[0]=='g' && str[1]=='4' && str[2]==0);
  }


// Check if stream contains a XPM
FXbool fxcheckXPM(FXStream& store){
  FXuchar signature[9];
  store.load(signature,9);
  store.position(-9,FXFromCurrent);
  return signature[0]=='/' && signature[1]=='*' && signature[2]==' ' && signature[3]=='X' && signature[4]=='P' && signature[5]=='M' && signature[6]==' ' && signature[7]=='*' && signature[8]=='/';
  }


// Load image from array of strings
FXbool fxloadXPM(const FXchar **pixels,FXColor*& data,FXint& width,FXint& height){
  FXchar  lookuptable[1024][8],name[100],word[100],flag,best;
  FXColor colortable[16384],*pix,color;
  const FXchar *src,*line;
  FXint   i,j,ncolors,cpp,c;

  // Null out
  data=nullptr;
  width=0;
  height=0;
  color=0;

  // NULL pointer passed in
  if(!pixels) return false;

  // Read pointer
  line=*pixels++;

  // No size description line
  if(!line) return false;

  // Parse size description
  __sscanf(line,"%d %d %u %u",&width,&height,&ncolors,&cpp);

  // Check size
  if(width<1 || height<1 || width>16384 || height>16384) return false;

  // Sensible inputs
  if(cpp<1 || cpp>8 || ncolors<1) return false;

  // Limited number of colors for long lookup strings
  if(cpp>2 && ncolors>1024) return false;

  // Allow more colors for short lookup strings
  if(ncolors>16384) return false;

  //FXTRACE((100,"fxloadXPM: width=%d height=%d ncolors=%d cpp=%d\n",width,height,ncolors,cpp));

  // Read the color table
  for(c=0; c<ncolors; c++){
    line=*pixels++;
    src=line+cpp;
    nextword(src,word);
    best='z';
    while(iskey(word)){
      flag=word[0];
      name[0]=0;
      while(nextword(src,word) && !iskey(word)){
        fxstrlcat(name,word,sizeof(name));
        }
      if(flag<best){                    // c < g < m < s
        color=colorFromName(name);
        best=flag;
        }
      }
    if(cpp==1){
      colortable[(FXuchar)line[0]]=color;
      }
    else if(cpp==2){
      colortable[(((FXuchar)line[1])<<7)+(FXuchar)line[0]]=color;
      }
    else{
      colortable[c]=color;
      fxstrlcpy(lookuptable[c],line,cpp);
      }
    }

  // Try allocate pixels
  if(!allocElms(data,width*height)){
    return false;
    }

  // Read the pixels
  for(i=0,pix=data; i<height; i++){
    line=*pixels++;
    for(j=0; j<width; j++){
      if(cpp==1){
        color=colortable[(FXuchar)line[0]];
        }
      else if(cpp==2){
        color=colortable[(((FXuchar)line[1])<<7)+(FXuchar)line[0]];
        }
      else{
        for(c=0; c<ncolors; c++){
          if(strncmp(lookuptable[c],line,cpp)==0){ color=colortable[c]; break; }
          }
        }
      line+=cpp;
      *pix++=color;
      }
    }
  return true;
  }


/*******************************************************************************/


// Load image from stream
FXbool fxloadXPM(FXStream& store,FXColor*& data,FXint& width,FXint& height){
  FXchar lookuptable[1024][8],line[100],name[100],word[100],flag,best,ch;
  FXColor colortable[16384],*pix,color;
  const FXchar *src;
  FXint i,j,ncolors,cpp,c;

  // Null out
  data=nullptr;
  width=0;
  height=0;
  color=0;

  // Read header line
  readline(store,name,sizeof(name));
  if(!strstr(name,"XPM")) return false;

  // Read description
  readtext(store,line,sizeof(line));

  // Parse size description
  if(__sscanf(line,"%d %d %u %u",&width,&height,&ncolors,&cpp)!=4) return false;

  // Check size
  if(width<1 || height<1 || width>16384 || height>16384) return false;

  // Sensible inputs
  if(cpp<1 || cpp>8 || ncolors<1) return false;

  // Limited number of colors for long lookup strings
  if(cpp>2 && ncolors>1024) return false;

  // Allow more colors for short lookup strings
  if(ncolors>16384) return false;

  //FXTRACE((100,"fxloadXPM: width=%d height=%d ncolors=%d cpp=%d\n",width,height,ncolors,cpp));

  // Read the color table
  for(c=0; c<ncolors; c++){
    readtext(store,line,sizeof(line));
    src=line+cpp;
    nextword(src,word);
    best='z';
    while(iskey(word)){
      flag=word[0];
      name[0]=0;
      while(nextword(src,word) && !iskey(word)){
        fxstrlcat(name,word,sizeof(name));
        }
      if(flag<best){                    // c < g < m < s
        color=colorFromName(name);
        best=flag;
        }
      }
    if(cpp==1){
      colortable[(FXuchar)line[0]]=color;
      }
    else if(cpp==2){
      colortable[(((FXuchar)line[1])<<7)+(FXuchar)line[0]]=color;
      }
    else{
      colortable[c]=color;
      fxstrlcpy(lookuptable[c],line,cpp);
      }
    }

  // Try allocate pixels
  if(!allocElms(data,width*height)){
    return false;
    }

  // Read the pixels
  for(i=0,pix=data; i<height; i++){
    while(!store.eof() && (store>>ch,ch!='"')){}
    for(j=0; j<width; j++){
      store.load(line,cpp);
      if(cpp==1){
        color=colortable[(FXuchar)line[0]];
        }
      else if(cpp==2){
        color=colortable[(((FXuchar)line[1])<<7)+(FXuchar)line[0]];
        }
      else{
        for(c=0; c<ncolors; c++){
          if(strncmp(lookuptable[c],line,cpp)==0){ color=colortable[c]; break; }
          }
        }
      *pix++=color;
      }
    while(!store.eof() && (store>>ch,ch!='"')){}
    }

  // We got the image, but we're not done yet; need to read few more bytes
  // the number of bytes read here must match the number of bytes written
  // by fxsaveXPM() so that the stream won't get out of sync
  while(!store.eof()){
    store >> ch;
    if(ch=='\n') break;
    }
  return true;
  }


/*******************************************************************************/


// Save image to a stream
FXbool fxsaveXPM(FXStream& store,const FXColor *data,FXint width,FXint height,FXbool fast){
  const FXchar printable[]=" .XoO+@#$%&*=-;:>,<1234567890qwertyuipasdfghjklzxcvbnmMNBVCZASDFGHJKLPIUYTREWQ!~^/()_`'][{}|";
  const FXchar quote='"';
  const FXchar comma=',';
  const FXchar newline='\n';
  FXColor   colormap[256];
  FXint     numpixels=width*height;
  FXint     ncolors,cpp,len,i,j,c1,c2;
  FXchar    buffer[200];
  FXColor   color;
  FXuchar  *pixels,*ptr,pix;

  // Must make sense
  if(!data || width<=0 || height<=0) return false;

  // Allocate temp buffer for pixels
  if(!allocElms(pixels,numpixels)) return false;

  // First, try EZ quantization, because it is exact; a previously
  // loaded XPM will be re-saved with exactly the same colors.
  if(!fxezquantize(pixels,data,colormap,ncolors,width,height,256)){
    if(fast){
      fxfsquantize(pixels,data,colormap,ncolors,width,height,256);
      }
    else{
      fxwuquantize(pixels,data,colormap,ncolors,width,height,256);
      }
    }

  FXASSERT(ncolors<=256);

  // How many characters needed to represent one pixel, characters per line
  cpp=(ncolors>MAXPRINTABLE)?2:1;

  // Save header
  store.save("/* XPM */\nstatic char * image[] = {\n",36);

  // Save values
  len=__snprintf(buffer,sizeof(buffer),"\"%d %d %d %d\",\n",width,height,ncolors,cpp);
  store.save(buffer,len);

  // Save the colors
  for(i=0; i<ncolors; i++){
    color=colormap[i];
    c1=printable[i%MAXPRINTABLE];
    c2=printable[i/MAXPRINTABLE];
    if(FXALPHAVAL(color)){
      len=__snprintf(buffer,sizeof(buffer),"\"%c%c c #%02x%02x%02x\",\n",c1,c2,FXREDVAL(color),FXGREENVAL(color),FXBLUEVAL(color));
      store.save(buffer,len);
      }
    else{
      len=__snprintf(buffer,sizeof(buffer),"\"%c%c c None\",\n",c1,c2);
      store.save(buffer,len);
      }
    }

  // Save the image
  ptr=pixels;
  for(i=0; i<height; i++){
    store << quote;
    for(j=0; j<width; j++){
      pix=*ptr++;
      if(cpp==1){
        store << printable[pix];
        }
      else{
        store << printable[pix%MAXPRINTABLE];
        store << printable[pix/MAXPRINTABLE];
        }
      }
    store << quote;
    if(i<height-1){ store << comma; store << newline; }
    }
  store.save("};\n",3);
  freeElms(pixels);
  return true;
  }

}

