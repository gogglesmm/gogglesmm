/********************************************************************************
*                                                                               *
*                     R e c e n t   F i l e s   L i s t                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#ifndef FXRECENTFILES_H
#define FXRECENTFILES_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif

namespace FX {


class FXApp;


/**
* The Recent Files group manages a most recently used (MRU) file list by
* means of the standard system registry.
* When connected to a widget, like a menu command, the recent files object
* updates the menu commands label to the associated recent file name; when
* the menu command is invoked, the recent file object sends its target a
* SEL_COMMAND message with the message data set to the associated file name,
* of the type const char*.
* When adding or removing file names, the recent files object automatically
* updates the system registry to record these changes.
* The ID_ANYFILES may be connected to a menu separator to cause automatic
* hiding of the menu separator when there are no recent files.
* The number of file names is typically no more than 10.
* File names should not be empty.
*/
class FXAPI FXRecentFiles : public FXObject {
  FXDECLARE(FXRecentFiles)
private:
  FXSettings *settings;       // Settings database where list is kept
  FXObject   *target;         // Target object to send message
  FXSelector  message;        // Message to send
  FXString    group;          // MRU File group
  FXuint      maxfiles;       // Maximum number of files to track
private:
  static const FXchar key[32][7];
private:
  FXRecentFiles(const FXRecentFiles&);
  FXRecentFiles &operator=(const FXRecentFiles&);
public:
  long onCmdClear(FXObject*,FXSelector,void*);
  long onCmdFile(FXObject*,FXSelector,void*);
  long onUpdFile(FXObject*,FXSelector,void*);
  long onUpdAnyFiles(FXObject*,FXSelector,void*);
public:
  enum{
    ID_CLEAR,
    ID_ANYFILES,
    ID_FILE_1,
    ID_FILE_2,
    ID_FILE_3,
    ID_FILE_4,
    ID_FILE_5,
    ID_FILE_6,
    ID_FILE_7,
    ID_FILE_8,
    ID_FILE_9,
    ID_FILE_10,
    ID_FILE_11,
    ID_FILE_12,
    ID_FILE_13,
    ID_FILE_14,
    ID_FILE_15,
    ID_FILE_16,
    ID_FILE_17,
    ID_FILE_18,
    ID_FILE_19,
    ID_FILE_20,
    ID_FILE_21,
    ID_FILE_22,
    ID_FILE_23,
    ID_FILE_24,
    ID_FILE_25,
    ID_FILE_26,
    ID_FILE_27,
    ID_FILE_28,
    ID_FILE_29,
    ID_FILE_30,
    ID_FILE_31,
    ID_FILE_32,
    ID_LAST
    };
public:

  /**
  * Make new recent files group.
  * A Settings object and group name must be assigned prior to usage.
  */
  FXRecentFiles();

  /**
  * Make new recent files group, using settings database from application.
  * An optional target and message may be passed to invoke when one of the
  * list of files is invoked.
  */
  FXRecentFiles(FXApp* a,const FXString& gp="Recent Files",FXObject *tgt=nullptr,FXSelector sel=0);

  /**
  * Make new recent files group, using given settings database.
  * An optional target and message may be passed to invoke when one of the
  * list of files is invoked.
  */
  FXRecentFiles(FXSettings* st,const FXString& gp="Recent Files",FXObject *tgt=nullptr,FXSelector sel=0);

  /// Change settings database
  void setSettings(FXSettings* s){ settings=s; }

  /// Return settings database
  FXSettings* getSettings() const { return settings; }

  /// Change number of files we're tracking
  void setMaxFiles(FXuint mx);

  /// Return the maximum number of files being tracked
  FXuint getMaxFiles() const { return maxfiles; }

  /// Set group name
  void setGroupName(const FXString& name){ group=name; }

  /// Return group name
  FXString getGroupName() const { return group; }

  /// Change the target
  void setTarget(FXObject *t){ target=t; }

  /// Get the target
  FXObject *getTarget() const { return target; }

  /// Change the message
  void setSelector(FXSelector sel){ message=sel; }

  /// Return the message id
  FXSelector getSelector() const { return message; }

  /// Obtain the filename at index
  FXString getFile(FXuint index) const;

  /// Change the filename at index
  void setFile(FXuint index,const FXString& filename);

  /// Append a file
  void appendFile(const FXString& filename);

  /// Remove a file
  void removeFile(const FXString& filename);

  /// Clear the list of files
  void clear();

  /// Save to a stream
  virtual void save(FXStream& store) const;

  /// Load from a stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXRecentFiles();
  };

}

#endif
