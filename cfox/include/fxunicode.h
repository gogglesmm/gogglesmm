/********************************************************************************
*                                                                               *
*                   U N I C O D E   C h a r a c t e r   I n f o                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXUNICODE_H
#define FXUNICODE_H


/******* Generated on 2011/09/02 15:58:10 by unicode tool version 6.0.0 ********/


namespace FX {


/// General Category
enum {
  CatNotAssigned          = 0,       /// Cn Other, Not Assigned (no characters in the file have this property)
  CatControl              = 1,       /// Cc Other, Control
  CatFormat               = 2,       /// Cf Other, Format
  CatSurrogate            = 3,       /// Cs Other, Surrogate
  CatOther                = 4,       /// Co Other, Private Use
  CatMarkNonSpacing       = 5,       /// Mn Mark, Nonspacing
  CatMarkSpacingCombining = 6,       /// Mc Mark, Spacing Combining
  CatMarkEnclosing        = 7,       /// Me Mark, Enclosing
  CatSeparatorSpace       = 8,       /// Zs Separator, Space
  CatSeparatorLine        = 9,       /// Zl Separator, Line
  CatSeparatorParagraph   = 10,      /// Zp Separator, Paragraph
  CatLetterUpper          = 11,      /// Lu Letter, Uppercase
  CatLetterLower          = 12,      /// Ll Letter, Lowercase
  CatLetterTitle          = 13,      /// Lt Letter, Titlecase
  CatLetterModifier       = 14,      /// Lm Letter, Modifier
  CatLetterOther          = 15,      /// Lo Letter, Other
  CatNumberLetter         = 16,      /// Nl Number, Letter
  CatNumberDecimal        = 17,      /// Nd Number, Decimal Digit
  CatNumberOther          = 18,      /// No Number, Other
  CatPunctConnector       = 19,      /// Pc Punctuation, Connector
  CatPunctDash            = 20,      /// Pd Punctuation, Dash
  CatPunctOpen            = 21,      /// Ps Punctuation, Open
  CatPunctClose           = 22,      /// Pe Punctuation, Close
  CatPunctInitial         = 23,      /// Pi Punctuation, Initial quote (may behave like Ps or Pe depending on usage)
  CatPunctFinal           = 24,      /// Pf Punctuation, Final quote (may behave like Ps or Pe depending on usage)
  CatPunctOther           = 25,      /// Po Punctuation, Other
  CatSymbolMath           = 26,      /// Sm Symbol, Math
  CatSymbolCurrency       = 27,      /// Sc Symbol, Currency
  CatSymbolModifier       = 28,      /// Sk Symbol, Modifier
  CatSymbolOther          = 29       /// So Symbol, Other
  };


/// Bidi types
enum {
  DirL   = 0,       /// Left-to-Right
  DirLRE = 1,       /// Left-to-Right Embedding
  DirLRO = 2,       /// Left-to-Right Override
  DirR   = 3,       /// Right-to-Left
  DirAL  = 4,       /// Right-to-Left Arabic
  DirRLE = 5,       /// Right-to-Left Embedding
  DirRLO = 6,       /// Right-to-Left Override
  DirPDF = 7,       /// Pop Directional Format
  DirEN  = 8,       /// European Number
  DirES  = 9,       /// European Number Separator
  DirET  = 10,      /// European Number Terminator
  DirAN  = 11,      /// Arabic Number
  DirCS  = 12,      /// Common Number Separator
  DirNSM = 13,      /// Non-Spacing Mark
  DirBN  = 14,      /// Boundary Neutral
  DirB   = 15,      /// Paragraph Separator
  DirS   = 16,      /// Segment Separator
  DirWS  = 17,      /// Whitespace
  DirON  = 18       /// Other Neutrals
  };


/// Arabic joining types
enum {
  JoinTypeNonJoining   = 0,
  JoinTypeRightJoining = 1,
  JoinTypeDualJoining  = 2,
  JoinTypeJoinCausing  = 3,
  JoinTypeLeftJoining  = 4,
  JoinTypeTransparent  = 5
  };


/// Arabic joining groups
enum {
  JoinGroupNone                = 0,
  JoinGroupAin                 = 1,
  JoinGroupAlaph               = 2,
  JoinGroupAlef                = 3,
  JoinGroupBeh                 = 4,
  JoinGroupBeth                = 5,
  JoinGroupBurushaskiYehBarree = 6,
  JoinGroupDal                 = 7,
  JoinGroupDalathRish          = 8,
  JoinGroupE                   = 9,
  JoinGroupFarsiYeh            = 10,
  JoinGroupFe                  = 11,
  JoinGroupFeh                 = 12,
  JoinGroupFinalSemkath        = 13,
  JoinGroupGaf                 = 14,
  JoinGroupGamal               = 15,
  JoinGroupHah                 = 16,
  JoinGroupHamzaOnHehGoal      = 17,
  JoinGroupHe                  = 18,
  JoinGroupHeh                 = 19,
  JoinGroupHehGoal             = 20,
  JoinGroupHeth                = 21,
  JoinGroupKaf                 = 22,
  JoinGroupKaph                = 23,
  JoinGroupKhaph               = 24,
  JoinGroupKnottedHeh          = 25,
  JoinGroupLam                 = 26,
  JoinGroupLamadh              = 27,
  JoinGroupMeem                = 28,
  JoinGroupMim                 = 29,
  JoinGroupNoon                = 30,
  JoinGroupNun                 = 31,
  JoinGroupNya                 = 32,
  JoinGroupPe                  = 33,
  JoinGroupQaf                 = 34,
  JoinGroupQaph                = 35,
  JoinGroupReh                 = 36,
  JoinGroupReversedPe          = 37,
  JoinGroupSad                 = 38,
  JoinGroupSadhe               = 39,
  JoinGroupSeen                = 40,
  JoinGroupSemkath             = 41,
  JoinGroupShin                = 42,
  JoinGroupSwashKaf            = 43,
  JoinGroupSyriacWaw           = 44,
  JoinGroupTah                 = 45,
  JoinGroupTaw                 = 46,
  JoinGroupTehMarbuta          = 47,
  JoinGroupTehMarbutaGoal      = 48,
  JoinGroupTeth                = 49,
  JoinGroupWaw                 = 50,
  JoinGroupYeh                 = 51,
  JoinGroupYehBarree           = 52,
  JoinGroupYehWithTail         = 53,
  JoinGroupYudh                = 54,
  JoinGroupYudhHe              = 55,
  JoinGroupZain                = 56,
  JoinGroupZhain               = 57
  };


/// Combining class
enum {
  CombineOverlay       = 1,       /// Overlay
  CombineNukta         = 7,       /// Diacritic nukta mark
  CombineKanaVoicing   = 8,       /// Hiragana/Katakana voicing
  CombineVirama        = 9,       /// Virama
  CombineBelowLeftAtt  = 200,     /// Below left attached
  CombineBelowAtt      = 202,     /// Below attached
  CombineBelowRightAtt = 204,     /// Below right attached
  CombineLeftAtt       = 208,     /// Left attached (reordrant around single base character)
  CombineRightAtt      = 210,     /// Right attached
  CombineAboveLeftAtt  = 212,     /// Above left attached
  CombineAboveAtt      = 214,     /// Above attached
  CombineAboveRightAtt = 216,     /// Above right attached
  CombineBelowLeft     = 218,     /// Below left
  CombineBelow         = 220,     /// Below
  CombineBelowRight    = 222,     /// Below right
  CombineLeft          = 224,     /// Left (reordrant around single base character)
  CombineRight         = 226,     /// Right
  CombineAboveLeft     = 228,     /// Above left
  CombineAbove         = 230,     /// Above
  CombineAboveRight    = 232,     /// Above right
  CombineDoubleBelow   = 233,     /// Double below
  CombineDoubleAbove   = 234,     /// Double above
  CombineIotaSub       = 240      /// Below (iota subscript)
  };


/// Decompose types
enum {
  DecomposeNone      = 0,       /// Non-decomposable
  DecomposeCanonical = 1,       /// Canonical (equivalent)
  DecomposeCompat    = 2,       /// Compatible
  DecomposeFont      = 3,       /// A font variant (e.g. a blackletter form)
  DecomposeNoBreak   = 4,       /// A no-break version of a space or hyphen
  DecomposeInitial   = 5,       /// An initial presentation form (Arabic)
  DecomposeMedial    = 6,       /// A medial presentation form (Arabic)
  DecomposeFinal     = 7,       /// A final presentation form (Arabic)
  DecomposeIsolated  = 8,       /// An isolated presentation form (Arabic)
  DecomposeCircle    = 9,       /// An encircled form
  DecomposeSuper     = 10,      /// A superscript form
  DecomposeSub       = 11,      /// A subscript form
  DecomposeVertical  = 12,      /// A vertical layout presentation form
  DecomposeWide      = 13,      /// A wide (or zenkaku) compatibility character
  DecomposeNarrow    = 14,      /// A narrow (or hankaku) compatibility character
  DecomposeSmall     = 15,      /// A small variant form (CNS compatibility)
  DecomposeSquare    = 16,      /// A CJK squared font variant
  DecomposeFraction  = 17       /// A vulgar fraction form
  };


/// Line break types
enum {
  BreakUnknown    = 0,       /// XX Unknown
  BreakMandatory  = 1,       /// BK Mandatory Break (after)
  BreakReturn     = 2,       /// CR Carriage Return
  BreakLineFeed   = 3,       /// LF Line Feed
  BreakCombMark   = 4,       /// CM Attached Characters and Combining Marks (no break)
  BreakNextLine   = 5,       /// NL Next Line
  BreakSurrogate  = 6,       /// SG Surrogates
  BreakWordJoiner = 7,       /// WJ Word Joiner (no break before/after)
  BreakZWSpace    = 8,       /// ZW Zero Width Space (break opportunity)
  BreakGlue       = 9,       /// GL Non-breaking Glue (no break)
  BreakSpace      = 10,      /// SP Space
  BreakBoth       = 11,      /// B2 Break Opportunity Before and After
  BreakAfter      = 12,      /// BA Break Opportunity After
  BreakBefore     = 13,      /// BB Break Opportunity Before
  BreakHyphen     = 14,      /// HY Hyphen
  BreakContingent = 15,      /// CB Contingent Break Opportunity
  BreakClosePunct = 16,      /// CL Closing Punctuation (no break before)
  BreakCloseParen = 17,      /// CP Close Parenthesis (no break before)
  BreakExclaim    = 18,      /// EX Exclamation/Interrogation (no break before)
  BreakInsep      = 19,      /// IN Inseparable
  BreakNonStart   = 20,      /// NS Non Starter
  BreakOpenPunct  = 21,      /// OP Opening Punctuation (no break after)
  BreakQuote      = 22,      /// QU Quotation
  BreakInfix      = 23,      /// IS Numeric Infix Separator (no break before or after)
  BreakNumeric    = 24,      /// NU Numeric
  BreakPostfix    = 25,      /// PO Numeric Prefix (%, etc.)
  BreakPrefix     = 26,      /// PR Numeric Postfix ($, etc.)
  BreakSymbol     = 27,      /// SY Symbols Allowing Breaks
  BreakAmbiguous  = 28,      /// AI Ordinary Alphabetic and Symbol Characters
  BreakAlphabetic = 29,      /// AL Ordinary Alphabetic and Symbol Characters
  BreakHangul2    = 30,      /// H2 Hangul LV
  BreakHangul3    = 31,      /// H3 Hangul LVT
  BreakIdeograph  = 32,      /// ID Ideographic
  BreakJamoL      = 33,      /// JL
  BreakJamoV      = 34,      /// JV
  BreakJamoT      = 35,      /// JT
  BreakComplex    = 36       /// SA Complex Context (South East Asian)
  };


/// Scripts
enum {
  ScriptUnknown               = 0,       /// Zzzz
  ScriptCommon                = 1,       /// Zyyy
  ScriptInherited             = 2,       /// Zinh
  ScriptLatin                 = 3,       /// Latn
  ScriptGreek                 = 4,       /// Grek
  ScriptCyrillic              = 5,       /// Cyrl
  ScriptArmenian              = 6,       /// Armn
  ScriptHebrew                = 7,       /// Hebr
  ScriptArabic                = 8,       /// Arab
  ScriptSyriac                = 9,       /// Syrc
  ScriptThaana                = 10,      /// Thaa
  ScriptDevanagari            = 11,      /// Deva
  ScriptBengali               = 12,      /// Beng
  ScriptGurmukhi              = 13,      /// Guru
  ScriptGujarati              = 14,      /// Gujr
  ScriptOriya                 = 15,      /// Orya
  ScriptTamil                 = 16,      /// Taml
  ScriptTelugu                = 17,      /// Telu
  ScriptKannada               = 18,      /// Knda
  ScriptMalayalam             = 19,      /// Mlym
  ScriptSinhala               = 20,      /// Sinh
  ScriptThai                  = 21,      /// Thai
  ScriptLao                   = 22,      /// Laoo
  ScriptTibetan               = 23,      /// Tibt
  ScriptMyanmar               = 24,      /// Mymr
  ScriptGeorgian              = 25,      /// Geor
  ScriptHangul                = 26,      /// Hang
  ScriptEthiopic              = 27,      /// Ethi
  ScriptCherokee              = 28,      /// Cher
  ScriptCanadianAboriginal    = 29,      /// Cans
  ScriptOgham                 = 30,      /// Ogam
  ScriptRunic                 = 31,      /// Runr
  ScriptKhmer                 = 32,      /// Khmr
  ScriptMongolian             = 33,      /// Mong
  ScriptHiragana              = 34,      /// Hira
  ScriptKatakana              = 35,      /// Kana
  ScriptBopomofo              = 36,      /// Bopo
  ScriptHan                   = 37,      /// Hani
  ScriptYi                    = 38,      /// Yiii
  ScriptOldItalic             = 39,      /// Ital
  ScriptGothic                = 40,      /// Goth
  ScriptDeseret               = 41,      /// Dsrt
  ScriptTagalog               = 42,      /// Tglg
  ScriptHanunoo               = 43,      /// Hano
  ScriptBuhid                 = 44,      /// Buhd
  ScriptTagbanwa              = 45,      /// Tagb
  ScriptLimbu                 = 46,      /// Limb
  ScriptTaiLe                 = 47,      /// Tale
  ScriptLinearB               = 48,      /// Linb
  ScriptUgaritic              = 49,      /// Ugar
  ScriptShavian               = 50,      /// Shaw
  ScriptOsmanya               = 51,      /// Osma
  ScriptCypriot               = 52,      /// Cprt
  ScriptBraille               = 53,      /// Brai
  ScriptBuginese              = 54,      /// Bugi
  ScriptCoptic                = 55,      /// Copt
  ScriptNewTaiLue             = 56,      /// Talu
  ScriptGlagolitic            = 57,      /// Glag
  ScriptTifinagh              = 58,      /// Tfng
  ScriptSylotiNagri           = 59,      /// Sylo
  ScriptOldPersian            = 60,      /// Xpeo
  ScriptKharoshthi            = 61,      /// Khar
  ScriptBalinese              = 62,      /// Bali
  ScriptCuneiform             = 63,      /// Xsux
  ScriptPhoenician            = 64,      /// Phnx
  ScriptPhagsPa               = 65,      /// Phag
  ScriptNKo                   = 66,      /// Nkoo
  ScriptSundanese             = 67,      /// Sund
  ScriptLepcha                = 68,      /// Lepc
  ScriptOlChiki               = 69,      /// Olck
  ScriptVai                   = 70,      /// Vaii
  ScriptSaurashtra            = 71,      /// Saur
  ScriptKayahLi               = 72,      /// Kali
  ScriptRejang                = 73,      /// Rjng
  ScriptLycian                = 74,      /// Lyci
  ScriptCarian                = 75,      /// Cari
  ScriptLydian                = 76,      /// Lydi
  ScriptCham                  = 77,      /// Cham
  ScriptTaiTham               = 78,      /// Lana
  ScriptTaiViet               = 79,      /// Tavt
  ScriptAvestan               = 80,      /// Avst
  ScriptEgyptianHieroglyphs   = 81,      /// Egyp
  ScriptSamaritan             = 82,      /// Samr
  ScriptLisu                  = 83,      /// Lisu
  ScriptBamum                 = 84,      /// Bamu
  ScriptJavanese              = 85,      /// Java
  ScriptMeiteiMayek           = 86,      /// Mtei
  ScriptImperialAramaic       = 87,      /// Armi
  ScriptOldSouthArabian       = 88,      /// Sarb
  ScriptInscriptionalParthian = 89,      /// Prti
  ScriptInscriptionalPahlavi  = 90,      /// Phli
  ScriptOldTurkic             = 91,      /// Orkh
  ScriptKaithi                = 92,      /// Kthi
  ScriptBatak                 = 93,      /// Batk
  ScriptBrahmi                = 94,      /// Brah
  ScriptMandaic               = 95       /// Mand
  };


namespace Unicode {

/// Character wide character category
extern FXAPI FXuint charCategory(FXwchar ucs);

/// Get character wide character direction
extern FXAPI FXuint charDirection(FXwchar ucs);

/// Get wide character symmetry
extern FXAPI FXuint isSymmetric(FXwchar ucs);

/// Get wide character decompose type
extern FXAPI FXuint decomposeType(FXwchar ucs);

/// Return number of wide characters in decomposition
extern FXAPI FXuint charNumDecompose(FXwchar ucs);

/// Return wide character decomposition
extern FXAPI const FXwchar* charDecompose(FXwchar ucs);

/// Return wide character composition from ucsa and ucsb
extern FXAPI FXwchar charCompose(FXwchar ucsa,FXwchar ucsb);

/// Get wide character combining type; zero means starter
extern FXAPI FXuint charCombining(FXwchar ucs);

/// Get numeric value of wide character (this includes hex value)
extern FXAPI FXint digitValue(FXwchar ucs);

/// Get linebreak type of wide character
extern FXAPI FXuint lineBreakType(FXwchar ucs);

/// Get mirror image of wide character or character itself
extern FXAPI FXwchar mirrorImage(FXwchar ucs);

/// Get wide character joining type
extern FXAPI FXuint joiningType(FXwchar ucs);

/// Get wide character joining group
extern FXAPI FXuint joiningGroup(FXwchar ucs);

/// Script type of wide character
extern FXAPI FXuint scriptType(FXwchar ucs);

/// Unicode character types
extern FXAPI FXbool hasCase(FXwchar ucs);
extern FXAPI FXbool isUpper(FXwchar ucs);
extern FXAPI FXbool isLower(FXwchar ucs);
extern FXAPI FXbool isTitle(FXwchar ucs);
extern FXAPI FXbool isAscii(FXwchar ucs);
extern FXAPI FXbool isLetter(FXwchar ucs);
extern FXAPI FXbool isDigit(FXwchar ucs);
extern FXAPI FXbool isAlphaNumeric(FXwchar ucs);
extern FXAPI FXbool isControl(FXwchar ucs);
extern FXAPI FXbool isSpace(FXwchar ucs);
extern FXAPI FXbool isBlank(FXwchar ucs);
extern FXAPI FXbool isPunct(FXwchar ucs);
extern FXAPI FXbool isGraph(FXwchar ucs);
extern FXAPI FXbool isPrint(FXwchar ucs);
extern FXAPI FXbool isHexDigit(FXwchar ucs);
extern FXAPI FXbool isSymbol(FXwchar ucs);
extern FXAPI FXbool isMark(FXwchar ucs);
extern FXAPI FXbool isSep(FXwchar ucs);

/// Case conversion
extern FXAPI FXwchar toUpper(FXwchar ucs);
extern FXAPI FXwchar toLower(FXwchar ucs);
extern FXAPI FXwchar toTitle(FXwchar ucs);

}

}

#endif
