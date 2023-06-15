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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxchar.h"
#include "fxmath.h"
#include "fxascii.h"
#include "fxunicode.h"
#include "FXElement.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXException.h"
#include "FXRex.h"


/*
  The Story:
  ==========

  As with most regex implementations, this one is inspired by Henry Spencer's
  original implementation.

  This is however an ab-initio implementation, with the following goals:

        o Full C++ implementation, no simple C++ wrapper.
        o Trade speed and memory in favor of speed, but keep it compact where possible.
        o Thread-safe:
            - No global variables used during parsing or execution.
            - Multiple threads could use same single FXRex at the same time when
              matching strings.
        o Perl-like syntax for character classes.
        o Additional features such as lazy/greedy/possessive closures, counting
          repeats, back references, trailing context.
        o Forward and backward subject string scanning mode.
        o Single line/multi line matching modes.
        o 8-bit safe (you can use it to grep binary data!).
        o When parsing fails, or when created with default constructor, FXRex is
          initialized to a "fallback" program; its thus safe to invoke match at any time.
        o The default fallback program will fail to match anything.
        o Convenient feature: disallow empty string matches; this is nice as it prevents
          a common problem, for example searching for "a*" in "bbba"; without the NotEmpty
          option, this matches "" and not the "a".
        o Another convenient feature is the ability to compile verbatim strings.
          This is practical as it allows FXRex to be populated with a simple string with no
          interpretation of special characters ^*?+{}()\$.[].

  Because this is completely new implementation of regular expressions, and not merely an
  extension of a previous implementation, some people may want to adapt it for use outside
  of FOX.  This is perfectly OK with me.

  However:

        o The Author is not responsible for the consequences of using this software.

        o Recipient should be clearly informed of the origin of the software; and if alterations
          have been made, this must be stated clearly.

        o Software will continue to fall under Lesser GPL, unless specific permission from the
          Author has been obtained.


  Implementation notes:
  =====================

  This implementation does not use "nodes" with "next" pointers; instead, the "next" opcode is
  located implicitly by simple sequentiality.  This has beneficial effect on speed, as one can
  simply add to the program counter instead of performing a memory reference.

  Sometimes one needs to jump out of sequence; this is implemented by an explicit jump instruction.
  Because it works with relative offsets, there is no need to distinguish between forward and
  backward jumps.

  Henry Spencer implemented a trick to "prefix" simple single-character matching opcodes with a
  closure operator, by shifting down already generated code and inserting the closure operator
  in front of it.

  FXRex uses the same trick of shifting down code; but this same trick is also useful for branches!

  FXRex only generates a branch node when an alternative has in fact been seen; if no alternative
  is there, we've saved ourselves both a branch node and a jump node!

  This has interesting side-effects:

        o The common case of 1 single branch now no longer needs a branch opcode and corresponding
          jump opcode at all!

        o When an alternative is found, we insert a branch node in front and a jump node behind
          the already generated code.  This can be done easily as branches and jumps within the
          shifted block of code are relative, and have already been resolved!

        o The matching algorithm for a branch opcode simplifies as well:- either it matches the
          first branch, or it continues after the jump.

        o Its easier to dig out some info from the program, and in fact so easy that this digging
          can be moved to the execute phase.

  When a repeat is prefixed in front of a simple single-character match, counted repeats are
  simplified: {1}, {1,1} is eliminated, {}, {0,} becomes *, {1,} becomes +, and {0,1} becomes ?.

  Because single-character repeats are prefixed with a repeat instruction, there is no recursion
  needed; single character repeats are therefore very fast.

  Complex repeats are implemented using branch loop constructs and may involve recursions (in fact
  only the fixed repeat is non-recursive!).  Hence complex repeats should be avoided when single-
  character repeats will suffice.

  OP_BRANCH and OP_BRANCHREV implement alternatives. For OP_BRANCH, first the inline code immediately
  following the offset is executed; if the inline code fails, OP_BRANCH takes a jump to the new
  location and tries the alternative.

  For OP_BRANCHREV, it works the opposite way: OP_BRANCHREV takes the alternative first, before
  trying the inline code.

  Having both OP_BRANCH and OP_BRANCHREV substantially simplifies the design of complex greedy or
  lazy matches:- the greedy and lazy match turn out to have the same code structure, except we're
  using OP_BRANCHREV for the lazy matches and OP_BRANCH for the greedy ones.

  OP_JUMP is an unconditional jump to a new location. OP_JUMPLT and OP_JUMPGT jump when the loop
  counter is less than or greater than the given value, respectively.


  Atomic Matching Groups
  ======================

  For example, trying to match pattern "\d+foo" against subject string "123456bar", the matcher will
  eat all digits up till "6", and then backtrack by trying digits up till 5, and so on.  An atomic
  subgroup match will simply fail if the sub-pattern fails to match at the end.  This can be written
  as: "(?>\d+)foo".
  Atomic groups are thus more efficient since no repeated tries are being made.


  Greedy, Lazy, and Possessive Matches
  ====================================

  Greedy: the "a*" in pattern "a*ardvark" matching subject string "aardvark" will match "aa", then
  backtrack and match "a", after which the match succeeds.

  Lazy: the "a*?" in pattern "a*?ardvark" will first match "", then try match "a" after which the
  match succeeds.

  Possessive: the "a*+" in pattern "a*+ardvark" will match "aa", then fail without backing off.

  Possessive matches and Atomic matching groups are closely related in terms of controlling the
  recursion depth of the matcher.

  Atomic subgroups: when a subgroup successfully matches, doesn't backtrack into the subgroup
  again for another try but fails.  For example,

        "(?>A+)[A-Z]C"

  will fail to match "AAC" because once the "(?>A+)" matches "AA" it will not backtrack, while
  a normal group "(A+)" would backtrack to match a single "A".  Likewise,

        "(?>A|.B)C"

  will not match "ABC" because after "(?>A|.B)" matches "A", it will not backtrack, while a
  normal group "(A|.B)" would backtrack and match "AB".


  Zero-Width Look-Aheads and Look-Behinds
  =======================================

  FXRex can do both zero-width positive and negative look-aheads and positive and negative
  look-behinds.  The newer version will remove the fixed-size limitation to the look-behinds.
  This is made possible by implementation of a full backward-scanning matcher, together with
  a new method to reverse the regular expression pattern.
  There are still a few limitations when the patterns are to be reversed; backreferences will
  not be allowed in backward scanning patterns.

  Small gotchas:

    - In backward scanning mode, a look-ahead changes direction and scans forward, while a
      look-behind mode scans backwards.

    - In forward scannng mode, a look-ahead scans forward, while a look-behind changes direction
      and scans backward.


  Grammar:
  ========

      exp        ::= branch ( "|" branch ) *

      branch     ::= ( piece ) +

      piece      ::= atom [ rep ]

      rep        ::= ( "*" | "+" | "?" | counts ) [ "?" | "+" ]

      counts     ::= "{" digits ["," digits ] "}"

      atom       ::= "(" exp ")" | "[" [^] range "]" | characters

      range      ::= ( character ( "-" character ) ? ) +

      characters ::= ( character ) *

      digits     ::= ( digit ) *


  Special Characters:
  ===================

  Characters '*', '+', '?', '{', '}', '^', '$', '.', '(', ')', '[', ']', '|', and '\' have special meanings,
  and must be escaped if the character itself is needed.  For example, to match zero or more '*', one
  would have to use the pattern '\**'.

  Syntax:
  =======

      Special Constructs
      ==================

      ( X )     Match sub-group (with capturing if enabled)
      (?i X )   Match sub-group case insensitive
      (?I X )   Match sub-group case sensitive
      (?n X )   Match sub-group with newlines
      (?N X )   Match sub-group with no newlines
      (?: X )   Non-capturing parentheses
      (?= X )   Zero width positive lookahead
      (?! X )   Zero width negative lookahead
      (?<= X )  Zero width positive lookbehind
      (?<! X )  Zero width negative lookbehind
      (?> X )   Atomic grouping (possessive match)

      Logical Operators
      =================

      X Y       X followed by Y
      X | Y     Either X or Y
      ( X )     Sub pattern (capturing if FXRex::Capture)

      Greedy Quantifiers
      ==================

      X *       Match 0 or more
      X +       Match 1 or more
      X ?       Match 0 or 1
      X {}      Match 0 or more
      X {n}     Match n times
      X {,m}    Match no more than m times
      X {n,}    Match n or more
      X {n,m}   Match at least n but no more than m times

      Lazy Quantifiers
      ================

      X *?      Match 0 or more
      X +?      Match 1 or more
      X ??      Match 0 or 1
      X {}?     Match 0 or more times
      X {n}?    Match n times
      X {,m}?   Match no more than m times
      X {n,}?   Match n or more
      X {n,m}?  Match at least n but no more than m times

      Possessive (non-backtracking) Quantifiers
      =========================================

      X *+      Match 0 or more
      X ++      Match 1 or more
      X ?+      Match 0 or 1
      X {}+     Match 0 or more times
      X {n}+    Match n times
      X {,m}+   Match no more than m times
      X {n,}+   Match n or more
      X {n,m}+  Match at least n but no more than m times

      Boundary Matching
      =================

      ^         Match begin of line [if at begin of pattern]
      $         Match end of line [if at end of pattern]
      \<        Begin of word
      \>        End of word
      \b        Word boundary
      \B        Word interior
      \A        Match only beginning of string
      \Z        Match only and end of string

      Character Classes
      =================

      [abc]     Match a, b, or c
      [^abc]    Match any but a, b, or c
      [a-zA-Z]  Match upper- or lower-case a through z
      []]       Matches ]
      [-]       Matches -

      Predefined character classes
      ============================

      .         Match any character
      \d        Digit [0-9]
      \D        Non-digit [^0-9]
      \s        Space [ \t\n\r\f\v]
      \S        Non-space [^ \t\n\r\f\v]
      \w        Word character [a-zA-Z_0-9]
      \W        Non-word character [^a-zA-Z0-9_]
      \l        Letter [a-zA-Z]
      \L        Non-letter [^a-zA-Z]
      \h        Hex digit [0-9a-fA-F]
      \H        Non-hex digit [^0-9a-fA-F]
      \u        Single uppercase character
      \U        Single lowercase character
      \p        Punctuation (not including '_')
      \P        Non punctuation

      Back References
      ===============

      \1        Reference to 1st capturing group
      \2        Reference to 2nd capturing group
      ...
      \9        Reference to 9th capturing group

      Characters
      ==========

      x           Any character
      \\          Back slash character
      \xHH        Hexadecimal number (exactly two hex-digits H)
      \cX         Control character, where X is '@', 'A'...'Z', '[', '\', ']', '^', '_', or '?' for DEL.
      \033        Octal
      \a          Alarm, bell
      \b          Only in character classes: backspace; otherwise: word-boundary
      \e          Escape character
      \t          Tab
      \f          Form feed
      \n          Newline
      \r          Return
      \v          Vertical tab
      \X          Just X unless X is one of [dDsSwWlLhHuUpP\0123456789xcaetfnrv]
      \u1FD1      Unicode U+1FD1 (GREEK SMALL LETTER IOTA WITH MACRON) (UNICODE mode)
      \U00010450  Wide unicode U+10450 (SHAVIAN LETTER PEEP) (UNICODE mode)

      Unicode general character categories (UNICODE mode)
      ===================================================

      \p{C}     Other (Cn | Cc | Cf | Cs | Co)
      \p{Cc}    Control
      \p{Cf}    Format
      \p{Cn}    Unassigned
      \p{Co}    Private use
      \p{Cs}    Surrogate

      \p{M}     Mark (Mn | Mc | Me)
      \p{Mn}    Non-spacing mark
      \p{Mc}    Spacing mark
      \p{Me}    Enclosing mark

      \p{Z}     Separator (Zs | Zl | Zp)
      \p{Zs}    Space separator
      \p{Zl}    Line separator
      \p{Zp}    Paragraph separator

      \p{L}     Letter (Lu | Ll | Lt | Lm | Lo)
      \p{Lu}    Upper case letter
      \p{Ll}    Lower case letter
      \p{Lt}    Title case letter
      \p{Lm}    Modifier letter
      \p{Lo}    Other letter

      \p{N}     Number (Nl | Nd | No)
      \p{Nl}    Letter number
      \p{Nd}    Decimal number
      \p{No}    Other number

      \p{P}     Punctuation (Pc | Pd | Ps | Pe | Pi | Pf | Po)
      \p{Pc}    Connector punctuation
      \p{Pd}    Dash punctuation
      \p{Ps}    Open punctuation
      \p{Pe}    Close punctuation
      \p{Pf}    Final punctuation
      \p{Pi}    Initial punctuation
      \p{Po}    Other punctuation

      \p{S}     Symbol (Sm | Sc | Sk | So)
      \p{Sm}    Mathematical symbol
      \p{Sc}    Currency symbol
      \p{Sk}    Modifier symbol
      \p{So}    Other symbol


      Unicode script categories (UNICODE mode)
      ========================================

      \p{Arab}  Arabic
      \p{Armn}  Armenian
      \p{Beng}  Bengali
      \p{Bopo}  Bopomofo
      \p{Brai}  Braille
      \p{Bugi}  Buginese
      \p{Buhd}  Buhid
      \p{Cans}  Canadian_Aboriginal
      \p{Cher}  Cherokee
      \p{Copt}  Coptic (Qaac)
      \p{Cprt}  Cypriot
      \p{Cyrl}  Cyrillic
      \p{Deva}  Devanagari
      \p{Dsrt}  Deseret
      \p{Ethi}  Ethiopic
      \p{Geor}  Georgian
      \p{Glag}  Glagolitic
      \p{Goth}  Gothic
      \p{Grek}  Greek
      \p{Gujr}  Gujarati
      \p{Guru}  Gurmukhi
      \p{Hang}  Hangul
      \p{Hani}  Han
      \p{Hano}  Hanunoo
      \p{Hebr}  Hebrew
      \p{Hira}  Hiragana
      \p{Hrkt}  Katakana_Or_Hiragana
      \p{Ital}  Old_Italic
      \p{Kana}  Katakana
      \p{Khar}  Kharoshthi
      \p{Khmr}  Khmer
      \p{Knda}  Kannada
      \p{Laoo}  Lao
      \p{Latn}  Latin
      \p{Limb}  Limbu
      \p{Linb}  Linear_B
      \p{Mlym}  Malayalam
      \p{Mong}  Mongolian
      \p{Mymr}  Myanmar
      \p{Ogam}  Ogham
      \p{Orya}  Oriya
      \p{Osma}  Osmanya
      \p{Qaai}  Inherited
      \p{Runr}  Runic
      \p{Shaw}  Shavian
      \p{Sinh}  Sinhala
      \p{Sylo}  Syloti_Nagri
      \p{Syrc}  Syriac
      \p{Tagb}  Tagbanwa
      \p{Tale}  Tai_Le
      \p{Talu}  New_Tai_Lue
      \p{Taml}  Tamil
      \p{Telu}  Telugu
      \p{Tfng}  Tifinagh
      \p{Tglg}  Tagalog
      \p{Thaa}  Thaana
      \p{Thai}  Thai
      \p{Tibt}  Tibetan
      \p{Ugar}  Ugaritic
      \p{Xpeo}  Old_Persian
      \p{Yiii}  Yi
      \p{Zyyy}  Common

  To do:
  ======

  - Look into optimizing character class when possible (e.g. collapse [0-9] to OP_DIGIT
    and [^A] into OP_NOT_CHAR).
  - Repeating back references, only possible if capturing parentheses are known NOT to match "".
  - Note the \uXXXX and \UXXXXXXXX is going to be used for UNICODE:
    See: http://www.unicode.org/unicode/reports/tr18.
  - Old character class structure (OP_IN, OP_NOT_IN):

           byte0 ... byte31

    A total of 32 bytes, to store 256 individual bits.  Not the best use
    of space.

  - The new character class structures; method #1:

                  <N>
           ( <lo_1> <hi_1> )
           ( <lo_2> <hi_2> )
              ...    ...
           ( <lo_N> <hi_N> )

    And/or heterogeneous set; method #2 (OP_ANY_OF/OP_ANY_BUT):

                  <N>
           c1, c2, c3, ... cN

    Choose which based on the input; for example:

        [a-z]        Method #1
        [a82d]       Method #2

    Either method has an <N> count number, this is needed so we know how much space
    to skip in the program. For unicode, [XYZ] may be better implemented as combination
    of (X|Y|Z), to avoid some complexities.
  - Careful about reversing unicode, don't reverse bytes but characters.
  - Possibly implement parser base class, with actions in derived class; easier to
    keep up additions to syntax this way and keep reversal code in sync with regular
    parsing code.
  - The new return code ErrMore means not all of the input pattern was consumed.
    It could treated as ErrOK.  However, if not ALL of the input pattern IS parsed,
    there should be a way to find out where the parse stopped.
  - When generating code for "reverse mode", backward references don't work, but
    forward references do. Can we fix this, or are we OK with "forward references"
    in reverse mode?
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_DETAIL    1013
//#define TOPIC_REXDUMP   1014


// As close to infinity as we're going to get; this seems big enough.  We can not make
// it too large as this may wrap around when added to something else!
#define ONEINDIG      16384

// Number of capturing sub-expressions allowed
#define NSUBEXP       10

// Recursion limit
#define MAXRECURSION  10000


// Maximum number of pieces reversed
#define MAXPIECES     256


// Access to opcode
#define SETOP(p,op)   (*(p)=(op))

// Access to argument
#if defined(__i386__) || defined(__x86_64__)            // No alignment limits on shorts
#define SETARG(p,val) (*((FXshort*)(p))=(val))
#define GETARG(p)     (*((const FXshort*)(p)))
#elif (FOX_BIGENDIAN == 1)                              // Big-endian machines
#define SETARG(p,val) ((p)[0]=(val)>>8,(p)[1]=(val))
#define GETARG(p)     ((((FXshort)(FXuchar)(p)[0])<<8)|(FXuchar)(p)[1])
#else                                                   // Little-endian machines
#define SETARG(p,val) ((p)[0]=(val),(p)[1]=(val)>>8)
#define GETARG(p)     ((FXuchar)(p)[0]|(((FXshort)(FXuchar)(p)[1])<<8))
#endif

// Set manipulations
#define EXCL(set,ch)   (set[ch>>3]&=~(1<<((ch)&7)))
#define INCL(set,ch)   (set[ch>>3]|=(1<<((ch)&7)))
#define ISIN(set,ch)   (set[(ch)>>3]&(1<<((ch)&7)))
#define UNION(set,src) (set[0]|=src[0],set[1]|=src[1],set[2]|=src[2],set[3]|=src[3],set[4]|=src[4],set[5]|=src[5],set[6]|=src[6],set[7]|=src[7],set[8]|=src[8],set[9]|=src[9],set[10]|=src[10],set[11]|=src[11],set[12]|=src[12],set[13]|=src[13],set[14]|=src[14],set[15]|=src[15],set[16]|=src[16],set[17]|=src[17],set[18]|=src[18],set[19]|=src[19],set[20]|=src[20],set[21]|=src[21],set[22]|=src[22],set[23]|=src[23],set[24]|=src[24],set[25]|=src[25],set[26]|=src[26],set[27]|=src[27],set[28]|=src[28],set[29]|=src[29],set[30]|=src[30],set[31]|=src[31])
#define ZERO(set)      (set[0]=0,set[1]=0,set[2]=0,set[3]=0,set[4]=0,set[5]=0,set[6]=0,set[7]=0,set[8]=0,set[9]=0,set[10]=0,set[11]=0,set[12]=0,set[13]=0,set[14]=0,set[15]=0,set[16]=0,set[17]=0,set[18]=0,set[19]=0,set[20]=0,set[21]=0,set[22]=0,set[23]=0,set[24]=0,set[25]=0,set[26]=0,set[27]=0,set[28]=0,set[29]=0,set[30]=0,set[31]=0)

using namespace FX;

/*******************************************************************************/

namespace FX {


namespace {


// Subpattern complexity
enum {
  FLG_WORST  = 0,           // Worst case
  FLG_WIDTH  = 1,           // Matches >=1 character
  FLG_SIMPLE = 2            // Simple
  };


// Greediness flags
enum {
  GREEDY    = 0,            // Greedily match subject string
  LAZY      = 1,            // Lazily match subject string
  GRABBY    = 2             // Once matched, don't backtrack
  };


// Opcodes of the engine; these are numbered in a certain way so that
// its easy to determine if these are asserts, simple single character matches,
// unicode matches, and so on.
enum {
  OP_FAIL=0,            // Always fail
  OP_PASS,              // Always succeed
  OP_JUMP,              // Jump to another location
  OP_BRANCH,            // Jump after trying following code, recursive
  OP_BRANCHREV,         // Jump before trying following code, recursive
  OP_ATOMIC,            // Match atomic subgroup
  OP_IF,                // Optionally match subgroup, with no backtrack
  OP_WHILE,             // Zero or more times, match wubgroup, with no backtrack
  OP_UNTIL,             // One or more times, match wubgroup, with no backtrack
  OP_FOR,               // Repeat minimum and maximum times, no backtrack

