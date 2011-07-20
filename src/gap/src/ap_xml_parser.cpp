#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_xml_parser.h"

#include <expat.h>

namespace ap {

XMLStream::XMLStream() : parser(NULL), depth(1),skip(0) {
  parser = XML_ParserCreate(NULL);
  XML_SetUserData((XML_Parser)parser,this);
  XML_SetElementHandler((XML_Parser)parser,xml_element_start,xml_element_end);
  XML_SetCharacterDataHandler((XML_Parser)parser,xml_element_data);
  }

XMLStream::~XMLStream() {
  XML_ParserFree((XML_Parser)parser);
  }


void XMLStream::xml_print_error() {
  fxmessage("Parse Error (line %ld, column %ld): %s\n",XML_GetCurrentLineNumber((XML_Parser)parser),XML_GetCurrentColumnNumber((XML_Parser)parser),XML_ErrorString(XML_GetErrorCode((XML_Parser)parser)));
  }

FXbool XMLStream::parse(const FXchar * buffer,FXint length) {
  XML_Status code = XML_Parse((XML_Parser)parser,buffer,length,1);
  if (code==XML_STATUS_ERROR) {
    xml_print_error();
    return false;
    }
  return true;
  }

FXbool XMLStream::parse(const FXString & buffer) {
  return parse(buffer.text(),buffer.length());
  }

void XMLStream::xml_element_start(void*ptr,const FXchar * element,const FXchar ** attributes) {
  XMLStream * stream = reinterpret_cast<XMLStream*>(ptr);
  if (!stream->skip) {
    if (!stream->begin(element,attributes)) {
      stream->skip=stream->depth;
      }
    }
  stream->depth++;
  }

void XMLStream::xml_element_end(void*ptr,const FXchar * element) {
  XMLStream * stream = reinterpret_cast<XMLStream*>(ptr);

  if (!stream->skip)
    stream->end(element);

  stream->depth--;

  // turn off skip
  if (stream->skip==stream->depth)
    stream->skip=0;
  }

void XMLStream::xml_element_data(void*ptr,const FXchar * data,FXint len) {
  XMLStream * stream = reinterpret_cast<XMLStream*>(ptr);
  stream->data(data,len);
  }



}
