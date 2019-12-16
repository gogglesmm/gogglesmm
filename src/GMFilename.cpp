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
#include "gmdefs.h"
#include "GMTrack.h"
#include "GMFilename.h"

const char * gmcodecnames[]={
  "7-bit Ascii",
  "UTF-8 Unicode",
  "ISO 8859-1",
  "ISO 8859-2",
  "ISO 8859-3",
  "ISO 8859-4",
  "ISO 8859-5",
  "ISO 8859-6",
  "ISO 8859-7",
  "ISO 8859-8",
  "ISO 8859-9",
  "ISO 8859-10",
  "ISO 8859-11",
  "ISO 8859-13",
  "ISO 8859-14",
  "ISO 8859-15",
  "ISO 8859-16",
  "CP-437",
  "CP-850",
  "CP-852",
  "CP-855",
  "CP-856",
  "CP-857",
  "CP-860",
  "CP-861",
  "CP-862",
  "CP-863",
  "CP-864",
  "CP-865",
  "CP-866",
  "CP-869",
  "CP-874",
  "CP-1250",
  "CP-1251",
  "CP-1252",
  "CP-1253",
  "CP-1254",
  "CP-1255",
  "CP-1256",
  "CP-1257",
  "CP-1258",
  "KOI8-R",
  nullptr,
  };


using namespace gm;

