#ifndef AP_UTILS_H
#define AP_UTILS_H

extern GMAPI void ap_get_device(FXString&);

extern GMAPI void ap_set_thread_name(const FXchar *);

extern FXuint ap_wait(FXInputHandle handle);

extern FXuint ap_wait(FXInputHandle h1,FXInputHandle h2);

#endif

