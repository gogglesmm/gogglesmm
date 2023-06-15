/********************************************************************************
*                                                                               *
*                 R e g u l a r   E x p r e s s i o n   C l a s s               *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2023 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXREX_H
#define FXREX_H

namespace FX {


/**
* FXRex is a regular expression class implementing a NFA matcher.
* It supports capturing parentheses, non-capturing parentheses, positive or negative
* lookahead, backreferences, case-insensitive matching, counted repetitions, greedy, lazy and
* possessive matches, and PERL-like matching operators.
* The subject string may be searched forwards or backwards, and may contain any of
* 256 possible byte values.
*
* When parsing a regular expression pattern, the mode parameter is the bitwise OR of
* a set of flags and affects the match algorithm.  Passing the flag Capture enables
* capturing parentheses and back references, and allows the matcher engine to return
* the locations of the string matching these sub-patterns. The flag IgnoreCase enables
* case-insensitive matching.
*
* When the flag Newline is passed, newlines are treated like normal characters, and
* not line-separators.  If Newline flag is not passed, character classes such as '.',
* '\D', '\s', [^a-z] etc. will NOT match newlines.  The flag Verbatim disables all
* special character interpretation, making the entire pattern a literal string to be
* matched against a string.
*
* When the Exact flag is passed, a match succeeds only if the entire string is matched,
* i.e. the entire input presented to FXRex must match against the pattern; otherwise,
* only a (possibly empty) substring of the input is matched against the pattern.
* If the NotEmpty flag is passed, the pattern must match at least one character in order
* to succeed, and empty matches are considered non-matching.
*
* If the flag Syntax will check the pattern for correct syntax only, and not generate a
* matching engine; it will just reset the engine to the empty pattern; use this flag to
* verify the syntax of the pattern without compiling it.
*
* When matching a compiled pattern, the mode parameter is the bitwise OR of a set of
* flags that affects how the match is performed.  Passing the flags NotBol and/or NotEol
* causes the begin and end of the subject string NOT to be considered a line start or
* line end.
*
* Patterns which cause inordinate amounts of recursion may cause FXRex to fail where
* otherwise it would succeed to match.
* FXRex uses no global variables, and thus multiple threads may simultaneously use it;
* moreover, multiple threads may use the same instance to perform a match.
*/
class FXAPI FXRex {
private:
  FXString code;
private:
  static const FXchar *const errors[];
public:

  /// Regular expression flags
  enum {

    /// Flags for both parse and match mode
    Normal     = 0,     /// Normal mode (default)
    Unicode    = 1,     /// Unicode mode

    /// Regular expression parse flags
    Syntax     = 2,     /// Perform syntax check only
    Verbatim   = 4,     /// Literal pattern mode with no magic characters
    Capture    = 8,     /// Perform capturing parentheses
    IgnoreCase = 16,    /// Ignore case differences
    Newline    = 32,    /// Match-any operators match newline too
    Exact      = 64,    /// Exact match to entire string (\A..\Z)
    NotEmpty   = 128,   /// A successful match must not be empty
    Reverse    = 256,   /// Reverse expression mode
    Words      = 512,   /// Match whole words (\<..\>)

    /// Regular expression match flags
    NotBol     = 1024,  /// Start of string is NOT begin of line
    NotEol     = 2048   /// End of string is NOT end of line
    };

  /// Regular expression error codes
  enum Error {
    ErrOK      = 0,     /// No errors
    ErrEmpty   = 1,     /// Empty pattern
    ErrMore    = 2,     /// More characters after pattern
    ErrParent  = 3,     /// Unmatched parenthesis
    ErrBracket = 4,     /// Unmatched bracket
    ErrBrace   = 5,     /// Unmatched brace
    ErrRange   = 6,     /// Bad character range
    ErrEscape  = 7,     /// Bad escape sequence
    ErrCount   = 8,     /// Bad counted repeat
    ErrNoAtom  = 9,     /// No atom preceding repetition
    ErrRepeat  = 10,    /// Repeat following repeat
    ErrBackRef = 11,    /// Bad backward reference
    ErrClass   = 12,    /// Bad character class
    ErrComplex = 13,    /// Expression too complex
    ErrMemory  = 14,    /// Out of memory
    ErrToken   = 15,    /// Illegal token
    ErrLong    = 16,    /// Pattern too long
    ErrSupport = 17     /// Unsupported
    };

public:

