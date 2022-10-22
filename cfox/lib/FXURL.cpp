/********************************************************************************
*                                                                               *
*                       U R L   M a n i p u l a t i o n                         *
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
#include "FXStream.h"
#include "FXString.h"
#include "FXPath.h"
#include "FXSystem.h"
#include "FXURL.h"


/*
  Notes:

  - Functions contributed by Sean Hubbell and Sander Jansen.

  - About drive letters in URL's, Daniel Gehriger has some some
    empirical tests, and determined the following:

     NS = works on Netscape
     IE = works on IE
     O  = works on Opera

     - file:///C|/TEMP/                    NS, IE, O
     - file:///C:/TEMP/                    NS, IE, O

     - file://localhost/C:/TEMP/           NS, IE, O
     - file://localhost/C|/TEMP/           NS, IE, O

     - file://C:/TEMP/                     NS, IE, --
     - file:///C/TEMP/                     --, --, --

    The conclusion seems to be we should probably try to handle all
    of these possibilities, although keeping the ':' seems favorable.

  - Syntax (as per rfc3986):

      URI           =  scheme ":" hier-part [ "?" query ] [ "#" fragment ]

      hier-part     =  "//" authority path-abempty
                    /  path-absolute
                    /  path-rootless
                    /  path-empty

      URI-reference =  URI / relative-ref

      absolute-URI  =  scheme ":" hier-part [ "?" query ]

      relative-ref  =  relative-part [ "?" query ] [ "#" fragment ]

      relative-part =  "//" authority path-abempty
                    /  path-absolute
                    /  path-noscheme
                    /  path-empty

      scheme        =  ALPHA  *( ALPHA / DIGIT / "+" / "-" / "." )

      authority     =  [ userinfo "@" ] host [ ":" port ]

      userinfo      =  *( unreserved / pct-encoded / sub-delims / ":" )

      host          =  IP-literal / IPv4address / reg-name

      IP-literal    =  "[" ( IPv6address / IPvFuture  ) "]"

      IPvFuture     =  "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )

      IPv6address   =                             6( h16 ":" ) ls32
                    /                        "::" 5( h16 ":" ) ls32
                    /  [               h16 ] "::" 4( h16 ":" ) ls32
                    /  [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
                    /  [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
                    /  [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
                    /  [ *4( h16 ":" ) h16 ] "::"              ls32
                    /  [ *5( h16 ":" ) h16 ] "::"              h16
                    /  [ *6( h16 ":" ) h16 ] "::"

      ls32          =  ( h16 ":" h16 ) / IPv4address                   ; least-significant 32 bits of address

      h16           = 1*4HEXDIG                                        ; 16 bits of address represented in hexadecimal


      IPv4address   =  dec-octet "." dec-octet "." dec-octet "." dec-octet

      dec-octet     =  DIGIT                                            ; 0-9
                    /  %x31-39 DIGIT                                    ; 10-99
                    /  "1" 2DIGIT                                       ; 100-199
                    /  "2" %x30-34 DIGIT                                ; 200-249
                    /  "25" %x30-35                                     ; 250-255

      reg-name      = *( unreserved / pct-encoded / sub-delims )

      port          =  *DIGIT

      path          =  path-abempty                                     ; begins with "/" or is empty
                    /  path-absolute                                    ; begins with "/" but not "//"
                    /  path-noscheme                                    ; begins with a non-colon segment
                    /  path-rootless                                    ; begins with a segment
                    /  path-empty                                       ; zero characters

      path-abempty  =  *( "/" segment )

      path-absolute =  "/" [ segment-nz *( "/" segment ) ]

      path-noscheme =  segment-nz-nc *( "/" segment )

      path-rootless =  segment-nz *( "/" segment )

      path-empty    =  0<pchar>

      segment       =  *pchar

      segment-nz    =  1*pchar

      segment-nz-nc =  1*( unreserved / pct-encoded / sub-delims / "@" ) ; non-zero-length segment without any colon ":"

      pchar         =  unreserved / pct-encoded / sub-delims / ":" / "@"

      query         =  *( pchar / "/" / "?" )

      fragment      =  *( pchar / "/" / "?" )

      pct-encoded   =  "%" HEXDIG HEXDIG

      unreserved    =  ALPHA / DIGIT / "-" / "." / "_" / "~"

      reserved      =  gen-delims / sub-delims

      gen-delims    =  ":" / "/" / "?" / "#" / "[" / "]" / "@"

      sub-delims    =  "!" / "$" / "&" / "'" / "(" / ")"
                    /  "*" / "+" / "," / ";" / "="

  - Also, encode all non-ascii bytes from a string.
*/

