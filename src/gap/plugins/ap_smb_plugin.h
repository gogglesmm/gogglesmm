/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2012 by Sander Jansen. All Rights Reserved      *
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
#ifdef HAVE_SMB_PLUGIN
#ifndef INPUT_SMB_DEVICE_H
#define INPUT_SMB_DEVICE_H

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

}
#endif
#endif
