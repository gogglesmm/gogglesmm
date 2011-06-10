#ifdef HAVE_CDDA_PLUGIN
#include <cdio/cdio.h>
#include <cdio/cdda.h>

namespace ap {

class CDDAInput : public InputPlugin {
private:
  CDDAInput(const CDDAInput&);
  CDDAInput &operator=(const CDDAInput&);
protected:
  cdrom_drive_t * drive;
  lsn_t           sector;
  track_t         track;
  track_t         ntracks;
protected:
  FXlong read_raw(void*data,FXival ncount);
public:
  CDDAInput(InputThread*);

  FXbool open(const FXString & uri);

  /// Set Position
  FXlong position(FXlong offset,FXuint from);

  /// Get Position
  FXlong position() const;

  /// Size
  FXlong size();

  /// End of Input
  FXbool eof();

  /// Serial
  FXbool serial() const;

  /// Get plugin type
  FXuint plugin() const;

/*
  FXbool open(const FXString &);
  FXbool isSerial() const;
  FXlong position() const;
  FXlong position(FXlong offset,FXuint from=FXIO::Begin);
  FXival readBlock(void* data,FXival count);
  FXlong size();
  FXbool eof();
*/
  void setTrack(FXint n);
  ~CDDAInput();
  };

}
#endif
