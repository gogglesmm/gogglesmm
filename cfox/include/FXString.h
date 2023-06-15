/********************************************************************************
*                                                                               *
*                           S t r i n g   O b j e c t                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXSTRING_H
#define FXSTRING_H

namespace FX {


class FXStream;


/**
* FXString provides essential string manipulation capabilities in FOX.
* Internally, all strings are stored as an array of bytes preceeded by
* the length of the string; an end-of-string character is appended at the end,
* but is not included in the length of the string.
* An empty string is represented by unique empty-string value which comprises
* a length of 0, followed by an end-of-string character.
* Thus, it is ALWAYS safe to dereference FXString as it will never be NULL.
* FXString can be initialized direcly from literal strings in UTF-8 encoding,
* but supports automatic conversion from UTF-16 encoded ("narrow") character
* strings and UTF-32 encoded ("wide") character strings.
*/
class FXAPI FXString {
private:
  FXchar* str;
public:
  static const FXchar null[];
public:

  /// Construct empty string
  FXString();

  /// Construct copy of another string
  FXString(const FXString& s);

  /// Construct and init from string
  FXString(const FXchar* s);

  /// Construct and init from narrow character string
  FXString(const FXnchar* s);

  /// Construct and init from wide character string
  FXString(const FXwchar* s);

  /// Construct and init with substring
  FXString(const FXchar* s,FXint n);

  /// Construct and init with narrow character substring
  FXString(const FXnchar* s,FXint n);

  /// Construct and init with wide character substring
  FXString(const FXwchar* s,FXint n);

  /// Construct and fill with constant
  FXString(FXchar c,FXint n);

  /// Length of text in bytes
  FXint length() const { return *(((FXint*)str)-1); }

  /// Change the length of the string to len
  FXbool length(FXint len);

  /// Return wide character starting at p
  FXwchar wc(FXint p) const;

  /// Return wide character at p and advance to next
  FXwchar wcnxt(FXint& p) const;

  /// Retreat to wide character previous to p and return it
  FXwchar wcprv(FXint& p) const;

  /// Increment byte offset by one utf8 character
  FXint inc(FXint p) const;

  /// Increment byte offset by n utf8 characters
  FXint inc(FXint p,FXint n) const;

  /// Decrement byte offset by one utf8 character
  FXint dec(FXint p) const;

  /// Decrement byte offset by n utf8 characters
  FXint dec(FXint p,FXint n) const;

  /// Count number of utf8 characters
  FXint count() const;

  /// Count number of utf8 characters in subrange start...end
  FXint count(FXint start,FXint end) const;

  /// Return byte offset of utf8 character at index
  FXint offset(FXint indx) const;

  /// Return index of utf8 character at byte offset
  FXint index(FXint offs) const;

  /// Validate position to point to begin of utf8 character
  FXint validate(FXint p) const;

  /// Get text contents as pointer
  FXchar* text(){ return str; }

  /// Get text contents as const pointer
  const FXchar* text() const { return str; }

  /// See if string is empty
  FXbool empty() const { return (((FXint*)str)[-1]==0); }

  /// See if string is empty
  FXbool operator!() const { return (((FXint*)str)[-1]==0); }

  /// Return a non-const reference to the ith character
  FXchar& operator[](FXint i){ return str[i]; }

  /// Return a const reference to the ith character
  const FXchar& operator[](FXint i) const { return str[i]; }

  /// Return a non-const reference to the ith character
  FXchar& at(FXint i){ return str[i]; }

  /// Return a const reference to the ith character
  const FXchar& at(FXint i) const { return str[i]; }

  /// Return a non-const reference to the first character
  FXchar& head(){ return str[0]; }

  /// Return a const reference to the first character
  const FXchar& head() const { return str[0]; }

  /// Return a non-const reference to the last character
  FXchar& tail(){ return str[length()-1]; }

  /// Return a const reference to the last character
  const FXchar& tail() const { return str[length()-1]; }

  /// Assign a constant string to this string
  FXString& operator=(const FXchar* s);

  /// Assign a narrow character string to this string
  FXString& operator=(const FXnchar* s);

  /// Assign a wide character string to this string
  FXString& operator=(const FXwchar* s);

