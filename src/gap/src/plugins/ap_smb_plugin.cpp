#include "ap_config.h"
#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_event.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_buffer.h"
#include "ap_input_plugin.h"
#include "ap_smb_plugin.h"

#include <libsmbclient.h>

using namespace ap;

namespace ap {


static void smb_auth(const char * /*srv*/, const char */*shr*/, char */*wg*/, int /*wglen*/, char */*un*/, int /*unlen*/, char */*pw*/, int /*pwlen*/){
	//wglen = unlen = pwlen = 0;
  }

SMBInput::SMBInput(InputThread * i) : InputPlugin(i), fd(-1) {
  }

SMBInput::~SMBInput() {
  if (fd>=SMBC_BASE_FD){
    smbc_close(fd);
    fd=-1;
    }
  }

FXbool SMBInput::open(const FXString & uri) {

  /// Perhaps we need an init function...
  if (smbc_init(smb_auth,0))
    return false;

  fd=smbc_open(uri.text(),O_RDONLY,0);
  if (fd>=SMBC_BASE_FD){
    filename=uri;
    return true;
    }
  return false;
  }

FXival SMBInput::io_read(void*data,FXival ncount) {
  return smbc_read(fd,data,ncount);
  }

FXlong SMBInput::position(FXlong offset,FXuint from) {
  return smbc_lseek(fd,offset,from);
  }

FXlong SMBInput::position() const {
  FXlong pos = smbc_lseek(fd,0,SEEK_CUR);
  if (pos<0)
    return 0;
  else
    return pos;
  }

FXlong SMBInput::size() {
  struct stat data;
  if (smbc_fstat(fd,&data)==0) return data.st_size;
  return -1;
  }

FXbool SMBInput::eof()  {
  if(fd>=SMBC_BASE_FD){
    register FXival pos=position();
    return 0<=pos && size()<=pos;
    }
  return true;
  }

FXbool SMBInput::serial() const {
  return false;
  }

FXuint SMBInput::plugin() const {
  FXString extension=FXPath::extension(filename);
  return ap_format_from_extension(extension);
  }


}