  // Assertions
  OP_NOT_EMPTY,         // Match not empty
  OP_STR_BEG,           // Beginning of string
  OP_STR_END,           // End of string
  OP_LINE_BEG,          // Beginning of line
  OP_LINE_END,          // End of line
  OP_WORD_BEG,          // Beginning of word
  OP_WORD_END,          // End of word
  OP_WORD_BND,          // Word boundary
  OP_WORD_INT,          // Word interior
  OP_UWORD_BEG,         // Unicode beginning of word
  OP_UWORD_END,         // Unicode end of word
  OP_UWORD_BND,         // Unicode word boundary
  OP_UWORD_INT,         // Unicode word interior

  // Literal runs
  OP_CHARS,             // Match literal string
  OP_CHARS_CI,          // Match literal string, case insensitive
  OP_UCHARS,            // Unicode literal string
  OP_UCHARS_CI,         // Unicode literal string, case insensitive

  // Single character matching; these can be in simple repeat
  OP_ANY,               // Any character but no newline
  OP_ANY_NL,            // Any character including newline
  OP_IN,                // Any character in set
  OP_NOT_IN,            // Any character not in set
  OP_ANY_OF,            // Any character in list
  OP_ANY_BUT,           // Any character not in list
  OP_UPPER,             // Match upper case
  OP_LOWER,             // Match lower case
  OP_SPACE,             // White space
  OP_SPACE_NL,          // White space including newline
  OP_NOT_SPACE,         // Non-white space
  OP_DIGIT,             // Digit
  OP_NOT_DIGIT,         // Non-digit
  OP_NOT_DIGIT_NL,      // Non-digit including newline
  OP_HEX,               // Hex digit
  OP_NOT_HEX,           // Non hex digit
  OP_NOT_HEX_NL,        // Non hex digit including newline
  OP_LETTER,            // Letter
  OP_NOT_LETTER,        // Non-letter
  OP_NOT_LETTER_NL,     // Non-letter including newline
  OP_PUNCT,             // Punctuation
  OP_NOT_PUNCT,         // Non punctuation
  OP_NOT_PUNCT_NL,      // Non punctuation including newline
  OP_WORD,              // Word character
  OP_NOT_WORD,          // Non-word character
  OP_NOT_WORD_NL,       // Non-word character including newline
  OP_CHAR,              // Single character
  OP_CHAR_CI,           // Single character, case insensitive

  // Single unicode character matching
  OP_UANY,              // Unicode any character, except newline
  OP_UANY_NL,           // Unicode any  character, including newline
  OP_UIN,               // Unicode any character in set
  OP_UNOT_IN,           // Unicode any character not in set
  OP_UANY_OF,           // Unicode any character in list
  OP_UANY_BUT,          // Unicode any character not in list
  OP_UUPPER,            // Unicode uppercase
  OP_ULOWER,            // Unicode lowercase
  OP_UTITLE,            // Unicode title case
  OP_USPACE,            // Unicode space, except newline
  OP_USPACE_NL,         // Unicode space, including newline
  OP_UNOT_SPACE,        // Unicode non-space
  OP_UDIGIT,            // Unicode digit
  OP_UNOT_DIGIT,        // Unicode non-digit, except newline
  OP_UNOT_DIGIT_NL,     // Unicode non-digit, including newline
  OP_ULETTER,           // Unicode letter
  OP_UNOT_LETTER,       // Unicode non-letter, except newline
  OP_UNOT_LETTER_NL,    // Unicode non-letter, including newline
  OP_UPUNCT,            // Unicode punctuation
  OP_UNOT_PUNCT,        // Unicode non-punctuation, except newline
  OP_UNOT_PUNCT_NL,     // Unicode non-punctuation, including newline
  OP_UCAT,              // Unicode character from categories
  OP_UNOT_CAT,          // Unicode character NOT from categories, except newline
  OP_UNOT_CAT_NL,       // Unicode character NOT from categories, including newline
  OP_USCRIPT,           // Unicode character from script
  OP_UNOT_SCRIPT,       // Unicode character NOT from script, except newline
  OP_UNOT_SCRIPT_NL,    // Unicode character NOT from script, including newline
  OP_UCHAR,             // Unicode single character
  OP_UCHAR_CI,          // Unicode single character, case-insensitive

  // Simple repeats
  OP_STAR,              // Greedy * (simple)
  OP_MIN_STAR,          // Lazy * (simple)
  OP_POS_STAR,          // Possessive * (simple)
  OP_PLUS,              // Greedy + (simple)
  OP_MIN_PLUS,          // Lazy + (simple)
  OP_POS_PLUS,          // Possessive + (simple)
  OP_QUEST,             // Greedy ? (simple)
  OP_MIN_QUEST,         // Lazy ? (simple)
  OP_POS_QUEST,         // Possessive ? (simple)
  OP_REP,               // Greedy counted repeat (simple)
  OP_MIN_REP,           // Lazy counted repeat (simple)
  OP_POS_REP,           // Possessive counted repeat (simple)

  // Positive or negative lookahead/lookbehind
  OP_AHEAD_NEG,         // Negative look-ahead
  OP_AHEAD_POS,         // Positive look-ahead
  OP_BEHIND_NEG,        // Negative look-behind
  OP_BEHIND_POS,        // Positive look-behind

  // Capturing groups
  OP_SUB_BEG_0,         // Start of substring 0, 1, ...
  OP_SUB_BEG_1,
  OP_SUB_BEG_2,
  OP_SUB_BEG_3,
  OP_SUB_BEG_4,
  OP_SUB_BEG_5,
  OP_SUB_BEG_6,
  OP_SUB_BEG_7,
  OP_SUB_BEG_8,
  OP_SUB_BEG_9,
  OP_SUB_END_0,         // End of substring 0, 1, ...
  OP_SUB_END_1,
  OP_SUB_END_2,
  OP_SUB_END_3,
  OP_SUB_END_4,
  OP_SUB_END_5,
  OP_SUB_END_6,
  OP_SUB_END_7,
  OP_SUB_END_8,
  OP_SUB_END_9,

  // Backreferences
  OP_REF_0,             // Back reference to substring 0, 1, ...
  OP_REF_1,
  OP_REF_2,
  OP_REF_3,
  OP_REF_4,
  OP_REF_5,
  OP_REF_6,
  OP_REF_7,
  OP_REF_8,
  OP_REF_9,
  OP_REF_CI_0,          // Case insensitive back reference to substring 0, 1, ...
  OP_REF_CI_1,
  OP_REF_CI_2,
  OP_REF_CI_3,
  OP_REF_CI_4,
  OP_REF_CI_5,
  OP_REF_CI_6,
  OP_REF_CI_7,
  OP_REF_CI_8,
  OP_REF_CI_9,

  // Couning constructs
  OP_ZERO_0,            // Zero counter 0, 1, ...
  OP_ZERO_1,
  OP_ZERO_2,
  OP_ZERO_3,
  OP_ZERO_4,
  OP_ZERO_5,
  OP_ZERO_6,
  OP_ZERO_7,
  OP_ZERO_8,
  OP_ZERO_9,
  OP_INCR_0,            // Increment counter 0, 1, ...
  OP_INCR_1,
  OP_INCR_2,
  OP_INCR_3,
  OP_INCR_4,
  OP_INCR_5,
  OP_INCR_6,
  OP_INCR_7,
  OP_INCR_8,
  OP_INCR_9,
  OP_JUMPLT_0,          // Jump if counter 0, 1, ... less than value
  OP_JUMPLT_1,
  OP_JUMPLT_2,
  OP_JUMPLT_3,
  OP_JUMPLT_4,
  OP_JUMPLT_5,
  OP_JUMPLT_6,
  OP_JUMPLT_7,
  OP_JUMPLT_8,
  OP_JUMPLT_9,
  OP_JUMPGT_0,          // Jump if counter 0, 1, ... greater than value
  OP_JUMPGT_1,
  OP_JUMPGT_2,
  OP_JUMPGT_3,
  OP_JUMPGT_4,
  OP_JUMPGT_5,
  OP_JUMPGT_6,
  OP_JUMPGT_7,
  OP_JUMPGT_8,
  OP_JUMPGT_9,

