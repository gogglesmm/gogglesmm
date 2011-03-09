/********************************************************************************
*                                                                               *
*                       U R L   M a n i p u l a t i o n                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2010 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "gmdefs.h"
#if FOXVERSION < FXVERSION(1,7,0)

#define UNRESERVED   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~" // Unreserved characters
#define IPV6DIGITS   "abcdefABCDEF0123456789:."                                           // Stuff in IPv6 numbers
#define PCTENCODED   "%0123456789abcdefABCDEF"                                            // Percent encoded characters
#define GENDELIMS    ":/?#[]@"                                                            // General delimiters
#define SUBDELIMS    "!$&'()*+,;="                                                        // Sub-delimiters
#define RESERVED     ":/?#[]@!$&'()*+,;="                                                 // Reserved characters (GENDELIMS + SUBDELIMS)
#define UNSAFE       "<>#%{}|^~[]`\" "                                                    // Unsafe characters
#define ENCODE_THESE "<>#%{}|^~[]`\"?$&'*,;="                                             // Encode these for pathnames

using namespace FX;

/*******************************************************************************/

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
  register FXint s=0;

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
    while(string[s] && strchr(UNRESERVED SUBDELIMS "%",string[s])){
      s++;
      }

    // Parse password
    user[1]=pass[0]=s;
    if(string[s]==':'){
      pass[0]=++s;
      while(string[s] && strchr(UNRESERVED SUBDELIMS "%",string[s])){
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
    while(string[s] && strchr(UNRESERVED SUBDELIMS "%",string[s])){
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

#ifdef WIN32
  // Parse path, allowing for \ path delimiters (legacy urls)
  path[0]=s;
  while(string[s] && strchr(UNRESERVED SUBDELIMS "%:@/\\ ",string[s])){
    s++;
    }
#else
  // Parse path
  path[0]=s;
  while(string[s] && strchr(UNRESERVED SUBDELIMS "%:@/ ",string[s])){
    s++;
    }
#endif

  // Parse query
  path[1]=quer[0]=s;
  if(string[s]=='?'){
    quer[0]=++s;
    while(string[s] && strchr(UNRESERVED SUBDELIMS "%:@/?",string[s])){
      s++;
      }
    }

  // Parse fragment
  quer[1]=frag[0]=s;
  if(string[s]=='#'){
    frag[0]=++s;
    while(string[s] && strchr(UNRESERVED SUBDELIMS "%:@/?",string[s])){
      s++;
      }
    }
  frag[1]=s;
  }


// Encode control characters and characters from set using %-encoding
FXString GMURL::encode(const FXString& url,const FXchar* set){
  FXString result;
  if(!url.empty()){
    register FXint p,q,c;
    for(p=q=0; p<url.length(); ++p){
      c=(FXuchar)url[p];
      if(c<0x20 || c=='%' || (set && strchr(set,c))){
        q+=3;
        continue;
        }
      q++;
      }
    result.length(q);
    for(p=q=0; p<url.length(); ++p){
      c=(FXuchar)url[p];
      if(c<0x20 || c=='%' || (set && strchr(set,c))){
        result[q++]='%';
        result[q++]=FXString::hex[(c>>4)&15];
        result[q++]=FXString::hex[c&15];
//        result[q++]=FXString::value2Digit[c>>4];
//        result[q++]=FXString::value2Digit[c&15];
        continue;
        }
      result[q++]=c;
      }
    }
  return result;
  }


// Decode string containing %-encoded characters
FXString GMURL::decode(const FXString& url){
  FXString result;
  if(!url.empty()){
    register FXint p,q,c;
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


// Return URL of filename
FXString GMURL::fileToURL(const FXString& file){
#ifdef WIN32
  if(ISPATHSEP(file[0]) && ISPATHSEP(file[1])){
    return "file:"+encode(FXPath::convert(file,'/','\\'),ENCODE_THESE);         // file://share/path-with-slashes
    }
  if(Ascii::isLetter(file[0]) && file[1]==':'){
    return "file:///"+encode(FXPath::convert(file,'/','\\'),ENCODE_THESE);      // file:///c:/path-with-slashes
    }
  return "file://"+encode(FXPath::convert(file,'/','\\'),ENCODE_THESE);         // file://path-with-slashes
#else
  return "file://"+encode(file,ENCODE_THESE);                                   // file://path
#endif
  }


// Return filename from URL, empty if url is not a local file
FXString GMURL::fileFromURL(const FXString& string){
  if(string[0]=='f' && string[1]=='i' && string[2]=='l' && string[3]=='e' && string[4]==':'){
#ifdef WIN32
    URL url(string);
    if(url.host[0]<url.host[1]){
      return "\\\\"+string.mid(url.host[0],url.host[1]-url.host[0])+decode(FXPath::convert(string.mid(url.path[0],url.path[1]-url.path[0]),'\\','/'));
      }
    return decode(FXPath::convert(string.mid(url.path[0],url.path[1]-url.path[0]),'\\','/'));
#else
    URL url(string);
    return decode(string.mid(url.path[0],url.path[1]-url.path[0]));
#endif
    }
  return FXString::null;
  }


// Parse scheme from url
FXString GMURL::scheme(const FXString& string){
  URL url(string);
  return string.mid(url.prot[0],url.prot[1]-url.prot[0]);
  }


// Parse username from string containing url
FXString GMURL::username(const FXString& string){
  URL url(string);
  return string.mid(url.user[0],url.user[1]-url.user[0]);
  }


// Parse password from string containing url
FXString GMURL::password(const FXString& string){
  URL url(string);
  return string.mid(url.pass[0],url.pass[1]-url.pass[0]);
  }


// Parse hostname from string containing url
FXString GMURL::host(const FXString& string){
  URL url(string);
  return string.mid(url.host[0],url.host[1]-url.host[0]);
  }


// Parse port number from string containing url
FXint GMURL::port(const FXString& string){
  register FXint result=0;
  URL url(string);
  while(url.port[0]<url.port[1]){
    result=result*10+Ascii::digitValue(string[url.port[0]++]);
    }
  return result;
  }


// Parse path from string containing url
FXString GMURL::path(const FXString& string){
  URL url(string);
  return string.mid(url.path[0],url.path[1]-url.path[0]);
  }


// Parse query from string containing url
FXString GMURL::query(const FXString& string){
  URL url(string);
  return string.mid(url.quer[0],url.quer[1]-url.quer[0]);
  }


// Parse fragment from string containing url
FXString GMURL::fragment(const FXString& string){
  URL url(string);
  return string.mid(url.frag[0],url.frag[1]-url.frag[0]);
  }

#endif
