/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2014 by Sander Jansen. All Rights Reserved      *
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
#include "ap_config.h"
#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_xml_parser.h"

#include <expat.h>

namespace ap {

#define NUM_NODES 32

XmlParser::XmlParser() : nnodes(NUM_NODES),level(0) {
  allocElms(nodes,nnodes);
  nodes[0]=Elem_None;
  }

XmlParser::~XmlParser() {
  freeElms(nodes);
  }

void XmlParser::element_start(const FXchar*element,const FXchar**attributes){
  FXint n =0 ;
  if (nodes[level])
    n = begin(element,attributes);
  level++;
  if (level==nnodes){
    nnodes+=NUM_NODES;
    resizeElms(nodes,nnodes);
    }
  nodes[level] = n;
  }

void XmlParser::element_end(const FXchar * element) {
  if (nodes[level])
    end(element);
  level--;
  }

void XmlParser::element_data(const FXchar * d,FXint len) {
  if (nodes[level])
    data(d,len);
  }


static void element_start(void*ptr,const FXchar*element,const FXchar**attributes){
  XmlParser * parser = reinterpret_cast<XmlParser*>(ptr);
  parser->element_start((const FXchar*)element,(const FXchar**)attributes);
  }

static void element_end(void*ptr,const FXchar * element) {
  XmlParser * parser = reinterpret_cast<XmlParser*>(ptr);
  parser->element_end((const FXchar*)element);
  }

static void element_data(void*ptr,const FXchar * data,FXint len) {
  XmlParser * parser = reinterpret_cast<XmlParser*>(ptr);
  parser->element_data((const FXchar*)data,len);
  }

static int unknown_encoding(void*,const XML_Char * name,XML_Encoding * info){
  GM_DEBUG_PRINT("expat unknown_encoding \"%s\"\n",name);
  FXTextCodec * codec = ap_get_textcodec(name);
  if (codec) {
    /* Only works for single byte codecs */
    FXwchar w;FXuchar c;
    for (FXint i=0;i<256;i++) {
      c=i;  
      codec->mb2wc(w,(const FXchar*)&c,1);
      info->map[i]  = w;
      info->convert = NULL;
      info->release = NULL;
      }
    delete codec;
    return XML_STATUS_OK;
    }
  return XML_STATUS_ERROR;
  }


FXbool XmlParser::parse(const FXString & buffer,const FXString & encoding) {
  if (!encoding.empty())
    GM_DEBUG_PRINT("XmlParser::parse with encoding %s\n",encoding.text()); 

  XML_Parser parser;

  if (encoding.empty())
    parser = XML_ParserCreate(NULL);
  else
    parser = XML_ParserCreate(encoding.text());

  XML_SetUserData((XML_Parser)parser,this);
  XML_SetElementHandler((XML_Parser)parser,ap::element_start,ap::element_end);
  XML_SetCharacterDataHandler((XML_Parser)parser,ap::element_data);
  XML_SetUnknownEncodingHandler((XML_Parser)parser,unknown_encoding,this);

  XML_Status code = XML_Parse((XML_Parser)parser,buffer.text(),buffer.length(),1);
  if (code==XML_STATUS_ERROR) {
    XML_ParserFree(parser);
    return false;
    }
  XML_ParserFree(parser);
  return true;
  }

}
