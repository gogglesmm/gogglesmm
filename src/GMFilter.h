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

/*
  Rule: filter specific column with operator for specific value.
*/
class Rule {
public:
  FXint column  = ColumnTitle;    // Column to filter on
  FXint opcode  = OperatorLike;   // Operator
  FXint value   = 0;              // Integer Value
  FXString text;                  // String Value
public:
  enum {
    ColumnTitle         = 0,
    ColumnArtist        = 1,
    ColumnAlbumArtist   = 2,
    ColumnComposer      = 3,
    ColumnConductor     = 4,
    ColumnAlbum         = 5,
    ColumnPath          = 6,
    ColumnTag           = 7,
    ColumnYear          = 8,
    ColumnTime          = 9,
    ColumnTrackNumber   = 10,
    ColumnDiscNumber    = 11,
    ColumnRating        = 12,
    ColumnPlayCount     = 13,
    ColumnPlayDate      = 14,
    ColumnImportDate    = 15,
    ColumnFileType      = 16,
    ColumnChannels      = 17,
    ColumnBitRate       = 18,
    ColumnSampleRate    = 19,
    ColumnSampleSize    = 20
    };
  enum {
    OperatorLike        = 0, // LIKE
    OperatorNotLike     = 1, // NOT LIKE
    OperatorEquals      = 2, // ==
    OperatorNotEqual    = 3, // !=
    OperatorPrefix      = 4, // prefix%
    OperatorSuffix      = 5, // %suffix
    OperatorGreater     = 6, // >=
    OperatorLess        = 7, // <=
    OperatorMatch       = 8  // Regular Expression Matcher using FXRex
    };
public:
  // Default Constructor
  Rule() {} 
 
  // Initialize Rule for integer input
  Rule(FXint c,FXint o,FXint v) : column(c),opcode(o),value(v) {}

  // Get sql match string
  FXString getMatch() const;

  // Load from stream
  void load(FXStream &);

  // ave to stream
  void save(FXStream &) const;
  };


/*
  SortLimit: Column and sort order information for limit queries
*/
class SortLimit {
public:
  FXint  column    = Rule::ColumnArtist; // column to sort by
  FXbool ascending = true;               // ASC or DESC
public:
  // Get sql match string
  FXString getMatch() const;

  // Load from stream
  void load(FXStream &);

  // Save to stream
  void save(FXStream &) const;
  };


class GMFilter {
public:
  static FXint nextid;                 // Generator for unique id
public:
  FXint              id    = 0;        // unique filter id
  FXString           name;             // filter name
  FXArray<Rule>      rules;            // list of filter rules
  FXArray<SortLimit> order;            // list of columns to order by
  FXint              limit = 0;        // Maximum row limit
  FXint              match = MatchAll; // Match mode AND or OR.
public:
  enum {
    MatchAll = 0,  // Filter matches all rules (AND)
    MatchAny = 1,  // Filter matches any rules (OR)
    };
public:
  // Default Filter
  GMFilter();

  // Construct integer input filter with name, column, opcode and value
  GMFilter(const FXString & name,FXint column,FXint opcode,FXint value);

  // Get sql match string
  FXString getMatch() const;

  // Load from stream
  void load(FXStream &);

  // Save to stream
  void save(FXStream &) const;
  };

#endif
