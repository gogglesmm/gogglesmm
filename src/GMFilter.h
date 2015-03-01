/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2015-2015 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMFILTER_H
#define GMFILTER_H

struct Rule {
public:
  FXint column;
  FXint opcode;
  FXint value;
  FXString text;
public:
  enum {
    ColumnTitle         = 0,
    ColumnArtist        = 1,
    ColumnAlbumArtist   = 2,
    ColumnComposer      = 3,
    ColumnConductor     = 4,
    ColumnAlbum         = 5,
    ColumnYear          = 6,
    ColumnTime          = 7,
    ColumnPlaydate      = 8,
    ColumnImportdate    = 9,
    ColumnPlaycount     = 10,
    ColumnAudioChannels = 11,
    ColumnAudioRate     = 12,
    ColumnTrackNumber   = 13,
    ColumnDiscNumber    = 14
    };
  enum {
    OperatorLike        = 0,
    OperatorNotLike     = 1,
    OperatorEquals      = 2,
    OperatorNotEqual    = 3,
    OperatorPrefix      = 4,
    OperatorSuffix      = 5,
    OperatorGreater     = 6,
    OperatorLess        = 7,
    OperatorMatch       = 8  // FXRex
    };
public:
  Rule() : column(ColumnTitle),opcode(OperatorLike) {}
  Rule(FXint c,FXint o,FXint v) : column(c),opcode(o),value(v) {}

  FXString getMatch() const;

  void load(FXStream &);
  void save(FXStream &) const;
  };


class SortLimit {
public:
  FXint  column;
  FXbool ascending;
public:
  SortLimit() : column(Rule::ColumnArtist), ascending(true) {}

  FXString getMatch() const;

  void load(FXStream &);

  void save(FXStream &) const;
  };


class GMFilter {
public:
  static FXint nextid;
public:
  FXint              id;
  FXString           name;
  FXArray<Rule>      rules;
  FXArray<SortLimit> order;
  FXint              limit;
  FXint              match;
public:
  enum {
    MatchAll = 0,
    MatchAny = 1,
    };
public:
  GMFilter();

  GMFilter(const FXString & name,FXint column,FXint opcode,FXint value);

  FXString getMatch() const;

  void load(FXStream &);

  void save(FXStream &) const;
  };

#endif