  /// Assign another string to this string
  FXString& operator=(const FXString& s);

  /// Append single character to this string
  FXString& operator+=(FXchar c);

  /// Append constant string after this string
  FXString& operator+=(const FXchar* s);

  /// Append narrow character string after this string
  FXString& operator+=(const FXnchar* s);

  /// Append wide characeter string after this string
  FXString& operator+=(const FXwchar* s);

  /// Append another string after this string
  FXString& operator+=(const FXString& s);

  /// Adopt string s, leaving s empty
  FXString& adopt(FXString& s);

  /// Assign character c to this string
  FXString& assign(FXchar c);

  /// Assign n characters c to this string
  FXString& assign(FXchar c,FXint n);

  /// Assign string s to this string
  FXString& assign(const FXchar* s);

  /// Assign narrow character string s to this string
  FXString& assign(const FXnchar* s);

  /// Assign wide character string s to this string
  FXString& assign(const FXwchar* s);

  /// Assign first n characters of string s to this string
  FXString& assign(const FXchar *s,FXint n);

  /// Assign first n characters of narrow character string s to this string
  FXString& assign(const FXnchar *s,FXint n);

  /// Assign first n characters of wide character string s to this string
  FXString& assign(const FXwchar *s,FXint n);

  /// Assign string s to this string
  FXString& assign(const FXString& s);

  /// Insert character at specified position
  FXString& insert(FXint pos,FXchar c);

  /// Insert n characters c at specified position
  FXString& insert(FXint pos,FXchar c,FXint n);

  /// Insert string at specified position
  FXString& insert(FXint pos,const FXchar* s);

  /// Insert narrow character string at specified position
  FXString& insert(FXint pos,const FXnchar* s);

  /// Insert wide character string at specified position
  FXString& insert(FXint pos,const FXwchar* s);

  /// Insert first n characters of string at specified position
  FXString& insert(FXint pos,const FXchar* s,FXint n);

  /// Insert first n characters of narrow character string at specified position
  FXString& insert(FXint pos,const FXnchar* s,FXint n);

  /// Insert first n characters of wide character string at specified position
  FXString& insert(FXint pos,const FXwchar* s,FXint n);

  /// Insert string at specified position
  FXString& insert(FXint pos,const FXString& s);

  /// Prepend string with input character
  FXString& prepend(FXchar c);

  /// Prepend string with n characters c
  FXString& prepend(FXchar c,FXint n);

  /// Prepend string with string s
  FXString& prepend(const FXchar* s);

  /// Prepend string with narrow character string
  FXString& prepend(const FXnchar* s);

  /// Prepend string with wide character string
  FXString& prepend(const FXwchar* s);

  /// Prepend first n characters of string s
  FXString& prepend(const FXchar* s,FXint n);

  /// Prepend first n characters of narrow character string s
  FXString& prepend(const FXnchar* s,FXint n);

  /// Prepend first n characters of wide character string s
  FXString& prepend(const FXwchar* s,FXint n);

  /// Prepend string with string s
  FXString& prepend(const FXString& s);

  /// Append character c to this string
  FXString& append(FXchar c);

  /// Append n characters c to this string
  FXString& append(FXchar c,FXint n);

  /// Append string s to this string
  FXString& append(const FXchar* s);

  /// Append narrow character string s to this string
  FXString& append(const FXnchar* s);

  /// Append wide character string s to this string
  FXString& append(const FXwchar* s);

  /// Append first n characters of string s to this string
  FXString& append(const FXchar* s,FXint n);

  /// Append first n characters of narrow character string s to this string
  FXString& append(const FXnchar* s,FXint n);

  /// Append first n characters of wide character string s to this string
  FXString& append(const FXwchar* s,FXint n);

  /// Append string s to this string
  FXString& append(const FXString& s);

  /// Replace a single character
  FXString& replace(FXint pos,FXchar c);

  /// Replace the r characters at pos with n characters c
  FXString& replace(FXint pos,FXint r,FXchar c,FXint n);

  /// Replace the r characters at pos with string s
  FXString& replace(FXint pos,FXint r,const FXchar* s);

