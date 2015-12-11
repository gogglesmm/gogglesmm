/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2015 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#include "ap_config.h"
#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_event.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_buffer.h"
#include "ap_input_plugin.h"

#include <libsmbclient.h>

using namespace ap;

namespace ap {


class SMBInput : public InputPlugin {
protected:
  FXString      filename;
  FXInputHandle fd;
private:
  SMBInput(const SMBInput&);
  SMBInput &operator=(const SMBInput&);
protected:
  FXival        io_read(void*,FXival);
  FXInputHandle io_handle() const { return fd; }
public:
  /// Constructor
  SMBInput(InputThread*);

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

  /// Destructor
  virtual ~SMBInput();
  };




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
    FXival pos=position();
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

InputPlugin * ap_smb_plugin(InputThread*input) {
  return new SMBInput(input);
  }

}
