#ifdef HAVE_CDDA_PLUGIN
#include <cdio/cdio.h>
#include <cdio/cdda.h>

namespace ap {

class AudioCD : public FXIO {
private:
  AudioCD(const AudioCD&);
  AudioCD &operator=(const AudioCD&);
protected:
  cdrom_drive_t * drive;
  lsn_t           sector;
  track_t         track;
  track_t         ntracks;
public:
  AudioCD();
  FXbool open(const FXString &);
  FXbool isSerial() const;
  FXlong position() const;
  FXlong position(FXlong offset,FXuint from=FXIO::Begin);
  FXival readBlock(void* data,FXival count);
  FXlong size();
  FXbool eof();
  void setTrack(FXint n);
  ~AudioCD();
  };

}
#endif
