#ifndef AP_COMMON_H
#define AP_COMMON_H

#include <ap_xml_parser.h>

namespace ap {

extern GMAPI void ap_parse_pls(const FXString & data,FXStringList & mrl);

extern GMAPI void ap_parse_m3u(const FXString & data,FXStringList & mrl);

extern GMAPI void ap_parse_xspf(const FXString & data,FXStringList & mrl,FXString & title);

}

#endif