#define ENCODE_THESE "<>#%{}|^~[]`\"?$&'*,;="           // Encode these for pathnames

using namespace FX;

/*******************************************************************************/

namespace FX {

// Character classes
enum {
  UNRESERVED =  1,
  PERCENT    =  2,
  SUBDELIM   =  4,
  GENDELIM   =  8,
  PATHCHAR   = 16,
  QUERYCHAR  = 32
  };


// Table of character classes
static const FXuchar properties[256]={
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x10,0x04,0x00,0x08,0x04,0x32,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x01,0x01,0x38,
  0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x38,0x04,0x00,0x04,0x00,0x28,
  0x38,0x03,0x03,0x03,0x03,0x03,0x03,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x08,0x10,0x08,0x00,0x01,
  0x00,0x03,0x03,0x03,0x03,0x03,0x03,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x01,0x00,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  };


// URL parts
class URL {
public:
  FXint prot[2];
  FXint user[2];
  FXint pass[2];
  FXint host[2];
  FXint port[2];
  FXint path[2];
  FXint quer[2];
  FXint frag[2];
public:
  URL(const FXString& string);
  };


// Parse string to url parts
URL::URL(const FXString& string){
  FXint s=0;
  FXuchar c;

  prot[0]=prot[1]=0;

  // Parse protocol
  if(Ascii::isLetter(string[0])){
    s++;

    // Scan till end of scheme name
    while(Ascii::isAlphaNumeric(string[s]) || string[s]=='+' || string[s]=='-' || string[s]=='.') s++;

    // Scheme end found
    if(string[s]==':' && s>1){
      prot[1]=s++;
      }
    else{
      s=prot[0];                                // Reset:- wasn't protocol after all since no ':' found
      }
    }

  user[0]=user[1]=s;
  pass[0]=pass[1]=s;
  host[0]=host[1]=s;
  port[0]=port[1]=s;

  // Parse hier part
  if(string[s]=='/' && string[s+1]=='/'){
    s+=2;

    // Parse username
    user[0]=s;
    while((c=string[s])!='\0' && (properties[c]&(UNRESERVED|SUBDELIM|PERCENT))){
      s++;
      }

    // Parse password
    user[1]=pass[0]=s;
    if(string[s]==':'){
      pass[0]=++s;
      while((c=string[s])!='\0' && (properties[c]&(UNRESERVED|SUBDELIM|PERCENT))){
        s++;
        }
      }
    pass[1]=s;

    // Check for @ after user:pass
    if(string[s]=='@'){
      s++;
      }
    else{
      s=pass[0]=pass[1]=user[1]=user[0];        // Reset:- wasn't user:pass after all since no '@' found
      }

    // Parse hostname
    host[0]=s;
    while((c=string[s])!='\0' && (properties[c]&(UNRESERVED|SUBDELIM|PERCENT))){
      s++;
      }

    // Parse port number
    host[1]=port[0]=s;
    if(string[s]==':'){
      port[0]=++s;
      while(Ascii::isDigit(string[s])) s++;
      }
    port[1]=s;
    }

  // Parse path, allowing for \ path delimiters (legacy urls)
  path[0]=s;
  while((c=string[s])!='\0' && (properties[c]&(UNRESERVED|SUBDELIM|PATHCHAR))){
    s++;
    }

  // Parse query
  path[1]=quer[0]=s;
  if(string[s]=='?'){
    quer[0]=++s;
    while((c=string[s])!='\0' && (properties[c]&(UNRESERVED|SUBDELIM|QUERYCHAR))){
      s++;
      }
    }

  // Parse fragment
  quer[1]=frag[0]=s;
  if(string[s]=='#'){
    frag[0]=++s;
    while((c=string[s])!='\0' && (properties[c]&(UNRESERVED|SUBDELIM|QUERYCHAR))){
      s++;
      }
    }
  frag[1]=s;
  }


// Encode control characters and characters from set using %-encoding
FXString FXURL::encode(const FXString& url,const FXchar* set){
  FXString result;
  if(!url.empty()){
    FXint p,q,c;
    for(p=q=0; p<url.length(); ++p){
      c=(FXuchar)url[p];
      if(c<0x20 || 128<=c || c=='%' || (set && strchr(set,c))){
        q+=3;
        continue;
        }
      q++;
      }
    result.length(q);
    for(p=q=0; p<url.length(); ++p){
      c=(FXuchar)url[p];
      if(c<0x20 || 128<=c || c=='%' || (set && strchr(set,c))){
        result[q++]='%';
        result[q++]=FXString::value2Digit[c>>4];
        result[q++]=FXString::value2Digit[c&15];
        continue;
        }
      result[q++]=c;
      }
    }
  return result;
  }


// Decode string containing %-encoded characters
FXString FXURL::decode(const FXString& url){
  FXString result;
  if(!url.empty()){
    FXint p,q,c;
    for(p=q=0; p<url.length(); ++p){
      c=(FXuchar)url[p];
      if(c=='%' && Ascii::isHexDigit(url[p+1]) && Ascii::isHexDigit(url[p+2])){
        p+=2;
        }
      q++;
      }
    result.length(q);
    for(p=q=0; p<url.length(); ++p){
      c=(FXuchar)url[p];
      if(c=='%' && Ascii::isHexDigit(url[p+1]) && Ascii::isHexDigit(url[p+2])){
        c=(Ascii::digitValue(url[p+1])<<4)+Ascii::digitValue(url[p+2]);
        p+=2;
        }
      result[q++]=c;
      }
    }
  return result;
  }

/*******************************************************************************/

// Convert path from using 'sepfm' to use 'septo' path-separators
static FXString convertPathSep(const FXString& file,FXchar septo,FXchar sepfm){
  if(!file.empty()){
    FXString result(file);
    FXint p=0;
    FXint q=0;
#if defined(WIN32)
    if(result[q]==sepfm || result[q]==septo){                   // UNC
      result[p++]=septo; q++;
      if(result[q]==sepfm || result[q]==septo){
        result[p++]=septo; q++;
        while(result[q]==sepfm || result[q]==septo) q++;
        }
      }
    else if(Ascii::isLetter(result[q]) && result[q+1]==':'){    // C:
      result[p++]=result[q++];
      result[p++]=':'; q++;
      if(result[q]==sepfm || result[q]==septo){
        result[p++]=septo; q++;
        while(result[q]==sepfm || result[q]==septo) q++;
        }
      }
    while(result[q]){
      if(result[q]==sepfm || result[q]==septo){                 // FIXME don't convert escaped path separators!!
        result[p++]=septo; q++;
        while(result[q]==sepfm || result[q]==septo) q++;
        continue;
        }
      result[p++]=result[q++];
      }
    return result.trunc(p);
#else
    if(result[q]==sepfm || result[q]==septo){
      result[p++]=septo; q++;
      while(result[q]==sepfm || result[q]==septo) q++;
      }
    while(result[q]){
      if(result[q]==sepfm || result[q]==septo){                 // FIXME don't convert escaped path separators!!
        result[p++]=septo; q++;
        while(result[q]==sepfm || result[q]==septo) q++;
        continue;
        }
      result[p++]=result[q++];
      }
    return result.trunc(p);
#endif
    }
  return FXString::null;
  }


/*******************************************************************************/

// Return URL of filename
FXString FXURL::fileToURL(const FXString& file){
#ifdef WIN32
  if(ISPATHSEP(file[0]) && ISPATHSEP(file[1])){
    return "file:"+encode(convertPathSep(file,'/','\\'),ENCODE_THESE);         // file://share/path-with-slashes
    }
  if(Ascii::isLetter(file[0]) && file[1]==':'){
    return "file:///"+encode(convertPathSep(file,'/','\\'),ENCODE_THESE);      // file:///c:/path-with-slashes
    }
  return "file://"+encode(convertPathSep(file,'/','\\'),ENCODE_THESE);         // file://path-with-slashes
#else
  return "file://"+encode(file,ENCODE_THESE);                                   // file://path
#endif
  }


// Return filename from URL, empty if url is not a local file
FXString FXURL::fileFromURL(const FXString& string){
  if(FXString::comparecase(string,"file:",5)==0){
#ifdef WIN32
    URL url(string);
    if(url.host[0]<url.host[1]){
      return "\\\\"+string.mid(url.host[0],url.host[1]-url.host[0])+decode(convertPathSep(string.mid(url.path[0],url.path[1]-url.path[0]),'\\','/'));
      }
    return decode(convertPathSep(string.mid(url.path[0],url.path[1]-url.path[0]),'\\','/'));
#else
    URL url(string);
    return decode(string.mid(url.path[0],url.path[1]-url.path[0]));
#endif
    }
  return FXString::null;
  }

/*******************************************************************************/

// Make URI list from array of filenames
FXString FXURL::filesToURIList(const FXString* files){
  FXString result;
  if(files){
    FXint n=0;
    while(!files[n].empty()){
      result.append(FXURL::fileToURL(files[n++]));
      result.append("\r\n");
      }
    }
  return result;
  }


// Make array of filenames from URI list
FXString* FXURL::filesFromURIList(const FXString& urilist){
  FXString* result=nullptr;
  if(!urilist.empty()){
    FXint beg,end,n=0;
    result=new FXString [urilist.contains("\r\n")+2];
    for(beg=n=0; beg<urilist.length(); beg=end+2){
      if((end=urilist.find("\r\n",beg))<0) end=urilist.length();
      result[n++]=FXURL::fileFromURL(urilist.mid(beg,end-beg));
      }
    }
  return result;
  }

/*******************************************************************************/

// Parse scheme from url
FXString FXURL::scheme(const FXString& string){
  URL url(string);
  return string.mid(url.prot[0],url.prot[1]-url.prot[0]);
  }


// Parse username from string containing url
FXString FXURL::username(const FXString& string){
  URL url(string);
  return string.mid(url.user[0],url.user[1]-url.user[0]);
  }


// Parse password from string containing url
FXString FXURL::password(const FXString& string){
  URL url(string);
  return string.mid(url.pass[0],url.pass[1]-url.pass[0]);
  }


// Parse hostname from string containing url
FXString FXURL::host(const FXString& string){
  URL url(string);
  return string.mid(url.host[0],url.host[1]-url.host[0]);
  }


// Parse port number from string containing url
FXint FXURL::port(const FXString& string,FXint def){
  FXint result=def;
  URL url(string);
  if(url.port[0]<url.port[1]){
    result=Ascii::digitValue(string[url.port[0]++]);
    while(url.port[0]<url.port[1]){
      result=result*10+Ascii::digitValue(string[url.port[0]++]);
      }
    }
  return result;
  }


// Parse path from string containing url
FXString FXURL::path(const FXString& string){
  URL url(string);
  return string.mid(url.path[0],url.path[1]-url.path[0]);
  }


// Parse query from string containing url
FXString FXURL::query(const FXString& string){
  URL url(string);
  return string.mid(url.quer[0],url.quer[1]-url.quer[0]);
  }


// Parse fragment from string containing url
FXString FXURL::fragment(const FXString& string){
  URL url(string);
  return string.mid(url.frag[0],url.frag[1]-url.frag[0]);
  }

}