  /// Replace the r characters at pos with narrow character string s
  FXString& replace(FXint pos,FXint r,const FXnchar* s);

  /// Replace the r characters at pos with wide character string s
  FXString& replace(FXint pos,FXint r,const FXwchar* s);

  /// Replaces the r characters at pos with first n characters of string s
  FXString& replace(FXint pos,FXint r,const FXchar* s,FXint n);

  /// Replaces the r characters at pos with first n characters of narrow character string s
  FXString& replace(FXint pos,FXint r,const FXnchar* s,FXint n);

  /// Replaces the r characters at pos with first n characters of wide character string s
  FXString& replace(FXint pos,FXint r,const FXwchar* s,FXint n);

  /// Replace the r characters at pos with string s
  FXString& replace(FXint pos,FXint r,const FXString& s);

  /// Move range of n characters from src position to dst position
  FXString& move(FXint dst,FXint src,FXint n);

  /// Remove one character
  FXString& erase(FXint pos);

  /// Remove substring
  FXString& erase(FXint pos,FXint n);

  /// Truncate string at pos
  FXString& trunc(FXint pos);

  /// Clear
  FXString& clear();

  /// Return number of occurrences of ch in string
  FXint contains(FXchar ch) const;

  /// Return number of occurrences of string sub in string
  FXint contains(const FXchar* sub,FXint n) const;

  /// Return number of occurrences of string sub in string
  FXint contains(const FXchar* sub) const;

  /// Return number of occurrences of string sub in string
  FXint contains(const FXString& sub) const;

  /// Substitute one character by another
  FXString& substitute(FXchar org,FXchar sub,FXbool all=true);

  /// Substitute one string by another
  FXString& substitute(const FXchar* org,FXint olen,const FXchar *rep,FXint rlen,FXbool all=true);

  /// Substitute one string by another
  FXString& substitute(const FXchar* org,const FXchar *rep,FXbool all=true);

  /// Substitute one string by another
  FXString& substitute(const FXString& org,const FXString& rep,FXbool all=true);

  /// Convert to lower case
  FXString& lower();

  /// Convert to upper case
  FXString& upper();

  /// Simplify whitespace in string
  FXString& simplify();

  /// Remove leading and trailing whitespace
  FXString& trim();

  /// Remove leading whitespace
  FXString& trimBegin();

  /// Remove trailing whitespace
  FXString& trimEnd();

  /// Get left most part
  FXString left(FXint n) const;

  /// Get right most part
  FXString right(FXint n) const;

  /// Get some part in the middle
  FXString mid(FXint pos,FXint n) const;

  /// Return num partition(s) from a given start partition in a string separated by delimiters delim.
  FXString section(FXchar delim,FXint start,FXint num=1) const;

  /// Return num partition(s) from a given start partition in a string separated by set of delimiters from delim of size n
  FXString section(const FXchar* delim,FXint n,FXint start,FXint num) const;

  /// Return num partition(s) from a given start partition in a string separated by set of delimiters from delim.
  FXString section(const FXchar* delim,FXint start,FXint num=1) const;

  /// Return num partition(s) from a given start partition in a string separated by set of delimiters from delim.
  FXString section(const FXString& delim,FXint start,FXint num=1) const;

  /**
  * Return all characters before the n-th occurrence of ch,
  * searching from the beginning of the string. If the character
  * is not found, return the entire string.  If n<=0, return
  * the empty string.
  */
  FXString before(FXchar ch,FXint n=1) const;

  /**
  * Return all characters before the n-th occurrence of ch,
  * searching from the end of the string. If the character
  * is not found, return the empty string. If n<=0, return
  * the entire string.
  */
  FXString rbefore(FXchar ch,FXint n=1) const;

  /**
  * Return all characters after the nth occurrence of ch,
  * searching from the beginning of the string. If the character
  * is not found, return the empty string.  If n<=0, return
  * the entire string.
  */
  FXString after(FXchar ch,FXint n=1) const;

  /**
  * Return all characters after the nth occurrence of ch,
  * searching from the end of the string. If the character
  * is not found, return the entire string. If n<=0, return
  * the empty string.
  */
  FXString rafter(FXchar ch,FXint n=1) const;