namespace gm {



// Apply filter rules without character encoding conversion
FXString TextConverter::apply_filters(const FXString & src) const {
  FXString dst;
  FXint n=0;

  // First measure destination string size
  for (FXint i=0;i<src.length();i=src.inc(i)) {

    if (Unicode::isSpace(src.wc(i))) {

      // Simplify space
      while(Unicode::isSpace(src.wc(src.inc(i))))
        i=src.inc(i);

      // No space at end or beginning
      if (n>0 && src.inc(i)<src.length())
        n+=1; // always use 0x20 or _
      }
    else if (Unicode::isAscii(src.wc(i))) {
      if (Ascii::isPrint(src[i]) && forbidden.find(src[i])==-1) {
        n+=1;
        }
      }
    else if (Unicode::isPrint(src.wc(i))) {
      if (modifiers&LOWERCASE)
        n+=wc2utf(Unicode::toLower(src.wc(i)));
      else if (modifiers&UPPERCASE)
        n+=wc2utf(Unicode::toUpper(src.wc(i)));
      else
        n+=src.extent(i);
      }
    }

  // Resize destination
  dst.length(n);
  n=0;

  for (FXint i=0;i<src.length();i=src.inc(i)) {

    if (Unicode::isSpace(src.wc(i))) {

      // Simplify Space
      while(Unicode::isSpace(src.wc(src.inc(i))))
        i=src.inc(i);

      // No space at end or beginning
      if (n>0 && src.inc(i)<src.length()){
        if (modifiers&NOSPACE)
          dst[n++] = '_';
        else
          dst[n++] = ' ';
        }

      }
    else if (Ascii::isAscii(src[i])) {

      if (Ascii::isPrint(src[i]) && forbidden.find(src[i])==-1) {
        if (modifiers&LOWERCASE)
          dst[n++]=Ascii::toLower(src[i]);
        else if (modifiers&UPPERCASE)
          dst[n++]=Ascii::toUpper(src[i]);
        else
          dst[n++]=src[i];
        }
      }
    else if (Unicode::isPrint(src.wc(i))) {
      if (modifiers&LOWERCASE)
        n+=wc2utf(&dst[n],Unicode::toLower(src.wc(i)));
      else if (modifiers&UPPERCASE)
        n+=wc2utf(&dst[n],Unicode::toUpper(src.wc(i)));
      else
        n+=wc2utf(&dst[n],src.wc(i));
      }
    }
  return dst;
  }



FXString TextConverter::apply_codec(const FXString & src) const {
  const FXchar undefined_character = 0x1a;
  FXString dst;
  FXint n=0;
  FXchar c;

  // Measure
  for (FXint i=0;i<src.length();i=src.inc(i)) {
    if (Ascii::isAscii(src[i])) {
      if (Ascii::isPrint(src[i]) || Ascii::isSpace(src[i]))
        n++;
      }
    else if (Unicode::isPrint(src.wc(i))) {
      if (codec->wc2mb(&c,1,src.wc(i))==1) {
        if (c==undefined_character) {
          FXString dcm = FX::FXString::decompose(src.mid(i,src.extent(i)),false);
          for (FXint j=0;j<dcm.length();j+=dcm.inc(j)) {
            if (Unicode::isPrint(dcm.wc(j)) && codec->wc2mb(&c,1,dcm.wc(j)) && c!=0x1a) {
              n++;
              }
            }
          }
        else {
          n++;
          }
        }
      }
    }

  dst.length(n);
  for (FXint i=0,n=0;i<src.length();i=src.inc(i)) {
    if ((Ascii::isAscii(src[i]) && (Ascii::isPrint(src[i]) || Ascii::isSpace(src[i]))) ||
        (Unicode::isPrint(src.wc(i)))) {
      if (codec->wc2mb(&c,1,src.wc(i))==1) {

        if (c==undefined_character) {
          /* If codec didn't contain a mapping to the required character,
             do a compatibility decomposition and try mapping those */
          FXString dcm = FX::FXString::decompose(src.mid(i,src.extent(i)),false);
          for (FXint j=0;j<dcm.length();j+=dcm.inc(j)) {
            if (Unicode::isPrint(dcm.wc(j)) && codec->wc2mb(&c,1,dcm.wc(j)) && c!=undefined_character) {
              dst[n++]=c;
              }
            }
          }
        else {
          dst[n++]=c;
          }
        }
      }
    }
  return dst;
  }


FXString TextConverter::convert_to_ascii(const FXString & input) const {
  FXString src = FX::FXString::decompose(input,false);
  FXString dst;
  FXint i,n=0;

  for (i=0;i<src.length();i++) {

    if (Ascii::isSpace(src[i])) {

      // simplify white space
      while (Ascii::isSpace(src[i+1]))
        i++;

      // don't start or end with a space
      if (n>0 && i+1<src.length())
        n++;

      }

    // Allow only printable
    else if (Ascii::isPrint(src[i]) && forbidden.find(src[i])==-1)
      n++;
    }

  if (n) {
    dst.length(n);
    for (i=0,n=0;i<src.length();i++) {
      if (Ascii::isSpace(src[i])) {

        // simplify white space
        while (Ascii::isSpace(src[i+1]))
          i++;

        // don't start or end with a space
        if (n>0 && i+1<src.length()) {
          if (modifiers&NOSPACE)
            dst[n++] = '_';
          else
            dst[n++] = ' ';
          }

        }

      // Allow only printable
      else if (Ascii::isPrint(src[i]) && forbidden.find(src[i])==-1) {
        if (modifiers&LOWERCASE)
          dst[n++] = Ascii::toLower(src[i]);
        else if (modifiers&UPPERCASE)
          dst[n++] = Ascii::toUpper(src[i]);
        else
          dst[n++] = src[i];
        }
      }
    }
  return dst;
  }

FXString TextConverter::convert_to_codec(const FXString & input) const {
  FXASSERT(codec);

  FXString output = apply_filters(input);

  if (codec->mibEnum()!=106) // not utf8
    output = apply_codec(output);

  return output;
  }


FXString TextConverter::convert(const FXString & input) const {
  if (codec==nullptr)
    return convert_to_ascii(input);
  else
    return convert_to_codec(input);
  }


/*----------------------------------------------------------------------------*/


const FXchar TrackFormatter::valid[]="TAPpGwcNndy";


TrackFormatter::TrackFormatter(const FXString & msk,const FXTextCodec * c, const FXString & f, FXuint m)
  : TextConverter(c,f,m), mask(msk) {
  }

TrackFormatter::TrackFormatter(const FXString & msk,const FXTextCodec * c)
  : TextConverter(c,FXString::null,0), mask(msk) {
  }


FXString TrackFormatter::get_field(const FXchar field,const GMTrack & track) const {
  switch(field) {
    case 'T': return convert(track.title); break;
    case 'A': return convert(track.album); break;
    case 'P': return convert(track.album_artist); break;
    case 'p': return convert(track.artist); break;
    case 'G': return convert(track.tags[0]); break;
    case 'w': return convert(track.composer); break;
    case 'c': return convert(track.conductor); break;
    case 'N': return FXString::value("%.2d",track.getTrackNumber()); break;
    case 'n': return FXString::value(track.getTrackNumber());  break;
    case 'd': return FXString::value(track.getDiscNumber()); break;
    case 'y': return FXString::value(track.year); break;
    }
  return FXString::null;
  }


FXbool TrackFormatter::has_field(const FXchar field,const GMTrack & track,FXString & value) const {
  switch(field) {
    case 'N':
    case 'n': if (track.getTrackNumber()==0) return false; break;
    case 'd': if (track.getDiscNumber()==0) return false; break;
    case 'y': if (track.year==0) return false; break;
    default : break;
    };
  value = get_field(field,track);
  return !value.empty();
  }


FXString TrackFormatter::format_fields(const GMTrack & track,const FXString & path) const{
  FXString field;
  FXString result;
  FXwchar w;

  /// Condition Stack
  FXint depth=0;
  FXArray<FXbool> cs(1);
  cs[0]=true;


  /// Parse the field entries
  for (FXint i=0;i<path.length();i=path.inc(i)){

    /// Check for end of condition or else statement
    if (depth) {

      /// Else clause, the condition state is the opposite of the 'if clause', but only if
      /// the parent condition is true. if the parent clause is false, than both if and else clause
      /// should be false and we shouldn't change it.
      if (path[i]=='|') {
        if (cs[depth-1]==true)
          cs[depth]=!cs[depth];
        continue;
        }
      else if (path[i]=='>') {
        depth--;
        cs.erase(cs.no()-1);
        continue;
        }
      }

    /// Check beginning of conditional
    if (path[i]=='?' && strchr(valid,path[i+1]) ) {

      /// Condition with if else clause
      if (path[i+2]=='<') {
        /// condition is true if eval_fied returns true and the parent condition is also true.
        cs.append(cs[depth] && has_field(path[i+1],track,field));
        i+=2;
        depth++;
        continue;
        }
      /// Simplified condition just display if not empty
      else if (cs[depth]) {
        if (has_field(path[i+1],track,field)) {
          result+=field;
          }
        i+=1;
        continue;
        }
      }

    /// only add stuff if our current condition allows it
    if (cs[depth]) {

      /// get field
      if (path[i]=='%' && strchr(valid,path[i+1]) ) {
        field = get_field(path[i+1],track);
        if (field.empty())
          result.trim();
        else
          result+=field;
        i+=1;
        continue;
        }

      /// Add anything else
      w = path.wc(i);
      result.append(&w,1);
      }

    }
  result.trim().simplify();
  return result;
  }


FXString TrackFormatter::getPath(const GMTrack & track) const {

  // Expand Environment Variables and such...
  FXString path = FXPath::expand(mask);

  // Make absolute
  if (!FXPath::isAbsolute(path)) {
    path = FXPath::absolute(FXPath::directory(track.url),path);
    }

  // Simplify things
  path = FXPath::simplify(path);

  // Format field entries
  FXString result = format_fields(track,path);

  // Add extension
  result+=".";
  if (modifiers&LOWERCASE_EXTENSION || modifiers&LOWERCASE)
    result.append(FXPath::extension(track.url).lower());
  else
    result.append(FXPath::extension(track.url));

  return result;
  }


FXString TrackFormatter::getName(const GMTrack & track) const {
  return format_fields(track,mask);
  }

}


