/*******************************************************************************
*                             Audio Converter                                  *
********************************************************************************
*           Copyright (C) 2010-2010 by Sander Jansen. All Rights Reserved      *
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
#ifndef AUDIO_TOOLS_H
#define AUDIO_TOOLS_H

enum {
  /* File Types */
  FILE_FLAC    = 0,
  FILE_MP3     = 1,
  FILE_OGG     = 2,
  FILE_MP4     = 3,
  FILE_MPC     = 4,
  FILE_OPUS    = 5,

  /* File Type Counter */
  FILE_NTYPES  = 6,

  /* Special Types */
  FILE_NONE    = 6,
  FILE_COPY    = 7,
  FILE_INVALID = 8,

  /* Bits */
  FILE_FLAC_BIT = (1<<FILE_FLAC),
  FILE_MP3_BIT  = (1<<FILE_MP3),
  FILE_OGG_BIT  = (1<<FILE_OGG),
  FILE_MP4_BIT  = (1<<FILE_MP4),
  FILE_MPC_BIT  = (1<<FILE_MPC),
  FILE_OPUS_BIT = (1<<FILE_OPUS)
  };

enum {
  FLAC_DECODER  = 0,
  FLAC_ENCODER  = 1,
  MP3_DECODER   = 2,
  MP3_ENCODER   = 3,
  OGG_DECODER   = 4,
  OGG_ENCODER   = 5,
  MP4_DECODER   = 6,
  MP4_ENCODER   = 7,
  MPC_DECODER   = 8,
  MPC_ENCODER   = 9,
  OPUS_DECODER  = 10,
  OPUS_ENCODER  = 11,
  NTOOLS        = 12,
  };

struct ToolConfig {
  const FXchar * section;
  const FXchar * quiet;
  FXString       bin;
  FXString       options;
  FXuint         types;   /// bitmask which inputs are supported
  FXchar         found;
  ToolConfig();
  };


class AudioTools {
friend class AudioConverter;
protected:
  FXbool     dryrun;
protected:
  ToolConfig tools[NTOOLS];
protected:
  FXString runTool(FXuint type,const FXString & input,const FXString & output) const;
public:
  AudioTools();

  /// Return desired extension for given file type;
  const FXchar * extension(FXuint type) const;

  FXbool check(FXuint from,FXuint to);

  FXbool encoder_supports(FXuint type,FXuint input) const;

  FXString decode(FXuint type,const FXString & input,const FXString & output) const;
  FXString encode(FXuint type,const FXString & input,const FXString & output) const;

  // Set Quiet Operation
  void quiet(FXbool enable=true);

  void load_rc(FXSettings&);
  void init_rc(FILE*);

  };
#endif
