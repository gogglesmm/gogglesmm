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
  FXival io_read(void*,FXival);
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
