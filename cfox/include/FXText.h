/********************************************************************************
*                                                                               *
*                   M u l t i - L i n e   T e x t   W i d g e t                 *
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
#ifndef FXTEXT_H
#define FXTEXT_H

#ifndef FXSCROLLAREA_H
#include "FXScrollArea.h"
#endif

namespace FX {


/// Text widget options
enum {
  TEXT_READONLY      = 0x00100000,      /// Text is NOT editable
  TEXT_WORDWRAP      = 0x00200000,      /// Wrap at word breaks
  TEXT_OVERSTRIKE    = 0x00400000,      /// Overstrike mode
  TEXT_FIXEDWRAP     = 0x00800000,      /// Fixed wrap columns
  TEXT_NO_TABS       = 0x01000000,      /// Insert spaces for tabs
  TEXT_AUTOINDENT    = 0x02000000,      /// Autoindent
  TEXT_SHOWACTIVE    = 0x04000000,      /// Show active line
  TEXT_SHOWMATCH     = 0x08000000,      /// Show matching brace
  };


/// Highlight style entry
struct FXHiliteStyle {
  FXColor normalForeColor;            /// Normal text foreground color
  FXColor normalBackColor;            /// Normal text background color
  FXColor selectForeColor;            /// Selected text foreground color
  FXColor selectBackColor;            /// Selected text background color
  FXColor hiliteForeColor;            /// Highlight text foreground color
  FXColor hiliteBackColor;            /// Highlight text background color
  FXColor activeBackColor;            /// Active text background color
  FXuint  style;                      /// Highlight text style
  };


/**
* Text mutation callback data passed with the SEL_INSERTED,
* SEL_REPLACED, and SEL_DELETED messages; both old and new
* text is available on behalf of the undo system as well as
* syntax highlighting.
*/
struct FXTextChange {
  FXint         pos;          /// Position in buffer
  FXint         ndel;         /// Number characters deleted at position
  FXint         nins;         /// Number characters inserted at position
  const FXchar *del;          /// Text deleted at position
  const FXchar *ins;          /// Text inserted at position
  };


/**
* Text selection data.
* A range-selection is in effect when startpos<endpos, while a block-select
* is in effect when startcol<endcol additionally.
*/
struct FXTextSelection {
  FXint  startpos;      /// Start of selection (begin of first line if block-select)
  FXint  endpos;        /// End of selection (start of line past last if block-select)
  FXint  startcol;      /// Start column, if block-select (actually, indent)
  FXint  endcol;        /// End column, if block select
  };


