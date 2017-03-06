/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2017 by Sander Jansen. All Rights Reserved      *
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
#include "ap_defs.h"
#include "ap_input_plugin.h"

using namespace ap;

namespace ap {


class FileInput : public InputPlugin {
protected:
  FXFile   file;
  FXString filename;
private:
  FileInput(const FileInput&);
  FileInput &operator=(const FileInput&);
public:
  /// Constructor
  FileInput(IOContext*);

  FXbool open(const FXString & uri);

	/// Read
	FXival read(void*,FXival) override;

	/// Preview
	FXival preview(void*,FXival) override;

  /// Set Position
  FXlong position(FXlong offset,FXuint from) override;

  /// Get Position
  FXlong position() const override;

  /// Size
  FXlong size() override;

  /// End of Input
  FXbool eof() override;

  /// Serial
  FXbool serial() const override;

  /// Get plugin type
  FXuint plugin() const override;

  /// Destructor
  virtual ~FileInput();
  };


FileInput::FileInput(IOContext * ctx) : InputPlugin(ctx) {
  }

FileInput::~FileInput() {
  }

FXbool FileInput::open(const FXString & url) {

  // Get filename
  filename=FXURL::fileFromURL(url);
  if (filename.empty()) filename=url;

  // Open file
  if (!file.open(filename,FXIO::Reading)){
    filename.clear();
    return false;
    }

  return true;
  }

FXival FileInput::preview(void*data,FXival count) {
	FXlong pos = file.position();
	FXival n = file.readBlock(data,count);
	file.position(pos,FXIO::Begin);
	return n;
  }

FXival FileInput::read(void*data,FXival ncount) {
  return file.readBlock(data,ncount);
  }

FXlong FileInput::position(FXlong offset,FXuint from) {
  return file.position(offset,from);
  }

FXlong FileInput::position() const {
  return file.position();
  }

FXlong FileInput::size() {
  return file.size();
  }

FXbool FileInput::eof()  {
  return file.eof();
  }

FXbool FileInput::serial() const {
  return file.isSerial();
  }

FXuint FileInput::plugin() const {
  FXString extension=FXPath::extension(filename);
  return ap_format_from_extension(extension);
  }


InputPlugin * ap_file_plugin(IOContext * context) {
  return new FileInput(context);
  }


}