  // Final code
  OP_LAST
  };


/*******************************************************************************/


// Unicode character categories
static const FXchar unicat[][3]={
  {"Cn"},
  {"Cc"},
  {"Cf"},
  {"Cs"},
  {"Co"},
  {"Mn"},
  {"Mc"},
  {"Me"},
  {"Zs"},
  {"Zl"},
  {"Zp"},
  {"Lu"},
  {"Ll"},
  {"Lt"},
  {"Lm"},
  {"Lo"},
  {"Nl"},
  {"Nd"},
  {"No"},
  {"Pc"},
  {"Pd"},
  {"Ps"},
  {"Pe"},
  {"Pi"},
  {"Pf"},
  {"Po"},
  {"Sm"},
  {"Sc"},
  {"Sk"},
  {"So"},
  };


// Unicode character scripts
static const FXchar uniscript[][5]={
  {"Zzzz"},
  {"Zyyy"},
  {"Zinh"},
  {"Latn"},
  {"Grek"},
  {"Cyrl"},
  {"Armn"},
  {"Hebr"},
  {"Arab"},
  {"Syrc"},
  {"Thaa"},
  {"Deva"},
  {"Beng"},
  {"Guru"},
  {"Gujr"},
  {"Orya"},
  {"Taml"},
  {"Telu"},
  {"Knda"},
  {"Mlym"},
  {"Sinh"},
  {"Thai"},
  {"Laoo"},
  {"Tibt"},
  {"Mymr"},
  {"Geor"},
  {"Hang"},
  {"Ethi"},
  {"Cher"},
  {"Cans"},
  {"Ogam"},
  {"Runr"},
  {"Khmr"},
  {"Mong"},
  {"Hira"},
  {"Kana"},
  {"Bopo"},
  {"Hani"},
  {"Yiii"},
  {"Ital"},
  {"Goth"},
  {"Dsrt"},
  {"Tglg"},
  {"Hano"},
  {"Buhd"},
  {"Tagb"},
  {"Limb"},
  {"Tale"},
  {"Linb"},
  {"Ugar"},
  {"Shaw"},
  {"Osma"},
  {"Cprt"},
  {"Brai"},
  {"Bugi"},
  {"Copt"},
  {"Talu"},
  {"Glag"},
  {"Tfng"},
  {"Sylo"},
  {"Xpeo"},
  {"Khar"},
  {"Bali"},
  {"Xsux"},
  {"Phnx"},
  {"Phag"},
  {"Nkoo"},
  {"Sund"},
  {"Lepc"},
  {"Olck"},
  {"Vaii"},
  {"Saur"},
  {"Kali"},
  {"Rjng"},
  {"Lyci"},
  {"Cari"},
  {"Lydi"},
  {"Cham"},
  {"Lana"},
  {"Tavt"},
  {"Avst"},
  {"Egyp"},
  {"Samr"},
  {"Lisu"},
  {"Bamu"},
  {"Java"},
  {"Mtei"},
  {"Armi"},
  {"Sarb"},
  {"Prti"},
  {"Phli"},
  {"Orkh"},
  {"Kthi"},
  {"Batk"},
  {"Brah"},
  {"Mand"},
  };


// Characters that are not special inside atom()
static const FXuchar safeset[32]={
  0xfe,0xff,0xff,0xff,0xef,0xb0,0xff,0x7f,0xff,0xff,0xff,0x87,0xff,0xff,0xff,0xc7,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  };


// Set of word characters
static const FXuchar wordset[32]={
  0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x03,0xfe,0xff,0xff,0x87,0xfe,0xff,0xff,0x07,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

// Set of non-word characters
static const FXuchar nonwordset[32]={
  0xff,0xff,0xff,0xff,0xff,0xff,0x00,0xfc,0x01,0x00,0x00,0x78,0x01,0x00,0x00,0xf8,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

// Set of space characters
static const FXuchar spaceset[32]={
  0x00,0x3e,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

// Set of non-space characters
static const FXuchar nonspaceset[32]={
  0xff,0xc1,0xff,0xff,0xfe,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

// Set of digit characters
static const FXuchar digitset[32]={
  0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

// Set of non-digit characters
static const FXuchar nondigitset[32]={
  0xff,0xff,0xff,0xff,0xff,0xff,0x00,0xfc,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

// Set of hexdigit characters
static const FXuchar hexdigitset[32]={
  0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x03,0x7e,0x00,0x00,0x00,0x7e,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

// Set of non-hexdigit characters
static const FXuchar nonhexdigitset[32]={
  0xff,0xff,0xff,0xff,0xff,0xff,0x00,0xfc,0x81,0xff,0xff,0xff,0x81,0xff,0xff,0xff,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

// Set of letter characters
static const FXuchar letterset[32]={
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe,0xff,0xff,0x07,0xfe,0xff,0xff,0x07,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

// Set of non-letter characters
static const FXuchar nonletterset[32]={
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x01,0x00,0x00,0xf8,0x01,0x00,0x00,0xf8,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

// Set of uppercase characters
static const FXuchar upperset[32]={
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe,0xff,0xff,0x07,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

// Set of lowercase characters
static const FXuchar lowerset[32]={
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe,0xff,0xff,0x07,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

// Set of punctuation (not including _) characters
static const FXuchar delimset[32]={
  0x00,0x00,0x00,0x00,0xfe,0xff,0x00,0xfc,0x01,0x00,0x00,0x78,0x01,0x00,0x00,0x78,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

// Set of non-punctuation characters
static const FXuchar nondelimset[32]={
  0xff,0xff,0xff,0xff,0x01,0x00,0xff,0x03,0xfe,0xff,0xff,0x87,0xfe,0xff,0xff,0x87,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

/*******************************************************************************/

// Character in list at prg
static inline FXbool INLIST(const FXchar* p,FXuchar ch){
  const FXchar* q=p+(FXuchar)*p;
  while(p<q && *++p!=ch){ }
  return (*p==ch);
  }

#if 0
// Check if wide-character at wcs is in enumerated wide-character set
static FXbool inwcset(const FXchar* set,const FXchar* wcs){
  const FXuchar* s=(const FXuchar*)set;
  const FXuchar* w=(const FXuchar*)wcs;
  while(s[0]!='\0'){
    if((s[0]==w[0]) && ((w[0]<=0x7F) || ((s[1]==w[1]) && ((w[0]<=0xDF) || ((s[2]==w[2]) && ((w[0]<=0xEF) || (s[3]==w[3]))))))){
      return true;
      }
    s++;
    }
  return false;
  }
#endif

/*******************************************************************************/


// Structure used during compiling
class FXCompile {
  FXchar        *code;              // Program code
  FXchar        *pc;                // Program counter
  const FXchar  *pat;               // Pattern string pointer
  FXint          mode;              // Compile mode
  FXint          nbra;              // Number of counting braces
  FXint          npar;              // Number of capturing parentheses
public:

  // Construct compile engine
  FXCompile();

  // Parse pattern
  FXRex::Error compile(FXchar *prog,const FXchar* p,FXint m);

  // Return size of generated code
  FXival size() const { return pc-code; }

  // Parsing
  FXRex::Error verbatim();
  FXRex::Error expression(FXshort& flags,FXshort& smin,FXshort& smax);
  FXRex::Error branch(FXshort& flags,FXshort& smin,FXshort& smax);
  FXRex::Error piece(FXshort& flags,FXshort& smin,FXshort& smax);
  FXRex::Error atom(FXshort& flags,FXshort& smin,FXshort& smax);
  FXRex::Error charset();

  // Code generation
  FXchar* append(FXuchar op);
  FXchar* append(FXuchar op,FXshort arg);
  FXchar* append(FXuchar op,FXshort arg1,FXshort arg2);
  FXchar* appendset(FXuchar op,const FXuchar set[]);

  // Insert node at ptr
  FXchar* insert(FXchar *ptr,FXuchar op);
  FXchar* insert(FXchar *ptr,FXuchar op,FXshort arg);
  FXchar* insert(FXchar *ptr,FXuchar op,FXshort arg1,FXshort arg2);
  FXchar* insert(FXchar *ptr,FXuchar op,FXshort arg1,FXshort arg2,FXshort arg3);

  // Patch branches
  void patch(FXchar *fm,FXchar *to);
  };

/*******************************************************************************/

// Construct compile engine
FXCompile::FXCompile():code(nullptr),pc(nullptr),pat(nullptr),mode(0),nbra(0),npar(0){
  }


// Compiler main
FXRex::Error FXCompile::compile(FXchar *prog,const FXchar* p,FXint m){
  FXRex::Error err;
  FXshort  flags=0;
  FXshort  smin=0;
  FXshort  smax=0;

  FXASSERT_STATIC(OP_LAST<=256);

  // Initialize parser data
  code=pc=prog;
  pat=p;
  mode=m;
  nbra=0;
  npar=0;

  // Unicode support
  if(mode&FXRex::Unicode) return FXRex::ErrSupport;

  // Empty pattern
  if(*pat=='\0') return FXRex::ErrEmpty;

  // Assert start of string
  if(mode&FXRex::Exact){ append(OP_STR_BEG); }

  // Assert begin of word
  if(mode&FXRex::Words){ append(OP_WORD_BEG); }

  // Verbatim mode
  if(mode&FXRex::Verbatim){
    if((err=verbatim())!=FXRex::ErrOK) return err;
    }

  // Regular expression mode
  else{
    if((err=expression(flags,smin,smax))!=FXRex::ErrOK) return err;
    }

  // Assert end of word
  if(mode&FXRex::Words){ append(OP_WORD_END); }

  // Assert end of string
  if(mode&FXRex::Exact){ append(OP_STR_END); }

  // Assert non-empty match
  if(mode&FXRex::NotEmpty) append(OP_NOT_EMPTY);

  // Success if we got this far
  append(OP_PASS);

  // Code is too long
  if(code+32767<pc) return FXRex::ErrLong;

  // Not at the end of the pattern
  if(*pat!='\0') return FXRex::ErrMore;

  return FXRex::ErrOK;
  }


// Parse without interpretation of magic characters
// For "word search" feature:
//  1) First character of search string is word-character, prepend OP_WORD_BEG
//  2) Last character of search string is word-character, append OP_WORD_END
FXRex::Error FXCompile::verbatim(){
  FXint len=strlen(pat);
  if(mode&FXRex::IgnoreCase){
    append(OP_CHARS_CI,len);
    while(*pat){
      append(Ascii::toLower(*pat++));
      }
    }
  else{
    append(OP_CHARS,len);
    while(*pat){
      append(*pat++);
      }
    }
  return FXRex::ErrOK;
  }


// Parse expression
FXRex::Error FXCompile::expression(FXshort& flags,FXshort& smin,FXshort& smax){
  FXRex::Error err;
  FXchar  *jp=nullptr;
  FXchar  *at=pc;
  FXshort  flg=0;
  FXshort  smn=0;
  FXshort  smx=0;

  // Parse branch
  if((err=branch(flags,smin,smax))!=FXRex::ErrOK) return err;
  while(*pat=='|'){
    pat++;

    // Insert branch in front
    insert(at,OP_BRANCH,pc-at+5);

    // Append jump behind
    append(OP_JUMP,jp?jp-pc-1:0);

    // Set ready for the next one
    jp=pc-2;
    at=pc;

    // Parse branch
    if((err=branch(flg,smn,smx))!=FXRex::ErrOK) return err;

    // Update flags for expression thus far
    if(!(flg&FLG_WIDTH)) flags&=~FLG_WIDTH;

    // Update size range
    if(smx>smax) smax=smx;
    if(smn<smin) smin=smn;
    }

  // Patch the jumps
  patch(jp,pc);
  return FXRex::ErrOK;
  }


// Parse branch; a branch must be at least one piece
FXRex::Error FXCompile::branch(FXshort& flags,FXshort& smin,FXshort& smax){
  FXRex::Error err;
  FXshort flg=0;
  FXshort smn=0;
  FXshort smx=0;
  flags=FLG_WORST;
  smin=0;
  smax=0;
  do{

    // Parse piece
    if((err=piece(flg,smn,smx))!=FXRex::ErrOK) return err;

    // Update flags for branch based on pieces seen thus far
    if(flg&FLG_WIDTH) flags|=FLG_WIDTH;

    // Update size range
    smax=FXMIN(smax+smx,ONEINDIG);
    smin=smin+smn;
    }
  while(*pat!='\0' && *pat!='|' && *pat!=')');
  return FXRex::ErrOK;
  }


// Parse piece
FXRex::Error FXCompile::piece(FXshort& flags,FXshort& smin,FXshort& smax){
  FXRex::Error err;
  FXuchar ch;

  // Remember point before atom
  FXchar* ptr=pc;

  // Process atom
  if((err=atom(flags,smin,smax))!=FXRex::ErrOK) return err;

  // Check if atom is followed by repetition
  if((ch=*pat)=='*' || ch=='+' || ch=='?' || ch=='{'){
    FXshort greediness=GREEDY;
    FXshort rep_min=1;
    FXshort rep_max=1;

    // We can not repeat an empty match
    if(!(flags&FLG_WIDTH)) return FXRex::ErrNoAtom;

    pat++;

    // Handle repetition type
    switch(ch){
      case '*':                                         // Repeat E [0..INF>
        rep_min=0;
        rep_max=ONEINDIG;
        smin=0;
        smax=ONEINDIG;
        flags&=~FLG_WIDTH;                              // No width!
        break;
      case '+':                                         // Repeat E [1..INF>
        rep_min=1;
        rep_max=ONEINDIG;
        smax=ONEINDIG;
        break;
      case '?':                                         // Repeat E [0..1]
        rep_min=0;
        rep_max=1;
        smin=0;
        flags&=~FLG_WIDTH;                              // No width!
        break;
      case '{':                                         // Repeat E [N..M]
        rep_min=0;
        rep_max=ONEINDIG;
        if(*pat!='}'){
          while(Ascii::isDigit(*pat)){
            rep_min=10*rep_min+(*pat++ - '0');
            }
          rep_max=rep_min;
          if(*pat==','){
            pat++;
            rep_max=ONEINDIG;
            if(*pat!='}'){
              rep_max=0;
              while(Ascii::isDigit(*pat)){
                rep_max=10*rep_max+(*pat++ - '0');
                }
              }
            }
          if(rep_max<=0) return FXRex::ErrCount;        // Bad count
          if(rep_max>ONEINDIG) return FXRex::ErrCount;  // Bad count
          if(rep_min>rep_max) return FXRex::ErrRange;   // Illegal range
          }
        if(*pat!='}') return FXRex::ErrBrace;           // Unmatched brace
        pat++;
        smin=rep_min*smin;
        smax=FXMIN(rep_max*smax,ONEINDIG);
        if(rep_min==0) flags&=~FLG_WIDTH;               // No width!
        break;
      }

    // Handle greedy, lazy, or possessive forms
    if(*pat=='?'){
      greediness=LAZY;
      pat++;
      }
    else if(*pat=='+'){
      greediness=GRABBY;
      pat++;
      }

    // Non-trivial repetition?
    if(rep_min!=1 || rep_max!=1){

      // For simple repeats we prefix the last operation
      if(flags&FLG_SIMPLE){
        if(rep_min==0 && rep_max==ONEINDIG){            // *
          insert(ptr,OP_STAR+greediness);
          }
        else if(rep_min==1 && rep_max==ONEINDIG){       // +
          insert(ptr,OP_PLUS+greediness);
          }
        else if(rep_min==0 && rep_max==1){              // ?
          insert(ptr,OP_QUEST+greediness);
          }
        else{                                           // {M,N}
          insert(ptr,OP_REP+greediness,rep_min,rep_max);
          }
        }

      // For complex repeats we build loop constructs
      else{
        if(greediness==GRABBY){
          if(rep_min==0 && rep_max==ONEINDIG){          // (...)*+
            /*
            **
            ** --WHILE--(...)--T--+--
            **     \______________|
            */
            insert(ptr,OP_WHILE,pc-ptr+3);
            append(OP_PASS);
            }
          else if(rep_min==1 && rep_max==ONEINDIG){     // (...)++
            /*
            **
            ** --UNTIL--(...)--T--+--
            **      \_____________|
            */
            insert(ptr,OP_UNTIL,pc-ptr+3);
            append(OP_PASS);
            }
          else if(rep_min==0 && rep_max==1){            // (...)?+
            /*
            **
            ** --IF--(...)--T--+--
            **    \____________|
            */
            insert(ptr,OP_IF,pc-ptr+3);
            append(OP_PASS);
            }
          else{                                         // (...){M,N}+
            /*
            **
            ** --FOR--(...)--T--+--
            **    \_____________|
            */
            insert(ptr,OP_FOR,pc-ptr+7,rep_min,rep_max);
            append(OP_PASS);
            }
          }
        else{
          if(rep_min==0 && rep_max==ONEINDIG){          // (...)*
            /*    ________
            **   |        \
            ** --B--(...)--J--+--
            **    \___________|
            */
            insert(ptr,greediness?OP_BRANCHREV:OP_BRANCH,pc-ptr+5);
            append(OP_JUMP,ptr-pc-1);
            }
          else if(rep_min==1 && rep_max==ONEINDIG){     // (...)+
            /*    ________
            **   |        \
            ** --+--(...)--B--
            **
            */
            append(greediness?OP_BRANCH:OP_BRANCHREV,ptr-pc-1);
            }
          else if(rep_min==0 && rep_max==1){            // (...)?
            /*
            **
            ** --B--(...)--+--
            **    \________|
            */
            insert(ptr,greediness?OP_BRANCHREV:OP_BRANCH,pc-ptr+2);
            }
          else if(0<rep_min && rep_min==rep_max){       // (...){M,N}, where M>0
            /*       ___________
            **      |           \
            ** --Z--+--(...)--I--L--
            **
            */
            if(nbra>=NSUBEXP) return FXRex::ErrComplex;
            insert(ptr,OP_ZERO_0+nbra);
            append(OP_INCR_0+nbra);
            append(OP_JUMPLT_0+nbra,rep_min,ptr-pc-2);
            nbra++;
            }
          else if(rep_min==0 && rep_max<ONEINDIG){      // (...){0,N}, while N finite
            /*       ___________
            **      |           \
            ** --Z--B--(...)--I--L--+--
            **       \______________|
            */
            if(nbra>=NSUBEXP) return FXRex::ErrComplex;
            insert(ptr,OP_ZERO_0+nbra);
            insert(ptr+1,greediness?OP_BRANCHREV:OP_BRANCH,pc-ptr+7);
            append(OP_INCR_0+nbra);
            append(OP_JUMPLT_0+nbra,rep_max,ptr-pc-2);
            nbra++;
            }
          else if(0<rep_min && rep_max==ONEINDIG){      // (...){M,}, where M>0
            /*       ________________
            **      |   ___________  \
            **      |  |           \  \
            ** --Z--+--+--(...)--I--L--B--
            */
            if(nbra>=NSUBEXP) return FXRex::ErrComplex;
            insert(ptr,OP_ZERO_0+nbra);
            append(OP_INCR_0+nbra);
            append(OP_JUMPLT_0+nbra,rep_min,ptr-pc-2);
            append(greediness?OP_BRANCH:OP_BRANCHREV,ptr-pc);
            nbra++;
            }
          else{                                         // (...){M,N}
            /*       ___________________
            **      |   ___________     \
            **      |  |           \     \
            ** --Z--+--+--(...)--I--L--G--B--+--
            **                          \____|
            */
            if(nbra>=NSUBEXP) return FXRex::ErrComplex;
            insert(ptr,OP_ZERO_0+nbra);
            append(OP_INCR_0+nbra);
            append(OP_JUMPLT_0+nbra,rep_min,ptr-pc-2);
            append(OP_JUMPGT_0+nbra,rep_max-1,5);
            append(greediness?OP_BRANCH:OP_BRANCHREV,ptr-pc);
            nbra++;
            }
          }
        }
      }
    }
  return FXRex::ErrOK;
  }


// Parse atom
FXRex::Error FXCompile::atom(FXshort& flags,FXshort& smin,FXshort& smax){
  const FXchar* savepat;
  FXshort level,len;
  FXRex::Error err;
  FXchar *ptr;
  FXint save;
  FXuchar ch;
  flags=FLG_WORST;                                      // Assume the worst
  smin=0;
  smax=0;
  switch(*pat){
    case '(':                                           // Subexpression grouping
      pat++;
      if(*pat=='?'){
        pat++;
        if(*pat==':'){                                  // Non capturing parentheses
          pat++;
          if((err=expression(flags,smin,smax))!=FXRex::ErrOK) return err;
          }
        else if((ch=*pat)=='i' || ch=='I' || ch=='n' || ch=='N'){       // Sub-expression with some mode switches
          pat++;
          save=mode;
          if(ch=='i') mode|=FXRex::IgnoreCase;
          if(ch=='I') mode&=~FXRex::IgnoreCase;
          if(ch=='n') mode|=FXRex::Newline;
          if(ch=='N') mode&=~FXRex::Newline;
          if((err=expression(flags,smin,smax))!=FXRex::ErrOK) return err;
          mode=save;
          }
        else if(*pat=='>'){                             // Possessive subgroup
          pat++;
          ptr=append(OP_ATOMIC,0);
          if((err=expression(flags,smin,smax))!=FXRex::ErrOK) return err;
          append(OP_PASS);
          patch(ptr+1,pc);
          }
        else if(*pat=='='){                             // Positive look ahead
          pat++;
          ptr=append(OP_AHEAD_POS,0);
          if((err=expression(flags,smin,smax))!=FXRex::ErrOK) return err;
          append(OP_PASS);
          patch(ptr+1,pc);
          flags=FLG_WORST;                              // Look ahead has no width!
          smin=smax=0;
          }
        else if(*pat=='!'){                             // Negative look ahead
          pat++;
          ptr=append(OP_AHEAD_NEG,0);
          if((err=expression(flags,smin,smax))!=FXRex::ErrOK) return err;
          append(OP_PASS);
          patch(ptr+1,pc);
          flags=FLG_WORST;                              // Look ahead has no width!
          smin=smax=0;
          }
        else if(*pat=='<' && *(pat+1)=='='){            // Positive look-behind
          pat+=2;
          ptr=append(OP_BEHIND_POS,0);
          if((err=expression(flags,smin,smax))!=FXRex::ErrOK) return err;
          append(OP_PASS);
          patch(ptr+1,pc);                              // If trailing context matches (fails), go here!
          flags=FLG_WORST;                              // Look behind has no width!
          smin=smax=0;
          }
        else if(*pat=='<' && *(pat+1)=='!'){            // Negative look-behind
          pat+=2;
          ptr=append(OP_BEHIND_NEG,0);
          if((err=expression(flags,smin,smax))!=FXRex::ErrOK) return err;
          append(OP_PASS);
          patch(ptr+1,pc);                              // If trailing context matches (fails), go here!
          flags=FLG_WORST;                              // Look behind has no width!
          smin=smax=0;
          }
        else{                                           // Bad token
          return FXRex::ErrToken;
          }
        }
      else{
        if(mode&FXRex::Capture){                        // Capturing
          level=++npar;
          if(level>=NSUBEXP) return FXRex::ErrComplex;  // Expression too complex
          append(OP_SUB_BEG_0+level);
          if((err=expression(flags,smin,smax))!=FXRex::ErrOK) return err;
          append(OP_SUB_END_0+level);
          }
        else{                                           // Normal
          if((err=expression(flags,smin,smax))!=FXRex::ErrOK) return err;
          }
        }
      if(*pat!=')') return FXRex::ErrParent;            // Unmatched parenthesis
      pat++;
      flags&=~FLG_SIMPLE;
      return FXRex::ErrOK;
    case '.':                                           // Any character
      pat++;
      append((mode&FXRex::Newline)?OP_ANY_NL:OP_ANY);
      flags=FLG_WIDTH|FLG_SIMPLE;
      smin=smax=1;
      return FXRex::ErrOK;
    case '^':                                           // Begin of line
      pat++;
      append(OP_LINE_BEG);
      return FXRex::ErrOK;
    case '$':                                           // End of line
      pat++;
      append(OP_LINE_END);
      return FXRex::ErrOK;
    case '[':
      pat++;
      if((err=charset())!=FXRex::ErrOK) return err;
      if(*pat!=']') return FXRex::ErrBracket;           // Unmatched bracket
      pat++;
      flags=FLG_WIDTH|FLG_SIMPLE;
      smin=smax=1;
      return FXRex::ErrOK;
    case '\\':                                          // Escape sequences which are NOT part of simple character-run
      pat++;
      switch(*pat){
        case '\0':                                      // Unexpected pattern end
          return FXRex::ErrNoAtom;
        case 'w':                                       // Word character
          pat++;
          append(OP_WORD);
          flags=FLG_WIDTH|FLG_SIMPLE;
          smin=smax=1;
          return FXRex::ErrOK;
        case 'W':                                       // Non-word character
          pat++;
          append((mode&FXRex::Newline)?OP_NOT_WORD_NL:OP_NOT_WORD);
          flags=FLG_WIDTH|FLG_SIMPLE;
          smin=smax=1;
          return FXRex::ErrOK;
        case 's':                                       // Space
          pat++;
          append((mode&FXRex::Newline)?OP_SPACE_NL:OP_SPACE);
          flags=FLG_WIDTH|FLG_SIMPLE;
          smin=smax=1;
          return FXRex::ErrOK;
        case 'S':                                       // Non-space
          pat++;
          append(OP_NOT_SPACE);
          flags=FLG_WIDTH|FLG_SIMPLE;
          smin=smax=1;
          return FXRex::ErrOK;
        case 'd':                                       // Digit
          pat++;
          append(OP_DIGIT);
          flags=FLG_WIDTH|FLG_SIMPLE;
          smin=smax=1;
          return FXRex::ErrOK;
        case 'D':                                       // Non-digit
          pat++;
          append((mode&FXRex::Newline)?OP_NOT_DIGIT_NL:OP_NOT_DIGIT);
          flags=FLG_WIDTH|FLG_SIMPLE;
          smin=smax=1;
          return FXRex::ErrOK;
        case 'h':                                       // Hex digit
          pat++;
          append(OP_HEX);
          flags=FLG_WIDTH|FLG_SIMPLE;
          smin=smax=1;
          return FXRex::ErrOK;
        case 'H':                                       // Non-hex digit
          pat++;
          append((mode&FXRex::Newline)?OP_NOT_HEX_NL:OP_NOT_HEX);
          flags=FLG_WIDTH|FLG_SIMPLE;
          smin=smax=1;
          return FXRex::ErrOK;
        case 'p':                                       // Punctuation
          pat++;
          append(OP_PUNCT);
          flags=FLG_WIDTH|FLG_SIMPLE;
          smin=smax=1;
          return FXRex::ErrOK;
        case 'P':                                       // Non-punctuation
          pat++;
          append((mode&FXRex::Newline)?OP_NOT_PUNCT_NL:OP_NOT_PUNCT);
          flags=FLG_WIDTH|FLG_SIMPLE;
          smin=smax=1;
          return FXRex::ErrOK;
        case 'l':                                       // Letter
          pat++;
          append(OP_LETTER);
          flags=FLG_WIDTH|FLG_SIMPLE;
          smin=smax=1;
          return FXRex::ErrOK;
        case 'L':                                       // Non-letter
          pat++;
          append((mode&FXRex::Newline)?OP_NOT_LETTER_NL:OP_NOT_LETTER);
          flags=FLG_WIDTH|FLG_SIMPLE;
          smin=smax=1;
          return FXRex::ErrOK;
        case 'u':                                       // Upper case
          pat++;
          append(OP_UPPER);
          flags=FLG_WIDTH|FLG_SIMPLE;
          smin=smax=1;
          return FXRex::ErrOK;
        case 'U':                                       // Lower case
          pat++;
          append(OP_LOWER);
          flags=FLG_WIDTH|FLG_SIMPLE;
          smin=smax=1;
          return FXRex::ErrOK;
        case 'b':                                       // Word boundary
          pat++;
          append(OP_WORD_BND);
          return FXRex::ErrOK;
        case 'B':                                       // Word interior
          pat++;
          append(OP_WORD_INT);
          return FXRex::ErrOK;
        case 'A':                                       // Match only beginning of string
          pat++;
          append(OP_STR_BEG);
          return FXRex::ErrOK;
        case 'Z':                                       // Match only and end of string
          pat++;
          append(OP_STR_END);
          return FXRex::ErrOK;
        case '<':                                       // Begin of word
          pat++;
          append(OP_WORD_BEG);
          return FXRex::ErrOK;
        case '>':                                       // End of word
          pat++;
          append(OP_WORD_END);
          return FXRex::ErrOK;
        case '1':                                       // Back reference to previously matched subexpression
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          level=*pat++ - '0';                           // Get backreference level
          if(!(mode&FXRex::Capture)) return FXRex::ErrBackRef;  // Capturing is not on, so no backreferences are possible
          if(level>npar) return FXRex::ErrBackRef;              // Back reference out of range
          append((mode&FXRex::IgnoreCase)?(OP_REF_CI_0+level):(OP_REF_0+level));
          smin=0;
          smax=ONEINDIG;
          return FXRex::ErrOK;
        case 'a':                                       // Bell
          ch='\a';
          pat++;
          break;
        case 'e':                                       // Escape
          ch='\033';
          pat++;
          break;
        case 'f':                                       // Form feed
          ch='\f';
          pat++;
          break;
        case 'n':                                       // Newline
          ch='\n';
          pat++;
          break;
        case 'r':                                       // Return
          ch='\r';
          pat++;
          break;
        case 't':                                       // Tab
          ch='\t';
          pat++;
          break;
        case 'v':                                       // Vertical tab
          ch='\v';
          pat++;
          break;
        case 'c':                                       // Control character
          ch=*pat++;
          if('@'<=*pat && *pat<='_'){
            ch=*pat++ - '@';                            // Ctl-X
            }
          else if(ch=='?'){
            ch=127; pat++;                              // DEL
            }
          else{
            return FXRex::ErrToken;
            }
          break;
        case '0':                                       // Octal digit
          pat++;
          ch=0;
          if(Ascii::isOctDigit(*pat)){
            ch=*pat++ - '0';
            if(Ascii::isOctDigit(*pat)){
              ch=(ch<<3) + *pat++ - '0';
              if(Ascii::isOctDigit(*pat) && *(pat-2)<'4'){      // Leave last character if it would overflow
                ch=(ch<<3) + *pat++ - '0';
                }
              }
            }
          break;
        case 'x':                                       // Exactly two hex digits
          pat++;
          if(!Ascii::isHexDigit(*pat)) return FXRex::ErrToken;
          ch=Ascii::digitValue(*pat++);
          if(!Ascii::isHexDigit(*pat)) return FXRex::ErrToken;
          ch=(ch<<4)+Ascii::digitValue(*pat++);
          break;
        default:                                        // Single escaped characters should match exactly
          ch=*pat++;
          break;
        }
      append(OP_CHAR);                                  // Match one byte, not subject to case manipulation
      append(ch);
      flags=FLG_WIDTH|FLG_SIMPLE;
      smin=smax=1;
      return FXRex::ErrOK;
    case '*':                                           // No preceding atom
    case '+':
    case '?':
    case '{':
    case '|':
      return FXRex::ErrNoAtom;
    case ')':                                           // Unmatched parenthesis
      return FXRex::ErrParent;
    case '}':                                           // Unmatched brace
      return FXRex::ErrBrace;
    case ']':                                           // Unmatched bracket
      return FXRex::ErrBracket;
    case '\0':                                          // Illegal token
      return FXRex::ErrToken;
    default:                                            // Normal non-escaped character
      savepat=pat;
      while(ISIN(safeset,*pat)){
        pat++;
        }
      if(((ch=*pat)=='*' || ch=='+' || ch=='?' || ch=='{') && savepat+1<pat){
        pat--;
        }
      len=pat-savepat;
      if(1<len){
        flags=FLG_WIDTH;
        if(mode&FXRex::IgnoreCase){                     // Multiple characters, case insensitive
          append(OP_CHARS_CI,len);
          while(savepat<pat){
            append(Ascii::toLower(*savepat++));
            }
          }
        else{                                           // Multiple characters, case sensitive
          append(OP_CHARS,len);
          while(savepat<pat){
            append(*savepat++);
            }
          }
        }
      else{                                             // One character, case insensitive
        flags=FLG_WIDTH|FLG_SIMPLE;
        if(mode&FXRex::IgnoreCase){
          append(OP_CHAR_CI);
          append(Ascii::toLower(*savepat));
          }
        else{                                           // One character, case sensitive
          append(OP_CHAR);
          append(*savepat);
          }
        }
      smin=smax=len;
      return FXRex::ErrOK;
    }
  return FXRex::ErrOK;
  }


// Parse character class
FXRex::Error FXCompile::charset(){
  FXuchar set[32];
  FXint   first;
  FXint   last;
  FXuchar op;

  // Empty set
  ZERO(set);

  // Negated character class
  op=OP_IN;
  if(*pat=='^'){ op=OP_NOT_IN; pat++; }

  // Parse the character set
  first=-1;
  do{

    // Grab a character
    last=(FXuchar)*pat++;

    // Not at the end we hope
    if(*pat=='\0') return FXRex::ErrBracket;

    // Handle escapes
    if(last=='\\'){
      last=(FXuchar)*pat++;
      switch(last){
        case 'w':
          UNION(set,wordset);
          first=-1;
          continue;
        case 'W':
          UNION(set,nonwordset);
          first=-1;
          continue;
        case 's':
          UNION(set,spaceset);
          first=-1;
          continue;
        case 'S':
          UNION(set,nonspaceset);
          first=-1;
          continue;
        case 'd':
          UNION(set,digitset);
          first=-1;
          continue;
        case 'D':
          UNION(set,nondigitset);
          first=-1;
          continue;
        case 'h':
          UNION(set,hexdigitset);
          first=-1;
          continue;
        case 'H':
          UNION(set,nonhexdigitset);
          first=-1;
          continue;
        case 'p':
          UNION(set,delimset);
          first=-1;
          continue;
        case 'P':
          UNION(set,nondelimset);
          first=-1;
          continue;
        case 'l':
          UNION(set,letterset);
          first=-1;
          continue;
        case 'L':
          UNION(set,nonletterset);
          first=-1;
          continue;
        case 'u':
          UNION(set,upperset);
          first=-1;
          continue;
        case 'U':
          UNION(set,lowerset);
          first=-1;
          continue;
        case 'a':                               // Bell
          last='\a';
          break;
        case 'e':                               // Escape
          last='\033';
          break;
        case 'b':                               // Backspace
          last='\b';
          break;
        case 'f':                               // Form feed
          last='\f';
          break;
        case 'n':                               // Newline
          last='\n';
          break;
        case 'r':                               // Return
          last='\r';
          break;
        case 't':                               // Tab
          last='\t';
          break;
        case 'v':                               // Vertical tab
          last='\v';
          break;
        case 'c':                               // Control character
          if('@'<=*pat && *pat<='_'){
            last=*pat++ - '@';                  // Ctl-X
            }
          else if(*pat=='?'){
            last=127; pat++;                    // DEL
            }
          else{
            return FXRex::ErrToken;
            }
          break;
        case '0':                               // Octal digit
          last=0;
          if(Ascii::isOctDigit(*pat)){
            last=*pat++ - '0';
            if(Ascii::isOctDigit(*pat)){
              last=(last<<3) + *pat++ - '0';
              if(Ascii::isOctDigit(*pat) && *(pat-2)<'4'){      // Leave last character if it would overflow
                last=(last<<3) + *pat++ - '0';
                }
              }
            }
          break;
        case 'x':                               // Exactly two hex digits
          if(!Ascii::isHexDigit(*pat)) return FXRex::ErrToken;
          last=Ascii::digitValue(*pat++);
          if(!Ascii::isHexDigit(*pat)) return FXRex::ErrToken;
          last=(last<<4)+Ascii::digitValue(*pat++);
          break;
        case '\0':
          return FXRex::ErrNoAtom;              // Unexpected pattern end
        }
      }

    FXASSERT(0<=last && last<=255);

    // Include in set
    if(mode&FXRex::IgnoreCase){
      INCL(set,Ascii::toLower(last));
      INCL(set,Ascii::toUpper(last));
      }
    else{
      INCL(set,last);
      }

    // Seen begin of range?
    if(0<=first){
      if(first>=last) return FXRex::ErrRange;   // Bad range
      if(mode&FXRex::IgnoreCase){
        while(first<last){
          INCL(set,Ascii::toLower(first));
          INCL(set,Ascii::toUpper(first));
          first++;
          }
        }
      else{
        while(first<last){
          INCL(set,first);
          first++;
          }
        }
      first=-1;
      }

    // Range separator?
    else if(*pat=='-' && *(pat+1)!=']'){
      first=last;
      pat++;
      }
    }
  while(*pat!=']');

  // Are we matching newlines
  if((op==OP_NOT_IN) && !(mode&FXRex::Newline) && !ISIN(set,'\n')){
    INCL(set,'\n');
    }

  // Generate code
  appendset(op,set);

  return FXRex::ErrOK;
  }


// Append opcode
FXchar* FXCompile::append(FXuchar op){
  FXchar *ret=pc;
  if(code){
    SETOP(pc,op);
    }
  pc++;
  return ret;
  }


// Append one-argument opcode
FXchar* FXCompile::append(FXuchar op,FXshort arg){
  FXchar *ret=pc;
  if(code){
    SETOP(pc,op);
    SETARG(pc+1,arg);
    }
  pc+=3;
  return ret;
  }


// Append two-argument opcode
FXchar* FXCompile::append(FXuchar op,FXshort arg1,FXshort arg2){
  FXchar *ret=pc;
  if(code){
    SETOP(pc,op);
    SETARG(pc+1,arg1);
    SETARG(pc+3,arg2);
    }
  pc+=5;
  return ret;
  }


// Append character class opcode
FXchar* FXCompile::appendset(FXuchar op,const FXuchar set[]){
  FXchar *ret=pc;
  if(code){
    SETOP(pc,op);
    memcpy(pc+1,set,32);
    }
  pc+=33;
  return ret;
  }


// Insert opcode at ptr
FXchar* FXCompile::insert(FXchar *ptr,FXuchar op){
  if(code){
    memmove(ptr+1,ptr,pc-ptr);
    SETOP(ptr,op);
    }
  pc+=1;
  return ptr;
  }


// Insert one-argument opcode at ptr
FXchar* FXCompile::insert(FXchar *ptr,FXuchar op,FXshort arg){
  if(code){
    memmove(ptr+3,ptr,pc-ptr);
    SETOP(ptr,op);
    SETARG(ptr+1,arg);
    }
  pc+=3;
  return ptr;
  }


// Insert two-argument opcode at ptr
FXchar* FXCompile::insert(FXchar *ptr,FXuchar op,FXshort arg1,FXshort arg2){
  if(code){
    memmove(ptr+5,ptr,pc-ptr);
    SETOP(ptr,op);
    SETARG(ptr+1,arg1);
    SETARG(ptr+3,arg2);
    }
  pc+=5;
  return ptr;
  }


// Insert three-argument opcode at ptr
FXchar* FXCompile::insert(FXchar *ptr,FXuchar op,FXshort arg1,FXshort arg2,FXshort arg3){
  if(code){
    memmove(ptr+7,ptr,pc-ptr);
    SETOP(ptr,op);
    SETARG(ptr+1,arg1);
    SETARG(ptr+3,arg2);
    SETARG(ptr+5,arg3);
    }
  pc+=7;
  return ptr;
  }


// Patch linked set of branches or jumps
// Example:
//
//      Before:        After:
//      ==========================
//      0:  OP_JUMP    0:  OP_JUMP
//      1:  0          1:  9
//      2:  ....       2:  ....
//      3:  OP_JUMP    3:  OP_JUMP
//      4:  -3         4:  6
//      5:  ....       5:  ....
//      6:  ....       6:  ....
//      7:  OP_JUMP    7:  OP_JUMP
// fm-> 8:  -4         8:  2
//      9:  ....       9:  ....
// to->10:  ....      10:  ....
//
void FXCompile::patch(FXchar *fm,FXchar *to){
  FXshort delta;
  if(code && fm){
    do{
      delta=GETARG(fm);
      SETARG(fm,to-fm);
      fm+=delta;
      }
    while(delta);
    }
  }

/*******************************************************************************/

// Structure used during pattern reversal adjustment
class FXReverse {
  const FXchar *src;    // Original pattern source
  FXchar       *dst;    // Adjusted pattern destination
  FXint         mode;   // Compile mode
public:

  // Reversal engine initialize
  FXReverse(FXchar* d=NULL,const FXchar* s=NULL,FXint f=0):src(s),dst(d),mode(f){ }

  // Reverse expression
  static FXRex::Error reverse(FXchar* destination,const FXchar* source,FXint flags);

  // Parsing
  FXRex::Error verbatim();
  FXRex::Error expression();
  FXRex::Error branch();
  FXRex::Error piece();
  FXRex::Error atom();
  FXRex::Error charset();
  };


// Reverse expression
FXRex::Error FXReverse::reverse(FXchar* destination,const FXchar* source,FXint flags){
  FXReverse reverser(destination,source,flags);
  FXRex::Error err;

  // Verbatim mode
  if(flags&FXRex::Verbatim){
    if((err=reverser.verbatim())!=FXRex::ErrOK) return err;
    }

  // Regular expression mode
  else{
    if((err=reverser.expression())!=FXRex::ErrOK) return err;
    }

  return FXRex::ErrOK;
  }


// Reverse string
FXRex::Error FXReverse::verbatim(){
  if(mode&FXRex::Reverse){
    dst+=strlen(src);
    *dst='\0';
    while(*src){                // Copy reversed
      *--dst=*src++;
      }
    }
  else{
    while(*src){                // Copy forwards
      *dst++=*src++;
      }
    *dst='\0';
    }
  return FXRex::ErrOK;
  }


// Reverse expression
FXRex::Error FXReverse::expression(){
  FXRex::Error err;
  if((err=branch())!=FXRex::ErrOK) return err;
  while(*src=='|'){
    *dst++=*src++;
    if((err=branch())!=FXRex::ErrOK) return err;
    }
  *dst='\0';
  return FXRex::ErrOK;
  }


// Reverse branch
FXRex::Error FXReverse::branch(){
  const FXchar* point[MAXPIECES];
  FXchar*       ptr=dst;
  FXRex::Error  err;
  FXint         n=0;

  // Parse a run of pieces
  do{

    // Ensure not exceeding maximum # of pieces
    if(n>=MAXPIECES) return FXRex::ErrComplex;

    // Start of piece
    point[n++]=src;

    // Parse piece
    if((err=piece())!=FXRex::ErrOK) return err;
    }
  while(*src!='\0' && *src!='|' && *src!=')');

  // End of last piece
  point[n]=src;

  // Reverse order of pieces, if there's more than 1
  if((mode&FXRex::Reverse) && 1<n){
    dst=ptr;
    while(0<n){
      copyElms(dst,point[n-1],point[n]-point[n-1]);
      dst+=point[n]-point[n-1];
      n--;
      }
    }
  return FXRex::ErrOK;
  }


// Parse piece
FXRex::Error FXReverse::piece(){
  FXRex::Error err;
  FXchar ch;
  if((err=atom())!=FXRex::ErrOK) return err;
  if((ch=*src)=='*' || ch=='+' || ch=='?' || ch=='{'){
    *dst++=*src++;
    if(ch=='{'){                                        // Counted repeat
      while(Ascii::isDigit(*src)){
        *dst++=*src++;
        }
      if(*src==','){
        *dst++=*src++;
        while(Ascii::isDigit(*src)){
          *dst++=*src++;
          }
        }
      if(*src!='}') return FXRex::ErrBrace;             // Unmatched brace
      *dst++=*src++;
      }
    if(*src=='?' || *src=='+'){                         // Greedy, lazy, or possessive
      *dst++=*src++;
      }
    }
  return FXRex::ErrOK;
  }


// Reverse atom
FXRex::Error FXReverse::atom(){
  FXRex::Error err;
  FXint savemode;
  FXuchar ch;
  switch(*src){
    case '(':                                           // Subexpression grouping
      *dst++=*src++;
      savemode=mode;
      if(*src=='?'){
        *dst++=*src++;
        if((ch=*src)==':' || ch=='i' || ch=='I' || ch=='n' || ch=='N' || ch=='>'){      // Some type of sub-expression
          *dst++=*src++;
          }
        else if(ch=='=' || ch=='!'){                                                    // Positive or negative look-ahead
          *dst++=*src++;
          mode&=~FXRex::Reverse;                        // Non-reversed segment
          }
        else if(ch=='<' && (*(src+1)=='=' || *(src+1)=='!')){                           // Positive or negative look-behind
          *dst++=*src++;
          *dst++=*src++;
          mode|=FXRex::Reverse;                         // Reversed segment
          }
        else{                                           // Bad token
          return FXRex::ErrToken;
          }
        }
      if((err=expression())!=FXRex::ErrOK) return err;
      mode=savemode;
      if(*src!=')') return FXRex::ErrParent;            // Unmatched parenthesis
      *dst++=*src++;
      break;
    case '[':
      *dst++=*src++;
      if((err=charset())!=FXRex::ErrOK) return err;
      if(*src!=']') return FXRex::ErrBracket;           // Unmatched bracket
      *dst++=*src++;
      break;
    case '\\':                                          // Escape sequences which are NOT part of simple character-run
      *dst++=*src++;
      switch(*src){
        case '\0':                                      // Unexpected pattern end
          return FXRex::ErrNoAtom;
        case '1':                                       // Back reference
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          *dst++=*src++;
          break;
        case 'c':                                       // Control character
          *dst++=*src++;
          if(('@'<=*src && *src<='_') || (*src=='?')){
            *dst++=*src++;
            }
          else{
            return FXRex::ErrToken;
            }
          break;
        case '0':                                       // Octal digit
          *dst++=*src++;
          if(Ascii::isOctDigit(*src)){
            *dst++=*src++;
            if(Ascii::isOctDigit(*src)){
              *dst++=*src++;
              if(Ascii::isOctDigit(*src) && *(src-2)<'4'){      // Leave last character if it would overflow
                *dst++=*src++;
                }
              }
            }
          break;
        case 'x':                                       // Hex digit
          *dst++=*src++;
          if(!Ascii::isHexDigit(*src)) return FXRex::ErrToken;
          *dst++=*src++;
          if(!Ascii::isHexDigit(*src)) return FXRex::ErrToken;
          *dst++=*src++;
          break;
        default:                                        // Single escaped character
          *dst++=*src++;
          break;
        }
      break;
    case '*':                                           // No preceding atom
    case '+':
    case '?':
    case '{':
    case '|':
      return FXRex::ErrNoAtom;
    case ')':                                           // Unmatched parenthesis
      return FXRex::ErrParent;
    case '}':                                           // Unmatched brace
      return FXRex::ErrBrace;
    case ']':                                           // Unmatched bracket
      return FXRex::ErrBracket;
    case '\0':                                          // Illegal token
      return FXRex::ErrToken;
    default:                                            // Normal non-escaped character
      *dst++=*src++;
      break;
    }
  return FXRex::ErrOK;
  }


// Skip over charset
FXRex::Error FXReverse::charset(){
  if(*src=='^'){
    *dst++=*src++;
    }
  if(*src){
    do{
      if(*src=='\\'){ *dst++=*src++; }
      *dst++=*src++;
      }
    while(*src!='\0' && *src!=']');
    }
  return FXRex::ErrOK;
  }

/*******************************************************************************/

// Structure used during matching
class FXExecute {
  const FXchar  *anc;               // Anchor point
  const FXchar  *str;               // Current point
  const FXchar  *str_beg;           // Begin of string
  const FXchar  *str_end;           // End of string
  const FXchar  *bak_beg[NSUBEXP];  // Back reference start
  const FXchar  *bak_end[NSUBEXP];  // Back reference end
  FXint         *sub_beg;           // Begin of substring i
  FXint         *sub_end;           // End of substring i
  FXint          count[NSUBEXP];    // Counters for counted repeats
  FXint          npar;              // Number of capturing parentheses
  FXint          recs;              // Recursions
  FXint          mode;              // Match mode
public:

  // Construct match engine
  FXExecute(const FXchar* sbeg,const FXchar* send,FXint* b,FXint* e,FXint p,FXint m);

  // Attempt to match
  FXbool attempt(const FXchar* prog,const FXchar* ptr);

  // Search in string, starting at ptr
  FXint search(const FXchar* prog,const FXchar* fm,const FXchar* to);

  // Match at current string position
  FXbool match(const FXchar* prog);

  // Reverse-match at current string position
  FXbool revmatch(const FXchar* prog);
  };



// Construct match engine
FXExecute::FXExecute(const FXchar* sbeg,const FXchar* send,FXint* b,FXint* e,FXint p,FXint m):anc(nullptr),str(nullptr),str_beg(sbeg),str_end(send),sub_beg(b),sub_end(e),npar(p),recs(0),mode(m){
  bak_beg[0]=bak_end[0]=nullptr;
  bak_beg[1]=bak_end[1]=nullptr;
  bak_beg[2]=bak_end[2]=nullptr;
  bak_beg[3]=bak_end[3]=nullptr;
  bak_beg[4]=bak_end[4]=nullptr;
  bak_beg[5]=bak_end[5]=nullptr;
  bak_beg[6]=bak_end[6]=nullptr;
  bak_beg[7]=bak_end[7]=nullptr;
  bak_beg[8]=bak_end[8]=nullptr;
  bak_beg[9]=bak_end[9]=nullptr;
  switch(npar){
    case 10: sub_beg[9]=sub_end[9]=-1;
    case  9: sub_beg[8]=sub_end[8]=-1;
    case  8: sub_beg[7]=sub_end[7]=-1;
    case  7: sub_beg[6]=sub_end[6]=-1;
    case  6: sub_beg[5]=sub_end[5]=-1;
    case  5: sub_beg[4]=sub_end[4]=-1;
    case  4: sub_beg[3]=sub_end[3]=-1;
    case  3: sub_beg[2]=sub_end[2]=-1;
    case  2: sub_beg[1]=sub_end[1]=-1;
    case  1: sub_beg[0]=sub_end[0]=-1;
    }
  }


/*******************************************************************************/

// The workhorse
FXbool FXExecute::match(const FXchar* prog){
  if(recs<MAXRECURSION){
    FXint no,keep,rep_min,rep_max,greediness;
    const FXchar *save,*beg,*end;
    const FXchar *ptr;
    FXuchar op;
    FXwchar ch;

    // Recurse deeper
    ++recs;

    // Process expression
nxt:op=*prog++;
    switch(op){
      case OP_FAIL:                             // Fail (sub) pattern
        goto f;
      case OP_PASS:                             // Succeed (sub) pattern
        goto t;
      case OP_JUMP:
        prog+=GETARG(prog);
        goto nxt;
      case OP_BRANCH:                           // Jump AFTER trying following code
        save=str;
        if(match(prog+2)) goto t;
        str=save;
        prog+=GETARG(prog);
        goto nxt;
      case OP_BRANCHREV:                        // Jump BEFORE trying following code
        save=str;
        if(match(prog+GETARG(prog))) goto t;
        str=save;
        prog+=2;
        goto nxt;
      case OP_ATOMIC:                           // Atomic subgroup
        if(!match(prog+2)) goto f;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_IF:                               // Possessive match of subgroup
        save=str;
        if(match(prog+2)){
          save=str;
          }
        str=save;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_UNTIL:                            // Possessively match subgroup 1 or more times
        if(!match(prog+2)) goto f;
        /*FALL*/
      case OP_WHILE:                            // Possessively match subgroup 0 or more times
        save=str;
        while(match(prog+2)){
          save=str;
          }
        str=save;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_FOR:                              // Possessive match subgroup min...max times
        rep_min=GETARG(prog+2);
        rep_max=GETARG(prog+4);
        save=str;
        for(no=0; no<rep_max; ++no){
          if(!match(prog+6)) break;
          save=str;
          }
        str=save;
        if(no<rep_min) goto f;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_NOT_EMPTY:                        // Assert not empty
        if(str==anc) goto f;
        goto nxt;
      case OP_STR_BEG:                          // Must be at begin of entire string
        if(str!=str_beg) goto f;
        goto nxt;
      case OP_STR_END:                          // Must be at end of entire string
        if(str!=str_end) goto f;
        goto nxt;
      case OP_LINE_BEG:                         // Must be at begin of line
        if(str_beg<str){
          if(*(str-1)!='\n') goto f;
          goto nxt;
          }
        if(mode&FXRex::NotBol) goto f;
        goto nxt;
      case OP_LINE_END:                         // Must be at end of line
        if(str<str_end){
          if(*str!='\n') goto f;
          goto nxt;
          }
        if(mode&FXRex::NotEol) goto f;
        goto nxt;
      case OP_WORD_BEG:                         // Must be at begin of word (word at least one letter)
        if(str>=str_end) goto f;
        if(!Ascii::isWord(*str)) goto f;
        if(str<=str_beg) goto nxt;              // Start of buffer
        if(Ascii::isWord(*(str-1))) goto f;
        goto nxt;
      case OP_WORD_END:                         // Must be at end of word (word at least one letter)
        if(str<=str_beg) goto f;
        if(!Ascii::isWord(*(str-1))) goto f;
        if(str_end<=str) goto nxt;              // End of buffer
        if(Ascii::isWord(*str)) goto f;
        goto nxt;
      case OP_WORD_BND:                         // Must be at word boundary
        if((str<str_end && Ascii::isWord(*str)) == (str_beg<str && Ascii::isWord(*(str-1)))) goto f;
        goto nxt;
      case OP_WORD_INT:                         // Must be inside a word
        if(str_end<=str) goto f;
        if(str<=str_beg) goto f;
        if(!Ascii::isWord(*str)) goto f;
        if(!Ascii::isWord(*(str-1))) goto f;
        goto nxt;
      case OP_UWORD_BEG:                        // Unicode beginning of word
        if(str>=str_end) goto f;
        if(!Unicode::isAlphaNumeric(wc(str))) goto f;
        if(str<=str_beg) goto nxt;              // Start of buffer
        if(Unicode::isAlphaNumeric(wc(wcdec(str)))) goto f;
        goto nxt;
      case OP_UWORD_END:                        // Unicode end of word
        if(str<=str_beg) goto f;
        if(!Unicode::isAlphaNumeric(wc(wcdec(str)))) goto f;
        if(str_end<=str) goto nxt;              // End of buffer
        if(Unicode::isAlphaNumeric(wc(str))) goto f;
        goto nxt;
      case OP_UWORD_BND:                        // Unicode word boundary
        if((str<str_end && Unicode::isAlphaNumeric(wc(str))) == (str_beg<str && Unicode::isAlphaNumeric(wc(wcdec(str))))) goto f;
        goto nxt;
      case OP_UWORD_INT:                        // Unicode word interior
        if(str_end<=str) goto f;
        if(str<=str_beg) goto f;
        if(!Unicode::isAlphaNumeric(wc(str))) goto f;
        if(!Unicode::isAlphaNumeric(wc(wcdec(str)))) goto f;
        goto nxt;
      case OP_CHARS:                            // Match a run of 1 or more characters
        no=GETARG(prog);
        prog+=2;
        ptr=prog+no;
        while(prog<ptr){
          if(str_end<=str) goto f;
          ch=(FXuchar)*prog++;
          if(*str++ != ch) goto f;
          }
        goto nxt;
      case OP_CHARS_CI:                         // Match a run of 1 or more characters, case-insensitive
        no=GETARG(prog);
        prog+=2;
        ptr=prog+no;
        while(prog<ptr){
          if(str_end<=str) goto f;
          ch=(FXuchar)*prog++;
          if(Ascii::toLower(*str++)!=ch) goto f;
          }
        goto nxt;
      case OP_UCHARS:                           // Match a run of 1 or more unicode characters
        no=GETARG(prog);
        prog+=2;
        ptr=prog+no;
        while(prog<ptr){
          if(str_end<=str) goto f;
          ch=wcnxt(prog);
          if(wcnxt(str)!=ch) goto f;
          }
        goto nxt;
      case OP_UCHARS_CI:                        // Match a run of 1 or more unicode characters, case-insensitive
        no=GETARG(prog);
        prog+=2;
        ptr=prog+no;
        while(prog<ptr){
          if(str_end<=str) goto f;
          ch=wcnxt(prog);
          if(Unicode::toLower(wcnxt(str))!=ch) goto f;
          }
        goto nxt;
      case OP_ANY:                              // Match any character, except newline
        if(str_end<=str) goto f;
        ch=*str++;
        if(ch=='\n') goto f;
        goto nxt;
      case OP_ANY_NL:                           // Matches any character, including newline
        if(str_end<=str) goto f;
        str++;
        goto nxt;
      case OP_IN:                               // Match a character in a set
        if(str_end<=str) goto f;
        ch=*str++;
        if(!ISIN(prog,ch)) goto f;
        prog+=32;
        goto nxt;
      case OP_NOT_IN:                           // Match a character NOT in a set
        if(str_end<=str) goto f;
        ch=*str++;
        if(ISIN(prog,ch)) goto f;
        prog+=32;
        goto nxt;
/*
      case OP_INSIDE:                           // Match a character in ranges
        if(str_end<=str) goto f;
        no=(FXuchar)*prog++;     // Number of bytes
        ptr=prog;
        prog+=no;
        do{

//          if((ptr[0]<str[0] || (ptr[0]==str[0] && (str[0]<0xC0 || ptr[1]<str[1] || (ptr[1]==str[1] && (str[0]<0xE0 || ptr[2]<str[2] || (ptr[2]==str[2] && (str[0]<0xF0 || ptr[3]<=str[3]))))))) &&
//             (str[0]<ptr[0] || (str[0]==ptr[0] && (ptr[0]<0xC0 || str[1]<ptr[1] || (str[1]==ptr[1] && (ptr[0]<0xE0 || str[2]<ptr[2] || (str[2]==ptr[2] && (ptr[0]<0xF0 || str[3]<=ptr[3])))))))){ str++; goto nxt; }

          if(ptr[0]<=*str && *str<=ptr[1]){ str++; goto nxt; }
          ptr+=2;
          }
        while(ptr<prog);
        goto f;
      case OP_OUTSIDE:                          // Match a character NOT in ranges
        if(str_end<=str) goto f;
        no=(FXuchar)*prog++;     // Number of bytes
        ptr=prog;
        prog+=no;
        do{
          if(ptr[0]<=*str && *str<=ptr[1]){ goto f; }
          ptr+=2;
          }
        while(ptr<prog);
        str++;
        goto nxt;
*/
      case OP_ANY_OF:                           // Match character in list
        if(str_end<=str) goto f;
        ch=*str++;
        if(!INLIST(prog,ch)) goto f;
        prog+=1+(FXuchar)*prog;
        goto nxt;
      case OP_ANY_BUT:                          // Match character NOT in list
        if(str_end<=str) goto f;
        ch=*str++;
        if(INLIST(prog,ch)) goto f;
        prog+=1+(FXuchar)*prog;
        goto nxt;
      case OP_UPPER:                            // Match if uppercase
        if(str_end<=str) goto f;
        ch=*str++;
        if(!Ascii::isUpper(ch)) goto f;
        goto nxt;
      case OP_LOWER:                            // Match if lowercase
        if(str_end<=str) goto f;
        ch=*str++;
        if(!Ascii::isLower(ch)) goto f;
        goto nxt;
      case OP_SPACE:                            // Match space, except newline
        if(str_end<=str) goto f;
        ch=*str++;
        if(ch=='\n') goto f;
        if(!Ascii::isSpace(ch)) goto f;
        goto nxt;
      case OP_SPACE_NL:                         // Match space, including newline
        if(str_end<=str) goto f;
        ch=*str++;
        if(!Ascii::isSpace(ch)) goto f;
        goto nxt;
      case OP_NOT_SPACE:                        // Match non-space
        if(str_end<=str) goto f;
        ch=*str++;
        if(Ascii::isSpace(ch)) goto f;
        goto nxt;
      case OP_DIGIT:                            // Match a digit 0..9
        if(str_end<=str) goto f;
        ch=*str++;
        if(!Ascii::isDigit(ch)) goto f;
        goto nxt;
      case OP_NOT_DIGIT:                        // Match a non-digit, except newline
        if(str_end<=str) goto f;
        ch=*str++;
        if(ch=='\n') goto f;
        if(Ascii::isDigit(ch)) goto f;
        goto nxt;
      case OP_NOT_DIGIT_NL:                     // Match a non-digit, including newline
        if(str_end<=str) goto f;
        ch=*str++;
        if(Ascii::isDigit(ch)) goto f;
        goto nxt;
      case OP_HEX:                              // Match a hex digit 0..9A-Fa-f
        if(str_end<=str) goto f;
        ch=*str++;
        if(!Ascii::isHexDigit(ch)) goto f;
        goto nxt;
      case OP_NOT_HEX:                          // Match a non-hex digit, except newline
        if(str_end<=str) goto f;
        ch=*str++;
        if(ch=='\n') goto f;
        if(Ascii::isHexDigit(ch)) goto f;
        goto nxt;
      case OP_NOT_HEX_NL:                       // Match a non-hex digit, including newline
        if(str_end<=str) goto f;
        ch=*str++;
        if(Ascii::isHexDigit(ch)) goto f;
        goto nxt;
      case OP_LETTER:                           // Match a letter a..z, A..Z
        if(str_end<=str) goto f;
        ch=*str++;
        if(!Ascii::isLetter(ch)) goto f;
        goto nxt;
      case OP_NOT_LETTER:                       // Match a non-letter, except newline
        if(str_end<=str) goto f;
        ch=*str++;
        if(ch=='\n') goto f;
        if(Ascii::isLetter(ch)) goto f;
        goto nxt;
      case OP_NOT_LETTER_NL:                    // Match a non-letter, including newline
        if(str_end<=str) goto f;
        ch=*str++;
        if(Ascii::isLetter(ch)) goto f;
        goto nxt;
      case OP_PUNCT:                            // Match a punctuation
        if(str_end<=str) goto f;
        ch=*str++;
        if(!Ascii::isDelim(ch)) goto f;
        goto nxt;
      case OP_NOT_PUNCT:                        // Match a non-punctuation, except newline
        if(str_end<=str) goto f;
        ch=*str++;
        if(ch=='\n') goto f;
        if(Ascii::isDelim(ch)) goto f;
        goto nxt;
      case OP_NOT_PUNCT_NL:                     // Match a non-punctuation, including newline
        if(str_end<=str) goto f;
        ch=*str++;
        if(Ascii::isDelim(ch)) goto f;
        goto nxt;
      case OP_WORD:                             // Match a word character a..z,A..Z,0..9,_
        if(str_end<=str) goto f;
        ch=*str++;
        if(!Ascii::isWord(ch)) goto f;
        goto nxt;
      case OP_NOT_WORD:                         // Match a non-word character, except newline
        if(str_end<=str) goto f;
        ch=*str++;
        if(ch=='\n') goto f;
        if(Ascii::isWord(ch)) goto f;
        goto nxt;
      case OP_NOT_WORD_NL:                      // Match a non-word character, including newline
        if(str_end<=str) goto f;
        ch=*str++;
        if(Ascii::isWord(ch)) goto f;
        goto nxt;
      case OP_CHAR:                             // Match single character
        if(str_end<=str) goto f;
        ch=*str++;
        if(*prog++!=ch) goto f;
        goto nxt;
      case OP_CHAR_CI:                          // Match single character, case-insensitive
        if(str_end<=str) goto f;
        ch=*str++;
        if(*prog++!=Ascii::toLower(ch)) goto f;
        goto nxt;
      case OP_UANY:                             // Match any unicode character, except newline
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(ch=='\n') goto f;
        goto nxt;
      case OP_UANY_NL:                          // Matches any unicode characterm, including newline
        if(str_end<=str) goto f;
        wcnxt(str);
        goto nxt;
      case OP_UIN:                              // Unicode any character in set
        // FIXME
        FXASSERT(0);
        goto nxt;
      case OP_UNOT_IN:                          // Unicode any character not in set
        // FIXME
        FXASSERT(0);
        goto nxt;
      case OP_UANY_OF:                          // Unicode any character in list
        // FIXME
        FXASSERT(0);
        goto nxt;
      case OP_UANY_BUT:                         // Unicode any character not in list
        // FIXME
        FXASSERT(0);
        goto nxt;
      case OP_UUPPER:                           // Match if unicode uppercase
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(!Unicode::isUpper(ch)) goto f;
        goto nxt;
      case OP_ULOWER:                           // Match if unicode lowercase
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(!Unicode::isLower(ch)) goto f;
        goto nxt;
      case OP_UTITLE:                           // Match if unicode title case
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(!Unicode::isTitle(ch)) goto f;
        goto nxt;
      case OP_USPACE:                           // Match space, except newline
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(ch=='\n') goto f;
        if(!Unicode::isSpace(ch)) goto f;
        goto nxt;
      case OP_USPACE_NL:                        // Match space, including newline
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(!Unicode::isSpace(ch)) goto f;
        goto nxt;
      case OP_UNOT_SPACE:                       // Match non-space
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(Unicode::isSpace(ch)) goto f;
        goto nxt;
      case OP_UDIGIT:                           // Match a digit 0..9
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(!Unicode::isDigit(ch)) goto f;
        goto nxt;
      case OP_UNOT_DIGIT:                       // Match a non-digit, except newline
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(ch=='\n') goto f;
        if(Unicode::isDigit(ch)) goto f;
        goto nxt;
      case OP_UNOT_DIGIT_NL:                    // Match a non-digit, including newline
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(Unicode::isDigit(ch)) goto f;
        goto nxt;
      case OP_ULETTER:                          // Match a letter a..z, A..Z
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(!Unicode::isLetter(ch)) goto f;
        goto nxt;
      case OP_UNOT_LETTER:                      // Match a non-letter, except newline
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(ch=='\n') goto f;
        if(Unicode::isLetter(ch)) goto f;
        goto nxt;
      case OP_UNOT_LETTER_NL:                   // Match a non-letter, including newline
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(Unicode::isLetter(ch)) goto f;
        goto nxt;
      case OP_UPUNCT:                           // Match a punctuation
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(!Unicode::isPunct(ch)) goto f;
        goto nxt;
      case OP_UNOT_PUNCT:                       // Match a non-punctuation, except newline
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(ch=='\n') goto f;
        if(Unicode::isPunct(ch)) goto f;
        goto nxt;
      case OP_UNOT_PUNCT_NL:                    // Match a non-punctuation, including newline
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(Unicode::isPunct(ch)) goto f;
        goto nxt;
      case OP_UCAT:                             // Unicode character from category
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        ch=Unicode::charCategory(ch);
        if(!(prog[0]<=ch && ch<=prog[1])) goto f;
        prog+=2;
        goto nxt;
      case OP_UNOT_CAT:                         // Unicode character NOT from category, except newline
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(ch=='\n') goto f;
        ch=Unicode::charCategory(ch);
        if(prog[0]<=ch && ch<=prog[1]) goto f;
        prog+=2;
        goto nxt;
      case OP_UNOT_CAT_NL:                      // Unicode character NOT from category, including newline
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        ch=Unicode::charCategory(ch);
        if(prog[0]<=ch && ch<=prog[1]) goto f;
        prog+=2;
        goto nxt;
      case OP_USCRIPT:                          // Unicode character from script
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        ch=Unicode::scriptType(ch);
        if(*prog++!=ch) goto f;
        goto nxt;
      case OP_UNOT_SCRIPT:                      // Unicode character NOT from script, except newline
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        if(ch=='\n') goto f;
        ch=Unicode::scriptType(ch);
        if(*prog++==ch) goto f;
        goto nxt;
      case OP_UNOT_SCRIPT_NL:                   // Unicode character NOT from script, including newline
        if(str_end<=str) goto f;
        ch=wcnxt(str);
        ch=Unicode::scriptType(ch);
        if(*prog++==ch) goto f;
        goto nxt;
      case OP_UCHAR:                            // Match single unicode character
        if(str_end<=str) goto f;
        ch=wcnxt(prog);
        if(wcnxt(str)!=ch) goto f;
        goto nxt;
      case OP_UCHAR_CI:                         // Match unicode single character, case-insensitive
        if(str_end<=str) goto f;
        ch=wcnxt(prog);
        if(Unicode::toLower(wcnxt(str))!=ch) goto f;
        goto nxt;
      case OP_MIN_PLUS:                         // Lazy one or more repetitions
        rep_min=1;
        rep_max=INT_MAX;
        greediness=LAZY;
        goto rep;
      case OP_POS_PLUS:                         // Possessive one or more repetitions
        rep_min=1;
        rep_max=INT_MAX;
        greediness=GRABBY;
        goto rep;
      case OP_PLUS:                             // Greedy one or more repetitions
        rep_min=1;
        rep_max=INT_MAX;
        greediness=GREEDY;
        goto rep;
      case OP_MIN_QUEST:                        // Lazy zero or one
        rep_min=0;
        rep_max=1;
        greediness=LAZY;
        goto rep;
      case OP_POS_QUEST:                        // Possessive zero or one
        rep_min=0;
        rep_max=1;
        greediness=GRABBY;
        goto rep;
      case OP_QUEST:                            // Greedy zero or one
        rep_min=0;
        rep_max=1;
        greediness=GREEDY;
        goto rep;
      case OP_MIN_REP:                          // Lazy bounded repeat
        rep_min=GETARG(prog+0);
        rep_max=GETARG(prog+2);
        prog+=4;
        greediness=LAZY;
        goto rep;
      case OP_POS_REP:                          // Possessive bounded repeat
        rep_min=GETARG(prog+0);
        rep_max=GETARG(prog+2);
        prog+=4;
        greediness=GRABBY;
        goto rep;
      case OP_REP:                              // Greedy bounded repeat
        rep_min=GETARG(prog+0);
        rep_max=GETARG(prog+2);
        prog+=4;
        greediness=GREEDY;
        goto rep;
      case OP_MIN_STAR:                         // Lazy zero or more repetitions
        rep_min=0;
        rep_max=INT_MAX;
        greediness=LAZY;
        goto rep;
      case OP_POS_STAR:                         // Possessive zero or more repetitions
        rep_min=0;
        rep_max=INT_MAX;
        greediness=GRABBY;
        goto rep;
      case OP_STAR:                             // Greedy zero or more repetitions
        rep_min=0;
        rep_max=INT_MAX;
        greediness=GREEDY;
rep:    if(str+rep_min>str_end) goto f;         // Can't possibly succeed
        beg=str;
        no=0;                                   // Count number of matches
        op=*prog++;
        switch(op){
          case OP_ANY:
            while(str<str_end && no<rep_max && *str!='\n'){ ++str; ++no; }
            goto asc;
          case OP_ANY_NL:
            while(str<str_end && no<rep_max){ ++str; ++no; }
            goto asc;
          case OP_IN:
            while(str<str_end && no<rep_max && ISIN(prog,*str)){ ++str; ++no; }
            prog+=32;
            goto asc;
          case OP_NOT_IN:
            while(str<str_end && no<rep_max && !ISIN(prog,*str)){ ++str; ++no; }
            prog+=32;
            goto asc;
          case OP_ANY_OF:                       // Match character in list
            while(str<str_end && no<rep_max && INLIST(prog,*str)){ ++str; ++no; }
            prog+=1+(FXuchar)*prog;
            goto asc;
          case OP_ANY_BUT:                      // Match character NOT in list
            while(str<str_end && no<rep_max && !INLIST(prog,*str)){ ++str; ++no; }
            prog+=1+(FXuchar)*prog;
            goto asc;
          case OP_UPPER:
            while(str<str_end && no<rep_max && Ascii::isUpper(*str)){ ++str; ++no; }
            goto asc;
          case OP_LOWER:
            while(str<str_end && no<rep_max && Ascii::isLower(*str)){ ++str; ++no; }
            goto asc;
          case OP_SPACE:
            while(str<str_end && no<rep_max && *str!='\n' && Ascii::isSpace(*str)){ ++str; ++no; }
            goto asc;
          case OP_SPACE_NL:
            while(str<str_end && no<rep_max && Ascii::isSpace(*str)){ ++str; ++no; }
            goto asc;
          case OP_NOT_SPACE:
            while(str<str_end && no<rep_max && !Ascii::isSpace(*str)){ ++str; ++no; }
            goto asc;
          case OP_DIGIT:
            while(str<str_end && no<rep_max && Ascii::isDigit(*str)){ ++str; ++no; }
            goto asc;
          case OP_NOT_DIGIT:
            while(str<str_end && no<rep_max && *str!='\n' && !Ascii::isDigit(*str)){ ++str; ++no; }
            goto asc;
          case OP_NOT_DIGIT_NL:
            while(str<str_end && no<rep_max && !Ascii::isDigit(*str)){ ++str; ++no; }
            goto asc;
          case OP_HEX:
            while(str<str_end && no<rep_max && Ascii::isHexDigit(*str)){ ++str; ++no; }
            goto asc;
          case OP_NOT_HEX:
            while(str<str_end && no<rep_max && *str!='\n' && !Ascii::isHexDigit(*str)){ ++str; ++no; }
            goto asc;
          case OP_NOT_HEX_NL:
            while(str<str_end && no<rep_max && !Ascii::isHexDigit(*str)){ ++str; ++no; }
            goto asc;
          case OP_LETTER:
            while(str<str_end && no<rep_max && Ascii::isLetter(*str)){ ++str; ++no; }
            goto asc;
          case OP_NOT_LETTER:
            while(str<str_end && no<rep_max && *str!='\n' && !Ascii::isLetter(*str)){ ++str; ++no; }
            goto asc;
          case OP_NOT_LETTER_NL:
            while(str<str_end && no<rep_max && !Ascii::isLetter(*str)){ ++str; ++no; }
            goto asc;
          case OP_PUNCT:
            while(str<str_end && no<rep_max && Ascii::isDelim(*str)){ ++str; ++no; }
            goto asc;
          case OP_NOT_PUNCT:
            while(str<str_end && no<rep_max && *str!='\n' && !Ascii::isDelim(*str)){ ++str; ++no; }
            goto asc;
          case OP_NOT_PUNCT_NL:
            while(str<str_end && no<rep_max && !Ascii::isDelim(*str)){ ++str; ++no; }
            goto asc;
          case OP_WORD:
            while(str<str_end && no<rep_max && Ascii::isWord(*str)){ ++str; ++no; }
            goto asc;
          case OP_NOT_WORD:
            while(str<str_end && no<rep_max && *str!='\n' && !Ascii::isWord(*str)){ ++str; ++no; }
            goto asc;
          case OP_NOT_WORD_NL:
            while(str<str_end && no<rep_max && !Ascii::isWord(*str)){ ++str; ++no; }
            goto asc;
          case OP_CHAR:
            ch=(FXuchar)*prog++;
            while(str<str_end && no<rep_max && *str==ch){ ++str; ++no; }
            goto asc;
          case OP_CHAR_CI:
            ch=(FXuchar)*prog++;
            while(str<str_end && no<rep_max && Ascii::toLower(*str)==ch){ ++str; ++no; }
            goto asc;
          case OP_UANY:
            while(str<str_end && no<rep_max && *str!='\n'){ wcnxt(str); ++no; }
            goto uni;
          case OP_UANY_NL:
            while(str<str_end && no<rep_max){ wcnxt(str); ++no; }
            goto uni;
          case OP_UIN:
            FXASSERT(0);
            goto uni;
          case OP_UNOT_IN:
            FXASSERT(0);
            goto uni;
          case OP_UANY_OF:
            FXASSERT(0);
            goto uni;
          case OP_UANY_BUT:
            FXASSERT(0);
            goto uni;
          case OP_UUPPER:
            save=str;
            while(str<str_end && no<rep_max && Unicode::isUpper(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_ULOWER:
            save=str;
            while(str<str_end && no<rep_max && Unicode::isLower(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UTITLE:
            save=str;
            while(str<str_end && no<rep_max && Unicode::isTitle(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_USPACE:
            save=str;
            while(str<str_end && no<rep_max && *str!='\n' && Unicode::isSpace(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_USPACE_NL:
            save=str;
            while(str<str_end && no<rep_max && Unicode::isSpace(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_SPACE:
            save=str;
            while(str<str_end && no<rep_max && !Unicode::isSpace(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UDIGIT:
            save=str;
            while(str<str_end && no<rep_max && Unicode::isDigit(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_DIGIT:
            save=str;
            while(str<str_end && no<rep_max && *str!='\n' && !Unicode::isDigit(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_DIGIT_NL:
            save=str;
            while(str<str_end && no<rep_max && !Unicode::isDigit(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_ULETTER:
            save=str;
            while(str<str_end && no<rep_max && Unicode::isLetter(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_LETTER:
            save=str;
            while(str<str_end && no<rep_max && *str!='\n' && !Unicode::isLetter(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_LETTER_NL:
            save=str;
            while(str<str_end && no<rep_max && !Unicode::isLetter(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UPUNCT:
            save=str;
            while(str<str_end && no<rep_max && Unicode::isPunct(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_PUNCT:
            save=str;
            while(str<str_end && no<rep_max && *str!='\n' && !Unicode::isPunct(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_PUNCT_NL:
            save=str;
            while(str<str_end && no<rep_max && !Unicode::isPunct(wcnxt(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UCAT:
            save=str;
            while(str<str_end && no<rep_max && prog[0]<=(ch=Unicode::charCategory(wcnxt(str))) && ch<=prog[1]){ save=str; ++no; }
            str=save;
            prog+=2;
            goto uni;
          case OP_UNOT_CAT:
            save=str;
            while(str<str_end && no<rep_max && *str!='\n' && !(prog[0]<=(ch=Unicode::charCategory(wcnxt(str))) && ch<=prog[1])){ save=str; ++no; }
            str=save;
            prog+=2;
            goto uni;
          case OP_UNOT_CAT_NL:
            save=str;
            while(str<str_end && no<rep_max && !(prog[0]<=(ch=Unicode::charCategory(wcnxt(str))) && ch<=prog[1])){ save=str; ++no; }
            str=save;
            prog+=2;
            goto uni;
          case OP_USCRIPT:
            ch=*prog++;
            save=str;
            while(str<str_end && no<rep_max && Unicode::scriptType(wcnxt(str))==(FXuint)ch){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_SCRIPT:
            ch=*prog++;
            save=str;
            while(str<str_end && no<rep_max && *str!='\n' && Unicode::scriptType(wcnxt(str))!=(FXuint)ch){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_SCRIPT_NL:
            ch=*prog++;
            save=str;
            while(str<str_end && no<rep_max && Unicode::scriptType(wcnxt(str))!=(FXuint)ch){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UCHAR:
            ch=wcnxt(prog);
            save=str;
            while(str<str_end && no<rep_max && wcnxt(str)==ch){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UCHAR_CI:
            ch=wcnxt(prog);
            save=str;
            while(str<str_end && no<rep_max && Unicode::toLower(wcnxt(str))==ch){ save=str; ++no; }
            str=save;
            goto uni;
          default:
            fxerror("FXRex::match: bad opcode (%d) at: %p on line: %d\n",op,prog-1,__LINE__);
            goto f;
          }

        // Unicode match
uni:    if(no<rep_min) goto f;

        // Greedy match
        if(greediness==GREEDY){
          while(rep_min){beg=wcinc(beg);--rep_min;}
          end=str;
          while(beg<=end){
            str=end;
            if(match(prog)) goto t;
            end=wcdec(end);
            }
          goto f;
          }

        // Lazy match
        if(greediness==LAZY){
          while(rep_min){beg=wcinc(beg);--rep_min;}
          end=str;
          while(beg<=end){
            str=beg;
            if(match(prog)) goto t;
            beg=wcinc(beg);
            }
          goto f;
          }

        // Possessive match
        goto nxt;

        // Ascii match
asc:    if(no<rep_min) goto f;

        // Greedy match
        if(greediness==GREEDY){
          beg+=rep_min;
          end=str;
          while(beg<=end){
            str=end;
            if(match(prog)) goto t;
            end--;
            }
          goto f;
          }

        // Lazy match
        if(greediness==LAZY){
          beg+=rep_min;
          end=str;
          while(beg<=end){
            str=beg;
            if(match(prog)) goto t;
            beg++;
            }
          goto f;
          }

        // Possessive match
        goto nxt;
      case OP_AHEAD_NEG:                        // Negative look-ahead
        save=str;
        keep=match(prog+2);
        str=save;
        if(keep) goto f;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_AHEAD_POS:                        // Positive look-ahead
        save=str;
        keep=match(prog+2);
        str=save;
        if(!keep) goto f;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_BEHIND_NEG:                       // Negative look-behind
        save=str;
        keep=revmatch(prog+2);          // Reverse direction!
        str=save;
        if(keep) goto f;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_BEHIND_POS:                       // Positive look-behind
        save=str;
        keep=revmatch(prog+2);          // Reverse direction!
        str=save;
        if(!keep) goto f;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_SUB_BEG_0:                        // Capturing open parentheses
      case OP_SUB_BEG_1:
      case OP_SUB_BEG_2:
      case OP_SUB_BEG_3:
      case OP_SUB_BEG_4:
      case OP_SUB_BEG_5:
      case OP_SUB_BEG_6:
      case OP_SUB_BEG_7:
      case OP_SUB_BEG_8:
      case OP_SUB_BEG_9:
        no=op-OP_SUB_BEG_0;
        beg=bak_beg[no];                        // Save old backrefs
        end=bak_end[no];
        bak_beg[no]=save=str;                   // New backref start
        bak_end[no]=nullptr;
        if(match(prog)){
          if(no<npar && sub_beg[no]==-1){ sub_beg[no]=save-str_beg; }
          goto t;
          }
        bak_beg[no]=beg;                        // Reinstate old backrefs
        bak_end[no]=end;
        goto f;
      case OP_SUB_END_0:                        // Capturing close parentheses
      case OP_SUB_END_1:
      case OP_SUB_END_2:
      case OP_SUB_END_3:
      case OP_SUB_END_4:
      case OP_SUB_END_5:
      case OP_SUB_END_6:
      case OP_SUB_END_7:
      case OP_SUB_END_8:
      case OP_SUB_END_9:
        no=op-OP_SUB_END_0;
        end=bak_end[no];
        bak_end[no]=save=str;                   // New backref end
        if(match(prog)){
          if(no<npar && sub_end[no]==-1){ sub_end[no]=save-str_beg; }
          goto t;
          }
        bak_end[no]=end;                        // Reinstate old value
        goto f;
      case OP_REF_0:                            // Back reference to capturing parentheses
      case OP_REF_1:
      case OP_REF_2:
      case OP_REF_3:
      case OP_REF_4:
      case OP_REF_5:
      case OP_REF_6:
      case OP_REF_7:
      case OP_REF_8:
      case OP_REF_9:
        no=op-OP_REF_0;
        beg=bak_beg[no];                        // Get back reference start
        end=bak_end[no];                        // Get back reference end
        if(__unlikely(!beg)) goto f;
        if(__unlikely(!end)) goto f;
        FXASSERT(beg<=end);
        if(str+(end-beg)>str_end) goto f;       // Not enough characters left
        while(beg<end){                         // Match zero or more characters from back reference
          if(*beg!=*str) goto f;
          beg++;
          str++;
          }
        goto nxt;
      case OP_REF_CI_0:                         // Back reference to capturing parentheses
      case OP_REF_CI_1:
      case OP_REF_CI_2:
      case OP_REF_CI_3:
      case OP_REF_CI_4:
      case OP_REF_CI_5:
      case OP_REF_CI_6:
      case OP_REF_CI_7:
      case OP_REF_CI_8:
      case OP_REF_CI_9:
        no=op-OP_REF_CI_0;
        beg=bak_beg[no];                        // Get back reference start
        end=bak_end[no];                        // Get back reference end
        if(__unlikely(!beg)) goto f;
        if(__unlikely(!end)) goto f;
        FXASSERT(beg<=end);
        if(str+(end-beg)>str_end) goto f;       // Not enough characters left
        while(beg<end){                         // Match zero or more characters from back reference
          if(Ascii::toLower(*beg)!=Ascii::toLower(*str)) goto f;
          beg++;
          str++;
          }
        goto nxt;
      case OP_ZERO_0:                           // Initialize counter for counting repeat
      case OP_ZERO_1:
      case OP_ZERO_2:
      case OP_ZERO_3:
      case OP_ZERO_4:
      case OP_ZERO_5:
      case OP_ZERO_6:
      case OP_ZERO_7:
      case OP_ZERO_8:
      case OP_ZERO_9:
        count[op-OP_ZERO_0]=0;
        goto nxt;
      case OP_INCR_0:                           // Increment counter for counting repeat
      case OP_INCR_1:
      case OP_INCR_2:
      case OP_INCR_3:
      case OP_INCR_4:
      case OP_INCR_5:
      case OP_INCR_6:
      case OP_INCR_7:
      case OP_INCR_8:
      case OP_INCR_9:
        count[op-OP_INCR_0]++;
        goto nxt;
      case OP_JUMPLT_0:                         // Jump if counter less than value
      case OP_JUMPLT_1:
      case OP_JUMPLT_2:
      case OP_JUMPLT_3:
      case OP_JUMPLT_4:
      case OP_JUMPLT_5:
      case OP_JUMPLT_6:
      case OP_JUMPLT_7:
      case OP_JUMPLT_8:
      case OP_JUMPLT_9:
        no=GETARG(prog);
        prog+=2;
        if(count[op-OP_JUMPLT_0]<no){           // Compare with value
          prog+=GETARG(prog);
          goto nxt;
          }
        prog+=2;
        goto nxt;
      case OP_JUMPGT_0:                         // Jump if counter greater than value
      case OP_JUMPGT_1:
      case OP_JUMPGT_2:
      case OP_JUMPGT_3:
      case OP_JUMPGT_4:
      case OP_JUMPGT_5:
      case OP_JUMPGT_6:
      case OP_JUMPGT_7:
      case OP_JUMPGT_8:
      case OP_JUMPGT_9:
        no=GETARG(prog);
        prog+=2;
        if(count[op-OP_JUMPGT_0]>no){           // Compare with value
          prog+=GETARG(prog);
          goto nxt;
          }
        prog+=2;
        goto nxt;
      default:
        fxerror("FXRex::match: bad opcode (%d) at: %p on line: %d\n",op,prog-1,__LINE__);
      }

    // Return with success
t:  --recs;
    return true;

    // Return with failure
f:  --recs;
    }
  return false;
  }

/*******************************************************************************/

// Reverse-match at current string position
FXbool FXExecute::revmatch(const FXchar* prog){
  if(recs<MAXRECURSION){
    FXint no,keep,rep_min,rep_max,greediness;
    const FXchar *save,*beg,*end;
    const FXchar *ptr;
    FXuchar op;
    FXwchar ch;

    // Recurse deeper
    ++recs;

    // Process expression
nxt:op=*prog++;
    switch(op){
      case OP_FAIL:                             // Fail (sub) pattern
        goto f;
      case OP_PASS:                             // Succeed (sub) pattern
        goto t;
      case OP_JUMP:
        prog+=GETARG(prog);
        goto nxt;
      case OP_BRANCH:                           // Jump AFTER trying following code
        save=str;
        if(revmatch(prog+2)) goto t;
        str=save;
        prog+=GETARG(prog);
        goto nxt;
      case OP_BRANCHREV:                        // Jump BEFORE trying following code
        save=str;
        if(revmatch(prog+GETARG(prog))) goto t;
        str=save;
        prog+=2;
        goto nxt;
      case OP_ATOMIC:                           // Atomic subgroup
        if(!revmatch(prog+2)) goto f;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_IF:                               // Possessive match of subgroup
        save=str;
        if(revmatch(prog+2)){
          save=str;
          }
        str=save;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_UNTIL:                            // Possessively match subgroup 1 or more times
        if(!revmatch(prog+2)) goto f;
        /*FALL*/
      case OP_WHILE:                            // Possessively match subgroup 0 or more times
        save=str;
        while(revmatch(prog+2)){
          save=str;
          }
        str=save;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_FOR:                              // Possessive match subgroup min...max times
        rep_min=GETARG(prog+2);
        rep_max=GETARG(prog+4);
        save=str;
        for(no=0; no<rep_max; ++no){
          if(!revmatch(prog+6)) break;
          save=str;
          }
        str=save;
        if(no<rep_min) goto f;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_NOT_EMPTY:                        // Assert not empty
        if(str==anc) goto f;
        goto nxt;
      case OP_STR_BEG:                          // Must be at begin of entire string
        if(str!=str_beg) goto f;
        goto nxt;
      case OP_STR_END:                          // Must be at end of entire string
        if(str!=str_end) goto f;
        goto nxt;
      case OP_LINE_BEG:                         // Must be at begin of line
        if(str_beg<str){
          if(*(str-1)!='\n') goto f;
          goto nxt;
          }
        if(mode&FXRex::NotBol) goto f;
        goto nxt;
      case OP_LINE_END:                         // Must be at end of line
        if(str<str_end){
          if(*str!='\n') goto f;
          goto nxt;
          }
        if(mode&FXRex::NotEol) goto f;
        goto nxt;
      case OP_WORD_BEG:                         // Must be at begin of word (word at least one letter)
        if(str>=str_end) goto f;
        if(!Ascii::isWord(*str)) goto f;
        if(str<=str_beg) goto nxt;              // Start of buffer
        if(Ascii::isWord(*(str-1))) goto f;
        goto nxt;
      case OP_WORD_END:                         // Must be at end of word (word at least one letter)
        if(str<=str_beg) goto f;
        if(!Ascii::isWord(*(str-1))) goto f;
        if(str_end<=str) goto nxt;              // End of buffer
        if(Ascii::isWord(*str)) goto f;
        goto nxt;
      case OP_WORD_BND:                         // Must be at word boundary
        if((str<str_end && Ascii::isWord(*str)) == (str_beg<str && Ascii::isWord(*(str-1)))) goto f;
        goto nxt;
      case OP_WORD_INT:                         // Must be inside a word
        if(str_end<=str) goto f;
        if(str<=str_beg) goto f;
        if(!Ascii::isWord(*str)) goto f;
        if(!Ascii::isWord(*(str-1))) goto f;
        goto nxt;
      case OP_UWORD_BEG:                        // Unicode beginning of word
        if(str>=str_end) goto f;
        if(!Unicode::isAlphaNumeric(wc(str))) goto f;
        if(str<=str_beg) goto nxt;              // Start of buffer
        if(Unicode::isAlphaNumeric(wc(wcdec(str)))) goto f;
        goto nxt;
      case OP_UWORD_END:                        // Unicode end of word
        if(str<=str_beg) goto f;
        if(!Unicode::isAlphaNumeric(wc(wcdec(str)))) goto f;
        if(str_end<=str) goto nxt;              // End of buffer
        if(Unicode::isAlphaNumeric(wc(str))) goto f;
        goto nxt;
      case OP_UWORD_BND:                        // Unicode word boundary
        if((str<str_end && Unicode::isAlphaNumeric(wc(str))) == (str_beg<str && Unicode::isAlphaNumeric(wc(wcdec(str))))) goto f;
        goto nxt;
      case OP_UWORD_INT:                        // Unicode word interior
        if(str_end<=str) goto f;
        if(str<=str_beg) goto f;
        if(!Unicode::isAlphaNumeric(wc(str))) goto f;
        if(!Unicode::isAlphaNumeric(wc(wcdec(str)))) goto f;
        goto nxt;
      case OP_CHARS:                            // Match a run of 1 or more characters
        no=GETARG(prog);
        prog+=2;
        ptr=prog+no;
        while(prog<ptr){
          if(str<=str_beg) goto f;
          ch=(FXuchar)*prog++;
          if(*--str!=ch) goto f;
          }
        goto nxt;
      case OP_CHARS_CI:                         // Match a run of 1 or more characters, case-insensitive
        no=GETARG(prog);
        prog+=2;
        ptr=prog+no;
        while(prog<ptr){
          if(str<=str_beg) goto f;
          ch=(FXuchar)*prog++;
          if(Ascii::toLower(*--str)!=ch) goto f;
          }
        goto nxt;
      case OP_UCHARS:                           // Match a run of 1 or more unicode characters
        no=GETARG(prog);
        prog+=2;
        ptr=prog+no;
        while(prog<ptr){
          if(str<=str_beg) goto f;
          ch=wcnxt(prog);
          if(wcprv(str)!=ch) goto f;
          }
        goto nxt;
      case OP_UCHARS_CI:                        // Match a run of 1 or more unicode characters, case-insensitive
        no=GETARG(prog);
        prog+=2;
        ptr=prog+no;
        while(prog<ptr){
          if(str<=str_beg) goto f;
          ch=wcnxt(prog);
          if(Unicode::toLower(wcprv(str))!=ch) goto f;
          }
        goto nxt;
      case OP_ANY:                              // Match any character, except newline
        if(str<=str_beg) goto f;
        ch=*--str;
        if(ch=='\n') goto f;
        goto nxt;
      case OP_ANY_NL:                           // Matches any character, including newline
        if(str<=str_beg) goto f;
        --str;
        goto nxt;
      case OP_IN:                               // Match a character in a set
        if(str<=str_beg) goto f;
        ch=*--str;
        if(!ISIN(prog,ch)) goto f;
        prog+=32;
        goto nxt;
      case OP_NOT_IN:                           // Match a character NOT in a set
        if(str<=str_beg) goto f;
        ch=*--str;
        if(ISIN(prog,ch)) goto f;
        prog+=32;
        goto nxt;
      case OP_ANY_OF:                           // Match character in list
        if(str<=str_beg) goto f;
        ch=*--str;
        if(!INLIST(prog,ch)) goto f;
        prog+=1+(FXuchar)*prog;
        goto nxt;
      case OP_ANY_BUT:                          // Match character NOT in list
        if(str<=str_beg) goto f;
        ch=*--str;
        if(INLIST(prog,ch)) goto f;
        prog+=1+(FXuchar)*prog;
        goto nxt;
      case OP_UPPER:                            // Match if uppercase
        if(str<=str_beg) goto f;
        ch=*--str;
        if(!Ascii::isUpper(ch)) goto f;
        goto nxt;
      case OP_LOWER:                            // Match if lowercase
        if(str<=str_beg) goto f;
        ch=*--str;
        if(!Ascii::isLower(ch)) goto f;
        goto nxt;
      case OP_SPACE:                            // Match space, except newline
        if(str<=str_beg) goto f;
        ch=*--str;
        if(ch=='\n') goto f;
        if(!Ascii::isSpace(ch)) goto f;
        goto nxt;
      case OP_SPACE_NL:                         // Match space, including newline
        if(str<=str_beg) goto f;
        ch=*--str;
        if(!Ascii::isSpace(ch)) goto f;
        goto nxt;
      case OP_NOT_SPACE:                        // Match non-space
        if(str<=str_beg) goto f;
        ch=*--str;
        if(Ascii::isSpace(ch)) goto f;
        goto nxt;
      case OP_DIGIT:                            // Match a digit 0..9
        if(str<=str_beg) goto f;
        if(!Ascii::isDigit(*--str)) goto f;
        goto nxt;
      case OP_NOT_DIGIT:                        // Match a non-digit, except newline
        if(str<=str_beg) goto f;
        ch=*--str;
        if(ch=='\n') goto f;
        if(Ascii::isDigit(ch)) goto f;
        goto nxt;
      case OP_NOT_DIGIT_NL:                     // Match a non-digit, including newline
        if(str<=str_beg) goto f;
        ch=*--str;
        if(Ascii::isDigit(ch)) goto f;
        goto nxt;
      case OP_HEX:                              // Match a hex digit 0..9A-Fa-f
        if(str<=str_beg) goto f;
        if(!Ascii::isHexDigit(*--str)) goto f;
        goto nxt;
      case OP_NOT_HEX:                          // Match a non-hex digit, except newline
        if(str<=str_beg) goto f;
        ch=*--str;
        if(ch=='\n') goto f;
        if(Ascii::isHexDigit(ch)) goto f;
        goto nxt;
      case OP_NOT_HEX_NL:                       // Match a non-hex digit, including newline
        if(str<=str_beg) goto f;
        ch=*--str;
        if(Ascii::isHexDigit(ch)) goto f;
        goto nxt;
      case OP_LETTER:                           // Match a letter a..z, A..Z
        if(str<=str_beg) goto f;
        ch=*--str;
        if(!Ascii::isLetter(ch)) goto f;
        goto nxt;
      case OP_NOT_LETTER:                       // Match a non-letter, except newline
        if(str<=str_beg) goto f;
        ch=*--str;
        if(ch=='\n') goto f;
        if(Ascii::isLetter(ch)) goto f;
        goto nxt;
      case OP_NOT_LETTER_NL:                    // Match a non-letter, including newline
        if(str<=str_beg) goto f;
        ch=*--str;
        if(Ascii::isLetter(ch)) goto f;
        goto nxt;
      case OP_PUNCT:                            // Match a punctuation
        if(str<=str_beg) goto f;
        ch=*--str;
        if(!Ascii::isDelim(ch)) goto f;
        goto nxt;
      case OP_NOT_PUNCT:                        // Match a non-punctuation, except newline
        if(str<=str_beg) goto f;
        ch=*--str;
        if(ch=='\n') goto f;
        if(Ascii::isDelim(ch)) goto f;
        goto nxt;
      case OP_NOT_PUNCT_NL:                     // Match a non-punctuation, including newline
        if(str<=str_beg) goto f;
        ch=*--str;
        if(Ascii::isDelim(ch)) goto f;
        goto nxt;
      case OP_WORD:                             // Match a word character a..z,A..Z,0..9,_
        if(str<=str_beg) goto f;
        ch=*--str;
        if(!Ascii::isWord(ch)) goto f;
        goto nxt;
      case OP_NOT_WORD:                         // Match a non-word character, except newline
        if(str<=str_beg) goto f;
        ch=*--str;
        if(ch=='\n') goto f;
        if(Ascii::isWord(ch)) goto f;
        goto nxt;
      case OP_NOT_WORD_NL:                      // Match a non-word character, including newline
        if(str<=str_beg) goto f;
        ch=*--str;
        if(Ascii::isWord(ch)) goto f;
        goto nxt;
      case OP_CHAR:                             // Match single character
        if(str<=str_beg) goto f;
        ch=*--str;
        if(*prog++!=ch) goto f;
        goto nxt;
      case OP_CHAR_CI:                          // Match single character, case-insensitive
        if(str<=str_beg) goto f;
        ch=*--str;
        if(*prog++!=Ascii::toLower(ch)) goto f;
        goto nxt;
      case OP_UANY:                             // Match any unicode character, except newline
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(ch=='\n') goto f;
        goto nxt;
      case OP_UANY_NL:                          // Matches any unicode characterm, including newline
        if(str<=str_beg) goto f;
        wcprv(str);
        goto nxt;
      case OP_UIN:                              // Unicode any character in set
        // FIXME
        FXASSERT(0);
        goto nxt;
      case OP_UNOT_IN:                          // Unicode any character not in set
        // FIXME
        FXASSERT(0);
        goto nxt;
      case OP_UANY_OF:                          // Unicode any character in list
        // FIXME
        FXASSERT(0);
        goto nxt;
      case OP_UANY_BUT:                         // Unicode any character not in list
        // FIXME
        FXASSERT(0);
        goto nxt;
      case OP_UUPPER:                           // Match if unicode uppercase
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(!Unicode::isUpper(ch)) goto f;
        goto nxt;
      case OP_ULOWER:                           // Match if unicode lowercase
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(!Unicode::isLower(ch)) goto f;
        goto nxt;
      case OP_UTITLE:                           // Match if unicode title case
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(!Unicode::isTitle(ch)) goto f;
        goto nxt;
      case OP_USPACE:                           // Match space, except newline
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(ch=='\n') goto f;
        if(!Unicode::isSpace(ch)) goto f;
        goto nxt;
      case OP_USPACE_NL:                        // Match space, including newline
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(!Unicode::isSpace(ch)) goto f;
        goto nxt;
      case OP_UNOT_SPACE:                       // Match non-space
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(Unicode::isSpace(ch)) goto f;
        goto nxt;
      case OP_UDIGIT:                           // Match a digit 0..9
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(!Unicode::isDigit(ch)) goto f;
        goto nxt;
      case OP_UNOT_DIGIT:                       // Match a non-digit, except newline
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(ch=='\n') goto f;
        if(Unicode::isDigit(ch)) goto f;
        goto nxt;
      case OP_UNOT_DIGIT_NL:                    // Match a non-digit, including newline
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(Unicode::isDigit(ch)) goto f;
        goto nxt;
      case OP_ULETTER:                          // Match a letter a..z, A..Z
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(!Unicode::isLetter(ch)) goto f;
        goto nxt;
      case OP_UNOT_LETTER:                      // Match a non-letter, except newline
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(ch=='\n') goto f;
        if(Unicode::isLetter(ch)) goto f;
        goto nxt;
      case OP_UNOT_LETTER_NL:                   // Match a non-letter, including newline
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(Unicode::isLetter(ch)) goto f;
        goto nxt;
      case OP_UPUNCT:                           // Match a punctuation
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(!Unicode::isPunct(ch)) goto f;
        goto nxt;
      case OP_UNOT_PUNCT:                       // Match a non-punctuation, except newline
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(ch=='\n') goto f;
        if(Unicode::isPunct(ch)) goto f;
        goto nxt;
      case OP_UNOT_PUNCT_NL:                    // Match a non-punctuation, including newline
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(Unicode::isPunct(ch)) goto f;
        goto nxt;
      case OP_UCAT:                             // Unicode character from category
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        ch=Unicode::charCategory(ch);
        if(!(prog[0]<=ch && ch<=prog[1])) goto f;
        prog+=2;
        goto nxt;
      case OP_UNOT_CAT:                         // Unicode character NOT from category, except newline
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(ch=='\n') goto f;
        ch=Unicode::charCategory(ch);
        if(prog[0]<=ch && ch<=prog[1]) goto f;
        prog+=2;
        goto nxt;
      case OP_UNOT_CAT_NL:                      // Unicode character NOT from category, including newline
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        ch=Unicode::charCategory(ch);
        if(prog[0]<=ch && ch<=prog[1]) goto f;
        prog+=2;
        goto nxt;
      case OP_USCRIPT:                          // Unicode character from script
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        ch=Unicode::scriptType(ch);
        if(*prog++!=ch) goto f;
        goto nxt;
      case OP_UNOT_SCRIPT:                      // Unicode character NOT from script, except newline
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        if(ch=='\n') goto f;
        ch=Unicode::scriptType(ch);
        if(*prog++==ch) goto f;
        goto nxt;
      case OP_UNOT_SCRIPT_NL:                   // Unicode character NOT from script, including newline
        if(str<=str_beg) goto f;
        ch=wcprv(str);
        ch=Unicode::scriptType(ch);
        if(*prog++==ch) goto f;
        goto nxt;
      case OP_UCHAR:                            // Match single unicode character
        if(str<=str_beg) goto f;
        ch=wcnxt(prog);
        if(wcprv(str)!=ch) goto f;
        goto nxt;
      case OP_UCHAR_CI:                         // Match unicode single character, case-insensitive
        if(str<=str_beg) goto f;
        ch=wcnxt(prog);
        if(Unicode::toLower(wcprv(str))!=ch) goto f;
        goto nxt;
      case OP_MIN_PLUS:                         // Lazy one or more repetitions
        rep_min=1;
        rep_max=INT_MAX;
        greediness=LAZY;
        goto rep;
      case OP_POS_PLUS:                         // Possessive one or more repetitions
        rep_min=1;
        rep_max=INT_MAX;
        greediness=GRABBY;
        goto rep;
      case OP_PLUS:                             // Greedy one or more repetitions
        rep_min=1;
        rep_max=INT_MAX;
        greediness=GREEDY;
        goto rep;
      case OP_MIN_QUEST:                        // Lazy zero or one
        rep_min=0;
        rep_max=1;
        greediness=LAZY;
        goto rep;
      case OP_POS_QUEST:                        // Possessive zero or one
        rep_min=0;
        rep_max=1;
        greediness=GRABBY;
        goto rep;
      case OP_QUEST:                            // Greedy zero or one
        rep_min=0;
        rep_max=1;
        greediness=GREEDY;
        goto rep;
      case OP_MIN_REP:                          // Lazy bounded repeat
        rep_min=GETARG(prog+0);
        rep_max=GETARG(prog+2);
        prog+=4;
        greediness=LAZY;
        goto rep;
      case OP_POS_REP:                          // Possessive bounded repeat
        rep_min=GETARG(prog+0);
        rep_max=GETARG(prog+2);
        prog+=4;
        greediness=GRABBY;
        goto rep;
      case OP_REP:                              // Greedy bounded repeat
        rep_min=GETARG(prog+0);
        rep_max=GETARG(prog+2);
        prog+=4;
        greediness=GREEDY;
        goto rep;
      case OP_MIN_STAR:                         // Lazy zero or more repetitions
        rep_min=0;
        rep_max=INT_MAX;
        greediness=LAZY;
        goto rep;
      case OP_POS_STAR:                         // Possessive zero or more repetitions
        rep_min=0;
        rep_max=INT_MAX;
        greediness=GRABBY;
        goto rep;
      case OP_STAR:                             // Greedy zero or more repetitions
        rep_min=0;
        rep_max=INT_MAX;
        greediness=GREEDY;
rep:    if(str-rep_min<str_beg) goto f;         // Can't possibly succeed
        end=str;
        no=0;                                   // Count number of matches
        op=*prog++;
        switch(op){
          case OP_ANY:
            while(str_beg<str && no<rep_max && *(str-1)!='\n'){ --str; ++no; }
            goto asc;
          case OP_ANY_NL:
            while(str_beg<str && no<rep_max){ --str; ++no; }
            goto asc;
          case OP_IN:
            while(str_beg<str && no<rep_max && ISIN(prog,*(str-1))){ --str; ++no; }
            prog+=32;
            goto asc;
          case OP_NOT_IN:
            while(str_beg<str && no<rep_max && !ISIN(prog,*(str-1))){ --str; ++no; }
            prog+=32;
            goto asc;
          case OP_ANY_OF:                       // Match character in list
            while(str_beg<str && no<rep_max && INLIST(prog,*(str-1))){ --str; ++no; }
            prog+=1+(FXuchar)*prog;
            goto asc;
          case OP_ANY_BUT:                      // Match character NOT in list
            while(str_beg<str && no<rep_max && !INLIST(prog,*(str-1))){ --str; ++no; }
            prog+=1+(FXuchar)*prog;
            goto asc;
          case OP_UPPER:
            while(str_beg<str && no<rep_max && Ascii::isUpper(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_LOWER:
            while(str_beg<str && no<rep_max && Ascii::isLower(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_SPACE:
            while(str_beg<str && no<rep_max && *(str-1)!='\n' && Ascii::isSpace(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_SPACE_NL:
            while(str_beg<str && no<rep_max && Ascii::isSpace(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_NOT_SPACE:
            while(str_beg<str && no<rep_max && !Ascii::isSpace(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_DIGIT:
            while(str_beg<str && no<rep_max && Ascii::isDigit(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_NOT_DIGIT:
            while(str_beg<str && no<rep_max && *(str-1)!='\n' && !Ascii::isDigit(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_NOT_DIGIT_NL:
            while(str_beg<str && no<rep_max && !Ascii::isDigit(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_HEX:
            while(str_beg<str && no<rep_max && Ascii::isHexDigit(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_NOT_HEX:
            while(str_beg<str && no<rep_max && *(str-1)!='\n' && !Ascii::isHexDigit(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_NOT_HEX_NL:
            while(str_beg<str && no<rep_max && !Ascii::isHexDigit(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_LETTER:
            while(str_beg<str && no<rep_max && Ascii::isLetter(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_NOT_LETTER:
            while(str_beg<str && no<rep_max && *(str-1)!='\n' && !Ascii::isLetter(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_NOT_LETTER_NL:
            while(str_beg<str && no<rep_max && !Ascii::isLetter(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_PUNCT:
            while(str_beg<str && no<rep_max && Ascii::isDelim(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_NOT_PUNCT:
            while(str_beg<str && no<rep_max && *(str-1)!='\n' && !Ascii::isDelim(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_NOT_PUNCT_NL:
            while(str_beg<str && no<rep_max && !Ascii::isDelim(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_WORD:
            while(str_beg<str && no<rep_max && Ascii::isWord(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_NOT_WORD:
            while(str_beg<str && no<rep_max && *(str-1)!='\n' && !Ascii::isWord(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_NOT_WORD_NL:
            while(str_beg<str && no<rep_max && !Ascii::isWord(*(str-1))){ --str; ++no; }
            goto asc;
          case OP_CHAR:
            ch=(FXuchar)*prog++;
            while(str_beg<str && no<rep_max && *(str-1)==ch){ --str; ++no; }
            goto asc;
          case OP_CHAR_CI:
            ch=(FXuchar)*prog++;
            while(str_beg<str && no<rep_max && Ascii::toLower(*(str-1))==ch){ --str; ++no; }
            goto asc;
          case OP_UANY:
            while(str_beg<str && no<rep_max && *(str-1)!='\n'){ wcprv(str); ++no; }
            goto uni;
          case OP_UANY_NL:
            while(str_beg<str && no<rep_max){ wcprv(str); ++no; }
            goto uni;
          case OP_UIN:
            FXASSERT(0);
            goto uni;
          case OP_UNOT_IN:
            FXASSERT(0);
            goto uni;
          case OP_UANY_OF:
            FXASSERT(0);
            goto uni;
          case OP_UANY_BUT:
            FXASSERT(0);
            goto uni;
          case OP_UUPPER:
            save=str;
            while(str_beg<str && no<rep_max && Unicode::isUpper(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_ULOWER:
            save=str;
            while(str_beg<str && no<rep_max && Unicode::isLower(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UTITLE:
            save=str;
            while(str_beg<str && no<rep_max && Unicode::isTitle(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_USPACE:
            save=str;
            while(str_beg<str && no<rep_max && *(str-1)!='\n' && Unicode::isSpace(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_USPACE_NL:
            save=str;
            while(str_beg<str && no<rep_max && Unicode::isSpace(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_SPACE:
            save=str;
            while(str_beg<str && no<rep_max && !Unicode::isSpace(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UDIGIT:
            save=str;
            while(str_beg<str && no<rep_max && Unicode::isDigit(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_DIGIT:
            save=str;
            while(str_beg<str && no<rep_max && *(str-1)!='\n' && !Unicode::isDigit(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_DIGIT_NL:
            save=str;
            while(str_beg<str && no<rep_max && !Unicode::isDigit(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_ULETTER:
            save=str;
            while(str_beg<str && no<rep_max && Unicode::isLetter(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_LETTER:
            save=str;
            while(str_beg<str && no<rep_max && *(str-1)!='\n' && !Unicode::isLetter(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_LETTER_NL:
            save=str;
            while(str_beg<str && no<rep_max && !Unicode::isLetter(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UPUNCT:
            save=str;
            while(str_beg<str && no<rep_max && Unicode::isPunct(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_PUNCT:
            save=str;
            while(str_beg<str && no<rep_max && *(str-1)!='\n' && !Unicode::isPunct(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_PUNCT_NL:
            save=str;
            while(str_beg<str && no<rep_max && !Unicode::isPunct(wcprv(str))){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UCAT:
            save=str;
            while(str_beg<str && no<rep_max && prog[0]<=(ch=Unicode::charCategory(wcprv(str))) && ch<=prog[1]){ save=str; ++no; }
            str=save;
            prog+=2;
            goto uni;
          case OP_UNOT_CAT:
            save=str;
            while(str_beg<str && no<rep_max && *(str-1)!='\n' && !(prog[0]<=(ch=Unicode::charCategory(wcprv(str))) && ch<=prog[1])){ save=str; ++no; }
            str=save;
            prog+=2;
            goto uni;
          case OP_UNOT_CAT_NL:
            save=str;
            while(str_beg<str && no<rep_max && !(prog[0]<=(ch=Unicode::charCategory(wcprv(str))) && ch<=prog[1])){ save=str; ++no; }
            str=save;
            prog+=2;
            goto uni;
          case OP_USCRIPT:
            ch=*prog++;
            save=str;
            while(str_beg<str && no<rep_max && Unicode::scriptType(wcprv(str))==(FXuint)ch){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_SCRIPT:
            ch=*prog++;
            save=str;
            while(str_beg<str && no<rep_max && *(str-1)!='\n' && Unicode::scriptType(wcprv(str))!=(FXuint)ch){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UNOT_SCRIPT_NL:
            ch=*prog++;
            save=str;
            while(str_beg<str && no<rep_max && Unicode::scriptType(wcprv(str))!=(FXuint)ch){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UCHAR:
            ch=wcnxt(prog);
            save=str;
            while(str_beg<str && no<rep_max && wcprv(str)==ch){ save=str; ++no; }
            str=save;
            goto uni;
          case OP_UCHAR_CI:
            ch=wcnxt(prog);
            save=str;
            while(str_beg<str && no<rep_max && Unicode::toLower(wcprv(str))==ch){ save=str; ++no; }
            str=save;
            goto uni;
          default:
            fxerror("FXRex::revmatch: bad opcode (%d) at: %p on line: %d\n",op,prog-1,__LINE__);
            goto f;
          }

        // Unicode match
uni:    if(no<rep_min) goto f;

        // Greedy match
        if(greediness==GREEDY){
          beg=str;
          while(rep_min){end=wcdec(end);--rep_min;}
          while(beg<=end){
            str=beg;
            if(revmatch(prog)) goto t;
            beg=wcinc(beg);
            }
          goto f;
          }

        // Lazy match
        if(greediness==LAZY){
          beg=str;
          while(rep_min){end=wcinc(end);--rep_min;}
          while(beg<=end){
            str=end;
            if(revmatch(prog)) goto t;
            end=wcdec(end);
            }
          goto f;
          }

        // Possessive match
        goto nxt;

        // Ascii match
asc:    if(no<rep_min) goto f;

        // Greedy match
        if(greediness==GREEDY){
          beg=str;
          end-=rep_min;
          while(beg<=end){
            str=beg;
            if(revmatch(prog)) goto t;
            beg++;
            }
          goto f;
          }

        // Lazy match
        if(greediness==LAZY){
          beg=str;
          end-=rep_min;
          while(beg<=end){
            str=end;
            if(revmatch(prog)) goto t;
            end--;
            }
          goto f;
          }

        // Possessive match
        goto nxt;
      case OP_AHEAD_NEG:                        // Negative look-ahead
        save=str;
        keep=match(prog+2);             // Reverse direction
        str=save;
        if(keep) goto f;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_AHEAD_POS:                        // Positive look-ahead
        save=str;
        keep=match(prog+2);             // Reverse direction
        str=save;
        if(!keep) goto f;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_BEHIND_NEG:                       // Negative look-behind
        save=str;
        keep=revmatch(prog+2);
        str=save;
        if(keep) goto f;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_BEHIND_POS:                       // Positive look-behind
        save=str;
        keep=revmatch(prog+2);
        str=save;
        if(!keep) goto f;
        prog=prog+GETARG(prog);
        goto nxt;
      case OP_SUB_BEG_0:                        // Capturing open parentheses
      case OP_SUB_BEG_1:
      case OP_SUB_BEG_2:
      case OP_SUB_BEG_3:
      case OP_SUB_BEG_4:
      case OP_SUB_BEG_5:
      case OP_SUB_BEG_6:
      case OP_SUB_BEG_7:
      case OP_SUB_BEG_8:
      case OP_SUB_BEG_9:
        no=op-OP_SUB_BEG_0;
        beg=bak_beg[no];                        // Save old backrefs
        end=bak_end[no];
        bak_beg[no]=save=str;                   // New backref start
        bak_end[no]=nullptr;
        if(revmatch(prog)){
          if(no<npar && sub_beg[no]==-1){ sub_beg[no]=save-str_beg; }
          goto t;
          }
        bak_beg[no]=beg;                        // Reinstate old backrefs
        bak_end[no]=end;
        goto f;
      case OP_SUB_END_0:                        // Capturing close parentheses
      case OP_SUB_END_1:
      case OP_SUB_END_2:
      case OP_SUB_END_3:
      case OP_SUB_END_4:
      case OP_SUB_END_5:
      case OP_SUB_END_6:
      case OP_SUB_END_7:
      case OP_SUB_END_8:
      case OP_SUB_END_9:
        no=op-OP_SUB_END_0;
        end=bak_end[no];
        bak_end[no]=save=str;                   // New backref end
        if(revmatch(prog)){
          if(no<npar && sub_end[no]==-1){ sub_end[no]=save-str_beg; }
          goto t;
          }
        bak_end[no]=end;                        // Reinstate old value
        goto f;
      case OP_REF_0:                            // Back reference to capturing parentheses
      case OP_REF_1:
      case OP_REF_2:
      case OP_REF_3:
      case OP_REF_4:
      case OP_REF_5:
      case OP_REF_6:
      case OP_REF_7:
      case OP_REF_8:
      case OP_REF_9:
        no=op-OP_REF_0;
        beg=bak_beg[no];                        // Get back reference start
        end=bak_end[no];                        // Get back reference end
        if(__unlikely(!beg)) goto f;
        if(__unlikely(!end)) goto f;
        FXASSERT(beg<=end);
        if(str+(end-beg)>str_end) goto f;       // Not enough characters left
        while(beg<end){                         // Match zero or more characters from back reference
          if(*beg!=*str) goto f;
          beg++;
          str++;
          }
        goto nxt;
      case OP_REF_CI_0:                         // Back reference to capturing parentheses
      case OP_REF_CI_1:
      case OP_REF_CI_2:
      case OP_REF_CI_3:
      case OP_REF_CI_4:
      case OP_REF_CI_5:
      case OP_REF_CI_6:
      case OP_REF_CI_7:
      case OP_REF_CI_8:
      case OP_REF_CI_9:
        no=op-OP_REF_CI_0;
        beg=bak_beg[no];                        // Get back reference start
        end=bak_end[no];                        // Get back reference end
        if(__unlikely(!beg)) goto f;
        if(__unlikely(!end)) goto f;
        FXASSERT(beg<=end);
        if(str+(end-beg)>str_end) goto f;       // Not enough characters left
        while(beg<end){                         // Match zero or more characters from back reference
          if(Ascii::toLower(*beg)!=Ascii::toLower(*str)) goto f;
          beg++;
          str++;
          }
        goto nxt;
      case OP_ZERO_0:                           // Initialize counter for counting repeat
      case OP_ZERO_1:
      case OP_ZERO_2:
      case OP_ZERO_3:
      case OP_ZERO_4:
      case OP_ZERO_5:
      case OP_ZERO_6:
      case OP_ZERO_7:
      case OP_ZERO_8:
      case OP_ZERO_9:
        count[op-OP_ZERO_0]=0;
        goto nxt;
      case OP_INCR_0:                           // Increment counter for counting repeat
      case OP_INCR_1:
      case OP_INCR_2:
      case OP_INCR_3:
      case OP_INCR_4:
      case OP_INCR_5:
      case OP_INCR_6:
      case OP_INCR_7:
      case OP_INCR_8:
      case OP_INCR_9:
        count[op-OP_INCR_0]++;
        goto nxt;
      case OP_JUMPLT_0:                         // Jump if counter less than value
      case OP_JUMPLT_1:
      case OP_JUMPLT_2:
      case OP_JUMPLT_3:
      case OP_JUMPLT_4:
      case OP_JUMPLT_5:
      case OP_JUMPLT_6:
      case OP_JUMPLT_7:
      case OP_JUMPLT_8:
      case OP_JUMPLT_9:
        no=GETARG(prog);
        prog+=2;
        if(count[op-OP_JUMPLT_0]<no){           // Compare with value
          prog+=GETARG(prog);
          goto nxt;
          }
        prog+=2;
        goto nxt;
      case OP_JUMPGT_0:                         // Jump if counter greater than value
      case OP_JUMPGT_1:
      case OP_JUMPGT_2:
      case OP_JUMPGT_3:
      case OP_JUMPGT_4:
      case OP_JUMPGT_5:
      case OP_JUMPGT_6:
      case OP_JUMPGT_7:
      case OP_JUMPGT_8:
      case OP_JUMPGT_9:
        no=GETARG(prog);
        prog+=2;
        if(count[op-OP_JUMPGT_0]>no){           // Compare with value
          prog+=GETARG(prog);
          goto nxt;
          }
        prog+=2;
        goto nxt;
      default:
        fxerror("FXRex::revmatch: bad opcode (%d) at: %p on line: %d\n",op,prog-1,__LINE__);
      }

    // Return with success
t:  --recs;
    return true;

    // Return with failure
f:  --recs;
    }
  return false;
  }


/*******************************************************************************/

// Try match text buffer at given position ptr
FXbool FXExecute::attempt(const FXchar* prog,const FXchar* ptr){
  anc=str=ptr;
  if(match(prog)){
    if(0<npar){                         // Record matched range if we have room
      sub_beg[0]=anc-str_beg;
      sub_end[0]=str-str_beg;
      }
    return true;
    }
  return false;
  }

/*******************************************************************************/

// Search in string, starting at ptr
FXint FXExecute::search(const FXchar* prog,const FXchar* fm,const FXchar* to){
  FXchar ch;

  // Must be true
  FXASSERT(str_beg<=fm && fm<=str_end);
  FXASSERT(str_beg<=to && to<=str_end);

  //// Ascii mode
  if(!(mode&FXRex::Unicode)){

    //// Search forwards
    if(fm<=to){

      // Anchored at string start
      if(prog[0]==OP_STR_BEG){
        if((fm==str_beg) && attempt(prog,fm)) return 0;
        return -1;
        }

      // Anchored at BOL
      if(prog[0]==OP_LINE_BEG){
        if(fm==str_beg){
          if(!(mode&FXRex::NotBol) && attempt(prog,fm)) return 0;
          fm++;
          }
        while(fm<=to){
          if((*(fm-1)=='\n') && attempt(prog,fm)) return fm-str_beg;
          fm++;
          }
        return -1;
        }

      // Known starting character
      if(prog[0]==OP_CHAR || prog[0]==OP_CHARS){
        ch=(prog[0]==OP_CHAR)?prog[1]:prog[3];
        if(to==str_end) to--;
        while(fm<=to){
          if((*fm==ch) && attempt(prog,fm)) return fm-str_beg;
          fm++;
          }
        return -1;
        }

      // Known starting character, ignoring case
      if(prog[0]==OP_CHAR_CI || prog[0]==OP_CHARS_CI){
        ch=(prog[0]==OP_CHAR_CI)?prog[1]:prog[3];
        if(to==str_end) to--;
        while(fm<=to){
          if((Ascii::toLower(*fm)==ch) && attempt(prog,fm)) return fm-str_beg;
          fm++;
          }
        return -1;
        }

      // General case
      while(fm<=to){
        if(attempt(prog,fm)) return fm-str_beg;
        fm++;
        }
      }

    //// Search backwards
    else{

      // Anchored at string start
      if(prog[0]==OP_STR_BEG){
        if((to==str_beg) && attempt(prog,to)) return 0;
        return -1;
        }

      // Anchored at BOL
      if(prog[0]==OP_LINE_BEG){
        while(to<fm){
          if((*(fm-1)=='\n') && attempt(prog,fm)) return fm-str_beg;
          fm--;
          }
        if(fm==str_beg){
          if(!(mode&FXRex::NotBol) && attempt(prog,fm)) return 0;
          }
        return -1;
        }

      // Known starting character
      if(prog[0]==OP_CHAR || prog[0]==OP_CHARS){
        ch=(prog[0]==OP_CHAR)?prog[1]:prog[3];
        if(fm==str_end) fm--;
        while(to<=fm){
          if((*fm==ch) && attempt(prog,fm)) return fm-str_beg;
          fm--;
          }
        return -1;
        }

      // Known starting character, ignoring case
      if(prog[0]==OP_CHAR_CI || prog[0]==OP_CHARS_CI){
        ch=(prog[0]==OP_CHAR_CI)?prog[1]:prog[3];
        if(fm==str_end) fm--;
        while(to<=fm){
          if((Ascii::toLower(*fm)==ch) && attempt(prog,fm)) return fm-str_beg;
          fm--;
          }
        return -1;
        }

      // General case
      while(to<=fm){
        if(attempt(prog,fm)) return fm-str_beg;
        fm--;
        }
      }

    return -1;
    }

  //// Unicode mode

  // Search forwards
  if(fm<=to){

    // Anchored at string start
    if(prog[0]==OP_STR_BEG){
      if((fm==str_beg) && attempt(prog,fm)) return 0;
      return -1;
      }

    // Anchored at BOL
    if(prog[0]==OP_LINE_BEG){
      if(fm==str_beg){
        if(!(mode&FXRex::NotBol) && attempt(prog,fm)) return 0;
        fm=wcinc(fm);
        }
      while(fm<=to){
        if((*(fm-1)=='\n') && attempt(prog,fm)) return fm-str_beg;
        fm=wcinc(fm);
        }
      return -1;
      }

    // General case
    while(fm<=to){
      if(attempt(prog,fm)) return fm-str_beg;
      fm=wcinc(fm);
      }
    }

  // Search backwards
  else{

    // Anchored at string start
    if(prog[0]==OP_STR_BEG){
      if((to==str_beg) && attempt(prog,to)) return 0;
      return -1;
      }

    // Anchored at BOL
    if(prog[0]==OP_LINE_BEG){
      while(to<fm){
        if((*(fm-1)=='\n') && attempt(prog,fm)) return fm-str_beg;
        fm=wcdec(fm);
        }
      if(fm==str_beg){
        if(!(mode&FXRex::NotBol) && attempt(prog,fm)) return 0;
        }
      return -1;
      }

    // General case
    while(to<=fm){
      if(attempt(prog,fm)) return fm-str_beg;
      fm=wcdec(fm);
      }
    }

  return -1;
  }

}

/*******************************************************************************/


// Table of error messages
const FXchar *const FXRex::errors[]={
  "OK",
  "Empty pattern",
  "Extra characters at end",
  "Unmatched parenthesis",
  "Unmatched bracket",
  "Unmatched brace",
  "Bad character range",
  "Bad escape sequence",
  "Bad counted repeat",
  "No atom preceding repetition",
  "Repeat following repeat",
  "Bad backward reference",
  "Bad character class",
  "Expression too complex",
  "Out of memory",
  "Illegal token",
  "Pattern too long",
  "Unsupported feature"
  };


// Construct empty regular expression object
FXRex::FXRex(){
  FXTRACE((TOPIC_CONSTRUCT,"FXRex::FXRex()\n"));
  }


// Copy regex object
FXRex::FXRex(const FXRex& orig):code(orig.code){
  FXTRACE((TOPIC_CONSTRUCT,"FXRex::FXRex(FXRex)\n"));
  }


// Compile expression from pattern; fail if error
FXRex::FXRex(const FXchar* pattern,FXint mode,FXRex::Error* error){
  FXTRACE((TOPIC_CONSTRUCT,"FXRex::FXRex(%s,%u,%p)\n",pattern,mode,error));
  FXRex::Error err=parse(pattern,mode);
  if(error){ *error=err; }
  }


// Compile expression from pattern; fail if error
FXRex::FXRex(const FXString& pattern,FXint mode,FXRex::Error* error){
  FXTRACE((TOPIC_CONSTRUCT,"FXRex::FXRex(%s,%u,%p)\n",pattern.text(),mode,error));
  FXRex::Error err=parse(pattern.text(),mode);
  if(error){ *error=err; }
  }

/*******************************************************************************/

#ifdef TOPIC_REXDUMP
#include "fxrexdbg.h"
#endif

// Parse pattern
FXRex::Error FXRex::parse(const FXchar* pattern,FXint mode){
  FXRex::Error err=ErrEmpty;

  FXTRACE((TOPIC_DETAIL,"FXRex::parse(pattern=\"%s\",mode=%b)\n",pattern,mode));

  // Error
  clear();

  // Check
  if(pattern){
    FXString adjustedpattern;

    // Allocate adjusted pattern
    err=ErrMemory;
    if(adjustedpattern.length(strlen(pattern))){

      // Modify pattern
      if((err=FXReverse::reverse(adjustedpattern.text(),pattern,mode))==ErrOK){
        FXCompile cs;

        FXTRACE((TOPIC_DETAIL,"FXRex::parse: adjustedpattern: \"%s\"\n",adjustedpattern));

        // Check syntax and count the bytes
        if((err=cs.compile(nullptr,adjustedpattern.text(),mode))==ErrOK){

          // Compile code if we want to
          if(!(mode&FXRex::Syntax)){

            // Allocate program space
            if(code.length(cs.size())){

              // Now generate code for pattern
              if((err=cs.compile(code.text(),adjustedpattern.text(),mode))==ErrOK){

#ifdef TOPIC_REXDUMP
                if(getTraceTopic(TOPIC_REXDUMP)){ dump(adjustedpattern.text(),code.text()); }
#endif
                FXTRACE((TOPIC_DETAIL,"FXRex::parse: OK\n\n"));
                return ErrOK;
                }
              }
            }
          }
        }
      }
    }

  FXTRACE((TOPIC_DETAIL,"FXRex::parse: %s\n\n",getError(err)));
  return err;
  }


// Parse pattern, return error code if syntax error is found
FXRex::Error FXRex::parse(const FXString& pattern,FXint mode){
  return parse(pattern.text(),mode);
  }

/*******************************************************************************/

// Match pattern in string at position pos
FXbool FXRex::amatch(const FXchar* string,FXint len,FXint pos,FXint mode,FXint* beg,FXint* end,FXint npar) const {
  FXExecute ms(string,string+len,beg,end,npar,mode);
  return ms.attempt(code.text(),string+pos);
  }


// Match pattern in string at position pos
FXbool FXRex::amatch(const FXString& string,FXint pos,FXint mode,FXint* beg,FXint* end,FXint npar) const {
  FXExecute ms(string.text(),string.text()+string.length(),beg,end,npar,mode);
  return ms.attempt(code.text(),string.text()+pos);
  }

/*******************************************************************************/

// Search for pattern in string, starting at fm; return position or -1
FXint FXRex::search(const FXchar* string,FXint len,FXint fm,FXint to,FXint mode,FXint* beg,FXint* end,FXint npar) const {
  FXExecute ms(string,string+len,beg,end,npar,mode);
  return ms.search(code.text(),string+fm,string+to);
  }


// Search for pattern in string, starting at fm; return position or -1
FXint FXRex::search(const FXString& string,FXint fm,FXint to,FXint mode,FXint* beg,FXint* end,FXint npar) const {
  FXExecute ms(string.text(),string.text()+string.length(),beg,end,npar,mode);
  return ms.search(code.text(),string.text()+fm,string.text()+to);
  }

/*******************************************************************************/

// Return substitution string
FXString FXRex::substitute(const FXchar* string,FXint len,FXint* beg,FXint* end,const FXchar* replace,FXint npar){
  FXString result;
  FXint i=0;
  FXint n;
  FXuchar ch;
  if(__unlikely(!string || !replace || len<0 || !beg || !end || npar<1 || NSUBEXP<npar)){ fxerror("FXRex::substitute: bad argument.\n"); }
  while((ch=replace[i++])!='\0'){
    if(ch=='&' || (ch=='\\' && '1'<=replace[i] && replace[i]<='9')){
      n=(ch=='\\')?replace[i++]-'0':0;
      if(n<npar && 0<=beg[n] && end[n]<=len){
        result.append(&string[beg[n]],end[n]-beg[n]);
        }
      continue;
      }
    if(ch=='\\'){
      ch=replace[i++];
      switch(ch){
        case '\0':                // End of string
          ch='\\';
          break;
        case 'a':                 // Bell
          ch='\a';
          break;
        case 'b':                 // Backspace
          ch='\b';
          break;
        case 'e':                 // Escape
          ch='\033';
          break;
        case 'f':                 // Form feed
          ch='\f';
          break;
        case 'n':                 // Newline
          ch='\n';
          break;
        case 'r':                 // Return
          ch='\r';
          break;
        case 't':                 // Tab
          ch='\t';
          break;
        case 'v':                 // Vertical tab
          ch='\v';
          break;
        case 'c':                 // Control character
          if('@'<=replace[i] && replace[i]<='_'){
            ch=replace[i++]-'@';
            }
          break;
        case '0':                 // Octal digit
          ch=ch-'0';
          if(Ascii::isOctDigit(replace[i])){
            ch=replace[i++]-'0';
            if(Ascii::isOctDigit(replace[i])){
              ch=(ch<<3)+replace[i++]-'0';
              if(Ascii::isOctDigit(replace[i]) && replace[i-2]<'4'){    // Leave last character if it would overflow!
                ch=(ch<<3)+replace[i++]-'0';
                }
              }
            }
          break;
        case 'x':                 // Hex digit
          if(Ascii::isHexDigit(replace[i])){    // Leave 'x' if no hex follows
            ch=Ascii::digitValue(replace[i++]);
            if(Ascii::isHexDigit(replace[i])){
              ch=(ch<<4)+Ascii::digitValue(replace[i++]);
              }
            }
          break;
        }
      }
    result.append(ch);
    }
  return result;
  }


// Return substitution string
FXString FXRex::substitute(const FXString& string,FXint* beg,FXint* end,const FXchar* replace,FXint npar){
  return substitute(string.text(),string.length(),beg,end,replace,npar);
  }


// Return substitution string
FXString FXRex::substitute(const FXchar* string,FXint len,FXint* beg,FXint* end,const FXString& replace,FXint npar){
  return substitute(string,len,beg,end,replace.text(),npar);
  }


// Return substitution string
FXString FXRex::substitute(const FXString& string,FXint* beg,FXint* end,const FXString& replace,FXint npar){
  return substitute(string.text(),string.length(),beg,end,replace.text(),npar);
  }

/*******************************************************************************/

// Assignment
FXRex& FXRex::operator=(const FXRex& orig){
  code=orig.code;
  return *this;
  }


// Equality
FXbool FXRex::operator==(const FXRex& rex) const {
  return code==rex.code;
  }


// Inequality
FXbool FXRex::operator!=(const FXRex& rex) const {
  return !operator==(rex);
  }


// Save
FXStream& operator<<(FXStream& store,const FXRex& s){
  store << s.code;
  return store;
  }


// Load
FXStream& operator>>(FXStream& store,FXRex& s){
  store >> s.code;
  return store;
  }


// Clear program
void FXRex::clear(){
  code.clear();
  }


// Clean up
FXRex::~FXRex(){
  FXTRACE((TOPIC_CONSTRUCT,"FXRex::~FXRex()\n"));
  clear();
  }

}