  /**
  * Construct empty regular expression object, with the
  * fallback program installed.
  */
  FXRex();

  /**
  * Copy regular expression object  from another.
  */
  FXRex(const FXRex& orig);

  /// Compile expression from pattern; if error is not NULL, error code is returned
  FXRex(const FXchar* pattern,FXint mode=Normal,Error* error=nullptr);

  /// Compile expression from pattern; if error is not NULL, error code is returned
  FXRex(const FXString& pattern,FXint mode=Normal,Error* error=nullptr);

  /**
  * See if regular expression is empty; the regular expression
  * will be empty when it is unable to parse a pattern due to
  * a syntax error.
  */
  FXbool empty() const { return code.empty(); }

  /**
  * Parse pattern, return error code if syntax error is found.
  * The parse-mode flags control the compile options, and affect how
  * the generated matcher behaves.
  * If a parse fails, an error code is returned; in this case, the
  * expression matcher will be set up to a fallback program.
  */
  Error parse(const FXchar* pattern,FXint mode=Normal);
  Error parse(const FXString& pattern,FXint mode=Normal);

  /**
  * Perform anchored match of subject string of length len at position pos, returning true
  * if the pattern matches at this point.
  * If there is a match, the pattern and subpatterns are captured in the arrays beg[] and end[]
  * which must both be at least npar entries long.
  */
  FXbool amatch(const FXchar* string,FXint len,FXint pos=0,FXint mode=Normal,FXint* beg=nullptr,FXint* end=nullptr,FXint npar=0) const;
  FXbool amatch(const FXString& string,FXint pos=0,FXint mode=Normal,FXint* beg=nullptr,FXint* end=nullptr,FXint npar=0) const;


  /**
  * Search subject string of length len for a pattern, returning the location where the pattern
  * is found relative from the start of the string, or -1 if there is no match.
  * In case of a successful match, the pattern and subpatterns are captured in the arrays beg[] and end[]
  * which must be at least npar entries long.
  * The string is searched forwards (or backwards) starting from position fm toward to, both of which
  * must lie inside the string.
  */
  FXint search(const FXchar* string,FXint len,FXint fm,FXint to,FXint mode=Normal,FXint* beg=nullptr,FXint* end=nullptr,FXint npar=0) const;
  FXint search(const FXString& string,FXint fm,FXint to,FXint mode=Normal,FXint* beg=nullptr,FXint* end=nullptr,FXint npar=0) const;

  /**
  * After performing a regular expression match with capturing parentheses,
  * a substitution string is build from the replace string, where where "&"
  * is replaced by the entire matched pattern, and "\1" through "\9" are
  * replaced by captured expressions.  The original source string and its
  * length, and the match arrays beg and end must be passed.
  * The replace string may also contain regular escape sequences to embed special
  * characters.
  */
  static FXString substitute(const FXchar* string,FXint len,FXint* beg,FXint* end,const FXchar* replace,FXint npar=1);
  static FXString substitute(const FXchar* string,FXint len,FXint* beg,FXint* end,const FXString& replace,FXint npar=1);
  static FXString substitute(const FXString& string,FXint* beg,FXint* end,const FXchar* replace,FXint npar=1);
  static FXString substitute(const FXString& string,FXint* beg,FXint* end,const FXString& replace,FXint npar=1);

  /// Returns error message text for a given error code
  static const FXchar* getError(Error err){ return errors[err]; }

  /// Assign another regular expression to this one
  FXRex& operator=(const FXRex& orig);

  /// Comparison operators
  FXbool operator==(const FXRex& rex) const;
  FXbool operator!=(const FXRex& rex) const;

  /// Saving and loading
  friend FXAPI FXStream& operator<<(FXStream& store,const FXRex& s);
  friend FXAPI FXStream& operator>>(FXStream& store,FXRex& s);

  /**
  * Clear the expression object and reinstate the fallback program.
  */
  void clear();

  /// Delete
 ~FXRex();
  };


// Serialization
extern FXAPI FXStream& operator<<(FXStream& store,const FXRex& s);
extern FXAPI FXStream& operator>>(FXStream& store,FXRex& s);

}

#endif