  /// Find a character, searching forward; return position or -1
  FXint find(FXchar c,FXint pos=0) const;

  /// Find a character, searching backward; return position or -1
  FXint rfind(FXchar c,FXint pos=2147483647) const;

  /// Find n-th occurrence of character, searching forward; return position or -1
  FXint find(FXchar c,FXint pos,FXint n) const;

  /// Find n-th occurrence of character, searching backward; return position or -1
  FXint rfind(FXchar c,FXint pos,FXint n) const;

  /// Find a substring of length n, searching forward; return position or -1
  FXint find(const FXchar* substr,FXint n,FXint pos) const;

  /// Find a substring of length n, searching backward; return position or -1
  FXint rfind(const FXchar* substr,FXint n,FXint pos) const;

  /// Find a substring, searching forward; return position or -1
  FXint find(const FXchar* substr,FXint pos=0) const;

  /// Find a substring, searching backward; return position or -1
  FXint rfind(const FXchar* substr,FXint pos=2147483647) const;

  /// Find a substring, searching forward; return position or -1
  FXint find(const FXString& substr,FXint pos=0) const;

  /// Find a substring, searching backward; return position or -1
  FXint rfind(const FXString& substr,FXint pos=2147483647) const;

  /// Find first character in the set of size n, starting from pos; return position or -1
  FXint find_first_of(const FXchar* set,FXint n,FXint pos) const;

  /// Find first character in the set, starting from pos; return position or -1
  FXint find_first_of(const FXchar* set,FXint pos=0) const;

  /// Find first character in the set, starting from pos; return position or -1
  FXint find_first_of(const FXString& set,FXint pos=0) const;

  /// Find first character, starting from pos; return position or -1
  FXint find_first_of(FXchar c,FXint pos=0) const;

  /// Find last character in the set of size n, starting from pos; return position or -1
  FXint find_last_of(const FXchar* set,FXint n,FXint pos) const;

  /// Find last character in the set, starting from pos; return position or -1
  FXint find_last_of(const FXchar* set,FXint pos=2147483647) const;

  /// Find last character in the set, starting from pos; return position or -1
  FXint find_last_of(const FXString& set,FXint pos=2147483647) const;

  /// Find last character, starting from pos; return position or -1
  FXint find_last_of(FXchar c,FXint pos=2147483647) const;

  /// Find first character NOT in the set of size n, starting from pos; return position or -1
  FXint find_first_not_of(const FXchar* set,FXint n,FXint pos) const;

  /// Find first character NOT in the set, starting from pos; return position or -1
  FXint find_first_not_of(const FXchar* set,FXint pos=0) const;

  /// Find first character NOT in the set, starting from pos; return position or -1
  FXint find_first_not_of(const FXString& set,FXint pos=0) const;

  /// Find first character NOT equal to c, starting from pos; return position or -1
  FXint find_first_not_of(FXchar c,FXint pos=0) const;

  /// Find last character NOT in the set of size n, starting from pos; return position or -1
  FXint find_last_not_of(const FXchar* set,FXint n,FXint pos) const;

  /// Find last character NOT in the set, starting from pos; return position or -1
  FXint find_last_not_of(const FXchar* set,FXint pos=2147483647) const;

  /// Find last character NOT in the set, starting from pos; return position or -1
  FXint find_last_not_of(const FXString& set,FXint pos=2147483647) const;

  /// Find last character NOT equal to c, starting from pos; return position or -1
  FXint find_last_not_of(FXchar c,FXint pos=2147483647) const;

  /// Scan a string a-la scanf
  FXint scan(const FXchar* fmt,...) const FX_SCANF(2,3) ;
  FXint vscan(const FXchar* fmt,va_list args) const;

  /// Format a string a-la printf
  FXint format(const FXchar* fmt,...) FX_PRINTF(2,3) ;
  FXint vformat(const FXchar* fmt,va_list args);

  /// Convert to integer
  FXint toInt(FXint base=10,FXbool* ok=nullptr) const;

  /// Convert to unsigned integer
  FXuint toUInt(FXint base=10,FXbool* ok=nullptr) const;

  /// Convert to long integer
  FXlong toLong(FXint base=10,FXbool* ok=nullptr) const;