/**
* The text widget provides a multi-line text editing control.
* The widget supports both clipboard for cut-and-paste operations,
* as well as drag and drop of text.
* Text may be edited interactively by the user, or changed through
* the programmatic interface.
*
* During interactive editing, the text widget notifies its target
* of changes to the current cursor location by issueing SEL_CHANGED
* callbacks.  If text is inserted, deleted, or replaced, the widget
* will send SEL_INSERTED, SEL_DELETED, or SEL_REPLACED messages, and
* pass along an FXTextChange to indicate what changes were made to the
* text buffer, in sufficient detail for recording undo and redo info.
*
* When selections are made, SEL_SELECTED or SEL_DESELECTED messages
* are issued, passing along the range of affected text affected as
* an array of four numbers: position, number of bytes, column, number
* of columns.
*
* As the cursor is being moved, matching parentheses, brackets, and
* braces may be highlighted for a short time if the brace-matching
* feature has been enabled.
*
* An auto-indent feature, enabled by TEXT_AUTOINDENT flag, will automatically
* enter a number of spaces if a newline is entered, starting new text entry at
* the same level of indent as the previous line.
*
* When styled mode is turned on, the text widget will maintain a parallel
* style-buffer along side the text-buffer.  This style buffer is used to
* index into a style table (FXHiliteStyle), which describes what colors
* and visual attributes are to be used to draw the each character in the
* text buffer.
*
* When text is added to the widget programmatically, a style index may
* be passed in to apply to the newly added text.  It is also possible to
* change the style of the text without changing the text itself.  This
* could be used to form the basis of a colorizing text editor, for example.
*
* The first entry in the style table corresponds to a style index of 1.
* Style index 0 is used for the default text style, the style that would
* be shown if the styled mode is not in effect.
*
* In a typical scenario, the contents of the style buffer is either directly
* written when the text is added to the widget, or is continually modified
* by editing the text via syntax-based highlighting engine which
* colors the text based on syntactical patterns.
*
* The text widget has several controlling flags which affect its behaviour
* and display.  TEXT_READONLY sets the widget in read-only mode.  In this
* mode, the contents of the widget may only be changed programmatically,
* although it is possible for the user to select and paste out of the text
* buffer.
* The TEXT_WORDWRAP flag turns on line-wrapping.  Normally, long lines extend
* past the right-hand side of the visible buffer.  In wrap mode, text is folded
* at natural break points (breakable spaces, tabs, etc).  The default wrap mode
* is a fluid wrap, that is, the text is reflowed as the widget is resized.
* The flag TEXT_FIXEDWRAP disables the fluid wrap mode and makes the widget
* wrap at a fixed column, regardless of the width of the widget.
* The TEXT_OVERSTRIKE mode causes text entered by the user to overstrike
* already existing text, instead of inserting new next. This mode can be
* toggled interactively by hitting the INS key on the keyboard.
* The flag TEXT_NO_TABS causes a TAB key to be replaced by the appropriate
* number of spaces when entered interactively; however, the user can still
* enter a TAB by entering Ctrl+TAB.
* The TEXT_SHOWACTIVE mode will cause the line containing the current
* cursor location to be highlighted.  This makes it easier to find the insertion
* point when scrolling back and forth a large document.
* The flag TEXT_SHOWMATCH will briefly flash the matching brace, parenthesis, or
* bracket when the cursor is positioned at brace, parenthesis, or bracket.
*/
class FXAPI FXText : public FXScrollArea {
  FXDECLARE(FXText)
protected:
  FXchar         *buffer;               // Text buffer being edited
  FXchar         *sbuffer;              // Text style buffer
  FXint          *visrows;              // Starts of rows in buffer
  FXint           length;               // Length of the actual text in the buffer
  FXint           nvisrows;             // Number of visible rows
  FXint           nrows;                // Total number of rows
  FXint           gapstart;             // Start of the insertion point (the gap)
  FXint           gapend;               // End of the insertion point+1
  FXint           toppos;               // Start position of first visible row
  FXint           toprow;               // Row number of first visible row
  FXint           keeppos;              // Position to keep on top visible row
  FXTextSelection select;               // Text selection
  FXTextSelection hilite;               // Text highlight
  FXint           anchorpos;            // Anchor position
  FXint           anchorrow;            // Anchor row
  FXint           anchorcol;            // Anchor column (kept inside text)
  FXint           anchorvcol;           // Unconstrained anchor column
  FXint           cursorpos;            // Cursor position
  FXint           cursorrow;            // Cursor row
  FXint           cursorcol;            // Cursor column (kept inside text)
  FXint           cursorvcol;           // Unconstrained cursor column
  FXint           prefcol;              // Preferred cursor column
  FXint           margintop;            // Margins top
  FXint           marginbottom;         // Margin bottom
  FXint           marginleft;           // Margin left
  FXint           marginright;          // Margin right
  FXint           wrapwidth;            // Wrap width in pixels
  FXint           wrapcolumns;          // Wrap columns
  FXint           tabwidth;             // Tab width in pixels
  FXint           tabcolumns;           // Tab columns
  FXint           barwidth;             // Line number width
  FXint           barcolumns;           // Line number columns
  FXFont         *font;                 // Text font
  FXColor         textColor;            // Normal text color
  FXColor         selbackColor;         // Select background color
  FXColor         seltextColor;         // Select text color
  FXColor         hilitebackColor;      // Highlight background color
  FXColor         hilitetextColor;      // Highlight text color
  FXColor         activebackColor;      // Background color for active line
  FXColor         numberColor;          // Line number color
  FXColor         cursorColor;          // Cursor color
  FXColor         barColor;             // Bar background color
  FXint           textWidth;            // Total width of all text
  FXint           textHeight;           // Total height of all text
  const FXchar   *delimiters;           // Delimiters
  FXString        clipped;              // Clipped text
  FXint           vrows;                // Default visible rows
  FXint           vcols;                // Default visible columns
  FXString        help;                 // Status line help
  FXString        tip;                  // Tooltip
  FXHiliteStyle  *hilitestyles;         // Style definitions
  FXuint          blink;                // Next cursor blink state
  FXTime          matchtime;            // Match time (ns)
  FXint           grabx;                // Grab point x
  FXint           graby;                // Grab point y
  FXuchar         mode;                 // Mode widget is in
  FXbool          modified;             // User has modified text
protected:
  FXText();
  void movegap(FXint pos);
  void sizegap(FXint sz);
  FXint charWidth(FXwchar ch,FXint indent) const;
  FXint xoffset(FXint start,FXint pos) const;
  FXint wrap(FXint start) const;
  FXint rowFromPos(FXint pos) const;
  FXint posFromRow(FXint row) const;
  FXint columnFromPos(FXint start,FXint pos) const;
  FXint posFromColumn(FXint start,FXint col) const;
  FXint indentOfLine(FXint start,FXint pos) const;
  FXbool isdelimiter(FXwchar w) const;
  FXint measureText(FXint start,FXint end,FXint& wmax,FXint& hmax) const;
  void calcVisRows(FXint s,FXint e);
  void recompute();
  FXint matchForward(FXint pos,FXint end,FXwchar l,FXwchar r,FXint level) const;
  FXint matchBackward(FXint pos,FXint beg,FXwchar l,FXwchar r,FXint level) const;
  FXint findMatching(FXint pos,FXint beg,FXint end,FXwchar ch,FXint level) const;
  void flashMatching();
  void moveContents(FXint x,FXint y);
  FXint changeBeg(FXint pos) const;
  FXint changeEnd(FXint pos) const;
  void mutation(FXint pos,FXint ncins,FXint ncdel,FXint nrins,FXint nrdel);
  FXint overstruck(FXint start,FXint end,const FXchar *text,FXint num);
  void drawCursor(FXuint state);
  virtual void paintCursor(FXDCWindow& dc) const;
  virtual void eraseCursor(FXDCWindow& dc) const;
  virtual void eraseCursorOverhang();
  virtual void drawBufferText(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXint pos,FXint n,FXuint style) const;
  virtual void fillBufferRect(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXuint style) const;
  virtual FXuint styleOf(FXint beg,FXint end,FXint row,FXint col,FXint pos) const;
  virtual void drawTextRow(FXDCWindow& dc,FXint row) const;
  virtual void drawContents(FXDCWindow& dc) const;
  virtual void drawNumbers(FXDCWindow& dc) const;
  virtual void replace(FXint pos,FXint del,const FXchar *text,FXint ins,FXint style);
  void updateRow(FXint row) const;
  void updateLines(FXint startpos,FXint endpos) const;
  void updateRange(FXint startpos,FXint endpos) const;
  FXint shiftText(FXint startpos,FXint endpos,FXint shift,FXbool notify);
  FXbool deletePendingSelection(FXbool notify);
protected:
  enum {
    MOUSE_NONE,                 // No mouse operation
    MOUSE_CHARS,                // Selecting characters
    MOUSE_WORDS,                // Selecting words
    MOUSE_LINES,                // Selecting lines
    MOUSE_BLOCK,                // Select block
    MOUSE_SCROLL,               // Scrolling
    MOUSE_DRAG,                 // Dragging text
    MOUSE_TRYDRAG               // Tentative drag
    };
private:
  FXText(const FXText&);
  FXText& operator=(const FXText&);
public:
  long onPaint(FXObject*,FXSelector,void*);
  long onEnter(FXObject*,FXSelector,void*);
  long onLeave(FXObject*,FXSelector,void*);
  long onBlink(FXObject*,FXSelector,void*);
  long onFlash(FXObject*,FXSelector,void*);
  long onFocusIn(FXObject*,FXSelector,void*);
  long onFocusOut(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onAutoScroll(FXObject*,FXSelector,void*);
  long onLeftBtnPress(FXObject*,FXSelector,void*);
  long onLeftBtnRelease(FXObject*,FXSelector,void*);
  long onMiddleBtnPress(FXObject*,FXSelector,void*);
  long onMiddleBtnRelease(FXObject*,FXSelector,void*);
  long onRightBtnPress(FXObject*,FXSelector,void*);
  long onRightBtnRelease(FXObject*,FXSelector,void*);
  long onKeyPress(FXObject*,FXSelector,void*);
  long onKeyRelease(FXObject*,FXSelector,void*);
  long onUngrabbed(FXObject*,FXSelector,void*);
  long onBeginDrag(FXObject*,FXSelector,void*);
  long onEndDrag(FXObject*,FXSelector,void*);
  long onDragged(FXObject*,FXSelector,void*);
  long onDNDEnter(FXObject*,FXSelector,void*);
  long onDNDLeave(FXObject*,FXSelector,void*);
  long onDNDMotion(FXObject*,FXSelector,void*);
  long onDNDDrop(FXObject*,FXSelector,void*);
  long onDNDRequest(FXObject*,FXSelector,void*);
  long onSelectionLost(FXObject*,FXSelector,void*);
  long onSelectionGained(FXObject*,FXSelector,void*);
  long onSelectionRequest(FXObject*,FXSelector,void* ptr);
  long onClipboardLost(FXObject*,FXSelector,void*);
  long onClipboardGained(FXObject*,FXSelector,void*);
  long onClipboardRequest(FXObject*,FXSelector,void*);
  long onCmdSetTip(FXObject*,FXSelector,void*);
  long onCmdGetTip(FXObject*,FXSelector,void*);
  long onCmdSetHelp(FXObject*,FXSelector,void*);
  long onCmdGetHelp(FXObject*,FXSelector,void*);
  long onQueryTip(FXObject*,FXSelector,void*);
  long onQueryHelp(FXObject*,FXSelector,void*);
  long onUpdIsEditable(FXObject*,FXSelector,void*);
  long onUpdHaveSelection(FXObject*,FXSelector,void*);
  long onUpdHaveEditableSelection(FXObject*,FXSelector,void*);
  long onIMEStart(FXObject*,FXSelector,void*);
  long onTipTimer(FXObject*,FXSelector,void*);

  // Value access
  long onCmdSetStringValue(FXObject*,FXSelector,void*);
  long onCmdGetStringValue(FXObject*,FXSelector,void*);

  // Cursor movement
  long onCmdCursorTop(FXObject*,FXSelector,void*);
  long onCmdCursorBottom(FXObject*,FXSelector,void*);
  long onCmdCursorHome(FXObject*,FXSelector,void*);
  long onCmdCursorEnd(FXObject*,FXSelector,void*);
  long onCmdCursorRight(FXObject*,FXSelector,void*);
  long onCmdCursorLeft(FXObject*,FXSelector,void*);
  long onCmdCursorUp(FXObject*,FXSelector,void*);
  long onCmdCursorDown(FXObject*,FXSelector,void*);
  long onCmdCursorPageUp(FXObject*,FXSelector,void*);
  long onCmdCursorPageDown(FXObject*,FXSelector,void*);
  long onCmdCursorWordLeft(FXObject*,FXSelector,void*);
  long onCmdCursorWordRight(FXObject*,FXSelector,void*);

  // Cursor shift-drag movement
  long onCmdCursorShiftTop(FXObject*,FXSelector,void*);
  long onCmdCursorShiftBottom(FXObject*,FXSelector,void*);
  long onCmdCursorShiftHome(FXObject*,FXSelector,void*);
  long onCmdCursorShiftEnd(FXObject*,FXSelector,void*);
  long onCmdCursorShiftRight(FXObject*,FXSelector,void*);
  long onCmdCursorShiftLeft(FXObject*,FXSelector,void*);
  long onCmdCursorShiftUp(FXObject*,FXSelector,void*);
  long onCmdCursorShiftDown(FXObject*,FXSelector,void*);
  long onCmdCursorShiftPageUp(FXObject*,FXSelector,void*);
  long onCmdCursorShiftPageDown(FXObject*,FXSelector,void*);
  long onCmdCursorShiftWordLeft(FXObject*,FXSelector,void*);
  long onCmdCursorShiftWordRight(FXObject*,FXSelector,void*);

  // Cursor alt-drag movement
  long onCmdCursorAltUp(FXObject*,FXSelector,void*);
  long onCmdCursorAltDown(FXObject*,FXSelector,void*);
  long onCmdCursorAltLeft(FXObject*,FXSelector,void*);
  long onCmdCursorAltRight(FXObject*,FXSelector,void*);

  // Positioning
  long onCmdScrollUp(FXObject*,FXSelector,void*);
  long onCmdScrollDown(FXObject*,FXSelector,void*);
  long onCmdScrollTop(FXObject*,FXSelector,void*);
  long onCmdScrollBottom(FXObject*,FXSelector,void*);
  long onCmdScrollCenter(FXObject*,FXSelector,void*);

  // Inserting
  long onCmdInsertString(FXObject*,FXSelector,void*);
  long onCmdInsertNewline(FXObject*,FXSelector,void*);
  long onCmdInsertNewlineOnly(FXObject*,FXSelector,void*);
  long onCmdInsertNewlineIndent(FXObject*,FXSelector,void*);
  long onCmdInsertTab(FXObject*,FXSelector,void*);
  long onCmdInsertHardTab(FXObject*,FXSelector,void*);
  long onCmdInsertSoftTab(FXObject*,FXSelector,void*);

  // Manipulation Selection
  long onCmdCutSel(FXObject*,FXSelector,void*);
  long onCmdCopySel(FXObject*,FXSelector,void*);
  long onCmdPasteSel(FXObject*,FXSelector,void*);
  long onCmdPasteMiddle(FXObject*,FXSelector,void*);
  long onCmdDeleteSel(FXObject*,FXSelector,void*);
  long onCmdSelectChar(FXObject*,FXSelector,void*);
  long onCmdSelectWord(FXObject*,FXSelector,void*);
  long onCmdSelectLine(FXObject*,FXSelector,void*);
  long onCmdSelectMatching(FXObject*,FXSelector,void*);
  long onCmdSelectEnclosing(FXObject*,FXSelector,void*);
  long onCmdSelectAll(FXObject*,FXSelector,void*);
  long onCmdDeselectAll(FXObject*,FXSelector,void*);

  // Deletion
  long onCmdBackspaceChar(FXObject*,FXSelector,void*);
  long onCmdBackspaceWord(FXObject*,FXSelector,void*);
  long onCmdBackspaceBol(FXObject*,FXSelector,void*);
  long onCmdDeleteChar(FXObject*,FXSelector,void*);
  long onCmdDeleteWord(FXObject*,FXSelector,void*);
  long onCmdDeleteEol(FXObject*,FXSelector,void*);
  long onCmdDeleteAll(FXObject*,FXSelector,void*);
  long onCmdDeleteLine(FXObject*,FXSelector,void*);

  // Control commands
  long onCmdShiftText(FXObject*,FXSelector,void*);
  long onCmdChangeCase(FXObject*,FXSelector,void*);
  long onCmdCopyLine(FXObject*,FXSelector,void*);
  long onCmdMoveLineUp(FXObject*,FXSelector,void*);
  long onCmdMoveLineDown(FXObject*,FXSelector,void*);
  long onCmdJoinLines(FXObject*,FXSelector,void*);
  long onCmdBlockBeg(FXObject*,FXSelector,void*);
  long onCmdBlockEnd(FXObject*,FXSelector,void*);
  long onCmdGotoMatching(FXObject*,FXSelector,void*);
  long onCmdCursorPos(FXObject*,FXSelector,void*);
  long onUpdCursorPos(FXObject*,FXSelector,void*);
  long onCmdCursorRow(FXObject*,FXSelector,void*);
  long onUpdCursorRow(FXObject*,FXSelector,void*);
  long onCmdCursorColumn(FXObject*,FXSelector,void*);
  long onUpdCursorColumn(FXObject*,FXSelector,void*);
  long onCmdToggleEditable(FXObject*,FXSelector,void*);
  long onUpdToggleEditable(FXObject*,FXSelector,void*);
  long onCmdToggleOverstrike(FXObject*,FXSelector,void*);
  long onUpdToggleOverstrike(FXObject*,FXSelector,void*);
  long onUpdTextRows(FXObject*,FXSelector,void*);
  long onUpdTextSize(FXObject*,FXSelector,void*);
public:

  /// Internal style flags
  enum {
    STYLE_MASK      = 0x00FF,   // Mask color table
    STYLE_TEXT      = 0x0100,   // Draw some content
    STYLE_SELECTED  = 0x0200,   // Selected
    STYLE_CONTROL   = 0x0400,   // Control character
    STYLE_HILITE    = 0x0800,   // Highlighted
    STYLE_ACTIVE    = 0x1000,   // Active
    STYLE_INSERT    = 0x2000    // Column insert
    };

  /// Text highlight style flags
  enum {
    STYLE_UNDERLINE = 0x0001,   // Underline text
    STYLE_STRIKEOUT = 0x0002,   // Strike out text
    STYLE_BOLD      = 0x0004    // Bold text
    };

  /// Selection modes
  enum {
    SelectNone,                 // Select nothing
    SelectChars,                // Select characters
    SelectWords,                // Select words
    SelectRows,                 // Select rows
    SelectLines                 // Select lines
    };

  /// Default text delimiters
  static const FXchar textDelimiters[];

public:
  enum {
    ID_CURSOR_TOP=FXScrollArea::ID_LAST,
    ID_CURSOR_BOTTOM,
    ID_CURSOR_HOME,
    ID_CURSOR_END,
    ID_CURSOR_RIGHT,
    ID_CURSOR_LEFT,
    ID_CURSOR_UP,
    ID_CURSOR_DOWN,
    ID_CURSOR_PAGEUP,
    ID_CURSOR_PAGEDOWN,
    ID_CURSOR_WORD_LEFT,
    ID_CURSOR_WORD_RIGHT,
    ID_CURSOR_SHIFT_TOP,
    ID_CURSOR_SHIFT_BOTTOM,
    ID_CURSOR_SHIFT_HOME,
    ID_CURSOR_SHIFT_END,
    ID_CURSOR_SHIFT_UP,
    ID_CURSOR_SHIFT_DOWN,
    ID_CURSOR_SHIFT_LEFT,
    ID_CURSOR_SHIFT_RIGHT,
    ID_CURSOR_SHIFT_PAGEUP,
    ID_CURSOR_SHIFT_PAGEDOWN,
    ID_CURSOR_SHIFT_WORD_LEFT,
    ID_CURSOR_SHIFT_WORD_RIGHT,
    ID_CURSOR_ALT_UP,
    ID_CURSOR_ALT_DOWN,
    ID_CURSOR_ALT_LEFT,
    ID_CURSOR_ALT_RIGHT,
    ID_SCROLL_UP,
    ID_SCROLL_DOWN,
    ID_SCROLL_TOP,
    ID_SCROLL_BOTTOM,
    ID_SCROLL_CENTER,
    ID_INSERT_STRING,
    ID_INSERT_NEWLINE,
    ID_INSERT_NEWLINE_ONLY,
    ID_INSERT_NEWLINE_INDENT,
    ID_INSERT_TAB,
    ID_INSERT_HARDTAB,
    ID_INSERT_SOFTTAB,
    ID_CUT_SEL,
    ID_COPY_SEL,
    ID_DELETE_SEL,
    ID_PASTE_SEL,
    ID_PASTE_MIDDLE,
    ID_SELECT_CHAR,
    ID_SELECT_WORD,
    ID_SELECT_LINE,
    ID_SELECT_ALL,
    ID_SELECT_MATCHING,
    ID_SELECT_BRACE,
    ID_SELECT_BRACK,
    ID_SELECT_PAREN,
    ID_SELECT_ANG,
    ID_DESELECT_ALL,
    ID_BACKSPACE_CHAR,
    ID_BACKSPACE_WORD,
    ID_BACKSPACE_BOL,
    ID_DELETE_CHAR,
    ID_DELETE_WORD,
    ID_DELETE_EOL,
    ID_DELETE_ALL,
    ID_DELETE_LINE,
    ID_TOGGLE_EDITABLE,
    ID_TOGGLE_OVERSTRIKE,
    ID_CURSOR_POS,
    ID_CURSOR_ROW,
    ID_CURSOR_COLUMN,
    ID_JOIN_LINES,
    ID_SHIFT_LEFT,
    ID_SHIFT_RIGHT,
    ID_SHIFT_TABLEFT,
    ID_SHIFT_TABRIGHT,
    ID_CLEAN_INDENT,
    ID_COPY_LINE,
    ID_MOVE_LINE_UP,
    ID_MOVE_LINE_DOWN,
    ID_UPPER_CASE,
    ID_LOWER_CASE,
    ID_GOTO_MATCHING,
    ID_LEFT_BRACE,
    ID_LEFT_BRACK,
    ID_LEFT_PAREN,
    ID_LEFT_ANG,
    ID_RIGHT_BRACE,
    ID_RIGHT_BRACK,
    ID_RIGHT_PAREN,
    ID_RIGHT_ANG,
    ID_BLINK,
    ID_FLASH,
    ID_TEXT_ROWS,
    ID_TEXT_SIZE,
    ID_LAST
    };
public:

  /// Construct multi-line text widget
  FXText(FXComposite *p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=3,FXint pr=3,FXint pt=2,FXint pb=2);

  /// Create server-side resources
  virtual void create();

  /// Detach server-side resources
  virtual void detach();

  /// Get default width
  virtual FXint getContentWidth();

  /// Get default height
  virtual FXint getContentHeight();

  /// Return visible scroll-area x position
  virtual FXint getVisibleX() const;

  /// Return visible scroll-area y position
  virtual FXint getVisibleY() const;

  /// Return visible scroll-area width
  virtual FXint getVisibleWidth() const;

  /// Return visible scroll-area height
  virtual FXint getVisibleHeight() const;

  /// Return default width
  virtual FXint getDefaultWidth();

  /// Return default height
  virtual FXint getDefaultHeight();

  /// Perform layout
  virtual void layout();

  /// Enable the text widget
  virtual void enable();

  /// Disable the text widget
  virtual void disable();

  /// Need to recalculate size
  virtual void recalc();

  /// Returns true because a text widget can receive focus
  virtual FXbool canFocus() const;

  /// Move the focus to this window
  virtual void setFocus();

  /// Remove the focus from this window
  virtual void killFocus();

  /// Set modified flag
  void setModified(FXbool mod=true){ modified=mod; }

  /// Return true if text was modified
  FXbool isModified() const { return modified; }

  /// Set editable mode
  void setEditable(FXbool edit=true);

  /// Return true if text is editable
  FXbool isEditable() const;

  /// Set overstrike mode
  void setOverstrike(FXbool over=true);

  /// Return true if overstrike mode in effect
  FXbool isOverstrike() const;

  /// Return length of buffer
  FXint getLength() const { return length; }

  /// Return number of rows in buffer
  FXint getNumRows() const { return nrows; }

  /// Get byte at position in text buffer
  FXint getByte(FXint pos) const;

  /// Get wide character at position pos
  FXwchar getChar(FXint pos) const;

  /// Get length of wide character at position pos
  FXint getCharLen(FXint pos) const;

  /// Get style at position pos
  FXint getStyle(FXint pos) const;

  /// Retreat to the previous valid utf8 character start
  FXint dec(FXint pos) const;

  /// Advance to the next valid utf8 character start
  FXint inc(FXint pos) const;

  /// Return position of begin of line containing position pos
  FXint lineStart(FXint pos) const;

  /// Return position of end of line containing position pos
  FXint lineEnd(FXint pos) const;

  /// Return start of next line
  FXint nextLine(FXint pos,FXint nl=1) const;

  /// Return start of previous line
  FXint prevLine(FXint pos,FXint nl=1) const;

  /// Return row start
  FXint rowStart(FXint pos) const;

  /// Return row end
  FXint rowEnd(FXint pos) const;

  /// Return start of next row
  FXint nextRow(FXint pos,FXint nr=1) const;

  /// Return start of previous row
  FXint prevRow(FXint pos,FXint nr=1) const;

  /// Return end of previous word
  FXint leftWord(FXint pos) const;

  /// Return begin of next word
  FXint rightWord(FXint pos) const;

  /// Return begin of word
  FXint wordStart(FXint pos) const;

  /// Return end of word
  FXint wordEnd(FXint pos) const;

  /// Return validated utf8 character start position
  FXint validPos(FXint pos) const;

  /**
  * Count number of columns taken up by some text.
  * Start should be on a row start.
  */
  FXint countCols(FXint start,FXint end) const;

  /**
  * Count number of rows taken up by some text.
  * Start and end should be on a row start.
  */
  FXint countRows(FXint start,FXint end) const;

  /**
  * Count number of newlines.
  * Start should be on a line start.
  */
  FXint countLines(FXint start,FXint end) const;

  /// Change the text in the buffer to new text
  virtual FXint setText(const FXchar* text,FXint num,FXbool notify=false);
  virtual FXint setText(const FXString& text,FXbool notify=false);

  /// Change the text in the buffer to new text
  virtual FXint setStyledText(const FXchar* text,FXint num,FXint style=0,FXbool notify=false);
  virtual FXint setStyledText(const FXString& text,FXint style=0,FXbool notify=false);

  /// Change style of text range
  virtual FXint changeStyle(FXint pos,FXint num,FXint style);

  /// Change style of text range from style-array
  virtual FXint changeStyle(FXint pos,const FXchar* style,FXint num);
  virtual FXint changeStyle(FXint pos,const FXString& style);

  /// Append n bytes of text at the end of the buffer
  virtual FXint appendText(const FXchar *text,FXint num,FXbool notify=false);
  virtual FXint appendText(const FXString& text,FXbool notify=false);

  /// Append n bytes of text at the end of the buffer
  virtual FXint appendStyledText(const FXchar *text,FXint num,FXint style=0,FXbool notify=false);
  virtual FXint appendStyledText(const FXString& text,FXint style=0,FXbool notify=false);

  /// Replace del bytes at pos by ins characters of text
  virtual FXint replaceText(FXint pos,FXint del,const FXchar *text,FXint ins,FXbool notify=false);
  virtual FXint replaceText(FXint pos,FXint del,const FXString& text,FXbool notify=false);

  /// Replace del bytes at pos by ins characters of text
  virtual FXint replaceStyledText(FXint pos,FXint del,const FXchar *text,FXint ins,FXint style=0,FXbool notify=false);
  virtual FXint replaceStyledText(FXint pos,FXint del,const FXString& text,FXint style=0,FXbool notify=false);

  /// Replace text columns startcol to endcol in lines starting at startpos to endpos by new text
  virtual FXint replaceTextBlock(FXint startpos,FXint endpos,FXint startcol,FXint endcol,const FXchar *text,FXint num,FXbool notify=false);
  virtual FXint replaceTextBlock(FXint startpos,FXint endpos,FXint startcol,FXint endcol,const FXString& text,FXbool notify=false);

  /// Replace text columns startcol to endcol in lines starting at startpos to endpos by new text with given style
  virtual FXint replaceStyledTextBlock(FXint startpos,FXint endpos,FXint startcol,FXint endcol,const FXchar *text,FXint num,FXint style=0,FXbool notify=false);
  virtual FXint replaceStyledTextBlock(FXint startpos,FXint endpos,FXint startcol,FXint endcol,const FXString& text,FXint style=0,FXbool notify=false);

  /// Insert n bytes of text at position pos into the buffer
  virtual FXint insertText(FXint pos,const FXchar *text,FXint num,FXbool notify=false);
  virtual FXint insertText(FXint pos,const FXString& text,FXbool notify=false);

  /// Insert n bytes of text at position pos into the buffer
  virtual FXint insertStyledText(FXint pos,const FXchar *text,FXint num,FXint style=0,FXbool notify=false);
  virtual FXint insertStyledText(FXint pos,const FXString& text,FXint style=0,FXbool notify=false);

  /// Insert text columns at startcol in line starting at startpos to endpos
  virtual FXint insertTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXchar *text,FXint num,FXbool notify=false);
  virtual FXint insertTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXString& text,FXbool notify=false);

  /// Insert text columns at startcol in line starting at startpos to endpos with given style
  virtual FXint insertStyledTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXchar *text,FXint num,FXint style=0,FXbool notify=false);
  virtual FXint insertStyledTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXString& text,FXint style=0,FXbool notify=false);

  /// Overstrike text block
  virtual FXint overstrikeTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXchar *text,FXint num,FXbool notify=false);
  virtual FXint overstrikeTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXString& text,FXbool notify=false);

  /// Overstrike styled text block
  virtual FXint overstrikeStyledTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXchar *text,FXint num,FXint style=0,FXbool notify=false);
  virtual FXint overstrikeStyledTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXString& text,FXint style=0,FXbool notify=false);


  /// Return entire text as string
  FXString getText() const;

  /// Retrieve text into buffer
  void getText(FXchar* text,FXint num) const;

  /// Retrieve text into string
  void getText(FXString& text) const;

  /// Extract n bytes of text from position pos into already allocated buffer
  void extractText(FXchar *text,FXint pos,FXint num) const;

  /// Extract n bytes of text from position pos into string text
  void extractText(FXString& text,FXint pos,FXint num) const;

  /// Return n bytes of contents of text buffer from position pos
  FXString extractText(FXint pos,FXint num) const;

  /// Extract n bytes of style info from position pos into already allocated buffer
  void extractStyle(FXchar *style,FXint pos,FXint num) const;

  /// Extract n bytes of style info from position pos into string text
  void extractStyle(FXString& style,FXint pos,FXint num) const;

  /// Return n bytes of style info from buffer from position pos
  FXString extractStyle(FXint pos,FXint num) const;

  /// Extract text columns startcol to endcol from lines starting at startpos to endpos
  void extractTextBlock(FXString& text,FXint startpos,FXint endpos,FXint startcol,FXint endcol) const;

  /// Return text columns startcol to endcol from lines starting at startpos to endpos
  FXString extractTextBlock(FXint startpos,FXint endpos,FXint startcol,FXint endcol) const;


  /// Remove n bytes of text at position pos from the buffer
  virtual FXint removeText(FXint pos,FXint num,FXbool notify=false);

  /// Remove columns startcol to endcol from lines starting at startpos to endpos
  virtual FXint removeTextBlock(FXint startpos,FXint endpos,FXint startcol,FXint endcol,FXbool notify=false);

  /// Remove all text from the buffer
  virtual FXint clearText(FXbool notify=false);


  /// Select all text
  virtual FXbool selectAll(FXbool notify=false);

  /// Select range of len characters starting at given position pos
  virtual FXbool setSelection(FXint pos,FXint len,FXbool notify=false);

  /// Extend the primary selection from the anchor to the given position
  virtual FXbool extendSelection(FXint pos,FXuint sel=SelectChars,FXbool notify=false);

  /// Select block of characters within given box
  virtual FXbool setBlockSelection(FXint trow,FXint lcol,FXint brow,FXint rcol,FXbool notify=false);

  /// Extend primary selection from anchor to given row, column
  virtual FXbool extendBlockSelection(FXint row,FXint col,FXbool notify=false);

  /// Kill or deselect primary selection
  virtual FXbool killSelection(FXbool notify=false);


  /// Return true if position pos is selected
  FXbool isPosSelected(FXint pos) const;

  /// Return true if position pos (and column col) is selected
  FXbool isPosSelected(FXint pos,FXint col) const;

  /// Return selection start position
  FXint getSelStartPos() const { return select.startpos; }

  /// Return selection end position
  FXint getSelEndPos() const { return select.endpos; }

  /// Return selection start column
  FXint getSelStartColumn() const { return select.startcol; }

  /// Return selection end column
  FXint getSelEndColumn() const { return select.endcol; }


  /// Copy primary selection to clipboard
  FXbool copySelection();

  /// Cut primary selection to clipboard
  FXbool cutSelection(FXbool notify=false);

  /// Delete primary selection
  FXbool deleteSelection(FXbool notify=false);

  /// Paste primary ("middle-mouse") selection
  FXbool pasteSelection(FXbool notify=false);

  /// Paste clipboard
  FXbool pasteClipboard(FXbool notify=false);

  /// Replace primary selection by other text
  FXbool replaceSelection(const FXString& text,FXbool notify=false);

  /// Get selected text
  FXString getSelectedText() const;

  /// Highlight len characters starting at given position pos
  FXbool setHighlight(FXint start,FXint len);

  /// Unhighlight the text
  FXbool killHighlight();


  /// Make line containing pos the top line
  void setTopLine(FXint pos);

  /// Return position of top line
  FXint getTopLine() const;

  /// Make line containing pos the bottom line
  void setBottomLine(FXint pos);

  /// Return the position of the bottom line
  FXint getBottomLine() const;

  /// Make line containing pos the center line
  void setCenterLine(FXint pos);

  /// Return true if line containing position is fully visible
  FXbool isPosVisible(FXint pos) const;

  /// Scroll text to make the given position visible
  void makePositionVisible(FXint pos);


  /// Return text position at given visible x,y coordinate
  FXint getPosAt(FXint x,FXint y) const;

  /// Return text position containing x, y coordinate
  FXint getPosContaining(FXint x,FXint y) const;

  /// Return screen x-coordinate of pos
  FXint getXOfPos(FXint pos) const;

  /// Return screen y-coordinate of pos
  FXint getYOfPos(FXint pos) const;

  /**
  * Return closest position and (row,col) of given x,y coordinate.
  * The (row,col) is unconstrained, i.e. calculated as if tabs and
  * area past the newline is comprised of spaces; the returned position
  * however is inside the text.
  * Note that when using proportional fonts, the width of a logical space
  * inside a tab is variable, to account for the logical columns in a tab.
  */
  FXint getRowColumnAt(FXint x,FXint y,FXint& row,FXint& col) const;

  /// Return screen x-coordinate of unconstrained (row,col).
  FXint getXOfRowColumn(FXint row,FXint col) const;

  /// Return screen y-coordinate of unconstrained (row,col).
  FXint getYOfRowColumn(FXint row,FXint col) const;


  /// Set the cursor position
  virtual void setCursorPos(FXint pos,FXbool notify=false);

  /// Return the cursor position
  FXint getCursorPos() const { return cursorpos; }

  /// Set cursor row, column
  void setCursorRowColumn(FXint row,FXint col,FXbool notify=false);

  /// Set cursor row
  void setCursorRow(FXint row,FXbool notify=false);

  /// Return cursor row
  FXint getCursorRow() const { return cursorrow; }

  /// Set cursor column
  void setCursorColumn(FXint col,FXbool notify=false);

  /// Return cursor row, i.e. indent position
  FXint getCursorColumn() const { return cursorcol; }


  /// Set the anchor position
  void setAnchorPos(FXint pos);

  /// Return the anchor position
  FXint getAnchorPos() const { return anchorpos; }

  /// Set anchor row and column
  void setAnchorRowColumn(FXint row,FXint col);

  /// Return anchor row
  FXint getAnchorRow() const { return anchorrow; }

  /// Return anchor row
  FXint getAnchorColumn() const { return anchorcol; }


  /// Move cursor to position, and scroll into view
  void moveCursor(FXint pos,FXbool notify=false);

  /// Move cursor to row and column, and scroll into view
  void moveCursorRowColumn(FXint row,FXint col,FXbool notify=false);

  /// Move cursor to position, and extend the selection to this point
  void moveCursorAndSelect(FXint pos,FXuint sel,FXbool notify=false);

  /// Move cursor to row and column, and extend the block selection to this point
  void moveCursorRowColumnAndSelect(FXint row,FXint col,FXbool notify=false);


  /**
  * Search for string in text buffer, returning the extent of the string in beg and end.
  * The search starts from the given starting position, scans forward (SEARCH_FORWARD) or
  * backward (SEARCH_BACKWARD), and wraps around if SEARCH_WRAP has been specified.
  * If neither SEARCH_FORWARD or SEARCH_BACKWARD flags are set, an anchored match is performed
  * at the given starting position.
  * The search type is either a plain search (SEARCH_EXACT), case insensitive search
  * (SEARCH_IGNORECASE), or regular expression search (SEARCH_REGEX).
  * For regular expression searches, capturing parentheses are used if npar is greater than 1;
  * in this case, the number of entries in the beg[], end[] arrays must be npar also.
  * If either beg or end or both are NULL, internal arrays are used.
  */
  FXbool findText(const FXString& string,FXint* beg=nullptr,FXint* end=nullptr,FXint start=0,FXuint flags=SEARCH_FORWARD|SEARCH_WRAP|SEARCH_EXACT,FXint npar=1);


  /// Change text widget style
  void setTextStyle(FXuint style);

  /// Return text widget style
  FXuint getTextStyle() const;

  /// Change text font
  void setFont(FXFont* fnt);

  /// Return text font
  FXFont* getFont() const { return font; }

  /// Change number of visible rows
  void setVisibleRows(FXint rows);

  /// Return number of visible rows
  FXint getVisibleRows() const { return vrows; }

  /// Change number of visible columns
  void setVisibleColumns(FXint cols);

  /// Return number of visible columns
  FXint getVisibleColumns() const { return vcols; }

  /// Change top margin
  void setMarginTop(FXint pt);

  /// Return top margin
  FXint getMarginTop() const { return margintop; }

  /// Change bottom margin
  void setMarginBottom(FXint pb);

  /// Return bottom margin
  FXint getMarginBottom() const { return marginbottom; }

  /// Change left margin
  void setMarginLeft(FXint pl);

  /// Return left margin
  FXint getMarginLeft() const { return marginleft; }

  /// Change right margin
  void setMarginRight(FXint pr);

  /// Return right margin
  FXint getMarginRight() const { return marginright; }

  /// Return number of columns used for line numbers
  FXint getBarColumns() const { return barcolumns; }

  /// Change number of columns used for line numbers
  void setBarColumns(FXint cols);

  /// Set wrap columns
  void setWrapColumns(FXint cols);

  /// Return wrap columns
  FXint getWrapColumns() const { return wrapcolumns; }

  /// Change tab columns
  void setTabColumns(FXint cols);

  /// Return tab columns
  FXint getTabColumns() const { return tabcolumns; }

  /// Change text color
  void setTextColor(FXColor clr);

  /// Return text color
  FXColor getTextColor() const { return textColor; }

  /// Change selected background color
  void setSelBackColor(FXColor clr);

  /// Return selected background color
  FXColor getSelBackColor() const { return selbackColor; }

  /// Change selected text color
  void setSelTextColor(FXColor clr);

  /// Return selected text color
  FXColor getSelTextColor() const { return seltextColor; }

  /// Change highlighted text color
  void setHiliteTextColor(FXColor clr);

  /// Return highlighted text color
  FXColor getHiliteTextColor() const { return hilitetextColor; }

  /// Change highlighted background color
  void setHiliteBackColor(FXColor clr);

  /// Return highlighted background color
  FXColor getHiliteBackColor() const { return hilitebackColor; }

  /// Change active background color
  void setActiveBackColor(FXColor clr);

  /// Return active background color
  FXColor getActiveBackColor() const { return activebackColor; }

  /// Change cursor color
  void setCursorColor(FXColor clr);

  /// Return cursor color
  FXColor getCursorColor() const { return cursorColor; }

  /// Change line number color
  void setNumberColor(FXColor clr);

  /// Return line number color
  FXColor getNumberColor() const { return numberColor; }

  /// Change bar color
  void setBarColor(FXColor clr);

  /// Return bar color
  FXColor getBarColor() const { return barColor; }

  /// Set styled text mode; return true if success
  FXbool setStyled(FXbool styled=true);

  /// Return true if style buffer
  FXbool isStyled() const { return (sbuffer!=nullptr); }

  /**
  * Set highlight styles.
  * The table of styles is only referenced by the widget; it is not copied.
  * Thus, multiple widgets may share a common style table.
  * Some care must be taken to populate the style-buffer only with numbers
  * inside the style table.
  */
  void setHiliteStyles(FXHiliteStyle* styles);

  /// Return current value of the style table.
  FXHiliteStyle* getHiliteStyles() const { return hilitestyles; }

  /**
  * Change brace and parenthesis match highlighting time, in nanoseconds.
  * If highlighting for a small interval, only flash if the matching brace
  * is visible. If the highlighting interval is set to forever, highlighting
  * stays on till cursor moves, and the brace is highlighted even if not
  * (yet) visible.  Note that this may be expensive as a large part of the
  * text buffer could be visited to find the matching brace.
  */
  void setHiliteMatchTime(FXTime t){ matchtime=t; }

  /// Return brace and parenthesis match highlighting time, in nanoseconds
  FXTime getHiliteMatchTime() const { return matchtime; }

  /// Change delimiters of words
  void setDelimiters(const FXchar* delims=textDelimiters){ delimiters=delims; }

  /// Return word delimiters
  const FXchar* getDelimiters() const { return delimiters; }

  /// Set help text
  void setHelpText(const FXString& text){ help=text; }

  /// Return help text
  FXString getHelpText() const { return help; }

  /// Set the tool tip message for this text widget
  void setTipText(const FXString& text){ tip=text; }

  /// Get the tool tip message for this text widget
  FXString getTipText() const { return tip; }

  /// Save to a stream
  virtual void save(FXStream& store) const;

  /// Load from a stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXText();
  };

}

#endif
