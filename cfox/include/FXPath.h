/********************************************************************************
*                                                                               *
*                  P a t h   N a m e   M a n i p u l a t i o n                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXPATH_H
#define FXPATH_H

namespace FX {

namespace FXPath {

  /// Options for FXPath::match
  enum {
    PathName   = 1,        /// No wildcard can ever match "/'
    NoEscape   = 2,        /// Backslashes don't quote special chars
    DotFile    = 4,        /// Leading "." is matched only explicitly
    LeadDir    = 8,        /// Ignore "/..." after a match
    CaseFold   = 16        /// Compare without regard to case
    };

  /**
  * Return root of absolute path; on Unix, this is just "/". On
  * Windows, this is "\", "\\Janes PC\" or "C:\".
  * Returns the empty string if the given path is not absolute.
  */
  extern FXAPI FXString root(const FXString& file);

  /**
  * Return share name from Windows UNC filename, if any.
  * For example, share("\\Janes PC\Janes Documents\Document.doc")
  * returns "Janes PC".
  */
  extern FXAPI FXString share(const FXString& file);

  /**
  * Return the directory part of the path name.
  * Note that directory("/bla/bla/") is "/bla/bla" and NOT "/bla".
  * However, directory("/bla/bla") is "/bla" as we expect!
  */
  extern FXAPI FXString directory(const FXString& file);

  /**
  * Return name and extension part of the path name.
  * Note that name("/bla/bla/") is "" and NOT "bla".
  * However, name("/bla/bla") is "bla" as we expect!
  */
  extern FXAPI FXString name(const FXString& file);

  /**
  * Return file stem, i.e. document name only.
  */
  extern FXAPI FXString stem(const FXString& file);

  /**
  * Return extension part of the file name.
  * This returns the string after the last '.' in the last path-component of the
  * file, provided that the '.' was not the first character of the path-component.
  * For example, extension("/usr/share/icons/folder.png") returns "png", but the call to
  * extension("../path.name/.bashrc") returns "".
  */
  extern FXAPI FXString extension(const FXString& file);

  /**
  * Return file name less the extension.
  */
  extern FXAPI FXString stripExtension(const FXString& file);

  /**
  * Return the drive letter prefixing this file name (if any).
  */
  extern FXAPI FXString drive(const FXString& file);

  /**
  * Perform tilde or environment variable expansion.
  * A prefix of the form ~ or ~user is expanded to the user's home directory.
  * Environment variables of the form $HOME or ${HOME} are expanded by
  * substituting the value of the variable, recusively up to given level.
  * On Windows, only environment variables of the form %HOME% are expanded.
  */
  extern FXAPI FXString expand(const FXString& file,FXint level=4);

  /**
  * Convert a foreign path(s) or paths to local conventions,
  * replacing environment variables etc.
  */
  extern FXAPI FXString convert(const FXString& path);

  /// Contract path based on user name and environment variable
  extern FXAPI FXString contract(const FXString& file,const FXString& user=FXString::null,const FXString& var=FXString::null);

  /**
  * Simplify a file path; the path will remain relative if it was relative,
  * or absolute if it was absolute.  Also, a trailing "/" will be preserved
  * as this is important in other functions.  Finally, returned path should
  * be non-empty unless the input path was empty, and pathological paths
  * will be fixed.
  */
  extern FXAPI FXString simplify(const FXString& file);

  /// Return absolute path from current directory and file name
  extern FXAPI FXString absolute(const FXString& file);

  /// Return absolute path from base directory and file name
  extern FXAPI FXString absolute(const FXString& base,const FXString& file);

  /// Return relative path of file to the current directory
  extern FXAPI FXString relative(const FXString& file);

  /// Return relative path of file to given absolute base directory
  extern FXAPI FXString relative(const FXString& base,const FXString& file);

  /// Return path to directory above input directory name
  extern FXAPI FXString upLevel(const FXString& file);

  /// Return true if file positively inside base directory
  extern FXAPI FXbool isInside(const FXString& base,const FXString& file);

  /// Return true if file name is absolute
  extern FXAPI FXbool isAbsolute(const FXString& file);

  /// Return true if file name is relative
  extern FXAPI FXbool isRelative(const FXString& file);

  /// Return true if input directory is a top-level directory
  extern FXAPI FXbool isTopDirectory(const FXString& file);

  /// Return true if input path is a file share
  extern FXAPI FXbool isShare(const FXString& file);

  /// Return true if input path is a hidden file or directory
  extern FXAPI FXbool isHidden(const FXString& file);

  /// Return valid part of absolute path
  extern FXAPI FXString validPath(const FXString& file);

  /// Return true if path is valid
  extern FXAPI FXbool isValidPath(const FXString& file);

  /// Enquote filename to make safe for shell
  extern FXAPI FXString enquote(const FXString& file,FXbool force=false);

  /// Dequote filename to get original again
  extern FXAPI FXString dequote(const FXString& file);

  /**
  * Parse command string to argc and argv.
  * The input string may contain single- or double-quoted segments
  * or escape characters, as per shell rules.
  * Returns argc, the number of arguments identified in the input,
  * if successful.  If input is empty (NULL pointer or only whitespace),
  * or syntax errors are encountered, or system is out of memory, the
  * function returns 0.
  * Memory at argv may be released by a single call to freeElms().
  */
  extern FXAPI FXint parseArgs(FXchar**& argv,const FXchar* command);

  /**
  * Parse command string to argc and argv.
  * This is a convenience version for the parseArgv() above.
  */
  extern FXAPI FXint parseArgs(FXchar**& argv,const FXString& command);

  /**
  * Perform match of a string against a wildcard pattern.
  * The wildcard pattern may comprise ordinary characters or special
  * matching characters, as given below:
  *
  *  ?           Any single character.
  *  *           Zero or more of any character.
  *  [abc]       Any character from the set {a,b,c}.
  *  [^abc]      Any character BUT the ones from the set {a,b,c}.
  *  [!abc]      Any character BUT the ones from the set {a,b,c}.
  *  [a-zA-Z]    Matches single character, which must be one of a-z or A-Z.
  *  [^a-zA-Z]   Matches single character, which must be anything BUT a-z or A-Z.
  *  [!a-zA-Z]   Matches single character, which must be anything BUT a-z or A-Z.
  *  pat1|pat2   Match sub-pattern pat1 or pat2.
  *  pat1,pat2   Match sub-pattern pat1 or pat2.
  *  (pat1|pat2) Match sub-pattern pat1 or pat2; patterns may be nested.
  *  (pat1,pat2) Match sub-pattern pat1 or pat2; patterns may be nested.
  *
  * The special characters may be escaped to treat them as ordinary characters.
  * Matching may be influenced by a number of flags:
  *
  *  PathName      No wildcard can ever match /
  *  NoEscape      Backslashes don't quote special chars
  *  DotFile       Leading . is matched only explicitly, and not against wildcard
  *  LeadDir       Ignore /... after a match
  *  CaseFold      Compare without regard to case
  */
  extern FXAPI FXbool match(const FXchar* string,const FXchar* pattern="*",FXuint flags=(NoEscape|PathName));

  /**
  * Perform match of a string against a wildcard pattern.
  */
  extern FXAPI FXbool match(const FXString& string,const FXchar* pattern="*",FXuint flags=(NoEscape|PathName));

  /**
  * Perform match of a string against a wildcard pattern.
  */
  extern FXAPI FXbool match(const FXchar* string,const FXString& pattern,FXuint flags=(NoEscape|PathName));

  /**
  * Perform match of a string against a wildcard pattern.
  */
  extern FXAPI FXbool match(const FXString& string,const FXString& pattern,FXuint flags=(NoEscape|PathName));

  /**
  * Generate unique filename of the form pathnameXXX.ext, where
  * pathname.ext is the original input file, and XXX is a number,
  * possibly empty, that makes the file unique.
  */
  extern FXAPI FXString unique(const FXString& file);

  /**
  * Search path list for this file, return full path name for first occurrence.
  * Each path in the pathlist is expanded first.
  */
  extern FXAPI FXString search(const FXString& pathlist,const FXString& file);

  /**
  * Given search path list, return shortest relative path name that still
  * uniquely resolves to the given file name.
  */
  extern FXAPI FXString relativize(const FXString& pathlist,const FXString& file);

  /**
  * Check if the file has an extension from the list of known executable
  * extensions (Windows)
  */
  extern FXAPI FXbool hasExecExtension(const FXString& file);

  /**
  * Check if given name is controversial, i.e. reserved for other devices (Windows).
  */
  extern FXAPI FXbool isReservedName(const FXString& file);

  }

}

#endif