  /// Convert to unsigned long integer
  FXulong toULong(FXint base=10,FXbool* ok=nullptr) const;

  /// Convert to float
  FXfloat toFloat(FXbool* ok=nullptr) const;

  /// Convert to double
  FXdouble toDouble(FXbool* ok=nullptr) const;

  /// Convert from integer
  FXString& fromInt(FXint number,FXint base=10);

  /// Convert from unsigned integer
  FXString& fromUInt(FXuint number,FXint base=10);

  /// Convert from long integer
  FXString& fromLong(FXlong number,FXint base=10);

  /// Convert from unsigned long integer
  FXString& fromULong(FXulong number,FXint base=10);

  /// Convert from float; fmt is 0 (no exponent), 1 (force exponent), or 2 (exponent when needed)
  FXString& fromFloat(FXfloat number,FXint prec=6,FXint fmt=2);

  /// Convert from double; fmt is 0 (no exponent), 1 (force exponent), or 2 (exponent when needed)
  FXString& fromDouble(FXdouble number,FXint prec=6,FXint fmt=2);

  /**
  * Return a string value by converting an integer number to a string,
  * using the given number base, which must be between 2 and 16.
  */
  static FXString value(FXint num,FXint base=10);
  static FXString value(FXuint num,FXint base=10);

  /**
  * Return a string value by converting a long integer number to a string,
  * using the given number base, which must be between 2 and 16.
  */
  static FXString value(FXlong num,FXint base=10);
  static FXString value(FXulong num,FXint base=10);

  /**
  * Return a string value by converting real number to a string, using the given
  * procision and exponential notation mode, which may be 0 (never), 1 (always),
  * or 2 (when needed).
  */
  static FXString value(FXfloat num,FXint prec=6,FXint fmt=2);
  static FXString value(FXdouble num,FXint prec=6,FXint fmt=2);

  /**
  * Return a string value from printf-like format arguments.
  */
  static FXString value(const FXchar* fmt,...) FX_PRINTF(1,2) ;

  /**
  * Return a string value from vprintf-like format arguments.
  */
  static FXString vvalue(const FXchar* fmt,va_list args);


  /// Compute hash value of string
  FXuint hash() const;

  /// Compute hash value of string
  static FXuint hash(const FXchar* s);

  /// Compute hash value of string
  static FXuint hash(const FXchar* s,FXint n);

  /// Compare
  static FXint compare(const FXchar* s1,const FXchar* s2);
  static FXint compare(const FXchar* s1,const FXString& s2);
  static FXint compare(const FXString& s1,const FXchar* s2);
  static FXint compare(const FXString& s1,const FXString& s2);

  /// Compare up to n
  static FXint compare(const FXchar* s1,const FXchar* s2,FXint n);
  static FXint compare(const FXchar* s1,const FXString& s2,FXint n);
  static FXint compare(const FXString& s1,const FXchar* s2,FXint n);
  static FXint compare(const FXString& s1,const FXString& s2,FXint n);

  /// Compare case insensitive
  static FXint comparecase(const FXchar* s1,const FXchar* s2);
  static FXint comparecase(const FXchar* s1,const FXString& s2);
  static FXint comparecase(const FXString& s1,const FXchar* s2);
  static FXint comparecase(const FXString& s1,const FXString& s2);

  /// Compare case insensitive up to n
  static FXint comparecase(const FXchar* s1,const FXchar* s2,FXint n);
  static FXint comparecase(const FXchar* s1,const FXString& s2,FXint n);
  static FXint comparecase(const FXString& s1,const FXchar* s2,FXint n);
  static FXint comparecase(const FXString& s1,const FXString& s2,FXint n);

  /// Compare with natural interpretation of decimal numbers
  static FXint comparenatural(const FXchar* s1,const FXchar* s2);
  static FXint comparenatural(const FXchar* s1,const FXString& s2);
  static FXint comparenatural(const FXString& s1,const FXchar* s2);
  static FXint comparenatural(const FXString& s1,const FXString& s2);

