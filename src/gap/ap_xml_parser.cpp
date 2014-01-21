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
#include "ap_config.h"
#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_xml_parser.h"

#ifdef HAVE_EXPAT_PLUGIN
#include <expat.h>
#else
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>
#endif

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


#ifdef HAVE_EXPAT_PLUGIN
static void element_start(void*ptr,const FXchar*element,const FXchar**attributes){
#else
static void element_start(void*ptr,const FXuchar*element,const FXuchar**attributes){
#endif
  XmlParser * parser = reinterpret_cast<XmlParser*>(ptr);
  parser->element_start((const FXchar*)element,(const FXchar**)attributes);
  }

#ifdef HAVE_EXPAT_PLUGIN
static void element_end(void*ptr,const FXchar * element) {
#else
static void element_end(void*ptr,const FXuchar * element) {
#endif
  XmlParser * parser = reinterpret_cast<XmlParser*>(ptr);
  parser->element_end((const FXchar*)element);
  }

#ifdef HAVE_EXPAT_PLUGIN
static void element_data(void*ptr,const FXchar * data,FXint len) {
#else
static void element_data(void*ptr,const FXuchar * data,FXint len) {
#endif
  XmlParser * parser = reinterpret_cast<XmlParser*>(ptr);
  parser->element_data((const FXchar*)data,len);
  }


FXbool XmlParser::parse(const FXString & buffer) {
  return parseBuffer(buffer.text(),buffer.length());
  }


FXbool XmlParser::parseBuffer(const FXchar * buffer,FXint length) {
#ifdef HAVE_EXPAT_PLUGIN
  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData((XML_Parser)parser,this);
  XML_SetElementHandler((XML_Parser)parser,ap::element_start,ap::element_end);
  XML_SetCharacterDataHandler((XML_Parser)parser,ap::element_data);
  XML_Status code = XML_Parse((XML_Parser)parser,buffer,length,1);
  if (code==XML_STATUS_ERROR) {
    //xml_print_error();
    return false;
    }
  XML_ParserFree(parser);
#else
  xmlSAXHandler sax;
  memset(&sax,0,sizeof(xmlSAXHandler));
  sax.startElement = &ap::element_start;
  sax.endElement = &ap::element_end;
  sax.characters = &ap::element_data;
  xmlSAXUserParseMemory(&sax,this,buffer,length);
#endif
  return true;
  }



HtmlParser::HtmlParser() {
  }

HtmlParser::~HtmlParser() {
  }


#ifndef HAVE_EXPAT_PLUGIN
FXbool HtmlParser::parseBuffer(const FXchar * buffer,FXint length) {
  xmlSAXHandler sax;

  memset(&sax,0,sizeof(xmlSAXHandler));
  sax.startElement = &ap::element_start;
  sax.endElement = &ap::element_end;
  sax.characters = &ap::element_data;

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
#else
FXbool HtmlParser::parseBuffer(const FXchar *,FXint) {
  return false;
  }
#endif


}