using namespace GMFilename;

namespace GMFilename {

void parse(GMTrack & track,const FXString & mask,FXuint options) {
  FXint nsep=0,i,j;
  FXString input,field;
  FXchar sep,item;
  FXint beg=0,end=0;

  // Determine number of path separators
  for (i=0;i<mask.length();i++){
    if (mask[i]=='\\') i+=1;
    else if (mask[i]=='/') nsep++;
    }

  /// Remove path components from the front that are not part of the mask
  /// Or if the mask doesn't specifiy path components, we just take the title of the filename.
  if (nsep) {
    input=FXPath::title(track.url);
    FXString dir=FXPath::directory(track.url);
    for (i=0;i<nsep;i++) {
      input = FXPath::name(dir) + PATHSEPSTRING + input;
      dir=FXPath::upLevel(dir);
      }
    }
  else {
    input=FXPath::title(track.url);
    }

  // Start from fresh
  if (options&OVERWRITE)
    track.clear();

//  fxmessage("filename: %s\n",track.mrl.text());
//  fxmessage("mask: %s\n",mask.text());
//  fxmessage("input to scan: %s \n",input.text());


  for (i=0;i<mask.length()&&beg<input.length();i++) {
    if (mask[i]=='%' && ((i+1)<mask.length()) && ( i==0 || mask[i-1]!='\\')) {
      i+=1;
      item=mask[i];
      j=i+1;

      /// Determine Separator
      sep=' ';
      if (j>=mask.length()) {
        sep='\0';
        }
      else if (mask[j]!='%') {
        i++;
        if (mask[j]=='\\') {
          j++;i++;
          if (j<mask.length())
            sep=mask[j];
          else
            sep=' ';
          }
        else {
          sep=mask[j];
          }
        }

      /// Get substring until separator
      end=beg;
      while(input[end]!=sep && end<input.length()) end++;

      if (end-beg>0) {
        field=input.mid(beg,end-beg).simplify();
        if (options&REPLACE_UNDERSCORE) field.substitute('_',' ');
        switch(item) {
          case 'T' : if (options&OVERWRITE || track.title.empty()) track.title.adopt(field); break;
          case 'P' : if (options&OVERWRITE || track.album_artist.empty()) track.album_artist.adopt(field); break;
          case 'A' : if (options&OVERWRITE || track.album.empty()) track.album.adopt(field); break;
          case 'p' : if (options&OVERWRITE || track.artist.empty()) track.artist.adopt(field); break;
          case 'n' :
          case 'N' : if (options&OVERWRITE) track.setTrackNumber(field.toInt()); break;
          case 'd' : if (options&OVERWRITE) track.setDiscNumber(field.toInt()); break;
          case 'y' : if (options&OVERWRITE || track.year == 0) track.year = field.toInt(); break;
          default  : break;
          }
        //fxmessage("%%%c=\"%s\"\n",item,field.text());
        beg=end+1;
        }
      }
    else {
      //fxmessage("eat %c: %s\n",mask[i],&input[beg]);
      while(input[beg]!=mask[i] && beg<input.length()) beg++;
      beg++;
      }
    }
  }
}
