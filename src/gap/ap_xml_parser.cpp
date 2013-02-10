/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2012 by Sander Jansen. All Rights Reserved      *
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
#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_xml_parser.h"


namespace ap {

#ifdef USE_EXPAT
#include <expat.h>
#else
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>
#endif

#define NUM_NODES 32

XmlParser::XmlParser() : nnodes(NUM_NODES),level(0) {
  allocElms(nodes,nnodes);
  nodes[0]=Elem_None;
  }

XmlParser::~XmlParser() {
  freeElms(nodes);
  }

void XmlParser::element_start(void*ptr,const FXuchar*element,const FXuchar**attributes){
  XmlParser * parser = reinterpret_cast<XmlParser*>(ptr);
  FXint node = 0;

  if (parser->nodes[parser->level])
    node = parser->begin((const FXchar*)element,(const FXchar**)attributes);

  parser->level++;
  if (parser->level==parser->nnodes){
    parser->nnodes+=NUM_NODES;
    resizeElms(parser->nodes,parser->nnodes);
    }
  parser->nodes[parser->level] = node;
  }

void XmlParser::element_end(void*ptr,const FXuchar * element) {
  XmlParser * parser = reinterpret_cast<XmlParser*>(ptr);

  if (parser->nodes[parser->level]){
    parser->end((const FXchar*)element);
    }

  parser->level--; 
  }

void XmlParser::element_data(void*ptr,const FXuchar * data,FXint len) {
  XmlParser * parser = reinterpret_cast<XmlParser*>(ptr);
  if (parser->nodes[parser->level])
    parser->data((const FXchar*)data,len);
  }


FXbool XmlParser::parse(const FXString & buffer) {
  return parseBuffer(buffer.text(),buffer.length());
  }

FXbool XmlParser::parseBuffer(const FXchar * buffer,FXint length) {
#ifdef USE_EXPAT
  XML_Parser * parser = XML_ParserCreate(NULL);
  XML_SetUserData((XML_Parser)parser,this);
  XML_SetElementHandler((XML_Parser)parser,element_start,element_end);
  XML_SetCharacterDataHandler((XML_Parser)parser,element_data);
  XML_Status code = XML_Parse((XML_Parser)parser,buffer,length,1);
  if (code==XML_STATUS_ERROR) {
    xml_print_error();
    return false;
    }
  XML_ParserFree(parser);
#else
  xmlSAXHandler sax;
  memset(&sax,0,sizeof(xmlSAXHandler));
  sax.startElement = &element_start;
  sax.endElement = &element_end;
  sax.characters = &element_data;
  xmlSAXUserParseMemory(&sax,this,buffer,length);
#endif
  return true;
  }



HtmlParser::HtmlParser() {
  }

HtmlParser::~HtmlParser() {
  }


FXbool HtmlParser::parseBuffer(const FXchar * buffer,FXint length) {
  xmlSAXHandler sax;

  memset(&sax,0,sizeof(xmlSAXHandler));
  sax.startElement = &element_start;
  sax.endElement = &element_end;
  sax.characters = &element_data;

  htmlParserCtxtPtr ctxt;

  xmlInitParser();
  ctxt = htmlCreateMemoryParserCtxt(buffer,length);
  if (ctxt->sax)
    xmlFree(ctxt->sax);
  ctxt->sax = &sax;
  ctxt->userData = this;
  htmlParseDocument(ctxt);
  ctxt->sax = NULL;
  ctxt->userData = NULL;

  htmlFreeParserCtxt(ctxt);
  return true;
  }


}
