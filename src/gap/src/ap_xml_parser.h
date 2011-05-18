#ifndef AP_XML_PARSER_H
#define AP_XML_PARSER_H

#include <expat.h>

namespace ap {

class XMLStream {
protected:
  XML_Parser   parser;
  FXint        depth;
  FXint        skip;
private:
  static void xml_element_start(void*,const FXchar*,const FXchar**);
  static void xml_element_end(void*,const FXchar*);
  static void xml_element_data(void*,const FXchar*,FXint);
protected:
  virtual FXint begin(const FXchar *,const FXchar**) { return 1;}
  virtual void data(const FXchar *,FXint) {}
  virtual void end(const FXchar *){}
  void xml_print_error();
public:
  XMLStream();
  FXbool parse(const FXchar * buffer,FXint length);
  FXbool parse(const FXString & buffer);
  virtual ~XMLStream();
  };

}
#endif

