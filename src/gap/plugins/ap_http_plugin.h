/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2016 by Sander Jansen. All Rights Reserved      *
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
#ifndef INPUT_HTTP_DEVICE_H
#define INPUT_HTTP_DEVICE_H

namespace ap {


class HttpInput : public InputPlugin {
protected:
  HttpClient client;
	FXlong 		 content_position;
	FXuint		 content_type;
  FXint 		 icy_interval;
  FXint      icy_count;
  MemoryBuffer preview_buffer;
private:
  HttpInput(const HttpInput&);
  HttpInput &operator=(const HttpInput&);
protected:
	void check_headers();
	FXival icy_read(void*,FXival);
	void icy_parse(const FXString&);
public:
  /// Constructor
  HttpInput(InputThread*);

  FXbool open(const FXString & uri);

	/// Read
	FXival read(void*,FXival);

	/// Preview
	FXival preview(void*,FXival);

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
  virtual ~HttpInput();
  };

}
#endif
