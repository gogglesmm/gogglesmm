#ifndef AP_DEFS_H
#define AP_DEFS_H

#if __GNUC__ >= 4
  #define GMAPI __attribute__ ((visibility("default")))
#else
  #define GMAPI
#endif


#include "fox.h"

#if FOX_BIGENDIAN == 0
#define INT32_LE(x) (((x)[3]<<24) | ((x)[2]<<16) | ((x)[1]<<8) |  ((x)[0]))
#define INT32_BE(x) ((((FXuint)(x)[0]) << 24) | \
                     (((FXuint)(x)[1]) << 16) | \
                     (((FXuint)(x)[2]) << 8) | \
                     (((FXuint)(x)[3]) ))

#define INT16_BE(x) ((((FXshort)(x)[0]) << 8) | \
                     (((FXshort)(x)[1]) ))

#define INT24_BE(x) ((((FXuint)(x)[0]) << 16) | \
                     (((FXuint)(x)[1]) << 8) | \
                     (((FXuint)(x)[2]) ))
#else
#define INT32_LE(x) ((((FXuint)(x)[0]) << 24) | \
                     (((FXuint)(x)[1]) << 16) | \
                     (((FXuint)(x)[2]) << 8) | \
                     (((FXuint)(x)[3]) ))
#define INT32_BE(x) (((x)[3]<<24) | ((x)[2]<<16) | ((x)[1]<<8) |  ((x)[0]))
#define INT16_BE(x) (((x)[1]<<8) | ((x)[0]))
#endif



enum {
  FLAG_EOS = 0x1
  };






#endif
