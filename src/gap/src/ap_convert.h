#ifndef CONVERT_H
#define CONVERT_H
namespace ap {

extern void s24le3_to_s16(FXuchar * buffer,FXuint nsamples);
extern void  float_to_s16(FXuchar * buffer,FXuint nsamples);

extern void s24le3_to_s32(const FXuchar * buffer,FXuint nsamples,MemoryBuffer & out);
extern void  float_to_s32(FXuchar * buffer,FXuint nsamples);

}
#endif


