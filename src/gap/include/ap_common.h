#ifndef AP_COMMON_H
#define AP_COMMON_H

namespace ap {

extern GMAPI void ap_parse_pls(const FXString & data,FXStringList & mrl);

extern GMAPI void ap_parse_m3u(const FXString & data,FXStringList & mrl);

extern GMAPI void ap_parse_xspf(const FXString & data,FXStringList & mrl,FXString & title);

extern GMAPI FXbool ap_set_nonblocking(FXInputHandle fd);

extern GMAPI void ap_set_thread_name(const FXchar *);

}
#endif
