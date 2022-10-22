/********************************************************************************
*                                                                               *
*                        F i l e   A s s o c i a t i o n s                      *
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
#ifndef FXFILEASSOCIATIONS_H
#define FXFILEASSOCIATIONS_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif

namespace FX {


class FXIcon;
class FXIconSource;


/// Registers stuff to know about the extension
struct FXFileAssoc {
  FXString   command;           /// Command to execute
  FXString   extension;         /// Full extension name
  FXString   mimetype;          /// Mime type name
  FXIcon    *bigicon;           /// Big normal icon
  FXIcon    *bigiconopen;       /// Big open icon
  FXIcon    *miniicon;          /// Mini normal icon
  FXIcon    *miniiconopen;      /// Mini open icon
  FXDragType dragtype;          /// Registered drag type
  FXuint     flags;             /// Flags; 1=cd, 2=term
  };


// Dictionary of file associations
typedef FXDictionaryOf<FXFileAssoc> FXFileAssocDictionary;


/**
* The FileAssociations object manages file associations between a file extension
* and a FileAssoc record which contains command name, mime-type, icons, and other
* information about a type of file.
* Icons referenced by the FileAssoc record are managed by an icon cache, which
* guarantees that each icon is loaded into memory only once, when it is encountered
* for the first time.
* Associations for a file or directory are determined by the information in the
* FOX Registry setting under the "FILETYPES" section.
* Each entry maps a (partial) pathname to an association record comprising command
* name, description, large and small icons, mime-types, and flags:
*
*   command ';' description ';' bigicon [ ':' bigiconopen ] ';' icon [ ':' iconopen ] ';' mime [ ';' flags ]
*
* For example, the binding for "bmp" would be:
*
*   [FILETYPES]
*   bmp="eog %s &;Bitmap Image;bigimage.bmp;miniimage.bmp;image/x-ms-bmp;"
*
* And bindings for directories would be like:
*
*   [FILETYPES]
*   /=";Root Folder;bigroot.bmp:bigroot_open.bmp;miniroot.bmp:miniroot_open.bmp;application/x-folder;"
*   /home/jane=";Home Folder;mansion.bmp:mansion_open.bmp;hut.bmp:hut_open.bmp;application/x-folder;"
*
* Three types of pathnames are distinguished: regular files, directories, and
* executable files.
* The association for a regular file name is determined by first looking at the entire
* file name, then at the whole extension, and then at sub-extensions.
* For example, "package.tar.gz", "tar.gz", and "gz" can each be given a different
* file associations.
* If no extension is found, then a special fallback extension "defaultfilebinding"
* is consulted.  Thus, you can assign fallback properties for all reguler files
* by setting the "defaultfilebinding" entry under the "FILETYPES" section.
* The association for a directory name is found by first checking the full pathname,
* then dropping leading directory components in turn.
* For example: "/usr/local/include", "/local/include", and "/include" are checked in
* turn.  This scheme allows convenient assignment of an association for projects with
* common directory-structures but different project roots.
* If a directory association is not found, a fallback association "defaultdirbinding"
* is used to determine the association.
* The association for an executable file is found by looking up the binding for the
* "defaultexecbinding" under the "FILETYPES" section.
* The flags field is used for a number of bit-flags; two flags are currently
* defined: 'cd' and 'term'.  The first one is intended to cause a launcher
* to execute the application in the shown directory; the second one is meant
* to indicate that the application is to be ran inside a new terminal.
*/
class FXAPI FXFileAssociations : public FXObject {
  FXDECLARE(FXFileAssociations)
protected:
  FXFileAssocDictionary  bindings;      // File bindings dictionary
  FXIconCache            cache;         // Cache icons for rapid access
  FXSettings            *settings;      // Settings database for looking up extensions
protected:
  FXFileAssociations();
private:
  FXFileAssociations(const FXFileAssociations&);
  FXFileAssociations &operator=(const FXFileAssociations&);
public:

  /// Registry key used to find fallback executable icons
  static const FXchar defaultExecBinding[];

  /// Registry key used to find fallback directory icons
  static const FXchar defaultDirBinding[];

  /// Registry key used to find fallback document icons
  static const FXchar defaultFileBinding[];

public:

  /**
  * Construct a dictionary mapping file-extension to file associations,
  * using the application registry settings as a source for the bindings.
  * The pointer to the application class is passed down to the icon source
  * which is inside the icon dictionary.
  */
  FXFileAssociations(FXApp* app);

  /**
  * Construct a dictionary mapping file-extension to file associations,
  * using the specified settings database as a source for the bindings.
  * The pointer to the application class is passed down to the icon source
  * which is inside the icon dictionary.
  */
  FXFileAssociations(FXApp* app,FXSettings* sdb);

  /**
  * Change settings database being used to determine extension mappings.
  */
  void setSettings(FXSettings* sdb){ settings=sdb; }

  /**
  * Return settings database.
  */
  FXSettings* getSettings() const { return settings; }

  /**
  * Change the IconSource object used by the icon cache to load icons.
  */
  void setIconSource(FXIconSource* src){ cache.setIconSource(src); }

  /**
  * Return the current IconSource object.
  */
  FXIconSource* getIconSource() const { return cache.getIconSource(); }

  /**
  * Set the icon search paths for the icon cache.
  */
  void setIconPath(const FXString& path){ cache.setIconPath(path); }

  /**
  * Return the current icon search paths from the icon cache.
  */
  const FXString& getIconPath() const { return cache.getIconPath(); }

  /**
  * Parse string containing description of the association.
  */
  virtual FXFileAssoc* parse(const FXString& assoc);

  /**
  * Return mapping of input string to file-association; if no mapping
  * exists, try to add a new association mapping by consulting the
  * FILETYPES section of the settings database.
  * You can overload this function if you need to supply custom
  * mappings for selected extensions.
  */
  virtual FXFileAssoc* fetch(const FXString& ext);

  /**
  * Determine binding for the given file.
  * The default implementation tries the whole filename first,
  * then tries the extensions.
  * For example, for a file "source.tar.gz":
  *
  *  "source.tar.gz",
  *  "tar.gz",
  *  "gz"
  *
  * are tried in succession.  If no association is found the
  * key "defaultfilebinding" is tried as a fallback association.
  * A NULL is returned if no association of any kind is found.
  */
  virtual FXFileAssoc* findFileBinding(const FXString& pathname);

  /**
  * Find directory binding from registry.
  * The default implementation tries the whole pathname first,
  * then tries successively smaller parts of the path.
  * For example, a pathname "/usr/people/jeroen":
  *
  *   "/usr/people/jeroen"
  *   "/people/jeroen"
  *   "/jeroen"
  *
  * are tried in succession.  If no bindings are found, the
  * key "defaultdirbinding" is tried as a fallback association.
  * A NULL is returned if no association of any kind is found.
  */
  virtual FXFileAssoc* findDirBinding(const FXString& pathname);

  /**
  * Determine binding for the given executable.
  * The default implementation returns the fallback binding associated with
  * the key "defaultexecbinding".
  * A NULL is returned if no association of any kind is found.
  */
  virtual FXFileAssoc* findExecBinding(const FXString& pathname);

  /**
  * Delete all file-associations, and clear all icons from the cache.
  */
  void clear();

  /**
  * Save object to stream.
  */
  virtual void save(FXStream& store) const;

  /**
  * Load object from stream.
  */
  virtual void load(FXStream& store);

  /**
  * Delete all FileAssoc's, and the IconCache.
  */
  virtual ~FXFileAssociations();
  };

}

#endif
