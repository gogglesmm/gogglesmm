/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2015-2018 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMDATABASEQUERYSOURCE_H
#define GMDATABASEQUERYSOURCE_H

/*
  GMFilterSource: A filter source
*/
class GMFilterSource : public GMDatabaseSource {
FXDECLARE(GMFilterSource)
protected:
  static FXObjectListOf<GMFilterSource> sources;  // global list of all filters
public:
  // Initialize Filter Database
  static void init(GMTrackDatabase * database,GMSourceList &);

  // Save Filter Database
  static void save();

  // Create New Filter
  static void create(GMTrackDatabase * database);
protected:
  GMFilter match; // the actual filter
protected:
  GMFilterSource(){}
private:
  GMFilterSource(const GMFilterSource&);
  GMFilterSource& operator=(const GMFilterSource&);
public:
  enum {
    ID_EDIT = GMDatabaseSource::ID_LAST, // Edit Filter
    ID_REMOVE,                           // Remove Filter
    ID_LAST
    };
public:
  long onCmdEdit(FXObject*,FXSelector,void*);
  long onCmdRemove(FXObject*,FXSelector,void*);
public:
  // Construct Filter Source
  GMFilterSource(GMTrackDatabase * db);

  // Construct Filter Source
  GMFilterSource(GMTrackDatabase * db,const GMFilter & query);

  // Update View
  void updateView();

  // Configure
  void configure(GMColumnList&) override;

  // Source Name
  FXString getName() const override;

  // Source Type
  FXint getType() const override { return SOURCE_DATABASE_FILTER; }

  // Default Browsing
  FXbool defaultBrowse() const override { return true; }

  // Setting Key
  FXString settingKey() const override;

  // Source Menu
  FXbool source_menu(FXMenuPane*) override {return false;}

  // Source Context Menu
  FXbool source_context_menu(FXMenuPane*) override;

  // Destructor
  virtual ~GMFilterSource();
  };

#endif
