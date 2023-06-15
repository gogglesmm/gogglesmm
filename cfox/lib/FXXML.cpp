/********************************************************************************
*                                                                               *
*                       X M L   R e a d e r  &  W r i t e r                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2016,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxchar.h"
#include "fxmath.h"
#include "fxascii.h"
#include "fxunicode.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXString.h"
#include "FXParseBuffer.h"
#include "FXException.h"
#include "FXStringDictionary.h"
#include "FXCallback.h"
#include "FXXML.h"

/*
  Notes:

  XML in Extended Backus-Naur form:
  =================================

  - First couple of bytes are tested to determine encoding used in the stream, which is made available
    in the 'enc' member variable.  The parser expects the subclass to use this variable to deliver UTF8
    into the parse buffer, however.  Therefore, the fill() API must fetch bytes from the file/device,
    decode them (if not already in UTF8 format), and leave the UTF8 in the parse buffer.

  - Grammar rules:

    //// Document structure syntax
    document             ::=    ( prolog element Misc* ) - ( Char* RestrictedChar Char* )

    //// XML Header syntax
    prolog               ::=    XMLDecl Misc* (doctypedecl Misc*)?
    XMLDecl              ::=    '<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>'
    VersionInfo          ::=    S 'version' Eq ("'" VersionNum "'" | '"' VersionNum '"')
    VersionNum           ::=    '1.1'
    EncodingDecl         ::=    S 'encoding' Eq ('"' EncName '"' | "'" EncName "'" )
    EncName              ::=    [A-Za-z] ([A-Za-z0-9._] | '-')*
    SDDecl               ::=    S 'standalone' Eq (("'" ('yes' | 'no') "'") | ('"' ('yes' | 'no') '"'))

    //// Document type syntax
    doctypedecl          ::=    '<!DOCTYPE' S Name (S ExternalID)? S? ('[' intSubset ']' S?)? '>'

    ExternalID           ::=    'SYSTEM' S SystemLiteral | 'PUBLIC' S PubidLiteral S SystemLiteral
    SystemLiteral        ::=    ('"' [^"]* '"') | ("'" [^']* "'")
    PublicID             ::=    'PUBLIC' S PubidLiteral
    PubidLiteral         ::=    '"' PubidChar* '"' | "'" (PubidChar - "'")* "'"
    PubidChar            ::=    #x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%]

    intSubset            ::=    (markupdecl | DeclSep)*
    markupdecl           ::=    elementdecl | AttlistDecl | EntityDecl | NotationDecl | PI | Comment
    DeclSep              ::=    PEReference | S
    PEReference          ::=    '%' Name ';'

    //// Element declaration
    elementdecl          ::=    '<!ELEMENT' S Name S contentspec S? '>'
    contentspec          ::=    'EMPTY' | 'ANY' | Mixed | children
    Mixed                ::=    '(' S? '#PCDATA' (S? '|' S? Name)* S? ')*' | '(' S? '#PCDATA' S? ')'
    children             ::=    (choice | seq) ('?' | '*' | '+')?
    choice               ::=    '(' S? cp ( S? '|' S? cp)+ S? ')'
    seq                  ::=    '(' S? cp ( S? ',' S? cp )* S? ')'
    cp                   ::=    (Name | choice | seq) ('?' | '*' | '+')?

    //// Attribute declaration
    AttlistDecl          ::=    '<!ATTLIST' S Name AttDef* S? '>'
    AttDef               ::=    S Name S AttType S DefaultDecl
    AttType              ::=    StringType | TokenizedType | EnumeratedType
    StringType           ::=    'CDATA'
    TokenizedType        ::=    'ID' | 'IDREF' | 'IDREFS' | 'ENTITY' | 'ENTITIES' | 'NMTOKEN' | 'NMTOKENS'
    EnumeratedType       ::=    NotationType | Enumeration
    NotationType         ::=    'NOTATION' S '(' S? Name (S? '|' S? Name)* S? ')'
    Enumeration          ::=    '(' S? Nmtoken (S? '|' S?  Nmtoken)* S? ')'
    Nmtoken              ::=    (NameChar)+
    Nmtokens             ::=    Nmtoken (#x20 Nmtoken)*

    //// Default declaration
    DefaultDecl          ::=    '#REQUIRED' | '#IMPLIED' | (('#FIXED' S)? AttValue)

    //// Entity declarations
    EntityDecl           ::=    GEDecl | PEDecl
    GEDecl               ::=    '<!ENTITY' S Name S EntityDef S? '>'
    EntityDef            ::=    EntityValue | (ExternalID NDataDecl?)
    EntityValue          ::=    '"' ([^%&"] | PEReference | Reference)* '"' |  "'" ([^%&'] | PEReference | Reference)* "'"
    NDataDecl            ::=    S 'NDATA' S Name
    PEDecl               ::=    '<!ENTITY' S '%' S Name S PEDef S? '>'
    PEDef                ::=    EntityValue | ExternalID

    //// Notation declaration
    NotationDecl         ::=    '<!NOTATION' S Name S (ExternalID | PublicID) S? '>'


    //// Element syntax (the beaf)
    element              ::=    EmptyElemTag | STag content ETag
    EmptyElemTag         ::=    '<' Name (S Attribute)* S? '/>'
    STag                 ::=    '<' Name (S Attribute)* S? '>'
    ETag                 ::=    '</' Name S? '>'

    Attribute            ::=    Name Eq AttValue
    AttValue             ::=    '"' ([^<&"] | Reference)* '"' |  "'" ([^<&'] | Reference)* "'"

    //// Element contents
    content              ::=    CharData? ((element | Reference | CDSect | PI | Comment) CharData?)*

    //// Character reference
    Reference            ::=    EntityRef | CharRef
    EntityRef            ::=    '&' Name ';'
    CharRef              ::=    '&#' [0-9]+ ';' | '&#x' [0-9a-fA-F]+ ';'

    CDSect               ::=    CDStart CData CDEnd
    CDStart              ::=    '<![CDATA['
    CData                ::=    (Char* - (Char* ']]>' Char*))
    CDEnd                ::=    ']]>'

    //// Non-content, skipped over stuff
    Misc                 ::=    Comment | PI | S

    //// Processing instruction
    PI                   ::=    '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
    PITarget             ::=    Name - (('X' | 'x') ('M' | 'm') ('L' | 'l'))

    //// Comments
    Comment              ::=    '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'


    //// Character classes
    S                    ::=    (#x20 | #x9 | #xD | #xA)+

    Eq                   ::=    S? '=' S?

    Char                 ::=    [#x1-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]

    RestrictedChar       ::=    [#x1-#x8] | [#xB-#xC] | [#xE-#x1F] | [#x7F-#x84] | [#x86-#x9F]

    CharData             ::=    [^<&]* - ([^<&]* ']]>' [^<&]*)

    //// Name identifier
    Name                 ::=    NameStartChar (NameChar)*
    Names                ::=    Name (#x20 Name)*
    NameStartChar        ::=    ":" | [A-Z] | "_" | [a-z] | [#xC0-#xD6] | [#xD8-#xF6] | [#xF8-#x2FF] |
                                [#x370-#x37D] | [#x37F-#x1FFF] | [#x200C-#x200D] | [#x2070-#x218F] |
                                [#x2C00-#x2FEF] | [#x3001-#xD7FF] | [#xF900-#xFDCF] | [#xFDF0-#xFFFD] |
                                [#x10000-#xEFFFF]
    NameChar             ::=    NameStartChar | "-" | "." | [0-9] | #xB7 | [#x0300-#x036F] | [#x203F-#x2040]

    //// Unreachable productions ??
    conditionalSect      ::=    includeSect | ignoreSect
    includeSect          ::=    '<![' S? 'INCLUDE' S? '[' extSubsetDecl ']]>'
    extSubsetDecl        ::=    ( markupdecl | conditionalSect | DeclSep)*
    extSubset            ::=    TextDecl? extSubsetDecl
    TextDecl             ::=    '<?xml' VersionInfo? EncodingDecl S? '?>'

    ignoreSect           ::=    '<![' S? 'IGNORE' S? '[' ignoreSectContents* ']]>'
    ignoreSectContents   ::=    Ignore ('<![' ignoreSectContents ']]>' Ignore)*
    Ignore               ::=    Char* - (Char* ('<![' | ']]>') Char*)

    extParsedEnt         ::=    ( TextDecl? content ) - ( Char* RestrictedChar Char* )

  - Output formatting:

      o Element without children is printed as:

          <tag/>

      o Element with only text is printed as:

          <tag>text</foo>

      o Element with children is printed as:

          <tag>
            ...
          </tag>

  - Report characters to application when:

      o Buffer was completely filled with last call to fill(), or wptr==endptr.
        If wptr<endptr, no more data is available so no need to make room for next
        call to fill().

      o Free space in buffer drops below a certain minimum.

      o We're not at token or (multibyte) character boundary.

  - The column may be incorrect if input string is not UTF8; column number is incremented
    for every character-start [not for UTF8 follower characters, in other words].
*/