  /// Compare case insensitive with natural interpretation of decimal numbers
  static FXint comparenaturalcase(const FXchar* s1,const FXchar* s2);
  static FXint comparenaturalcase(const FXchar* s1,const FXString& s2);
  static FXint comparenaturalcase(const FXString& s1,const FXchar* s2);
  static FXint comparenaturalcase(const FXString& s1,const FXString& s2);

  /**
  * Check if the string contains special characters or leading or trailing whitespace,
  * or contains UTF8 if flag!=0.
  */
  static FXbool shouldEscape(const FXchar* str,FXchar lquote='\0',FXchar rquote='\0',FXint flag=0);
  static FXbool shouldEscape(const FXchar* str,FXint num,FXchar lquote='\0',FXchar rquote='\0',FXint flag=0);
  static FXbool shouldEscape(const FXString& str,FXchar lquote='\0',FXchar rquote='\0',FXint flag=0);

  /**
  * Escape special characters.
  * Optionally enclose return value with left and right quotes.
  * Flag: 0 don't escape UTF8 multi-byte characters.
  * Flag: 1 escapes UTF8 multi-byte characters to a sequence of hexadecimals of the form \xXX.
  * Flag: 2 escapes UTF8 multi-byte characters to the form \uXXXX or \uXXXX\uYYYY.
  * Ill-formed UTF8 will be escaped as hexadecimals \xXX regardless even if \uXXXX is requested.
  * Quotes will be escaped if quoting is performed.
  */
  static FXString escape(const FXchar* str,FXchar lquote='\0',FXchar rquote='\0',FXint flag=0);
  static FXString escape(const FXchar* str,FXint num,FXchar lquote='\0',FXchar rquote='\0',FXint flag=0);
  static FXString escape(const FXString& str,FXchar lquote='\0',FXchar rquote='\0',FXint flag=0);

  /**
  * Unescape special characters,
  * Optionally remove surrounding left and right quotes.
  * In particular, unescape \xXX to the hexadecimal character XX, \uXXXX to the multi-byte
  * UTF8 sequence representing unicode character XXXX, and unescape \t, \n, etc.
  * Handle surrogate pairs of the form \uXXXX\uYYYY properly.
  */
  static FXString unescape(const FXchar* str,FXchar lquote='\0',FXchar rquote='\0');
  static FXString unescape(const FXchar* str,FXint num,FXchar lquote='\0',FXchar rquote='\0');
  static FXString unescape(const FXString& str,FXchar lquote='\0',FXchar rquote='\0');

  /**
  * Expand tabs with the equivalent amount of spaces.
  * UTF8 encoded characters are counted as one column.
  */
  static FXString detab(const FXchar* str,FXint tabcols);
  static FXString detab(const FXchar* str,FXint num,FXint tabcols);
  static FXString detab(const FXString& str,FXint tabcols);


  /**
  * Compress runs of more than 2 spaces with tabs.
  * UTF8 characters are counted as one column.
  */
  static FXString entab(const FXchar* str,FXint tabcols);
  static FXString entab(const FXchar* str,FXint num,FXint tabcols);
  static FXString entab(const FXString& str,FXint tabcols);

  /**
  * Retabbify lines of text.
  * Given lines of text at given indent-level, insert of remove spaces or tabs to
  * align the text to new outdent-level, shifting over by a number of columns left-ward
  * or rightward. UTF8 characters are counted as one column, and trailing space will be
  * deleted.
  */
  static FXString tabbify(const FXchar* str,FXint tabcols,FXint indent,FXint outdent,FXint shift,FXbool tabs);
  static FXString tabbify(const FXchar* str,FXint num,FXint tabcols,FXint indent,FXint outdent,FXint shift,FXbool tabs);
  static FXString tabbify(const FXString& str,FXint tabcols,FXint indent,FXint outdent,FXint shift,FXbool tabs);

  /**
  * Return normalized string, i.e. reordering of diacritical marks.
  */
  static FXString normalize(const FXString& s);

  /**
  * Return normalized decomposition of string.
  */
  static FXString decompose(const FXString& s,FXbool canonical=true);

  /**
  * Return normalized composition of string; this first performs normalized
  * decomposition.
  */
  static FXString compose(const FXString& s,FXbool canonical=true);

