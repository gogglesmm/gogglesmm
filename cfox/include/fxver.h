#ifndef FXVER_H
#define FXVER_H


// FOX version
#define FOX_MAJOR  1
#define FOX_MINOR  7
#define FOX_LEVEL  74


// FOX byte order
#ifndef FOX_BIGENDIAN
#if defined(__BIG_ENDIAN__)
#define FOX_BIGENDIAN 1
#elif defined(__LITTLE_ENDIAN__)
#define FOX_BIGENDIAN 0
#else
#define FOX_BIGENDIAN 0
#endif
#endif


#endif