#define MINBUFFER  256 //1024         // Minimum buffer size
#define MAXTOKEN   128           // Maximum token size


using namespace FX;

namespace FX {

/*******************************************************************************/

// Character properties
enum {
  SPACE  =  1,
  LEGIT  =  2,
  ESCAPE =  4,
  LEAD   =  8,
  FOLLOW = 16,
  PUBID  = 32
  };


// Character properties for each possible byte value
static const FXuchar property_data[256]={
  0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x03,0x23,0x04,0x04,0x23,0x04,0x04,
  0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,
  0x23,0x22,0x06,0x22,0x22,0x22,0x06,0x26,0x22,0x22,0x22,0x22,0x22,0x32,0x32,0x22,
  0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x2a,0x26,0x06,0x22,0x06,0x22,
  0x22,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,
  0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x02,0x02,0x02,0x02,0x2a,
  0x02,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,
  0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x02,0x02,0x02,0x02,0x04,
  0x16,0x16,0x16,0x16,0x12,0x12,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,
  0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x12,
  0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
  0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
  0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,
  0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,
  0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,
  0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,
  };


// Return non-zero if ch is tag start character
static inline FXuchar nameStartChar(FXuchar ch){
  return property_data[ch]&LEAD;
  }

// Return non-zero if ch is tag character
static inline FXuchar nameChar(FXuchar ch){
  return property_data[ch]&(LEAD|FOLLOW);
  }

// Return non-zero if ch is legal character
static inline FXuchar legalChar(FXuchar ch){
  return property_data[ch]&LEGIT;
  }

// Return non-zero if ch is restricted character
static inline FXuchar restrictedChar(FXuchar ch){
  return property_data[ch]&ESCAPE;
  }

// Return non-zero if ch is whitespace character
static inline FXuchar spaceChar(FXuchar ch){
  return property_data[ch]&SPACE;
  }


// Error messages
const FXchar *const FXXML::errors[]={
  "OK",
  "Empty",
  "Unable to save",
  "Unable to load",
  "Expected a space",
  "Expected a '='",
  "Expected a name",
  "Expected a string",
  "Illegal token",
  "Expected digit",
  "Expected hex digit",
  "Expected semicolon",
  "Unknown reference",
  "Unmatched tag",
  "Unexpected end of file"
  };


// Encoding names
static const FXchar *const encodingName[]={
  "UNKOWN","UTF8","UTF16LE","UTF16BE","UTF32LE","UTF32BE"
  };


/*******************************************************************************/

// Collect some element info during the parse
class FXXML::Element {
private:
  Element          **instance;          // Instance pointer
  Element           *upper;             // Upper pointer
public:
  FXString           name;              // Tag name
  FXStringDictionary attributes;        // Attribute list
  FXbool             empty;             // Element is empty
public:

  // Enter
  Element(Element** ins):instance(ins),upper(*ins),empty(false){ *instance=this; }