  /// Swap two strings
  friend inline FXString& swap(FXString& dst,FXString& src);

  /// Saving to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXString& s);

  /// Load from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXString& s);

  /// Delete
 ~FXString();
  };


/// Swap two strings
inline FXString& swap(FXString& dst,FXString& src){
  FXchar* t=dst.str; dst.str=src.str; src.str=t;
  return dst;
  }


/// Equality operators
static inline FXbool operator==(const FXString& s1,const FXString& s2){ return FXString::compare(s1.text(),s2.text())==0; }
static inline FXbool operator==(const FXString& s1,const FXchar* s2){ return FXString::compare(s1.text(),s2)==0; }
static inline FXbool operator==(const FXchar* s1,const FXString& s2){ return FXString::compare(s1,s2.text())==0; }

/// Inequality operators
static inline FXbool operator!=(const FXString& s1,const FXString& s2){ return FXString::compare(s1.text(),s2.text())!=0; }
static inline FXbool operator!=(const FXString& s1,const FXchar* s2){ return FXString::compare(s1.text(),s2)!=0; }
static inline FXbool operator!=(const FXchar* s1,const FXString& s2){ return FXString::compare(s1,s2.text())!=0; }

/// Lexicographic less than operators
static inline FXbool operator<(const FXString& s1,const FXString& s2){ return FXString::compare(s1.text(),s2.text())<0; }
static inline FXbool operator<(const FXString& s1,const FXchar* s2){ return FXString::compare(s1.text(),s2)<0; }
static inline FXbool operator<(const FXchar* s1,const FXString& s2){ return FXString::compare(s1,s2.text())<0; }

/// Lexicographic less than or equal operators
static inline FXbool operator<=(const FXString& s1,const FXString& s2){ return FXString::compare(s1.text(),s2.text())<=0; }
static inline FXbool operator<=(const FXString& s1,const FXchar* s2){ return FXString::compare(s1.text(),s2)<=0; }
static inline FXbool operator<=(const FXchar* s1,const FXString& s2){ return FXString::compare(s1,s2.text())<=0; }

/// Lexicographic greater than operators
static inline FXbool operator>(const FXString& s1,const FXString& s2){ return FXString::compare(s1.text(),s2.text())>0; }
static inline FXbool operator>(const FXString& s1,const FXchar* s2){ return FXString::compare(s1.text(),s2)>0; }
static inline FXbool operator>(const FXchar* s1,const FXString& s2){ return FXString::compare(s1,s2.text())>0; }

/// Lexicographic greater than or equal operators
static inline FXbool operator>=(const FXString& s1,const FXString& s2){ return FXString::compare(s1.text(),s2.text())>=0; }
static inline FXbool operator>=(const FXString& s1,const FXchar* s2){ return FXString::compare(s1.text(),s2)>=0; }
static inline FXbool operator>=(const FXchar* s1,const FXString& s2){ return FXString::compare(s1,s2.text())>=0; }

/// Concatenate FXString and FXString
extern FXAPI FXString operator+(const FXString& s1,const FXString& s2);

/// Concatenate FXString and a string
extern FXAPI FXString operator+(const FXString& s1,const FXchar* s2);
extern FXAPI FXString operator+(const FXString& s1,const FXnchar* s2);
extern FXAPI FXString operator+(const FXString& s1,const FXwchar* s2);

/// Concatenate string and FXString
extern FXAPI FXString operator+(const FXchar* s1,const FXString& s2);
extern FXAPI FXString operator+(const FXnchar* s1,const FXString& s2);
extern FXAPI FXString operator+(const FXwchar* s1,const FXString& s2);

/// Concatenate string and single character
extern FXAPI FXString operator+(const FXString& s,FXchar c);
extern FXAPI FXString operator+(FXchar c,const FXString& s);

/// Saving to a stream
extern FXAPI FXStream& operator<<(FXStream& store,const FXString& s);

/// Load from a stream
extern FXAPI FXStream& operator>>(FXStream& store,FXString& s);

/// Convert unix string to dos string
extern FXAPI FXString& unixToDos(FXString& str);

/// Convert dos string to unix string
extern FXAPI FXString& dosToUnix(FXString& str);

}

#endif
