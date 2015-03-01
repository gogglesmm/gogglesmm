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
#include "gmdefs.h"
#include "GMIconTheme.h"
#include "GMFilter.h"
#include "GMFilterEditor.h"


void Rule::load(FXStream & store){
  store >> column;
  store >> opcode;
  store >> value;
  store >> text;
  }

void Rule::save(FXStream & store) const {
  store << column;
  store << opcode;
  store << value;
  store << text;
  }

static const FXchar * const column_lookup[]={
  "tracks.title",
  "track_artist.name",
  "album_artist.name",
  "composers.name",
  "conductors.name",
  "albums.name",
  "tracks.year",
  "tracks.time",
  "tracks.playdate",
  "tracks.importdate",
  "tracks.playcount",
  "tracks.channels",
  "tracks.samplerate",
  "tracks.no",
  "tracks.no",
  "tracks.filetype"
  };

static const FXchar * const operator_lookup[]={
  "LIKE",
  "NOT LIKE",
  "==",
  "!=",
  "LIKE",
  "LIKE",
  ">=",
  "<=",
  "REGEXP"
  };


FXString Rule::getMatch() const {
  switch(column) {
    case ColumnTitle:
    case ColumnArtist:
    case ColumnAlbum:
    case ColumnAlbumArtist:
    case ColumnConductor:
    case ColumnComposer:
      {
        switch(opcode) {
          case OperatorLike     :
          case OperatorNotLike  : return FXString::value("%s %s '%%%s%%'",column_lookup[column],operator_lookup[opcode],text.text()); break;
          case OperatorPrefix   : return FXString::value("%s %s '%s%%'",column_lookup[column],operator_lookup[opcode],text.text()); break;
          case OperatorSuffix   : return FXString::value("%s %s '%%%s'",column_lookup[column],operator_lookup[opcode],text.text()); break;
          case OperatorEquals   :
          case OperatorNotEqual :
          case OperatorLess     :
          case OperatorGreater  :
          case OperatorMatch    : return FXString::value("%s %s '%s'",column_lookup[column],operator_lookup[opcode],text.text()); break;
          }
      } break;
    case ColumnYear:
    case ColumnPlaycount:
    case ColumnAudioChannels:
    case ColumnAudioRate:
    case ColumnTime:
      {
        switch(opcode) {
          case OperatorEquals     :
          case OperatorNotEqual   :
          case OperatorLess       :
          case OperatorGreater    : return FXString::value("%s %s %d",column_lookup[column],operator_lookup[opcode],value); break;
          }
      } break;
    case ColumnTrackNumber:
      {
        switch(opcode) {
          case OperatorEquals     :
          case OperatorNotEqual   :
          case OperatorLess       :
          case OperatorGreater    : return FXString::value("(%s&65535) %s %d",column_lookup[column],operator_lookup[opcode],value); break;
          }
      } break;
    case ColumnDiscNumber:
      {
        switch(opcode) {
          case OperatorEquals     :
          case OperatorNotEqual   :
          case OperatorLess       :
          case OperatorGreater    : return FXString::value("(%s>>16) %s %d",column_lookup[column],operator_lookup[opcode],value); break;
          }
      } break;
    case ColumnPlaydate:
    case ColumnImportdate:
      {
        switch(opcode) {
          case OperatorLess       :
          case OperatorGreater    : return FXString::value("datetime(%s/1000000000,'unixepoch') %s datetime('now','-%d seconds')",column_lookup[column],operator_lookup[opcode],value); break;
          }
      } break;
    case ColumnFiletype:
      {
        switch(opcode) {
          case OperatorEquals     :
          case OperatorNotEqual   : return FXString::value("%s %s %d",column_lookup[column],operator_lookup[opcode],value); break;
          }
      } break;
    default: FXASSERT(0); break;
    }
  return FXString::null;
  }


FXString SortLimit::getMatch() const {
  return FXString::value("%s %s",column_lookup[column],ascending ? "ASC" : "DESC");
  }


void SortLimit::load(FXStream & store){
  store >> column;
  store >> ascending;
  }

void SortLimit::save(FXStream & store) const {
  store << column;
  store << ascending;
  }


FXint GMFilter::nextid = 0;


GMFilter::GMFilter() :
  id(nextid++),
  name("Untitled"),
  rules(1),
  limit(0),
  match(MatchAll){
  }

GMFilter::GMFilter(const FXString & n,FXint column,FXint opcode,FXint value) :
  id(nextid++),
  name(n),
  limit(0),
  match(MatchAll) {
  rules.append(Rule(column,opcode,value));
  }

FXString GMFilter::getMatch() const {
  FXString query;
  FXString rule = (match == MatchAll) ? " AND " : " OR ";

  for (FXint i=0;i<rules.no();i++) {
    if (!query.empty()) query+=rule;
    query += rules[i].getMatch();
    }
  if (!query.empty())
    query.prepend("WHERE ");

  if (limit>0) {
    if (order.no()) {
      query+=" ORDER BY ";
      for (FXint i=0;i<order.no();i++) {
        if (i>0) query+=", ";
        query+=order[i].getMatch();
        }
      }
    query+=FXString::value(" LIMIT %d",limit);
    }
  return query;
  }

void GMFilter::load(FXStream & store) {
  FXint nitems=0;
  store >> id;
  store >> name;

  GMFilter::nextid = FXMAX(id+1,nextid);

  // Rules
  store >> nitems;
  rules.no(nitems);
  for (FXint i=0;i<nitems;i++) {
    rules[i].load(store);
    }

  // Sort Columns
  store >> nitems;
  order.no(nitems);
  for (FXint i=0;i<nitems;i++) {
    order[i].load(store);
    }
  store >> limit;
  store >> match;
  }

void GMFilter::save(FXStream & store) const {
  FXint nitems;
  store << id;
  store << name;

  // Update nextid in case filters have been removed
  GMFilter::nextid = FXMAX(id+1,nextid);

  // Rules
  nitems = rules.no();
  store << nitems;
  for (FXint i=0;i<nitems;i++) {
    rules[i].save(store);
    }

  // Order
  nitems = order.no();
  store << nitems;
  for (FXint i=0;i<nitems;i++) {
    order[i].save(store);
    }
  store << limit;
  store << match;
  }