  // Leave
 ~Element(){ *instance=upper; }
  };

/*******************************************************************************/

// Decode escaped special characters from XML stream
FXbool FXXML::decode(FXString& dst,const FXString& src,FXuint flags){
  FXint p,q;
  FXwchar wc;

  // Measure the resulting string first
  p=q=0;
  while(q<src.length()){
    wc=src[q++];
    if(wc=='\r' && (flags&CRLF)){               // CR, CRLF -> LF
      if(src[q]=='\n'){ q++; }
      p++;
      continue;
      }
    if(wc=='\n' && (flags&CRLF)){               // LF, LFCR -> LF
      if(src[q]=='\r'){ q++; }
      p++;
      continue;
      }
    if(wc=='&' && (flags&REFS)){
      if(src[q]=='#'){
        if(src[q+1]=='x'){                      // &#xXXXX;
          q+=2;
          if(!Ascii::isHexDigit(src[q])) return false;  // Expected at least one hex digit
          wc=Ascii::digitValue(src[q++]);
          while(Ascii::isHexDigit(src[q])){
            wc=wc*16+Ascii::digitValue(src[q++]);
            }
          if(src[q++]!=';') return false;       // Expected semicolon
          }
        else{                                   // &#DDDD;
          q+=1;
          if(!Ascii::isDigit(src[q])) return false;     // Expected at least one digit
          wc=src[q++]-'0';
          while(Ascii::isDigit(src[q])){
            wc=wc*10+(src[q++]-'0');
            }
          if(src[q++]!=';') return false;       // Expected semicolon
          }
        p+=wc2utf(wc);
        continue;
        }
      if(src[q]=='q' && src[q+1]=='u' && src[q+2]=='o' && src[q+3]=='t' && src[q+4]==';'){      // &quot;
        q+=5;
        p++;
        continue;
        }
      if(src[q]=='a' && src[q+1]=='p' && src[q+2]=='o' && src[q+3]=='s' && src[q+4]==';'){      // &apos;
        q+=5;
        p++;
        continue;
        }
      if(src[q]=='a' && src[q+1]=='m' && src[q+2]=='p' && src[q+3]==';'){       // &amp;
        q+=4;
        p++;
        continue;
        }
      if(src[q]=='l' && src[q+1]=='t' && src[q+2]==';'){        // &lt;
        q+=3;
        p++;
        continue;
        }
      if(src[q]=='g' && src[q+1]=='t' && src[q+2]==';'){        // &gt;
        q+=3;
        p++;
        continue;
        }
      return false;                             // Unknown reference
      }
    p++;
    }

  // Now allocate space
  dst.length(p);

  // Now produce the result string
  p=q=0;
  while(q<src.length()){
    wc=src[q++];
    if(wc=='\r' && (flags&CRLF)){               // CR, CRLF -> LF
      if(src[q]=='\n'){ q++; }
      dst[p++]='\n';
      continue;
      }
    if(wc=='\n' && (flags&CRLF)){               // LF, LFCR -> LF
      if(src[q]=='\r'){ q++; }
      dst[p++]='\n';
      continue;
      }
    if(wc=='&' && (flags&REFS)){
      if(src[q]=='#'){
        if(src[q+1]=='x'){                      // &#xXXXX;
          q+=2;
          FXASSERT(Ascii::isHexDigit(src[q]));  // Expected at least one hex digit
          wc=Ascii::digitValue(src[q++]);
          while(Ascii::isHexDigit(src[q])){
            wc=wc*16+Ascii::digitValue(src[q++]);
            }
          FXASSERT(src[q]==';');                // Expected semicolon
          q++;
          }
        else{                                   // &#DDDD;
          q+=1;
          FXASSERT(Ascii::isDigit(src[q]));     // Expected at least one digit
          wc=src[q++]-'0';
          while(Ascii::isDigit(src[q])){
            wc=wc*10+(src[q++]-'0');
            }
          FXASSERT(src[q]==';');                // Expected semicolon
          q++;
          }
        p+=wc2utf(&dst[p],wc);
        continue;
        }
      if(src[q]=='q' && src[q+1]=='u' && src[q+2]=='o' && src[q+3]=='t' && src[q+4]==';'){      // &quot;
        q+=5;
        dst[p++]='\"';
        continue;
        }
      if(src[q]=='a' && src[q+1]=='p' && src[q+2]=='o' && src[q+3]=='s' && src[q+4]==';'){      // &apos;
        q+=5;
        dst[p++]='\'';
        continue;
        }
      if(src[q]=='a' && src[q+1]=='m' && src[q+2]=='p' && src[q+3]==';'){       // &amp;
        q+=4;
        dst[p++]='&';
        continue;
        }
      if(src[q]=='l' && src[q+1]=='t' && src[q+2]==';'){        // &lt;
        q+=3;
        dst[p++]='<';
        continue;
        }
      if(src[q]=='g' && src[q+1]=='t' && src[q+2]==';'){        // &gt;
        q+=3;
        dst[p++]='>';
        continue;
        }
      }
    dst[p++]=wc;
    }
  FXASSERT(p<=dst.length());
  return true;
  }

/*******************************************************************************/

// Encode special characters for inclusion into XML stream
FXbool FXXML::encode(FXString& dst,const FXString& src,FXuint flags){
  FXint p,q;
  FXuchar ch;

  // Measure the resulting string first
  p=q=0;
  while(q<src.length()){
    ch=src[q++];
    if(ch=='\n' && (flags&CRLF)){
#if defined(WIN32)
      p+=2;
#else
      p+=1;
#endif
      continue;
      }
    if(restrictedChar(ch) && (flags&REFS)){
      switch(ch){
      case '"':         // &quot;
        p+=6;
        continue;
      case '&':         // &amp;
        p+=5;
        continue;
      case '\'':        // &apos;
        p+=6;
        continue;
      case '<':         // &lt;
        p+=4;
        continue;
      case '>':         // &gt;
        p+=4;
        continue;
      default:          // &#XX;
        p+=6;
        continue;
        }
      }
    p++;
    }

  // Now allocate space
  dst.length(p);

  // Now produce the result string
  p=q=0;
  while(q<src.length()){
    ch=src[q++];
    if(ch=='\n' && (flags&CRLF)){
#if defined(WIN32)
      dst[p++]='\r';
      dst[p++]='\n';
#else
      dst[p++]='\n';
#endif
      continue;
      }
    if(restrictedChar(ch) && (flags&REFS)){
      switch(ch){
      case '"':         // &quot;
        dst[p+0]='&';
        dst[p+1]='q';
        dst[p+2]='u';
        dst[p+3]='o';
        dst[p+4]='t';
        dst[p+5]=';';
        p+=6;
        continue;
      case '&':         // &amp;
        dst[p+0]='&';
        dst[p+1]='a';
        dst[p+2]='m';
        dst[p+3]='p';
        dst[p+4]=';';
        p+=5;
        continue;
      case '\'':
        dst[p+0]='&';   // &apos;
        dst[p+1]='a';
        dst[p+2]='p';
        dst[p+3]='o';
        dst[p+4]='s';
        dst[p+5]=';';
        p+=6;
        continue;
      case '<':         // &lt;
        dst[p+0]='&';
        dst[p+1]='l';
        dst[p+2]='t';
        dst[p+3]=';';
        p+=4;
        continue;
      case '>':         // &gt;
        dst[p+0]='&';
        dst[p+1]='g';
        dst[p+2]='t';
        dst[p+3]=';';
        p+=4;
        continue;
      default:          // &#XX;
        dst[p+0]='&';
        dst[p+1]='#';
        dst[p+2]='x';
        dst[p+3]=Ascii::valueDigit(ch>>4);
        dst[p+4]=Ascii::valueDigit(ch&15);
        dst[p+5]=';';
        p+=6;
        continue;
        }
      }
    dst[p++]=ch;
    }
  FXASSERT(p<=dst.length());
  return true;
  }

/*******************************************************************************/

// Construct XML parser instance
FXXML::FXXML():offset(0),current(nullptr),column(0),line(1),enc(UTF8){
  FXTRACE((100,"FXXML::FXXML\n"));
  }


// Construct XML parser instance and pass it external buffer
FXXML::FXXML(FXchar* buffer,FXuval sz,Direction d):FXParseBuffer(buffer,sz,d),offset(0),current(nullptr),column(0),line(1),enc(UTF8){
  FXTRACE((100,"FXXML::FXXML(%p,%ld,%s)\n",buffer,sz,d==Load?"Load":d==Save?"Save":"Stop"));
  open(buffer,sz,d);
  }


// Open XML stream for given direction d
FXbool FXXML::open(FXchar* buffer,FXuval sz,Direction d){
  FXTRACE((101,"FXXML::open(%p,%ld,%s)\n",buffer,sz,d==Load?"Load":d==Save?"Save":"Stop"));
  if(FXParseBuffer::open(buffer,sz,d)){
    current=nullptr;
    column=0;
    offset=0;
    line=1;
    vers="1.0";
    enc=UTF8;
    return true;
    }
  return false;
  }

/*******************************************************************************/

// Boiler plate emitted prior to main document
static const FXchar boilerplate[]="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";


// Document start
FXXML::Error FXXML::startDocument(){
  if(dir==Save){
    if(!emit(boilerplate,sizeof(boilerplate)-1)) return ErrSave;
    offset+=sizeof(boilerplate)-1;
    return ErrOK;
    }
  return ErrSave;
  }


// Element start w/no attributes
FXXML::Error FXXML::startElement(const FXString& tag){
  if(dir==Save){
    if(!emit("<",1)) return ErrSave;
    offset+=1;
    if(!emit(tag.text(),tag.length())) return ErrSave;
    offset+=tag.length();
    if(!emit(">",1)) return ErrSave;
    offset+=1;
    return ErrOK;
    }
  return ErrSave;
  }


// Element start w/attributes
FXXML::Error FXXML::startElement(const FXString& tag,const FXStringDictionary& atts){
  FXString encodedatt;
  if(dir==Save){
    if(!emit("<",1)) return ErrSave;
    offset+=1;
    if(!emit(tag.text(),tag.length())) return ErrSave;
    offset+=tag.length();
    if(!atts.empty()){
      for(FXint i=0; i<atts.no(); ++i){
        if(!atts.empty(i)){
          if(!emit(" ",1)) return ErrSave;
          offset+=1;
          if(!emit(atts.key(i).text(),atts.key(i).length())) return ErrSave;
          offset+=atts.key(i).length();
          if(!emit("=\"",2)) return ErrSave;
          offset+=2;
          FXXML::encode(encodedatt,atts.data(i),REFS|CRLF);
          if(!emit(encodedatt.text(),encodedatt.length())) return ErrSave;
          offset+=encodedatt.length();
          if(!emit("\"",1)) return ErrSave;
          offset+=1;
          }
        }
      }
    if(!emit(">",1)) return ErrSave;
    offset+=1;
    return ErrOK;
    }
  return ErrSave;
  }


// Characters
FXXML::Error FXXML::characters(const FXString& text){
  if(dir==Save){
    FXString encodedtext;
    FXXML::encode(encodedtext,text,CRLF|REFS);
    if(!emit(encodedtext.text(),encodedtext.length())) return ErrSave;
    offset+=encodedtext.length();
    return ErrOK;
    }
  return ErrSave;
  }


// Comment
FXXML::Error FXXML::comment(const FXString& text){
  if(dir==Save){
    if(!emit("<!--",4)) return ErrSave;
    offset+=4;
    if(!emit(text.text(),text.length())) return ErrSave;
    offset+=text.length();
    if(!emit("-->",3)) return ErrSave;
    offset+=3;
    return ErrOK;
    }
  return ErrSave;
  }


// Processing instruction
FXXML::Error FXXML::processing(const FXString& target,const FXString& data){
  if(dir==Save){
    if(!emit("<?",2)) return ErrSave;
    offset+=2;
    if(!emit(target.text(),target.length())) return ErrSave;
    offset+=target.length();
    if(!data.empty()){
      if(!emit(" ",1)) return ErrSave;
      offset+=1;
      if(!emit(data.text(),data.length())) return ErrSave;
      offset+=data.length();
      }
    if(!emit("?>",2)) return ErrSave;
    offset+=2;
    return ErrOK;
    }
  return ErrSave;
  }


// Element end
FXXML::Error FXXML::endElement(const FXString& tag){
  if(dir==Save){
    if(!emit("</",2)) return ErrSave;
    offset+=2;
    if(!emit(tag.text(),tag.length())) return ErrSave;
    offset+=tag.length();
    if(!emit(">",1)) return ErrSave;
    offset+=1;
    return ErrOK;
    }
  return ErrSave;
  }


// Document end
FXXML::Error FXXML::endDocument(){
  if(dir==Save){
    if(!emit("\n",1)) return ErrSave;
    offset+=1;
    return ErrOK;
    }
  return ErrSave;
  }

/*******************************************************************************/

// Guess coded based on first 4 bytes of stream
// Careful: smallest (barely legal) XML file is 4 bytes: '<a/>'
// Check for non-zero bytes only: file may start with whitespace.
FXuint FXXML::guess(){

  // Quick check for UTF8 coded byte order mark (most likely case)
  if(sptr+3<=wptr && sptr[0]=='\xef' && sptr[1]=='\xbb' && sptr[2]=='\xbf'){ offset+=3; sptr+=3; return UTF8; }

  // Look for 32-bit byte order mark
  if(sptr+4<=wptr && sptr[0]=='\xff' && sptr[1]=='\xfe' && sptr[2]==0 && sptr[3]==0){ offset+=4; sptr+=4; return UTF32LE; }
  if(sptr+4<=wptr && sptr[0]==0 && sptr[1]==0 && sptr[2]=='\xfe' && sptr[3]=='\xff'){ offset+=4; sptr+=4; return UTF32BE; }

  // Look for 16-bit byte order mark
  if(sptr+2<=wptr && sptr[0]=='\xff' && sptr[1]=='\xfe'){ offset+=2; sptr+=2; return UTF16LE; }
  if(sptr+2<=wptr && sptr[0]=='\xfe' && sptr[1]=='\xff'){ offset+=2; sptr+=2; return UTF16BE; }

  // Otherwise, look for 32-bit wide characters
  if(sptr+4<=wptr && (sptr[0]!=0 || sptr[1]!=0) && (sptr[2]==0 && sptr[3]==0)){ return UTF32LE; }
  if(sptr+4<=wptr && (sptr[0]==0 && sptr[1]==0) && (sptr[2]!=0 || sptr[3]!=0)){ return UTF32BE; }

  // Finally, look for 16-bit wide characters
  if(sptr+2<=wptr && sptr[0]==0 && sptr[1]!=0){ return UTF16BE; }
  if(sptr+2<=wptr && sptr[0]!=0 && sptr[1]==0){ return UTF16LE; }

  // Unknown encoding
  return 0;
  }


// Parse just spaces
void FXXML::spaces(){
  while(need(MAXTOKEN)){
    rptr=sptr;
    switch(sptr[0]){
    case '\t':
      column+=(8-column%8);
      offset++;
      sptr++;
      continue;
    case '\r':
      if(sptr+1<wptr && sptr[1]=='\n'){ offset++; sptr++; }
    case '\n':
      column=0;
      offset++;
      sptr++;
      line++;
      continue;
    case ' ':
      column++;
    case '\v':
    case '\f':
      offset++;
      sptr++;
      continue;
    default:                            // We have a non-whitespace character
      return;
      }
    }
  }


// Parse name
FXbool FXXML::name(){
  if(nameStartChar(sptr[0])){
    rptr=sptr;
    do{
      column+=isUTF8(*sptr);            // Increment if UTF8 leader only
      offset++;
      sptr++;
      if(sptr>=wptr) return false;
      }
    while(nameChar(sptr[0]));
    return true;
    }
  return false;
  }


// Match name with given name str of length len
FXbool FXXML::match(FXchar ch){
  if(sptr[0]==ch){
    rptr=sptr;
    column++;
    offset++;
    sptr++;
    return true;
    }
  return false;
  }


// Match name with given name str of length len
FXbool FXXML::match(const FXchar* str,FXint len){
  if(name() && rptr+len==sptr){
    return FXString::compare(rptr,str,len)==0;
    }
  return false;
  }


// Parse string
FXXML::Error FXXML::parsestring(FXString& str){
  FXchar q=sptr[0];
  str.clear();
  if(q=='"' || q=='\''){
    column++;
    offset++;
    sptr++;
    rptr=sptr;
    while(need(MAXTOKEN)){
      switch(sptr[0]){
      case '\t':
        column+=(8-column%8);
        offset++;
        sptr++;
        continue;
      case '\r':
        if(sptr+1<wptr && sptr[1]=='\n'){ offset++; sptr++; }
      case '\n':
        column=0;
        offset++;
        sptr++;
        line++;
        continue;
      case ' ':
        column++;
      case '\v':
      case '\f':
        offset++;
        sptr++;
        continue;
      case '\'':
      case '"':
        if(sptr[0]!=q) goto nxt;
        str.append(rptr,sptr-rptr);
        offset++;
        column++;
        sptr++;
        rptr=sptr;
        return ErrOK;
      default:
nxt:    if((sptr-rptr)>=(endptr-begptr-MAXTOKEN)){
          str.append(rptr,sptr-rptr);
          rptr=sptr;
          }
        column+=isUTF8(*sptr);          // Increment if UTF8 leader only
        offset++;
        sptr++;
        continue;
        }
      }
    return ErrEof;
    }
  return ErrString;
  }


// Parse version string
FXXML::Error FXXML::parseversion(){
  spaces();
  if(match('=')){
    spaces();
    return parsestring(vers);
    }
  return ErrEquals;
  }


// Parse encoding
FXXML::Error FXXML::parseencoding(){
  FXString code;
  spaces();
  if(match('=')){
    spaces();
    return parsestring(code);
    }
  return ErrEquals;
  }


// Parse standalone flag
FXXML::Error FXXML::parsestandalone(){
  FXString truth;
  spaces();
  if(match('=')){
    spaces();
    return parsestring(truth);
    }
  return ErrEquals;
  }


// XML header
FXXML::Error FXXML::parsexml(){
  FXXML::Error err;
  rptr=sptr;
  while(need(MAXTOKEN)){
    switch(sptr[0]){
    case '\t':
      column+=(8-column%8);
      offset++;
      sptr++;
      continue;
    case '\r':
      if(sptr+1<wptr && sptr[1]=='\n'){ offset++; sptr++; }
    case '\n':
      column=0;
      offset++;
      line++;
      sptr++;
      continue;
    case ' ':
      column++;
    case '\v':
    case '\f':
      offset++;
      sptr++;
      continue;
    case '?':                   // End of xml declaration
      if(sptr[1]!='>') return ErrToken;
      column+=2;
      offset+=2;
      sptr+=2;
      return ErrOK;
    case 'v':
      if(sptr[1]!='e') return ErrToken;
      if(sptr[2]!='r') return ErrToken;
      if(sptr[3]!='s') return ErrToken;
      if(sptr[4]!='i') return ErrToken;
      if(sptr[5]!='o') return ErrToken;
      if(sptr[6]!='n') return ErrToken;
      column+=7;
      offset+=7;
      sptr+=7;
      if((err=parseversion())!=ErrOK) return err;
      rptr=sptr;
      continue;
    case 'e':
      if(sptr[1]!='n') return ErrToken;
      if(sptr[2]!='c') return ErrToken;
      if(sptr[3]!='o') return ErrToken;
      if(sptr[4]!='d') return ErrToken;
      if(sptr[5]!='i') return ErrToken;
      if(sptr[6]!='n') return ErrToken;
      if(sptr[7]!='g') return ErrToken;
      column+=8;
      offset+=8;
      sptr+=8;
      if((err=parseencoding())!=ErrOK) return err;
      rptr=sptr;
      continue;
    case 's':
      if(sptr[1]!='t') return ErrToken;
      if(sptr[2]!='a') return ErrToken;
      if(sptr[3]!='n') return ErrToken;
      if(sptr[4]!='d') return ErrToken;
      if(sptr[5]!='a') return ErrToken;
      if(sptr[6]!='l') return ErrToken;
      if(sptr[7]!='o') return ErrToken;
      if(sptr[8]!='n') return ErrToken;
      if(sptr[9]!='e') return ErrToken;
      column+=10;
      offset+=10;
      sptr+=10;
      if((err=parsestandalone())!=ErrOK) return err;
      rptr=sptr;
      continue;
    default:
      return ErrToken;
      }
    }
  return ErrEof;
  }


// Element declaration
FXXML::Error FXXML::parseelementdecl(){
  FXString elmname;
  spaces();
  if(name()){
    elmname.assign(rptr,sptr-rptr);
    spaces();
    //?//
    return ErrOK;
    }
  return ErrName;
  }


// External ID
FXXML::Error FXXML::parseexternalid(){
  FXXML::Error err;
  FXString syslit;
  FXString publit;
  spaces();
  if(match("SYSTEM",6)){
    spaces();
    if((err=parsestring(syslit))!=ErrOK) return err;
    ///
    FXTRACE((101,"SYSTEM \"%s\"\n",syslit.text()));
    return ErrOK;
    }
  if(match("PUBLIC",6)){
    spaces();
    if((err=parsestring(publit))!=ErrOK) return err;
    spaces();
    if((err=parsestring(syslit))!=ErrOK) return err;
    FXTRACE((101,"PUBLIC \"%s\" \"%s\"\n",publit.text(),syslit.text()));
    ///
    return ErrOK;
    }
  return ErrOK;
  }


// Internal subsey
FXXML::Error FXXML::parseinternalsubset(){
  FXXML::Error err;
  spaces();
  if(match('[')){
    FXTRACE((101,"internalsubset\n"));
    while(need(MAXTOKEN)){
      switch(sptr[0]){
      case '\t':
        column+=(8-column%8);
        offset++;
        sptr++;
        continue;
      case '\r':
        if(sptr+1<wptr && sptr[1]=='\n'){ offset++; sptr++; }
      case '\n':
        column=0;
        offset++;
        line++;
        sptr++;
        continue;
      case ' ':
        column++;
      case '\v':
      case '\f':
        offset++;
        sptr++;
        continue;
      case ']':
        column++;
        offset++;
        sptr++;
        return ErrOK;
      case '<':
        if(sptr[1]=='?'){
          column+=2;
          offset+=2;
          sptr+=2;
          if((err=parseprocessing())!=ErrOK) return err;
          continue;
          }
        if(sptr[1]=='!' && sptr[2]=='-' && sptr[3]=='-'){
          column+=4;
          offset+=4;
          sptr+=4;
          if((err=parsecomment())!=ErrOK) return err;
          continue;
          }
#if 0 // SKIP NOT-YET-IMPLEMENTED STUFF
        if(sptr[1]=='!' && sptr[2]=='E' && sptr[3]=='L' && sptr[4]=='E' && sptr[5]=='M' && sptr[6]=='E' && sptr[7]=='N' && sptr[8]=='T'){
          column+=9;
          sptr+=9;
          FXTRACE((101,"<!ELEMENT\n"));
          rptr=sptr;
          continue;
          }
        if(sptr[1]=='!' && sptr[2]=='A' && sptr[3]=='T' && sptr[4]=='T' && sptr[5]=='L' && sptr[6]=='I' && sptr[7]=='T'){
          column+=8;
          sptr+=8;
          FXTRACE((101,"<!ATTLIST\n"));
          rptr=sptr;
          continue;
          }
        if(sptr[1]=='!' && sptr[2]=='E' && sptr[3]=='N' && sptr[4]=='T' && sptr[5]=='I' && sptr[6]=='T' && sptr[7]=='Y'){
          column+=8;
          sptr+=8;
          FXTRACE((101,"<!ENTITY\n"));
          rptr=sptr;
          continue;
          }
        if(sptr[1]=='!' && sptr[2]=='N' && sptr[3]=='O' && sptr[4]=='T' && sptr[5]=='A' && sptr[6]=='T' && sptr[7]=='I' && sptr[8]=='O' && sptr[9]=='N'){
          column+=10;
          sptr+=10;
          FXTRACE((101,"<!NOTATION\n"));
          rptr=sptr;
          continue;
          }
#endif
//        return ErrToken;
      default:
        column++;
        offset++;
        sptr++;
//        return ErrToken;
        }
      }
    return ErrEof;
    }
  return ErrOK;
  }


// Document type
FXXML::Error FXXML::parsedeclarations(){
  FXXML::Error err;
  FXString docname;
  spaces();
  if(name()){
    docname.assign(rptr,sptr-rptr);
    FXTRACE((101,"docname=%s\n",docname.text()));
    if((err=parseexternalid())!=ErrOK) return err;
    if((err=parseinternalsubset())!=ErrOK) return err;
    spaces();
    if(!match('>')) return ErrToken;
    return ErrOK;
    }
  return ErrName;
  }


// Processing instruction
FXXML::Error FXXML::parseprocessing(){
  FXString target;
  FXString data;
  if(name()){
    target.assign(rptr,sptr-rptr);
    rptr=sptr;                          // Start of processing instruction data
    while(need(MAXTOKEN)){
      switch(sptr[0]){
      case '\t':
        column+=(8-column%8);
        offset++;
        sptr++;
        continue;
      case '\r':
        if(sptr+1<wptr && sptr[1]=='\n'){ offset++; sptr++; }
      case '\n':
        column=0;
        offset++;
        line++;
        sptr++;
        continue;
      case ' ':
        column++;
      case '\v':
      case '\f':
        offset++;
        sptr++;
        continue;
      case '?':
        if(sptr[1]!='>') goto nxt;      // Just a lone '?'
        data.append(rptr,sptr-rptr);    // Add last chunk of processing instruction
        column+=2;
        offset+=2;
        sptr+=2;
        rptr=sptr;
        return processingCB(target,data);
      default:
nxt:    if((sptr-rptr)>=(endptr-begptr-MAXTOKEN)){
          data.append(rptr,sptr-rptr);  // Add another chunk
          rptr=sptr;
          }
        column+=isUTF8(*sptr);          // Increment if UTF8 leader only
        offset++;
        sptr++;
        continue;
        }
      }
    return ErrEof;
    }
  return ErrName;
  }


// Scan comment
FXXML::Error FXXML::parsecomment(){
  FXString text;
  rptr=sptr;
  while(need(MAXTOKEN)){
    switch(sptr[0]){
    case '\t':
      column+=(8-column%8);
      offset++;
      sptr++;
      continue;
    case '\r':
      if(sptr+1<wptr && sptr[1]=='\n'){ offset++; sptr++; }
    case '\n':
      column=0;
      offset++;
      line++;
      sptr++;
      continue;
    case ' ':
      column++;
    case '\v':
    case '\f':
      offset++;
      sptr++;
      continue;
    case '-':
      if(sptr[1]!='-') goto nxt;        // Just a lone '-'
      if(sptr[2]!='>') goto nxt;        // Allow a '--' inside a comment; technically, not spec
      text.append(rptr,sptr-rptr);      // Pass comment undecoded
      column+=3;
      offset+=3;
      sptr+=3;
      rptr=sptr;
      return commentCB(text);
    default:
nxt:  if((sptr-rptr)>=(endptr-begptr-MAXTOKEN)){
        text.append(rptr,sptr-rptr);    // Pass comment undecoded
        rptr=sptr;
        }
      column+=isUTF8(*sptr);            // Increment if UTF8 leader only
      offset++;
      sptr++;
      continue;
      }
    }
  return ErrEof;
  }


// Parse key=value pair
FXXML::Error FXXML::parseattribute(Element& elm){
  FXXML::Error err;
  FXString value;
  FXString key;
  if(name()){
    key.assign(rptr,sptr-rptr);
    spaces();
    if(!match('=')) return ErrEquals;
    spaces();
    if((err=parsestring(value))!=ErrOK) return err;
    spaces();
    if(!FXXML::decode(elm.attributes[key],value,REFS|CRLF)) return ErrToken;
    return ErrOK;
    }
  return ErrName;
  }


// Start tag
FXXML::Error FXXML::parsestarttag(Element& elm){
  FXXML::Error err;
  if(!name()) return ErrName;
  elm.name.assign(rptr,sptr-rptr);
  while(need(MAXTOKEN)){
    rptr=sptr;
    switch(sptr[0]){
    case '\t':
      column+=(8-column%8);
      offset++;
      sptr++;
      continue;
    case '\r':
      if(sptr+1<wptr && sptr[1]=='\n'){ offset++; sptr++; }
    case '\n':
      column=0;
      offset++;
      line++;
      sptr++;
      continue;
    case ' ':
      column++;
    case '\v':
    case '\f':
      offset++;
      sptr++;
      continue;
    case '/':
      if(sptr[1]!='>') return ErrToken; // Improper empty tag syntax
      elm.empty=true;
      column+=2;
      offset+=2;
      sptr+=2;
      return ErrOK;                     // End of empty stag element
    case '>':
      elm.empty=false;
      column++;
      offset++;
      sptr++;
      return ErrOK;                     // End of stag
    default:
      if((err=parseattribute(elm))!=ErrOK) return err;
      continue;
      }
    }
  return ErrEof;
  }


// End tag
FXXML::Error FXXML::parseendtag(Element& elm){
  if(!match(elm.name.text(),elm.name.length())) return ErrNoMatch;
  while(need(MAXTOKEN)){
    rptr=sptr;
    switch(sptr[0]){
    case '\t':
      column+=(8-column%8);
      offset++;
      sptr++;
      continue;
    case '\r':
      if(sptr+1<wptr && sptr[1]=='\n'){ offset++; sptr++; }
    case '\n':
      column=0;
      offset++;
      line++;
      sptr++;
      continue;
    case ' ':
      column++;
    case '\v':
    case '\f':
      offset++;
      sptr++;
      continue;
    case '>':
      column++;
      offset++;
      sptr++;
      return ErrOK;                     // End of etag
    default:
      return ErrToken;
      }
    }
  return ErrEof;
  }


// CData
FXXML::Error FXXML::parsecdata(Element& elm){
  FXXML::Error err;
  FXString text;
  rptr=sptr;
  while(need(MAXTOKEN)){
    switch(sptr[0]){
    case '\t':
      column+=(8-column%8);
      offset++;
      sptr++;
      continue;
    case '\r':
      if(sptr+1<wptr && sptr[1]=='\n'){ offset++; sptr++; }
    case '\n':
      column=0;
      offset++;
      line++;
      sptr++;
      continue;
    case ' ':
      column++;
    case '\v':
    case '\f':
      offset++;
      sptr++;
      continue;
    case ']':
      if(sptr[1]!=']') goto nxt;
      if(sptr[2]!='>') goto nxt;
      if(!FXXML::decode(text,FXString(rptr,sptr-rptr),CRLF)) return ErrToken;
      if((err=charactersCB(text))!=ErrOK) return err;
      column+=3;
      offset+=3;
      sptr+=3;
      rptr=sptr;
      return ErrOK;
    default:
nxt:  if((sptr-rptr)>=(endptr-begptr-MAXTOKEN)){
        if(!FXXML::decode(text,FXString(rptr,sptr-rptr),CRLF)) return ErrToken;
        if((err=charactersCB(text))!=ErrOK) return err;
        rptr=sptr;
        }
      column+=isUTF8(*sptr);            // Increment if UTF8 leader only
      offset++;
      sptr++;
      continue;
      }
    }
  return ErrEof;
  }


// Parse it
FXXML::Error FXXML::parsecontents(Element& elm){
  FXXML::Error err;
  FXString text;
  FXint brk=1;                          // Allow break
  rptr=sptr;
  while(need(MAXTOKEN)){
    switch(sptr[0]){
    case '\t':
      column+=(8-column%8);
      offset++;
      sptr++;
      brk=1;
      continue;
    case '\r':
      if(sptr+1<wptr && sptr[1]=='\n'){ offset++; sptr++; }
    case '\n':
      column=0;
      offset++;
      line++;
      sptr++;
      brk=1;
      continue;
    case ' ':
      column++;
    case '\v':
    case '\f':
      offset++;
      sptr++;
      brk=1;
      continue;
    case '&':                           // Disallow break in character reference
      column++;
      offset++;
      sptr++;
      brk=0;
      continue;
    case ';':                           // Allow break now
      column++;
      offset++;
      sptr++;
      brk=1;
      continue;
    case '<':
      brk=1;

      // Try decode, translate both CRLF and references
      if(!FXXML::decode(text,FXString(rptr,sptr-rptr),CRLF|REFS)) return ErrToken;

      // Report final batch of characters
      if((err=charactersCB(text))!=ErrOK) return err;

      // Eat text
      rptr=sptr;

      // End tag
      if(sptr[1]=='/'){
        column+=2;
        offset+=2;
        sptr+=2;
        return ErrOK;
        }

      // Processing instruction
      if(sptr[1]=='?'){
        column+=2;
        offset+=2;
        sptr+=2;
        if((err=parseprocessing())!=ErrOK) return err;
        continue;
        }

      // Comment
      if(sptr[1]=='!' && sptr[2]=='-' && sptr[3]=='-'){
        column+=4;
        offset+=4;
        sptr+=4;
        if((err=parsecomment())!=ErrOK) return err;
        continue;
        }

      // CDATA segment
      if(sptr[1]=='!' && sptr[2]=='[' && sptr[3]=='C' && sptr[4]=='D' && sptr[5]=='A' && sptr[6]=='T' && sptr[7]=='A' && sptr[8]=='['){
        column+=9;
        offset+=9;
        sptr+=9;
        if((err=parsecdata(elm))!=ErrOK) return err;
        continue;
        }

      // Just eat the '<'
      column++;
      offset++;
      sptr++;

      // Parse child element
      if((err=parseelement())!=ErrOK) return err;

      rptr=sptr;
      continue;
    default:
      if(brk && (sptr-rptr)>=(endptr-begptr-MAXTOKEN)){

        // Try decode, translate both CRLF and references
        if(!FXXML::decode(text,FXString(rptr,sptr-rptr),CRLF|REFS)) return ErrToken;

        // Report batch of characters
        if((err=charactersCB(text))!=ErrOK) return err;

        // Eat text
        rptr=sptr;
        }
      column+=isUTF8(*sptr);            // Increment if UTF8 leader only
      offset++;
      sptr++;
      continue;
      }
    }
  return ErrEof;
  }


// Parse element
FXXML::Error FXXML::parseelement(){
  FXXML::Element instance(&current);
  FXXML::Error err;

  // Parse start tag
  if((err=parsestarttag(instance))!=ErrOK) return err;

  // Report element start
  if((err=startElementCB(instance.name,instance.attributes))!=ErrOK) return err;

  // Its a non-empty element
  if(!instance.empty){

    // Parse contents
    if((err=parsecontents(instance))!=ErrOK) return err;

    // Parse end tag
    if((err=parseendtag(instance))!=ErrOK) return err;
    }

  // Report element end
  if((err=endElementCB(instance.name))!=ErrOK) return err;

  // OK
  return ErrOK;
  }


// Parse it
FXXML::Error FXXML::parse(){
  FXXML::Error err;
  current=nullptr;
  if(need(MAXTOKEN)){
    enc=guess();
    FXTRACE((101,"encoding=%s\n",encodingName[enc]));
    while(need(MAXTOKEN)){
      rptr=sptr;
      switch(sptr[0]){
      case '\t':
        column+=(8-column%8);
        offset++;
        sptr++;
        continue;
      case '\r':
        if(sptr+1<wptr && sptr[1]=='\n'){ offset++; sptr++; }
      case '\n':
        column=0;
        offset++;
        line++;
        sptr++;
        continue;
      case ' ':
        column++;
      case '\v':
      case '\f':
        offset++;
        sptr++;
        continue;
      case '<':

        // Parse comment
        if(sptr[1]=='!' && sptr[2]=='-' && sptr[3]=='-'){
          column+=4;
          offset+=4;
          sptr+=4;
          if((err=parsecomment())!=ErrOK) return err;
          continue;
          }

        // Parse document type declarations
        if(sptr[1]=='!' && sptr[2]=='D' && sptr[3]=='O' && sptr[4]=='C' && sptr[5]=='T' && sptr[6]=='Y' && sptr[7]=='P' && sptr[8]=='E' && spaceChar(sptr[9])){
          column+=9;
          offset+=9;
          sptr+=9;
          if((err=parsedeclarations())!=ErrOK) return err;
          continue;
          }

        // Parse XML declaration
        if(sptr[1]=='?' && sptr[2]=='x' && sptr[3]=='m' && sptr[4]=='l' && spaceChar(sptr[5])){
          column+=5;
          offset+=5;
          sptr+=5;
          if((err=parsexml())!=ErrOK) return err;
          continue;
          }

        // Parse processing instruction
        if(sptr[1]=='?'){
          column+=2;
          offset+=2;
          sptr+=2;
          if((err=parseprocessing())!=ErrOK) return err;
          continue;
          }

        // Just eat the '<'
        column++;
        offset++;
        sptr++;

        // Report document start
        if((err=startDocumentCB())!=ErrOK) return err;

        // Parse document body
        if((err=parseelement())!=ErrOK) return err;

        // Report document end
        if((err=endDocumentCB())!=ErrOK) return err;

        // End with success
        return ErrOK;
      default:
        return ErrToken;
        }
      }
    }
  return ErrEmpty;
  }

/*******************************************************************************/

// Close it
FXbool FXXML::close(){
  FXTRACE((101,"XML::close()\n"));
  if(FXParseBuffer::close()){
    current=nullptr;
    return true;
    }
  return false;
  }


// Clean up
FXXML::~FXXML(){
  FXTRACE((100,"FXXML::~FXXML\n"));
  close();
  }

}
