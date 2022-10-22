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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "fxkeys.h"
#include "fxchar.h"
#include "fxascii.h"
#include "fxunicode.h"
#include "FXColors.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXElement.h"
#include "FXException.h"
#include "FXRex.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXObject.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXGIFIcon.h"
#include "FXScrollBar.h"
#include "FXText.h"
#include "FXComposeContext.h"
#include "icons.h"



/*
  Notes:
  - Generally, assume the following definitions in terms of how things work:

      position  Character position in the buffer; should avoid pointing to
                places other than the start of a UTF8 character.
      indent    logical character-index (not byte index) from the start of
                a line.
      line      A newline terminated sequence of characters. A line may be wrapped
                to multiple rows on the screen.
      row       Sequence of characters wrapped at the wrap-margin, therefore not
                necessarily ending at a newline
      column    Logical column from start of the line.

  - Line start array is one longer than number of visible lines; therefore,
    the length of each visible line is just visrows[i+1]-visrows[i].
  - Control characters in the buffer are allowed, and are represented as sequences
    like '^L'.
  - Wrapped lines contain at least 1 character; this is necessary in order to ensure
    the text widget has finite number of rows.
  - Viewport definition is established with virtual functions; overloadinging them
    in subclasses allows for custom items in subclassed FXText widgets.

      +------------------------------------------------+<-- 0
      |                                                |
      +----+--------------------------------------+----+<-- getVisibleY()
      |    |                                      |    |
      |    |           T e x t                    |    |
      |    |                                      |    |
      |    |                                      |    |
      +----+--------------------------------------+----+<-- getVisibleHeight()
      |                                                |
      +------------------------------------------------+<-- height
      ^    ^                                      ^    ^
      |    |                                      |    |
      0    |                                      |    width
         getVisibleX()             getVisibleWidth()

  - Buffer layout:

      Content  :  A  B  C  .  .  .  .  .  .  .  .  D  E  F  G
      Position :  0  1  2                          3  4  5  6    length=7
      Addresss :  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14    buffersize=7+11-3=15
                           ^                       ^
                           |                       |
                           gapstart=3              gapend=11     gaplen=11-3=8

    The gap is moved around the buffer so newly added text can be entered into the gap;
    when the gap becomes too small, the buffer is resized.
    This gapped-buffer technique minimizes the number of resizes of the buffer, and
    minimizes the number of block moves.

    The tail end of the visrows array will look like:

      visrow[0]= 0: "Q R S T U V W \n"
      visrow[1]= 8: "X Y Z"
      visrow[2]=11: <no text>
      visrow[3]=11: <no text>            length = 11

    The last legal position is length = 11.

  - While resizing window, we keep track of a position which should remain visible at the
    top of the visible buffer (keeppos).  Due to wrapping, the exact value of toppos may
    change but its always the case that keeppos is visible.
  - When changing text, try keep the top part of the visible buffer stationary, to avoid
    jumping text while typing.
  - If there is a style table, the style buffer is used as index into the style table,
    allowing for up to 255 styles (style index==0 is the default style).
    The style member in the FXHiliteStyle struct is used for underlining, strikeouts,
    and other effects.
  - Italic fonts are bit problematic on border between selected/unselected text
    due to kerning.
  - Possibly split off buffer management into separate text buffer class (allows for
    multiple views).
  - Maybe put all keyboard bindings into accelerator table.
  - Perhaps change text and style buffer to FXString for further complexity reduction.
  - When in overstrike mode and having a selection, entering a character should
    replace the selection, not delete the selection and then overstrike the character
    after the selection.
  - When pasting or dropping whole lines, insert at begin of line instead of at cursor;
    question:- how to know we're pasting whole lines?
  - Inserting lots of stuff should show cursor.
  - For now, right, top, and bottom bars are zero; subclasses may override
    and add space for text annotations.
  - Possible (minor) improvement to wrap(): don't break after space unless
    at least non-space was seen before that space.  This will cause a line
    to have at least some non-blank characters on it.
*/

#define TOPIC_KEYBOARD  1009
#define TOPIC_TEXT      1012
#define TOPIC_LAYOUT    1013

#define MINSIZE         100             // Minimum gap size
#define MAXSIZE         4000            // Minimum gap size
#define NVISROWS        20              // Initial visible rows
#define MAXTABCOLUMNS   32              // Maximum tab column setting

#define TEXT_MASK       (TEXT_FIXEDWRAP|TEXT_WORDWRAP|TEXT_OVERSTRIKE|TEXT_READONLY|TEXT_NO_TABS|TEXT_AUTOINDENT|TEXT_SHOWACTIVE|TEXT_SHOWMATCH)

#define CC(x,in)        (((x)=='\t')?tabcolumns-in%tabcolumns:1)        // Count Columns

using namespace FX;

/*******************************************************************************/

namespace FX {


// Furnish our own version
extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);


// Map
FXDEFMAP(FXText) FXTextMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXText::onPaint),
  FXMAPFUNC(SEL_MOTION,0,FXText::onMotion),
  FXMAPFUNC(SEL_DRAGGED,0,FXText::onDragged),
  FXMAPFUNC(SEL_ENTER,0,FXText::onEnter),
  FXMAPFUNC(SEL_LEAVE,0,FXText::onLeave),
  FXMAPFUNC(SEL_TIMEOUT,FXText::ID_BLINK,FXText::onBlink),
  FXMAPFUNC(SEL_TIMEOUT,FXText::ID_FLASH,FXText::onFlash),
  FXMAPFUNC(SEL_TIMEOUT,FXText::ID_TIPTIMER,FXText::onTipTimer),
  FXMAPFUNC(SEL_TIMEOUT,FXText::ID_AUTOSCROLL,FXText::onAutoScroll),
  FXMAPFUNC(SEL_FOCUSIN,0,FXText::onFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT,0,FXText::onFocusOut),
  FXMAPFUNC(SEL_BEGINDRAG,0,FXText::onBeginDrag),
  FXMAPFUNC(SEL_ENDDRAG,0,FXText::onEndDrag),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXText::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXText::onLeftBtnRelease),
  FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,0,FXText::onMiddleBtnPress),
  FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE,0,FXText::onMiddleBtnRelease),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,0,FXText::onRightBtnPress),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,0,FXText::onRightBtnRelease),
  FXMAPFUNC(SEL_UNGRABBED,0,FXText::onUngrabbed),
  FXMAPFUNC(SEL_DND_ENTER,0,FXText::onDNDEnter),
  FXMAPFUNC(SEL_DND_LEAVE,0,FXText::onDNDLeave),
  FXMAPFUNC(SEL_DND_DROP,0,FXText::onDNDDrop),
  FXMAPFUNC(SEL_DND_MOTION,0,FXText::onDNDMotion),
  FXMAPFUNC(SEL_DND_REQUEST,0,FXText::onDNDRequest),
  FXMAPFUNC(SEL_SELECTION_LOST,0,FXText::onSelectionLost),
  FXMAPFUNC(SEL_SELECTION_GAINED,0,FXText::onSelectionGained),
  FXMAPFUNC(SEL_SELECTION_REQUEST,0,FXText::onSelectionRequest),
  FXMAPFUNC(SEL_CLIPBOARD_LOST,0,FXText::onClipboardLost),
  FXMAPFUNC(SEL_CLIPBOARD_GAINED,0,FXText::onClipboardGained),
  FXMAPFUNC(SEL_CLIPBOARD_REQUEST,0,FXText::onClipboardRequest),
  FXMAPFUNC(SEL_KEYPRESS,0,FXText::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXText::onKeyRelease),
  FXMAPFUNC(SEL_QUERY_TIP,0,FXText::onQueryTip),
  FXMAPFUNC(SEL_QUERY_HELP,0,FXText::onQueryHelp),
  FXMAPFUNC(SEL_IME_START,0,FXText::onIMEStart),
  FXMAPFUNC(SEL_UPDATE,FXText::ID_TOGGLE_EDITABLE,FXText::onUpdToggleEditable),
  FXMAPFUNC(SEL_UPDATE,FXText::ID_TOGGLE_OVERSTRIKE,FXText::onUpdToggleOverstrike),
  FXMAPFUNC(SEL_UPDATE,FXText::ID_CURSOR_ROW,FXText::onUpdCursorRow),
  FXMAPFUNC(SEL_UPDATE,FXText::ID_CURSOR_COLUMN,FXText::onUpdCursorColumn),
  FXMAPFUNC(SEL_UPDATE,FXText::ID_CUT_SEL,FXText::onUpdHaveEditableSelection),
  FXMAPFUNC(SEL_UPDATE,FXText::ID_COPY_SEL,FXText::onUpdHaveSelection),
  FXMAPFUNC(SEL_UPDATE,FXText::ID_PASTE_SEL,FXText::onUpdIsEditable),
  FXMAPFUNC(SEL_UPDATE,FXText::ID_DELETE_SEL,FXText::onUpdHaveEditableSelection),
  FXMAPFUNC(SEL_UPDATE,FXText::ID_CLEAN_INDENT,FXText::onUpdHaveEditableSelection),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_TOP,FXText::onCmdCursorTop),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_BOTTOM,FXText::onCmdCursorBottom),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_HOME,FXText::onCmdCursorHome),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_END,FXText::onCmdCursorEnd),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_UP,FXText::onCmdCursorUp),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_DOWN,FXText::onCmdCursorDown),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_LEFT,FXText::onCmdCursorLeft),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_RIGHT,FXText::onCmdCursorRight),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_PAGEUP,FXText::onCmdCursorPageUp),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_PAGEDOWN,FXText::onCmdCursorPageDown),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_WORD_LEFT,FXText::onCmdCursorWordLeft),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_WORD_RIGHT,FXText::onCmdCursorWordRight),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_SHIFT_TOP,FXText::onCmdCursorShiftTop),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_SHIFT_BOTTOM,FXText::onCmdCursorShiftBottom),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_SHIFT_HOME,FXText::onCmdCursorShiftHome),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_SHIFT_END,FXText::onCmdCursorShiftEnd),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_SHIFT_UP,FXText::onCmdCursorShiftUp),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_SHIFT_DOWN,FXText::onCmdCursorShiftDown),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_SHIFT_LEFT,FXText::onCmdCursorShiftLeft),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_SHIFT_RIGHT,FXText::onCmdCursorShiftRight),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_SHIFT_PAGEUP,FXText::onCmdCursorShiftPageUp),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_SHIFT_PAGEDOWN,FXText::onCmdCursorShiftPageDown),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_SHIFT_WORD_LEFT,FXText::onCmdCursorShiftWordLeft),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_SHIFT_WORD_RIGHT,FXText::onCmdCursorShiftWordRight),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_ALT_UP,FXText::onCmdCursorAltUp),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_ALT_DOWN,FXText::onCmdCursorAltDown),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_ALT_LEFT,FXText::onCmdCursorAltLeft),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_ALT_RIGHT,FXText::onCmdCursorAltRight),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_SCROLL_UP,FXText::onCmdScrollUp),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_SCROLL_DOWN,FXText::onCmdScrollDown),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_SCROLL_TOP,FXText::onCmdScrollTop),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_SCROLL_BOTTOM,FXText::onCmdScrollBottom),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_SCROLL_CENTER,FXText::onCmdScrollCenter),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_INSERT_STRING,FXText::onCmdInsertString),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_INSERT_NEWLINE,FXText::onCmdInsertNewline),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_INSERT_NEWLINE_ONLY,FXText::onCmdInsertNewlineOnly),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_INSERT_NEWLINE_INDENT,FXText::onCmdInsertNewlineIndent),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_INSERT_TAB,FXText::onCmdInsertTab),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_INSERT_HARDTAB,FXText::onCmdInsertHardTab),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_INSERT_SOFTTAB,FXText::onCmdInsertSoftTab),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CUT_SEL,FXText::onCmdCutSel),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_COPY_SEL,FXText::onCmdCopySel),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_DELETE_SEL,FXText::onCmdDeleteSel),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_PASTE_SEL,FXText::onCmdPasteSel),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_PASTE_MIDDLE,FXText::onCmdPasteMiddle),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_SELECT_CHAR,FXText::onCmdSelectChar),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_SELECT_WORD,FXText::onCmdSelectWord),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_SELECT_LINE,FXText::onCmdSelectLine),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_SELECT_ALL,FXText::onCmdSelectAll),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_DESELECT_ALL,FXText::onCmdDeselectAll),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_BACKSPACE_CHAR,FXText::onCmdBackspaceChar),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_BACKSPACE_WORD,FXText::onCmdBackspaceWord),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_BACKSPACE_BOL,FXText::onCmdBackspaceBol),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_DELETE_CHAR,FXText::onCmdDeleteChar),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_DELETE_WORD,FXText::onCmdDeleteWord),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_DELETE_EOL,FXText::onCmdDeleteEol),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_DELETE_ALL,FXText::onCmdDeleteAll),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_DELETE_LINE,FXText::onCmdDeleteLine),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_TOGGLE_EDITABLE,FXText::onCmdToggleEditable),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_TOGGLE_OVERSTRIKE,FXText::onCmdToggleOverstrike),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_ROW,FXText::onCmdCursorRow),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CURSOR_COLUMN,FXText::onCmdCursorColumn),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_SETSTRINGVALUE,FXText::onCmdSetStringValue),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_GETSTRINGVALUE,FXText::onCmdGetStringValue),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_UPPER_CASE,FXText::onCmdChangeCase),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_LOWER_CASE,FXText::onCmdChangeCase),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_JOIN_LINES,FXText::onCmdJoinLines),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_GOTO_MATCHING,FXText::onCmdGotoMatching),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_SELECT_MATCHING,FXText::onCmdSelectMatching),
  FXMAPFUNCS(SEL_COMMAND,FXText::ID_SELECT_BRACE,FXText::ID_SELECT_ANG,FXText::onCmdSelectEnclosing),
  FXMAPFUNCS(SEL_COMMAND,FXText::ID_LEFT_BRACE,FXText::ID_LEFT_ANG,FXText::onCmdBlockBeg),
  FXMAPFUNCS(SEL_COMMAND,FXText::ID_RIGHT_BRACE,FXText::ID_RIGHT_ANG,FXText::onCmdBlockEnd),
  FXMAPFUNCS(SEL_COMMAND,FXText::ID_SHIFT_LEFT,FXText::ID_SHIFT_TABRIGHT,FXText::onCmdShiftText),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_COPY_LINE,FXText::onCmdCopyLine),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_MOVE_LINE_UP,FXText::onCmdMoveLineUp),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_MOVE_LINE_DOWN,FXText::onCmdMoveLineDown),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_CLEAN_INDENT,FXText::onCmdShiftText),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_SETHELPSTRING,FXText::onCmdSetHelp),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_GETHELPSTRING,FXText::onCmdGetHelp),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_SETTIPSTRING,FXText::onCmdSetTip),
  FXMAPFUNC(SEL_COMMAND,FXText::ID_GETTIPSTRING,FXText::onCmdGetTip),
  };


// Object implementation
FXIMPLEMENT(FXText,FXScrollArea,FXTextMap,ARRAYNUMBER(FXTextMap))


// Delimiters
const FXchar FXText::textDelimiters[]="~.,/\\`'!@#$%^&*()-=+{}|[]\":;<>?";

// Matching things
static const FXchar lefthand[]="{[(<";
static const FXchar righthand[]="}])>";

// Spaces, lots of spaces
static const FXchar spaces[MAXTABCOLUMNS+1]="                                ";

/*******************************************************************************/


// For deserialization
FXText::FXText(){
  flags|=FLAG_ENABLED|FLAG_DROPTARGET;
  buffer=nullptr;
  sbuffer=nullptr;
  visrows=nullptr;
  length=0;
  nvisrows=0;
  nrows=1;
  gapstart=0;
  gapend=0;
  toppos=0;
  toprow=0;
  keeppos=0;
  select.startpos=0;
  select.endpos=-1;
  select.startcol=0;
  select.endcol=-1;
  hilite.startpos=0;
  hilite.endpos=-1;
  hilite.startcol=0;
  hilite.endcol=-1;
  anchorpos=0;
  anchorrow=0;
  anchorcol=0;
  anchorvcol=0;
  cursorpos=0;
  cursorrow=0;
  cursorcol=0;
  cursorvcol=0;
  prefcol=-1;
  margintop=0;
  marginbottom=0;
  marginleft=0;
  marginright=0;
  wrapwidth=80;
  wrapcolumns=80;
  tabwidth=8;
  tabcolumns=8;
  barwidth=0;
  barcolumns=0;
  font=nullptr;
  textColor=0;
  selbackColor=0;
  seltextColor=0;
  hilitebackColor=0;
  hilitetextColor=0;
  activebackColor=0;
  numberColor=0;
  cursorColor=0;
  barColor=0;
  textWidth=0;
  textHeight=0;
  delimiters=textDelimiters;
  vrows=0;
  vcols=0;
  hilitestyles=nullptr;
  blink=FLAG_CARET;
  matchtime=0;
  grabx=0;
  graby=0;
  mode=MOUSE_NONE;
  modified=false;
  }


// Text widget
FXText::FXText(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXScrollArea(p,opts,x,y,w,h){
  flags|=FLAG_ENABLED|FLAG_DROPTARGET;
  target=tgt;
  message=sel;
  callocElms(buffer,MINSIZE);
  sbuffer=nullptr;
  callocElms(visrows,NVISROWS+1);
  length=0;
  nrows=1;
  nvisrows=NVISROWS;
  gapstart=0;
  gapend=MINSIZE;
  toppos=0;
  toprow=0;
  keeppos=0;
  select.startpos=0;
  select.endpos=-1;
  select.startcol=0;
  select.endcol=-1;
  hilite.startpos=0;
  hilite.endpos=-1;
  hilite.startcol=0;
  hilite.endcol=-1;
  anchorpos=0;
  anchorrow=0;
  anchorcol=0;
  anchorvcol=0;
  cursorpos=0;
  cursorrow=0;
  cursorcol=0;
  cursorvcol=0;
  prefcol=-1;
  margintop=pt;
  marginbottom=pb;
  marginleft=pl;
  marginright=pr;
  wrapwidth=80;
  wrapcolumns=80;
  tabwidth=8;
  tabcolumns=8;
  barwidth=0;
  barcolumns=0;
  font=getApp()->getNormalFont();
  hilitestyles=nullptr;
  blink=FLAG_CARET;
  defaultCursor=getApp()->getDefaultCursor(DEF_TEXT_CURSOR);
  dragCursor=getApp()->getDefaultCursor(DEF_TEXT_CURSOR);
  textColor=getApp()->getForeColor();
  selbackColor=getApp()->getSelbackColor();
  seltextColor=getApp()->getSelforeColor();
  hilitebackColor=FXRGB(255,128,128);
  hilitetextColor=getApp()->getForeColor();
  activebackColor=backColor;
  numberColor=textColor;
  cursorColor=getApp()->getForeColor();
  barColor=backColor;
  textWidth=0;
  textHeight=0;
  delimiters=textDelimiters;
  vrows=0;
  vcols=0;
  matchtime=0;
  grabx=0;
  graby=0;
  mode=MOUSE_NONE;
  modified=false;
  }


// Create window
void FXText::create(){
  FXScrollArea::create();
  font->create();
  tabwidth=tabcolumns*font->getTextWidth(" ",1);
  barwidth=barcolumns*font->getTextWidth("8",1);
  recalc();
  }


// Detach window
void FXText::detach(){
  FXScrollArea::detach();
  font->detach();
  }


// If window can have focus
FXbool FXText::canFocus() const {
  return true;
  }


// Into focus chain
void FXText::setFocus(){
  FXScrollArea::setFocus();
  setDefault(true);
  flags&=~FLAG_UPDATE;
  if(getApp()->hasInputMethod()){
    createComposeContext();
    getComposeContext()->setFont(font);
    getComposeContext()->focusIn();
    }
  }


// Enable the window
void FXText::enable(){
  if(!(flags&FLAG_ENABLED)){
    FXScrollArea::enable();
    update();
    }
  }


// Disable the window
void FXText::disable(){
  if(flags&FLAG_ENABLED){
    FXScrollArea::disable();
    update();
    }
  }


// Out of focus chain
void FXText::killFocus(){
  FXScrollArea::killFocus();
  setDefault(maybe);
  flags|=FLAG_UPDATE;
  if(getApp()->hasInputMethod()){
    destroyComposeContext();
    }
  }


// Return true if editable
FXbool FXText::isEditable() const {
  return (options&TEXT_READONLY)==0;
  }


// Set widget is editable or not
void FXText::setEditable(FXbool edit){
  options^=((edit-1)^options)&TEXT_READONLY;
  }


// Return true if text is in overstrike mode
FXbool FXText::isOverstrike() const {
  return (options&TEXT_OVERSTRIKE)!=0;
  }


// Set overstrike mode
void FXText::setOverstrike(FXbool over){
  options^=((0-over)^options)&TEXT_OVERSTRIKE;
  }


// Check if w is delimiter
FXbool FXText::isdelimiter(FXwchar w) const {
  FXchar wcs[5]={'\0','\0','\0','\0','\0'};
  if(128<=w){
    wc2utf(wcs,w);
    return (strstr(delimiters,wcs)!=nullptr);
    }
  return (strchr(delimiters,w)!=nullptr);
  }

/*******************************************************************************/

// Move the gap; gap is never moved inside utf character.
// Afterward, new characters may be inserted at position simply
// by dropping them at the start of the gap, thereby shrinking
// the gap as each character is inserted.  No big block moves
// are required until the gap has become too small for the next
// insert operation.
void FXText::movegap(FXint pos){
  FXint gaplen=gapend-gapstart;
  FXASSERT(0<=pos && pos<=length);
  FXASSERT(0<=gapstart && gapstart<=length);
  if(gapstart<pos){
    moveElms(&buffer[gapstart],&buffer[gapend],pos-gapstart);
    if(sbuffer){moveElms(&sbuffer[gapstart],&sbuffer[gapend],pos-gapstart);}
    gapend=pos+gaplen;
    gapstart=pos;
    }
  else if(pos<gapstart){
    moveElms(&buffer[pos+gaplen],&buffer[pos],gapstart-pos);
    if(sbuffer){moveElms(&sbuffer[pos+gaplen],&sbuffer[pos],gapstart-pos);}
    gapend=pos+gaplen;
    gapstart=pos;
    }
  }


// Grow or shrink the gap size, keeping gap start at the same position.
void FXText::sizegap(FXint sz){
  FXint gaplen=gapend-gapstart;
  FXASSERT(0<=sz);
  FXASSERT(0<=gapstart && gapstart<=length);
  if(sz>gaplen){
    if(!resizeElms(buffer,length+sz)){ fxerror("%s::sizegap: out of memory.\n",getClassName()); }
    moveElms(&buffer[gapstart+sz],&buffer[gapend],length-gapstart);
    if(sbuffer){
      if(!resizeElms(sbuffer,length+sz)){ fxerror("%s::sizegap: out of memory.\n",getClassName()); }
      moveElms(&sbuffer[gapstart+sz],&sbuffer[gapend],length-gapstart);
      }
    gapend=gapstart+sz;
    }
  else if(sz<gaplen){
    moveElms(&buffer[gapstart+sz],&buffer[gapend],length-gapstart);
    if(!resizeElms(buffer,length+sz)){ fxerror("%s::sizegap: out of memory.\n",getClassName()); }
    if(sbuffer){
      moveElms(&sbuffer[gapstart+sz],&sbuffer[gapend],length-gapstart);
      if(!resizeElms(sbuffer,length+sz)){ fxerror("%s::sizegap: out of memory.\n",getClassName()); }
      }
    gapend=gapstart+sz;
    }
  }

/*******************************************************************************/

// Get byte
FXint FXText::getByte(FXint pos) const {
  FXASSERT(0<=pos && pos<=length);
  return (FXuchar)buffer[pos+((gapend-gapstart)&((~pos+gapstart)>>31))];
  }


// Get character, assuming that gap never inside utf8 encoding
FXwchar FXText::getChar(FXint pos) const {
  FXASSERT(0<=pos && pos<=length);
  const FXuchar* ptr=(FXuchar*)&buffer[pos+((gapend-gapstart)&((~pos+gapstart)>>31))];
  FXwchar w=ptr[0];
  if(0xC0<=w){ w=(w<<6)^ptr[1]^0x3080;
  if(0x800<=w){ w=(w<<6)^ptr[2]^0x20080;
  if(0x10000<=w){ w=(w<<6)^ptr[3]^0x400080; }}}
  return w;
  }


// Get length of wide character at position pos
FXint FXText::getCharLen(FXint pos) const {
  FXASSERT(0<=pos && pos<=length);
  return lenUTF8(buffer[pos+((gapend-gapstart)&((~pos+gapstart)>>31))]);
  }


// Get style
FXint FXText::getStyle(FXint pos) const {
  FXASSERT(0<=pos && pos<=length);
  return (FXuchar)sbuffer[pos+((gapend-gapstart)&((~pos+gapstart)>>31))];
  }


/*******************************************************************************/

// Make a valid position, at the start of a wide character
FXint FXText::validPos(FXint pos) const {
  const FXchar* ptr=&buffer[(gapend-gapstart)&((~pos+gapstart)>>31)];
  if(pos<=0) return 0;
  if(pos>=length) return length;
  return (isUTF8(ptr[pos]) || --pos<=0 || isUTF8(ptr[pos]) || --pos<=0 || isUTF8(ptr[pos]) || --pos), pos;
  }

// Decrement; a wide character does not cross the gap, so if pos is at
// or below below the gap, we read from the segment below the gap
FXint FXText::dec(FXint pos) const {
  const FXchar* ptr=&buffer[(gapend-gapstart)&((gapstart-pos)>>31)];
  return (--pos<=0 || isUTF8(ptr[pos]) || --pos<=0 || isUTF8(ptr[pos]) || --pos<=0 || isUTF8(ptr[pos]) || --pos), pos;
  }


// Increment; since a wide character does not cross the gap, if we
// start under the gap the last character accessed is below the gap
FXint FXText::inc(FXint pos) const {
  const FXchar* ptr=&buffer[(gapend-gapstart)&((~pos+gapstart)>>31)];
  return (++pos>=length || isUTF8(ptr[pos]) || ++pos>=length || isUTF8(ptr[pos]) || ++pos>=length || isUTF8(ptr[pos]) || ++pos), pos;
  }

/*******************************************************************************/

// Its a little bit more complex than this:
// We need to deal with diacritics, i.e. non-spacing stuff.  When wrapping, scan till
// the next starter-character [the one with charCombining(c)==0].  Then measure the
// string from that point on. This means FXFont::getCharWidth() is really quite useless.
// Next, we also have the issue of ligatures [fi, AE] and kerning-pairs [VA].
// With possible kerning pairs, we should really measure stuff from the start of the
// line [but this is *very* expensive!!].  We may want to just back up a few characters;
// perhaps to the start of the word, or just the previous character, if not a space.
// Need to investigate this some more; for now assume Normalization Form C.

// Character width
FXint FXText::charWidth(FXwchar ch,FXint indent) const {
  if(' '<=ch) return font->getCharWidth(ch);
  if(ch=='\t') return (tabwidth-indent%tabwidth);
  return font->getCharWidth('^')+font->getCharWidth(ch|0x40);
  }


// Calculate X offset from line start to pos
FXint FXText::xoffset(FXint start,FXint pos) const {
  FXint w=0;
  FXASSERT(0<=start && start<=pos && pos<=length);
  while(start<pos){
    w+=charWidth(getChar(start),w);
    start+=getCharLen(start);
    }
  return w;
  }


// Start of next wrapped line
// Position returned is start of next line, i.e. after newline
// or after space where line got broken during line wrapping.
FXint FXText::wrap(FXint start) const {
  FXint lw,cw,p,s;
  FXwchar ch;
  FXASSERT(0<=start && start<=length);
  lw=0;
  p=s=start;
  while(p<length){
    ch=getChar(p);
    if(ch=='\n') return p+1;            // Newline always breaks
    cw=charWidth(ch,lw);
    if(lw+cw>wrapwidth){                // Technically, a tab-before-wrap should be as wide as space!
      if(s>start) return s;             // We remembered the last space we encountered; break there!
      if(p>start) return p;             // Got at least one character, so return that
      return p+getCharLen(p);           // Otherwise, advance one extra character
      }
    lw+=cw;
    p+=getCharLen(p);
    if(Unicode::isSpace(ch)) s=p;       // Remember potential break point!
    }
  return length;
  }

/*******************************************************************************/

// Find row number from position
// If position falls in visible area, scan visrows for the proper row;
// otherwise, count rows from start of row containing position to the
// first visible line, or from the last visible line to the position.
FXint FXText::rowFromPos(FXint pos) const {
  FXint maxrows=Math::imin(nrows-toprow-1,nvisrows);
  FXint row=0;
  if(pos<visrows[0]){                                                   // Above visible buffer
    if(pos<=0) return 0;
    return toprow-countRows(rowStart(pos),visrows[0]);
    }
  if(visrows[maxrows]<=pos){                                            // Below visible buffer
    if(pos>=length) return nrows-1;
    return toprow+maxrows+countRows(visrows[maxrows],rowStart(pos));
    }
  while(row<maxrows && visrows[row+1]<=pos) row++;
  FXASSERT(0<=row && row<=nvisrows);
  FXASSERT(countRows(visrows[0],rowStart(pos))==row);
  return toprow+row;
  }


// Find row start position from row number
// If row falls in visible area, we can directly return the row start position;
// otherwise, we scan backward from first visible line, or forward from last
// visible line, checking for start or end of buffer of course.
FXint FXText::posFromRow(FXint row) const {
  if(row<toprow){
    if(row<0) return 0;
    return prevRow(visrows[0],toprow-row);
    }
  if(row>=toprow+nvisrows){
    if(row>=nrows) return length;
    return nextRow(visrows[nvisrows-1],row-toprow-nvisrows+1);
    }
  return visrows[row-toprow];
  }


// Determine logical indent of position pos relative to line start.
// Stop at the end of the line (not row).
FXint FXText::columnFromPos(FXint start,FXint pos) const {
  FXint column=0; FXuchar c;
  FXASSERT(0<=start && pos<=length);
  while(start<pos && (c=getByte(start))!='\n'){
    column+=CC(c,column);
    start+=getCharLen(start);
    }
  return column;
  }


// Determine position of logical indent relative to line start.
// Stop at the end of the line (not row).
FXint FXText::posFromColumn(FXint start,FXint col) const {
  FXint column=0; FXuchar c;
  FXASSERT(0<=start && start<=length);
  while(start<length && (c=getByte(start))!='\n'){
    column+=CC(c,column);
    if(col<column) break;
    start+=getCharLen(start);
    }
  return start;
  }


// Determine logical indent of text on line at start, up to
// given position pos.
FXint FXText::indentOfLine(FXint start,FXint pos) const {
  FXint indent=0; FXuchar c;
  FXASSERT(0<=start && pos<=length);
  while(start<pos && ((c=getByte(start))==' ' || c=='\t')){
    indent+=CC(c,indent);
    start+=getCharLen(start);
    }
  return indent;
  }

/*******************************************************************************/

// Return position of begin of paragraph
FXint FXText::lineStart(FXint pos) const {
  FXASSERT(0<=pos && pos<=length);
  while(0<pos && getByte(pos-1)!='\n'){
    pos--;
    }
  FXASSERT(0<=pos && pos<=length);
  return pos;
  }


// Return position of end of paragraph
FXint FXText::lineEnd(FXint pos) const {
  FXASSERT(0<=pos && pos<=length);
  while(pos<length && getByte(pos)!='\n'){
    pos++;
    }
  FXASSERT(0<=pos && pos<=length);
  return pos;
  }


// Return start of next line
FXint FXText::nextLine(FXint pos,FXint nl) const {
  FXASSERT(0<=pos && pos<=length);
  if(0<nl){
    while(pos<length){
      if(getByte(pos++)=='\n' && --nl<=0) break;
      }
    }
  FXASSERT(0<=pos && pos<=length);
  return pos;
  }


// Return start of previous line
FXint FXText::prevLine(FXint pos,FXint nl) const {
  FXASSERT(0<=pos && pos<=length);
  if(0<nl){
    while(0<pos){
      if(getByte(pos-1)=='\n' && --nl<0) break;
      pos--;
      }
    }
  FXASSERT(0<=pos && pos<=length);
  return pos;
  }

/*******************************************************************************/

// Return row start
FXint FXText::rowStart(FXint pos) const {
  FXint p,t;
  FXASSERT(0<=pos && pos<=length);
  if(options&TEXT_WORDWRAP){
    p=pos;
    while(0<pos && getByte(pos-1)!='\n'){               // Find line start first
      pos--;
      }
    while(pos<p && (t=wrap(pos))<=p && t<length){       // Find row containing position, except if last row
      pos=t;
      }
    }
  else{
    while(0<pos && getByte(pos-1)!='\n'){               // Find line start
      pos--;
      }
    }
  FXASSERT(0<=pos && pos<=length);
  return pos;
  }


// Return row end
FXint FXText::rowEnd(FXint pos) const {
  FXint p,t;
  FXASSERT(0<=pos && pos<=length);
  if(options&TEXT_WORDWRAP){
    p=pos;
    while(0<pos && getByte(pos-1)!='\n'){               // Find line start first
      pos--;
      }
    while(pos<=p && pos<length){                        // Find row past position
      pos=wrap(pos);
      }
    if(p<pos){                                          // Back off if line broke at space
      t=dec(pos);
      if(Unicode::isSpace(getChar(t))) pos=t;
      }
    }
  else{
    while(pos<length && getByte(pos)!='\n'){            // Hunt for end of line
      pos++;
      }
    }
  FXASSERT(0<=pos && pos<=length);
  return pos;
  }


// Move to next row given start of line
FXint FXText::nextRow(FXint pos,FXint nr) const {
  FXint p,t;
  FXASSERT(0<=pos && pos<=length);
  if(0<nr){
    if(options&TEXT_WORDWRAP){
      p=pos;
      while(0<pos && getByte(pos-1)!='\n'){             // Find line start first
        pos--;
        }
      while(pos<p && (t=wrap(pos))<=p && t<length){     // Find row containing pos
        pos=t;
        }
      while(pos<length){                                // Then wrap until nth row after
        pos=wrap(pos);
        if(--nr<=0) break;
        }
      }
    else{
      while(pos<length){                                // Hunt for begin of nth next line
        if(getByte(pos++)=='\n' && --nr<=0) break;
        }
      }
    }
  FXASSERT(0<=pos && pos<=length);
  return pos;
  }


// Move to previous row given start of line
FXint FXText::prevRow(FXint pos,FXint nr) const {
  FXint p,q,t;
  FXASSERT(0<=pos && pos<=length);
  if(0<nr){
    if(options&TEXT_WORDWRAP){
      while(0<pos){
        p=pos;
        while(0<pos && getByte(pos-1)!='\n'){           // Find line start first
          pos--;
          }
        FXASSERT(0<=pos);
        q=pos;
        while(q<p && (t=wrap(q))<=p && t<length){       // Decrement number of rows to this point
          nr--;
          q=t;
          }
        while(nr<0){                                    // Went too far forward; try again from pos
          pos=wrap(pos);
          nr++;
          }
        FXASSERT(0<=nr);
        if(nr==0) break;
        if(pos==0) break;
        pos--;                                          // Skip over newline
        nr--;                                           // Which also counts as a row
        }
      }
    else{
      while(0<pos){                                     // Find previous line start
        if(getByte(pos-1)=='\n' && --nr<0) break;
        pos--;
        }
      }
    }
  FXASSERT(0<=pos && pos<=length);
  return pos;
  }

/*******************************************************************************/

// Find end of previous word
FXint FXText::leftWord(FXint pos) const {
  FXwchar ch;
  FXASSERT(0<=pos && pos<=length);
  if(0<pos){
    pos=dec(pos);
    ch=getChar(pos);
    if(isdelimiter(ch)){
      while(0<pos){
        ch=getChar(dec(pos));
        if(Unicode::isSpace(ch) || !isdelimiter(ch)) return pos;
        pos=dec(pos);
        }
      }
    else if(!Unicode::isSpace(ch)){
      while(0<pos){
        ch=getChar(dec(pos));
        if(Unicode::isSpace(ch) || isdelimiter(ch)) return pos;
        pos=dec(pos);
        }
      }
    while(0<pos){
      ch=getChar(dec(pos));
      if(!Unicode::isBlank(ch)) return pos;
      pos=dec(pos);
      }
    }
  return pos;
  }


// Find begin of next word
FXint FXText::rightWord(FXint pos) const {
  FXwchar ch;
  FXASSERT(0<=pos && pos<=length);
  if(pos<length){
    ch=getChar(pos);
    pos=inc(pos);
    if(isdelimiter(ch)){
      while(pos<length){
        ch=getChar(pos);
        if(Unicode::isSpace(ch) || !isdelimiter(ch)) return pos;
        pos=inc(pos);
        }
      }
    else if(!Unicode::isSpace(ch)){
      while(pos<length){
        ch=getChar(pos);
        if(Unicode::isSpace(ch) || isdelimiter(ch)) return pos;
        pos=inc(pos);
        }
      }
    while(pos<length){
      ch=getChar(pos);
      if(!Unicode::isBlank(ch)) return pos;
      pos=inc(pos);
      }
    }
  return pos;
  }


// Find begin of a word
FXint FXText::wordStart(FXint pos) const {
  FXwchar ch;
  FXASSERT(0<=pos && pos<=length);
  if(0<pos){
    ch=(pos<length)?getChar(pos):' ';
    if(ch=='\n') return pos;
    if(Unicode::isBlank(ch)){
      while(0<pos){
        ch=getChar(dec(pos));
        if(!Unicode::isBlank(ch)) return pos;
        pos=dec(pos);
        }
      }
    else if(isdelimiter(ch)){
      while(0<pos){
        ch=getChar(dec(pos));
        if(!isdelimiter(ch)) return pos;
        pos=dec(pos);
        }
      }
    else{
      while(0<pos){
        ch=getChar(dec(pos));
        if(isdelimiter(ch) || Unicode::isSpace(ch)) return pos;
        pos=dec(pos);
        }
      }
    }
  return pos;
  }


// Find end of word
FXint FXText::wordEnd(FXint pos) const {
  FXwchar ch;
  FXASSERT(0<=pos && pos<=length);
  if(pos<length){
    ch=getChar(pos);
    if(ch=='\n') return pos+1;
    if(Unicode::isBlank(ch)){
      while(pos<length){
        ch=getChar(pos);
        if(!Unicode::isBlank(ch)) return pos;
        pos=inc(pos);
        }
      }
    else if(isdelimiter(ch)){
      while(pos<length){
        ch=getChar(pos);
        if(!isdelimiter(ch)) return pos;
        pos=inc(pos);
        }
      }
    else{
      while(pos<length){
        ch=getChar(pos);
        if(isdelimiter(ch) || Unicode::isSpace(ch)) return pos;
        pos=inc(pos);
        }
      }
    }
  return pos;
  }


/*******************************************************************************/

// Count number of columns; start should be on a row start
FXint FXText::countCols(FXint start,FXint end) const {
  FXint result=0;
  FXint in=0;
  FXwchar c;
  FXASSERT(0<=start && start<=end && end<=length);
  while(start<end){
    c=getChar(start);
    if(c=='\n'){ start++; result=Math::imax(result,in); in=0; continue; }
    if(c=='\t'){ start++; in+=(tabcolumns-in%tabcolumns); continue; }
    start+=getCharLen(start);
    in++;
    }
  result=Math::imax(result,in);
  return result;
  }


// Count number of rows; start and end should be on a row start
FXint FXText::countRows(FXint start,FXint end) const {
  FXint result=0;
  if(options&TEXT_WORDWRAP){
    while(start<end){
      start=wrap(start);
      result++;
      }
    }
  else{
    while(start<end){
      if(getByte(start++)=='\n'){
        result++;
        }
      }
    }
  return result;
  }


// Count number of newlines
FXint FXText::countLines(FXint start,FXint end) const {
  FXint result=0;
  FXASSERT(0<=start && start<=end && end<=length);
  while(start<end){
    if(getByte(start++)=='\n'){
      result++;
      }
    }
  return result;
  }

/*******************************************************************************/

// Measure lines; start and end should be on a row start
FXint FXText::measureText(FXint start,FXint end,FXint& wmax,FXint& hmax) const {
  FXint result=0;
  FXint s=start;
  FXint b=start;
  FXint w=0;
  FXint cw;
  FXwchar c;
  FXASSERT(0<=start && start<=end && end<=length);
  if(options&TEXT_WORDWRAP){
    wmax=wrapwidth;
    while(start<end){
      c=getChar(start);
      if(c=='\n'){                      // Break at newline
        result++;
        start++;
        s=b=start;
        w=0;
        continue;
        }
      cw=charWidth(c,w);
      if(w+cw>wrapwidth){               // Break due to wrap
        result++;
        w=0;
        if(s<b){                        // Break past last space seen
          s=start=b;
          continue;
          }
        if(start==s){                   // Always at least one character on each line!
          start+=getCharLen(start);
          }
        s=b=start;
        continue;
        }
      w+=cw;
      start+=getCharLen(start);
      if(Unicode::isSpace(c)) b=start;  // Remember potential break point!
      }
    }
  else{
    wmax=0;
    while(start<end){
      c=getChar(start);
      if(c=='\n'){                      // Break at newline
        wmax=Math::imax(wmax,w);
        result++;
        start++;
        w=0;
        continue;
        }
      w+=charWidth(c,w);
      start+=getCharLen(start);
      }
    wmax=Math::imax(wmax,w);
    }
  hmax=result*font->getFontHeight();
  return result;
  }


// Recalculate line starts, by re-wrapping affected
// lines within the visible range of the text buffer.
void FXText::calcVisRows(FXint startline,FXint endline){
  FXASSERT(0<nvisrows);
  if(startline<1){
    visrows[0]=toppos;
    startline=1;
    }
  if(endline>nvisrows){
    endline=nvisrows;
    }
  if(startline<=endline){
    FXint pos=visrows[startline-1];
    if(options&TEXT_WORDWRAP){
      while(startline<=endline && pos<length){
        pos=wrap(pos);
        FXASSERT(0<=pos && pos<=length);
        visrows[startline++]=pos;
        }
      }
    else{
      while(startline<=endline && pos<length){
        pos=nextLine(pos);
        FXASSERT(0<=pos && pos<=length);
        visrows[startline++]=pos;
        }
      }
    while(startline<=endline){
      visrows[startline++]=length;
      }
    }
  }


// Recompute the text dimensions; this is based on font, margins, wrapping
// and line numbers, so if any of these things change it has to be redone.
void FXText::recompute(){
  FXint hh=font->getFontHeight();
  FXint botrow,ww1,hh1,ww2,hh2;

  // The keep position is where we want to have the top of the buffer be;
  // make sure this is still inside the text buffer!
  keeppos=FXCLAMP(0,keeppos,length);

  // Due to wrapping, toppos which USED to point to a row start may no
  // longer do so.  We back off till the nearest row start.  If we resize
  // the window repeatedly, toppos will not wander away indiscriminately.
  toppos=rowStart(keeppos);

  // Remeasure the text; first, the part above the visible buffer, then
  // the rest.  This avoids measuring the entire text twice, which is
  // quite expensive.
  toprow=measureText(0,toppos,ww1,hh1);

  FXTRACE((TOPIC_LAYOUT,"measureText(%d,%d,%d,%d) = %d\n",0,toppos,ww1,hh1,toprow));

  botrow=measureText(toppos,length,ww2,hh2);

  FXTRACE((TOPIC_LAYOUT,"measureText(%d,%d,%d,%d) = %d\n",toppos,length,ww2,hh2,botrow));

  // Update text dimensions in terms of pixels and rows; note one extra
  // row always added, as there is always at least one row, even though
  // it may be empty of any characters.
  textWidth=Math::imax(ww1,ww2);
  textHeight=hh1+hh2+hh;
  nrows=toprow+botrow+1;

  // Adjust position, keeping same fractional position. Do this AFTER having
  // determined toprow, which may have changed due to wrapping changes.
  pos_y=-toprow*hh-(-pos_y%hh);

  FXTRACE((TOPIC_LAYOUT,"recompute: textWidth=%d textHeight=%d nrows=%d\n",textWidth,textHeight,nrows));

  // All is clean
  flags&=~FLAG_RECALC;
  }

/*******************************************************************************/

// Determine content width of scroll area
FXint FXText::getContentWidth(){
  if(flags&FLAG_RECALC) recompute();
  return marginleft+marginright+textWidth;
  }


// Determine content height of scroll area
FXint FXText::getContentHeight(){
  if(flags&FLAG_RECALC) recompute();
  return margintop+marginbottom+textHeight;
  }


// Return visible scroll-area x position
FXint FXText::getVisibleX() const {
  return barwidth;
  }


// Return visible scroll-area y position
FXint FXText::getVisibleY() const {
  return 0;
  }


// Return visible scroll-area width
FXint FXText::getVisibleWidth() const {
  return width-vertical->getWidth()-barwidth;
  }


// Return visible scroll-area height
FXint FXText::getVisibleHeight() const {
  return height-horizontal->getHeight();
  }


// Get default width
FXint FXText::getDefaultWidth(){
  return 0<vcols ? marginleft+marginright+vcols*font->getTextWidth("8",1)+barwidth : FXScrollArea::getDefaultWidth()+barwidth;
  }


// Get default height
FXint FXText::getDefaultHeight(){
  return 0<vrows ? margintop+marginbottom+vrows*font->getFontHeight() : FXScrollArea::getDefaultHeight();
  }

/*******************************************************************************/

// Recalculate layout
void FXText::layout(){
  FXint fh=font->getFontHeight();
  FXint fw=font->getFontWidth();
  FXint oww=wrapwidth;
  FXint cursorstartpos;
  FXint anchorstartpos;

  // Compute new wrap width, which is either based on the wrap columns or on the
  // width of the window.  If a vertical scroll bar MAY be visible, assume it IS
  // so we don't get sudden surprises.
  // For mono-spaced fonts, wrapwidth is a integral multiple of font width.
  if(options&TEXT_FIXEDWRAP){
    wrapwidth=wrapcolumns*font->getTextWidth("x",1);
    }
  else{
    wrapwidth=width-barwidth-marginleft-marginright;
    if(!(options&VSCROLLER_NEVER)) wrapwidth-=vertical->getDefaultWidth();
    if(font->isFontMono()) wrapwidth=fw*(wrapwidth/fw);
    }

  // If we're wrapping, and wrap width changed, we may need to reflow the text.
  if((options&TEXT_WORDWRAP) && (wrapwidth!=oww)) flags|=FLAG_RECALC;

  // Adjust scrollbars; if necessary, remeasure reflowed text
  // This places the scrollbars, and thus sets the visible area.
  placeScrollBars(width-barwidth,height);

  // Number of visible lines depends on viewport height
  nvisrows=(getVisibleHeight()-margintop-marginbottom+fh+fh-1)/fh;
  if(nvisrows<1) nvisrows=1;

  // Resize line start array; the plus 1 is to keep track of the start
  // of the next line just beyond the last visible one; this ensures
  // we know how long the last visible line is.
  resizeElms(visrows,nvisrows+1);

  // Recompute line start array
  calcVisRows(0,nvisrows);

  // Scroll bar line/column sizes are based on font; set these now
  vertical->setLine(fh);
  horizontal->setLine(fw);

  // Hopefully, all is still in range
  FXASSERT(0<=toprow && toprow<=nrows);
  FXASSERT(0<=toppos && toppos<=length);

  // Update cursor location parameters
  cursorstartpos=rowStart(cursorpos);
  cursorrow=rowFromPos(cursorstartpos);
  cursorcol=columnFromPos(cursorstartpos,cursorpos);
  cursorvcol=cursorcol;

  // Update anchor location parameters
  anchorstartpos=rowStart(anchorpos);
  anchorrow=rowFromPos(anchorstartpos);
  anchorcol=columnFromPos(anchorstartpos,anchorpos);
  anchorvcol=anchorcol;

  // Force repaint
  update();

  // Done
  flags&=~FLAG_DIRTY;
  }


// Propagate size change
void FXText::recalc(){
  FXScrollArea::recalc();
  flags|=FLAG_RECALC;
  }

/*******************************************************************************/

// Search forward for match
FXint FXText::matchForward(FXint pos,FXint end,FXwchar l,FXwchar r,FXint level) const {
  FXwchar ch;
  FXASSERT(0<=end && end<=length);
  FXASSERT(0<=pos && pos<=length);
  while(pos<end){
    ch=getChar(pos);
    if(ch==r){
      level--;
      if(level<=0) return pos;
      }
    else if(ch==l){
      level++;
      }
    pos=inc(pos);
    }
  return -1;
  }


// Search backward for match
FXint FXText::matchBackward(FXint pos,FXint beg,FXwchar l,FXwchar r,FXint level) const {
  FXwchar ch;
  FXASSERT(0<=beg && beg<=length);
  FXASSERT(0<=pos && pos<=length);
  while(beg<=pos){
    ch=getChar(pos);
    if(ch==l){
      level--;
      if(level<=0) return pos;
      }
    else if(ch==r){
      level++;
      }
    pos=dec(pos);
    }
  return -1;
  }


// Search for matching character
FXint FXText::findMatching(FXint pos,FXint beg,FXint end,FXwchar ch,FXint level) const {
  FXASSERT(0<=level);
  FXASSERT(0<=pos && pos<=length);
  switch(ch){
    case '{': return matchForward(pos+1,end,'{','}',level);
    case '}': return matchBackward(pos-1,beg,'{','}',level);
    case '[': return matchForward(pos+1,end,'[',']',level);
    case ']': return matchBackward(pos-1,beg,'[',']',level);
    case '(': return matchForward(pos+1,end,'(',')',level);
    case ')': return matchBackward(pos-1,beg,'(',')',level);
    }
  return -1;
  }


// Flash matching braces or parentheses
// If flashing briefly, highlight only if visible; otherwise, highlight always
void FXText::flashMatching(){
  killHighlight();
  getApp()->removeTimeout(this,ID_FLASH);
  if((options&TEXT_SHOWMATCH) && 0<cursorpos){
    FXint beg=(matchtime<forever) ? visrows[0] : 0;
    FXint end=(matchtime<forever) ? visrows[nvisrows] : length;
    FXint matchpos=findMatching(cursorpos-1,beg,end,getByte(cursorpos-1),1);
    if(0<=matchpos){
      setHighlight(matchpos,1);
      if(0<matchtime && matchtime<forever){
        getApp()->addTimeout(this,ID_FLASH,matchtime);
        }
      }
    }
  }

/*******************************************************************************/

// Search for text
FXbool FXText::findText(const FXString& string,FXint* beg,FXint* end,FXint start,FXuint flgs,FXint npar){
  FXint rexmode=FXRex::Normal;
  FXRex rex;

  // Check arguments
  if(npar<1 || !beg || !end){ fxerror("%s::findText: bad argument.\n",getClassName()); }

  // Tweak parse flags a bit
  if(1<npar) rexmode|=FXRex::Capture;                           // Capturing parentheses
  if(flgs&SEARCH_IGNORECASE) rexmode|=FXRex::IgnoreCase;        // Case insensitivity
  if(!(flgs&SEARCH_REGEX)) rexmode|=FXRex::Verbatim;            // Verbatim match

  // Try parse the regex
  if(rex.parse(string,rexmode)==FXRex::ErrOK){

    // Make all characters contiguous in the buffer
    movegap(length);

    // Search forward
    if(flgs&SEARCH_FORWARD){
      if(start<=length){
        if(rex.search(buffer,length,Math::imax(start,0),length,FXRex::Normal,beg,end,npar)>=0) return true;
        }
      if((flgs&SEARCH_WRAP) && (start>0)){
        if(rex.search(buffer,length,0,Math::imin(start,length),FXRex::Normal,beg,end,npar)>=0) return true;
        }
      return false;
      }

    // Search backward
    if(flgs&SEARCH_BACKWARD){
      if(0<=start){
        if(rex.search(buffer,length,Math::imin(start,length),0,FXRex::Normal,beg,end,npar)>=0) return true;
        }
      if((flgs&SEARCH_WRAP) && (start<length)){
        if(rex.search(buffer,length,length,Math::imax(start,0),FXRex::Normal,beg,end,npar)>=0) return true;
        }
      return false;
      }

    // Anchored match
    return rex.amatch(buffer,length,start,FXRex::Normal,beg,end,npar);
    }
  return false;
  }

/*******************************************************************************/

// Localize position at x,y
FXint FXText::getPosAt(FXint x,FXint y) const {
  FXint linebeg,lineend,row,cx=0,cw,p;
  FXwchar c;
  x=x-pos_x-marginleft-getVisibleX();
  y=y-pos_y-margintop-getVisibleY();
  row=y/font->getFontHeight();
  if(row<toprow){                       // Above visible area
    if(row<0) return 0;                 // Before first row
    linebeg=prevRow(visrows[0],toprow-row);
    lineend=nextRow(linebeg);
    }
  else if(row>=toprow+nvisrows){        // Below visible area
    if(row>=nrows) return length;       // Below last row
    linebeg=nextRow(visrows[nvisrows-1],row-toprow-nvisrows+1);
    lineend=nextRow(linebeg);
    }
  else{                                 // Inside visible area
    FXASSERT(row-toprow<nvisrows);
    linebeg=visrows[row-toprow];
    lineend=visrows[row-toprow+1];
    }
  if(linebeg<lineend){                  // Backup past line-break character, space or newline
    p=dec(lineend);
    if(Unicode::isSpace(getChar(p))) lineend=p;
    }
  FXASSERT(0<=linebeg);
  FXASSERT(linebeg<=lineend);
  FXASSERT(lineend<=length);
  while(linebeg<lineend){
    c=getChar(linebeg);
    cw=charWidth(c,cx);
    if(x<=(cx+(cw>>1))) return linebeg; // Before middle of character
    linebeg+=getCharLen(linebeg);
    cx+=cw;
    }
  return lineend;
  }


// Return text position containing x, y coordinate
FXint FXText::getPosContaining(FXint x,FXint y) const {
  FXint linebeg,lineend,row,cx=0,cw,p;
  FXwchar c;
  x=x-pos_x-marginleft-getVisibleX();
  y=y-pos_y-margintop-getVisibleY();
  row=y/font->getFontHeight();
  if(row<toprow){                       // Above visible area
    if(row<0) return 0;                 // Before first row
    linebeg=prevRow(visrows[0],toprow-row);
    lineend=nextRow(linebeg);
    }
  else if(row>=toprow+nvisrows){        // Below visible area
    if(row>=nrows) return length;       // Below last row
    linebeg=nextRow(visrows[nvisrows-1],row-toprow-nvisrows+1);
    lineend=nextRow(linebeg);
    }
  else{                                 // Inside visible area
    FXASSERT(row-toprow<nvisrows);
    linebeg=visrows[row-toprow];
    lineend=visrows[row-toprow+1];
    }
  if(linebeg<lineend){                  // Backup past line-break character, space or newline
    p=dec(lineend);
    if(Unicode::isSpace(getChar(p))) lineend=p;
    }
  FXASSERT(0<=linebeg);
  FXASSERT(linebeg<=lineend);
  FXASSERT(lineend<=length);
  while(linebeg<lineend){
    c=getChar(linebeg);
    cw=charWidth(c,cx);
    if(x<cx+cw) return linebeg;         // Character contains x
    linebeg+=getCharLen(linebeg);
    cx+=cw;
    }
  return lineend;
  }


// Return closest position and (row,col) of given x,y coordinate.
// Computing the logical column inside of a tab, things can get tricky when
// the font is not a fixed-pitch.  Our solution is to stretch spaces to
// subdivide the tab into as many columns as needed, regardless of whether
// the space is a whole multiple of the regular space width.
// Also, control-characters are problematic as they're rendered as ^A,
// thus, take up two columns even for fixed-pitch fonts.
FXint FXText::getRowColumnAt(FXint x,FXint y,FXint& row,FXint& col) const {
  FXint spacew=font->getCharWidth(' ');
  FXint caretw=font->getCharWidth('^');
  FXint linebeg,lineend,cx=0,cw,cc,p;
  FXwchar c;
  x=x-pos_x-marginleft-getVisibleX();
  y=y-pos_y-margintop-getVisibleY();
  row=y/font->getFontHeight();          // Row is easy to find
  col=0;                                // Find column later
  if(row<toprow){                       // Above visible area
    linebeg=prevRow(visrows[0],toprow-row);
    lineend=nextRow(linebeg);
    }
  else if(row>=toprow+nvisrows){        // Below visible area
    linebeg=nextRow(visrows[nvisrows-1],row-toprow-nvisrows+1);
    lineend=nextRow(linebeg);
    }
  else{                                 // Inside visible area
    FXASSERT(row-toprow<nvisrows);
    linebeg=visrows[row-toprow];
    lineend=visrows[row-toprow+1];
    }
  if(linebeg<lineend){                  // Backup past line-break character, space or newline
    p=dec(lineend);
    if(Unicode::isSpace(getChar(p))) lineend=p;
    }
  FXASSERT(0<=linebeg);
  FXASSERT(linebeg<=lineend);
  FXASSERT(lineend<=length);
  while(linebeg<lineend){
    c=getChar(linebeg);
    if(' '<=c){                         // Normal character
      cw=font->getCharWidth(c);
      if((cx+(cw>>1))<x){
        linebeg+=getCharLen(linebeg);   // Advance over utf8 character
        col+=1;
        cx+=cw;
        continue;
        }
      return linebeg;
      }
    else if(c=='\t'){                   // Tab is really complex
      cw=tabwidth-cx%tabwidth;
      cc=tabcolumns-col%tabcolumns;
      if(cx+cw<=x){                     // Advance over entire tab
        linebeg+=1;
        col+=cc;
        cx+=cw;
        continue;
        }
      if(cx<x){                         // Calculate column inside tab
        col+=(cc*(x-cx)+(cw>>1))/cw;
        linebeg+=(x>=cx+(cw>>1));       // Round to nearest position
        }
      return linebeg;
      }
    else{                               // Control characters
      cw=caretw+font->getCharWidth(c|0x40);
      if((cx+(cw>>1))<x){
        linebeg+=1;
        col+=1;
        cx+=cw;
        continue;
        }
      return linebeg;
      }
    }
  if(cx<x){                             // Calculate column beyond end of line
    col+=(x+(spacew>>1)-cx)/spacew;
    }
  return linebeg;
  }


// Calculate X position of pos
FXint FXText::getXOfPos(FXint pos) const {
  FXint base=rowStart(pos);
  return getVisibleX()+marginleft+pos_x+xoffset(base,pos);
  }


// Determine Y from position pos
FXint FXText::getYOfPos(FXint pos) const {
  FXint h=font->getFontHeight();
  return getVisibleY()+margintop+pos_y+rowFromPos(pos)*h;
  }


// Return screen x-coordinate of row and column
FXint FXText::getXOfRowColumn(FXint row,FXint col) const {
  FXint spacew=font->getCharWidth(' ');
  FXint caretw=font->getCharWidth('^');
  FXint linebeg,lineend,tcol=0,twid=0,tadj=0,cx=0,cc=0,cw,p;
  FXwchar c;
  if(row<toprow){                       // Above visible area
    linebeg=prevRow(visrows[0],toprow-row);
    lineend=nextRow(linebeg);
    }
  else if(row>=toprow+nvisrows){        // Below visible area
    linebeg=nextRow(visrows[nvisrows-1],row-toprow-nvisrows+1);
    lineend=nextRow(linebeg);
    }
  else{                                 // Inside visible area
    linebeg=visrows[row-toprow];
    lineend=visrows[row-toprow+1];
    }
  if(linebeg<lineend){                  // Backup past line-break character, space or newline
    p=dec(lineend);
    if(Unicode::isSpace(getChar(p))) lineend=p;
    }
  FXASSERT(0<=linebeg);
  FXASSERT(linebeg<=lineend);
  FXASSERT(lineend<=length);
  while(cc<col){
    if(linebeg>=lineend){               // Column past end of line
      cx+=spacew*(col-cc);              // Add left-over columns and we're done
      break;
      }
    c=getChar(linebeg);
    if(' '<=c){                         // Normal character
      cx+=font->getCharWidth(c);
      cc+=1;
      linebeg+=getCharLen(linebeg);     // Advance over utf8 character
      continue;
      }
    if(c!='\t'){                        // Control character
      cx+=caretw+font->getCharWidth(c|0x40);
      cc+=1;
      linebeg+=1;
      continue;
      }
    if(tcol==0){                        // Tab character
      cw=tabwidth-cx%tabwidth;
      tcol=tabcolumns-cc%tabcolumns;
      twid=cw/tcol;
      tadj=cw-twid*tcol;
      }
    cx+=twid+(tadj>0);                  // Mete out bits of tab character
    tcol-=1;
    tadj-=1;
    cc+=1;
    linebeg+=(tcol==0);
    }
  return getVisibleX()+marginleft+pos_x+cx;
  }


// Return screen y-coordinate of row and column
FXint FXText::getYOfRowColumn(FXint row,FXint) const {
  return getVisibleY()+margintop+pos_y+row*font->getFontHeight();
  }

/*******************************************************************************/

// Make line containing pos the top visible line
void FXText::setTopLine(FXint pos){
  FXint h=font->getFontHeight();
  setPosition(pos_x,-rowFromPos(pos)*h);
  }


// Get top line
FXint FXText::getTopLine() const {
  return visrows[0];
  }


// Make line containing pos the bottom visible line
void FXText::setBottomLine(FXint pos){
  FXint h=font->getFontHeight();
  setPosition(pos_x,getVisibleHeight()-marginbottom-margintop-h-rowFromPos(pos)*h);
  }


// Get bottom line
FXint FXText::getBottomLine() const {
  return visrows[nvisrows-1];
  }


// Center line containing pos to center of the screen
void FXText::setCenterLine(FXint pos){
  FXint h=font->getFontHeight();
  setPosition(pos_x,((getVisibleHeight()-marginbottom-margintop)/2)-rowFromPos(pos)*h);
  }


// Return true if line containing position is fully visible
FXbool FXText::isPosVisible(FXint pos) const {
  if(visrows[0]<=pos && pos<=visrows[nvisrows]){
    FXint vy=getVisibleY();
    FXint vh=getVisibleHeight();
    FXint y=getYOfPos(pos);
    return vy+margintop<=y && y<=vy+vh-marginbottom-font->getFontHeight();
    }
  return false;
  }


// Force position to become fully visible
void FXText::makePositionVisible(FXint pos){
  FXint vx=getVisibleX();
  FXint vy=getVisibleY();
  FXint vw=getVisibleWidth();
  FXint vh=getVisibleHeight();
  FXint x=getXOfPos(pos);
  FXint y=getYOfPos(pos);
  FXint h=font->getFontHeight();
  FXint ny=pos_y;
  FXint nx=pos_x;

  // Check vertical visibility
  if(y<vy+margintop){
    ny=pos_y+vy+margintop-y;
    nx=0;
    }
  else if(y>vy+vh-marginbottom-h){
    ny=pos_y+vy+vh-marginbottom-h-y;
    nx=0;
    }

  // Check horizontal visibility
  if(x<vx+marginleft){
    nx=pos_x+vx+marginleft-x;
    }
  else if(x>vx+vw-marginright){
    nx=pos_x+vx+vw-marginright-x;
    }

  // If needed, scroll
  if(nx!=pos_x || ny!=pos_y){
    setPosition(nx,ny);
    }
  }


// Move content
void FXText::moveContents(FXint x,FXint y){
  FXint delta=-y/font->getFontHeight()-toprow;
  FXint vx=getVisibleX();
  FXint vy=getVisibleY();
  FXint vw=getVisibleWidth();
  FXint vh=getVisibleHeight();
  FXint dx=x-pos_x;
  FXint dy=y-pos_y;
  FXint i;

  // Erase fragments of cursor overhanging margins
  eraseCursorOverhang();

  // Scrolled up one or more lines
  if(delta<0){
    if(toprow+delta<=0){
      toppos=0;
      toprow=0;
      }
    else{
      toppos=prevRow(toppos,-delta);
      toprow=toprow+delta;
      }
    if(-delta<nvisrows){
      for(i=nvisrows; i>=-delta; i--) visrows[i]=visrows[delta+i];
      calcVisRows(0,-delta);
      }
    else{
      calcVisRows(0,nvisrows);
      }
    }

  // Scrolled down one or more lines
  else if(delta>0){
    if(toprow+delta>=nrows-1){
      toppos=rowStart(length);
      toprow=nrows-1;
      }
    else{
      toppos=nextRow(toppos,delta);
      toprow=toprow+delta;
      }
    if(delta<nvisrows){
      for(i=0; i<=nvisrows-delta; i++) visrows[i]=visrows[delta+i];
      calcVisRows(nvisrows-delta,nvisrows);
      }
    else{
      calcVisRows(0,nvisrows);
      }
    }

  // This is now the new keep position
  keeppos=toppos;

  // Hopefully, all is still in range
  FXASSERT(0<=toprow && toprow<=nrows);
  FXASSERT(0<=toppos && toppos<=length);

  // Scroll stuff in the bar only vertically
  scroll(0,vy+margintop,vx,vh-margintop-marginbottom,0,dy);

  // Scroll the text
  scroll(vx+marginleft,vy+margintop,vw-marginleft-marginright,vh-margintop-marginbottom,dx,dy);

  pos_x=x;
  pos_y=y;
  }

/*******************************************************************************/

// Move the cursor
void FXText::setCursorPos(FXint pos,FXbool notify){
  pos=validPos(pos);
  if(cursorpos!=pos){
    if(isEditable()) drawCursor(0);
    if(options&TEXT_SHOWACTIVE){ updateRow(cursorrow); }
    FXint cursorstartpos=rowStart(pos);
    cursorrow=rowFromPos(cursorstartpos);
    cursorcol=columnFromPos(cursorstartpos,pos);
    cursorvcol=cursorcol;
    cursorpos=pos;
    prefcol=-1;
    if(options&TEXT_SHOWACTIVE){ updateRow(cursorrow); }
    if(isEditable()) drawCursor(FLAG_CARET);
    if(target && notify){
      target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)cursorpos);
      }
    }
  blink=FLAG_CARET;
  }


// Set cursor row, column
void FXText::setCursorRowColumn(FXint row,FXint col,FXbool notify){
  row=Math::iclamp(0,row,nrows-1);
  col=Math::imax(col,0);
  if((row!=cursorrow) || (col!=cursorvcol)){
    FXint newstart=posFromRow(row);             // Row start of new row
    FXint newpos=posFromColumn(newstart,col);   // Position of column on that row
    setCursorPos(newpos,notify);
    cursorvcol=col;
    }
  }


// Set cursor row
void FXText::setCursorRow(FXint row,FXbool notify){
  setCursorRowColumn(row,(0<=prefcol)?prefcol:cursorcol,notify);
  }


// Set cursor column
void FXText::setCursorColumn(FXint col,FXbool notify){
  setCursorRowColumn(cursorrow,col,notify);
  }


// Move cursor
void FXText::moveCursor(FXint pos,FXbool notify){
  setCursorPos(pos,notify);
  setAnchorPos(pos);
  makePositionVisible(cursorpos);
  killSelection(notify);
  flashMatching();
  }


// Move cursor to row and column, and scroll into view
void FXText::moveCursorRowColumn(FXint row,FXint col,FXbool notify){
  setCursorRowColumn(row,col,notify);
  setAnchorRowColumn(row,col);
  makePositionVisible(cursorpos);
  killSelection(notify);
  flashMatching();
  }


// Move cursor and select
void FXText::moveCursorAndSelect(FXint pos,FXuint sel,FXbool notify){
  killHighlight();
  setCursorPos(pos,notify);
  makePositionVisible(cursorpos);
  extendSelection(cursorpos,sel,notify);
  }


// Move cursor to row and column, and extend the block selection to this point
void FXText::moveCursorRowColumnAndSelect(FXint row,FXint col,FXbool notify){
  killHighlight();
  setCursorRowColumn(row,col,notify);
  makePositionVisible(cursorpos);
  extendBlockSelection(row,col,notify);
  }


// Set anchor position
void FXText::setAnchorPos(FXint pos){
  pos=validPos(pos);
  if(anchorpos!=pos){
    FXint anchorstartpos=rowStart(pos);
    anchorrow=rowFromPos(anchorstartpos);
    anchorcol=columnFromPos(anchorstartpos,pos);
    anchorpos=pos;
    anchorvcol=anchorcol;
    }
  }


// Set anchor row and column
void FXText::setAnchorRowColumn(FXint row,FXint col){
  row=Math::iclamp(0,row,nrows-1);
  col=Math::imax(col,0);
  if((row!=anchorrow) || (col!=anchorvcol)){
    FXint newstart=posFromRow(row);             // Row start of new row
    FXint newpos=posFromColumn(newstart,col);   // Position of column on that row
    setAnchorPos(newpos);
    anchorvcol=col;
    }
  }

/*******************************************************************************/

// At position pos, ncdel old characters have been replaced by ncins new ones,
// and nrdel old rows have been replaced with nrins new rows. Recalculate the
// visrows[] array and ancillary buffer positioning information.
void FXText::mutation(FXint pos,FXint ncins,FXint ncdel,FXint nrins,FXint nrdel){
  FXint th=font->getFontHeight();
  FXint vx=getVisibleX();
  FXint vy=getVisibleY();
  FXint vw=getVisibleWidth();
  FXint vh=getVisibleHeight();
  FXint ncdelta=ncins-ncdel;
  FXint nrdelta=nrins-nrdel;
  FXint line,i,y;

  FXTRACE((TOPIC_LAYOUT,"BEFORE: pos=%d ncins=%d ncdel=%d nrins=%d nrdel=%d toppos=%d toprow=%d nrows=%d nvisrows=%d length=%d\n",pos,ncins,ncdel,nrins,nrdel,toppos,toprow,nrows,nvisrows,length));

  FXASSERT(0<=ncins && 0<=ncdel);
  FXASSERT(0<=nrins && 0<=nrdel);
  FXASSERT(0<=pos && pos<=length);

  // Changed text begins below first visible line
  if(visrows[0]<=pos){

    // Changed text begins above last visible line
    if(pos<=visrows[nvisrows]){

      // Scan to find line containing start of change
      for(line=0; line+1<nvisrows && visrows[line+1]<=pos && visrows[line]<visrows[line+1]; line++){ }
      FXASSERT(0<=line && line<nvisrows);

      // More lines
      if(nrdelta>0){
        for(i=nvisrows; i>=line+nrins; i--) visrows[i]=visrows[i-nrdelta]+ncdelta;
        calcVisRows(line,line+nrins);
        y=vy+pos_y+margintop+(toprow+line)*th;
        update(vx,y,vw,vh-y);                   // Repaint bottom part
        FXASSERT(0<=visrows[0]);
        FXASSERT(visrows[nvisrows]<=length);
        }

      // Fewer lines
      else if(nrdelta<0){
        for(i=line+nrdel; i<=nvisrows; i++) visrows[i+nrdelta]=visrows[i]+ncdelta;
        calcVisRows(line,line+nrins);
        calcVisRows(nvisrows+nrdelta,nvisrows);
        y=vy+pos_y+margintop+(toprow+line)*th;
        update(vx,y,vw,vh-y);                   // Repaint bottom part
        FXASSERT(0<=visrows[0]);
        FXASSERT(visrows[nvisrows]<=length);
        }

      // Same lines
      else{
        for(i=line+nrdel; i<=nvisrows; i++) visrows[i]=visrows[i]+ncdelta;
        calcVisRows(line,line+nrins);
        if(nrins==0){
          y=vy+pos_y+margintop+(toprow+line)*th;
          update(vx,y,vw,th);                  // Repaint one line
          }
        else{
          y=vy+pos_y+margintop+(toprow+line)*th;
          update(vx,y,vw,nrins*th);             // Repaint nrins lines
          }
        FXASSERT(0<=visrows[0]);
        FXASSERT(visrows[nvisrows]<=length);
        }
      }
    }

  // Changed text ends above last visible line
  else if(pos+ncdel<visrows[nvisrows]){

    // Changed text ends below first visible line
    if(visrows[0]<pos+ncdel){

      // Scan to find line containing end of change
      for(line=nvisrows; pos+ncdel<visrows[line]; line--){ }
      FXASSERT(0<=line && line<nvisrows);

      // Enough text to keep bottom part of buffer
      if(line<=toprow+nrdelta){
        toprow+=nrdelta;
        toppos=prevRow(visrows[line]+ncdelta,line);
        keeppos=toppos;
        FXASSERT(0<=toprow);
        //FXASSERT(nextRow(0,toprow)==toppos);
        pos_y-=nrdelta*th;
        for(i=line; i<=nvisrows; i++) visrows[i]=visrows[i]+ncdelta;
        calcVisRows(0,line);
        update(vx,vy,vw,pos_y+margintop+(toprow+line)*th);
        if(nrdelta) update(0,vy,vx,vh);         // Repaint line numbers
        }

      // To few lines left, scroll to top of document
      else{
        toprow=0;
        toppos=0;
        keeppos=0;
        pos_y=0;
        calcVisRows(0,nvisrows);
        update();                               // Repaint all
        }
      }

    // Changed text ends above first visible line
    else{
      toprow+=nrdelta;
      toppos+=ncdelta;
      keeppos=toppos;
      FXASSERT(0<=toprow);
      //FXASSERT(nextRow(0,toprow)==toppos);
      for(i=0; i<=nvisrows; i++) visrows[i]+=ncdelta;
      FXASSERT(0<=visrows[0]);
      FXASSERT(visrows[nvisrows]<=length);
      pos_y-=nrdelta*th;
      if(nrdelta) update(0,vy,vx,vh);           // Repaint only line numbers
      }
    }

  // Changed text begins above first and ends below last visible line
  else{
    toprow=Math::iclamp(0,toprow,nrows-nvisrows);
    toppos=nextRow(0,toprow);
    keeppos=toppos;
    pos_y=-toprow*th;
    calcVisRows(0,nvisrows);
    update();                                   // Repaint all
    }
  FXTRACE((TOPIC_LAYOUT,"AFTER : pos=%d ncins=%d ncdel=%d nrins=%d nrdel=%d toppos=%d toprow=%d nrows=%d nvisrows=%d length=%d\n",pos,ncins,ncdel,nrins,nrdel,toppos,toprow,nrows,nvisrows,length));
  }


// Adjust selection for change in text, if there is a selection
static void adjustSelection(FXTextSelection& sel,FXint pos,FXint ndel,FXint nins){
//#define SELECTION_SNIPPED 1
  if(sel.startpos<=sel.endpos){
    if(pos+ndel<=sel.startpos){         // No overlap with change, just adjust positions
      sel.startpos+=nins-ndel;
      sel.endpos+=nins-ndel;
      }
    else if(pos<=sel.startpos){
      if(pos+ndel<=sel.endpos){         // First part of selection inside change
        sel.endpos+=nins-ndel;
#ifdef SELECTION_SNIPPED
        sel.startpos=pos+nins;          // Snip selection
#else
        sel.startpos=pos;               // Expand selection
#endif
        }
      else{                             // Whole of selection inside change
#ifdef SELECTION_SNIPPED
        sel.startpos=0;                 // Snip selection to empty
        sel.endpos=-1;
        sel.startcol=0;
        sel.endcol=-1;
#else
        sel.startpos=pos;               // Expand selection to change
        sel.endpos=pos+nins;
#endif
        }
      }
    else if(pos<sel.endpos){
      if(sel.endpos<=pos+ndel){         // Last part of selection inside change
#ifdef SELECTION_SNIPPED
        sel.endpos=pos;                 // Snip selection
#else
        sel.endpos=pos+nins;            // Expand selection
#endif
        }
      else{                             // Whole of range inside selection
        sel.endpos+=nins-ndel;
        }
      }
    }
  }


// Backs up to the begin of the line preceding the line containing pos, or the
// start of the line containing pos if the preceding line terminated in a newline.
FXint FXText::changeBeg(FXint pos) const {
  FXint p1,p2,t;
  FXASSERT(0<=pos && pos<=length);
  p1=p2=lineStart(pos);
  if(options&TEXT_WORDWRAP){
    while(p2<pos && (t=wrap(p2))<=pos){
      p1=p2;
      p2=t;
      }
    }
  FXASSERT(0<=p1 && p1<=length);
  return p1;
  }


// Scan forward to the end of affected area, which is the start of the next
// paragraph; a change can cause the rest of the paragraph to reflow.
FXint FXText::changeEnd(FXint pos) const {
  FXASSERT(0<=pos && pos<=length);
  while(pos<length){
    if(getByte(pos)=='\n') return pos+1;
    pos++;
    }
  return length;
  }


// Replace #del characters at pos by #ins characters
void FXText::replace(FXint pos,FXint del,const FXchar *text,FXint ins,FXint style){
  FXint nrdel,nrins,ncdel,ncins,wbeg,wend,diff,wdel,hdel,wins,hins,cursorstartpos,anchorstartpos;

  FXTRACE((TOPIC_TEXT,"pos=%d del=%d ins=%d\n",pos,del,ins));

  // Delta in characters
  diff=ins-del;

  // Bracket potentially affected character range for wrapping purposes
  wbeg=changeBeg(pos);
  wend=changeEnd(pos+del);

  // Measure stuff before change
  nrdel=measureText(wbeg,wend,wdel,hdel);
  ncdel=wend-wbeg;

  FXTRACE((TOPIC_TEXT,"wbeg=%d wend=%d nrdel=%d ncdel=%d length=%d nrows=%d wdel=%d hdel=%d\n",wbeg,wend,nrdel,ncdel,length,nrows,wdel,hdel));

  // Move the gap to current position
  movegap(pos);

  // Grow the gap if too small
  if(diff>(gapend-gapstart)){ sizegap(diff+MINSIZE); }

  // Modify the buffer
  copyElms(&buffer[pos],text,ins);
  if(sbuffer){fillElms(&sbuffer[pos],style,ins);}
  gapstart+=ins;
  gapend+=del;
  length+=diff;

  // Shrink the gap if too large
  if(MAXSIZE<(gapend-gapstart)){ sizegap(MAXSIZE); }

  // Measure stuff after change
  nrins=measureText(wbeg,wend+diff,wins,hins);
  ncins=wend+diff-wbeg;

  // Adjust number of rows now
  nrows+=nrins-nrdel;

  FXTRACE((TOPIC_TEXT,"wbeg=%d wend+diff=%d nrins=%d ncins=%d length=%d nrows=%d wins=%d hins=%d\n",wbeg,wend+diff,nrins,ncins,length,nrows,wins,hins));

  // Update visrows array and other stuff
  mutation(wbeg,ncins,ncdel,nrins,nrdel);

  // Fix text metrics
  textHeight=textHeight+hins-hdel;
  textWidth=Math::imax(textWidth,wins);

  // Keep anchorpos at same place relative to its surrounding text.
  // When inside the changed region, move it to the end of the change.
  if(wbeg<=anchorpos){
    if(anchorpos<=wend){
      if(pos+del<=anchorpos) anchorpos+=diff;           // Beyond changed text
      else if(pos<=anchorpos) anchorpos=pos+ins;        // To end of changed text
      FXASSERT(0<=anchorpos && anchorpos<=length);
      anchorstartpos=rowStart(anchorpos);
      FXASSERT(0<=anchorstartpos && anchorstartpos<=length);
      anchorrow=rowFromPos(anchorstartpos);
      anchorcol=columnFromPos(anchorstartpos,anchorpos);
      anchorvcol=anchorcol;
      }
    else{
      anchorpos+=diff;                                  // Adjust position
      anchorrow+=nrins-nrdel;                           // Adjust row
      }
    }

  // Keep cursorpos at same place relative to its surrounding text.
  // When inside the changed region, move it to the end of the change.
  if(wbeg<=cursorpos){
    if(cursorpos<=wend){
      if(pos+del<=cursorpos) cursorpos+=diff;           // Beyond changed text
      else if(pos<=cursorpos) cursorpos=pos+ins;        // To end of changed text
      FXASSERT(0<=cursorpos && cursorpos<=length);
      cursorstartpos=rowStart(cursorpos);
      FXASSERT(0<=cursorstartpos && cursorstartpos<=length);
      cursorrow=rowFromPos(cursorstartpos);
      cursorcol=columnFromPos(cursorstartpos,cursorpos);
      cursorvcol=cursorcol;
      }
    else{
      cursorpos+=diff;                                  // Adjust position
      cursorrow+=nrins-nrdel;                           // Adjust row
      }
    }

  // Hopefully it all still makes sense
  FXASSERT(0<=anchorpos && anchorpos<=length);
  FXASSERT(0<=cursorpos && cursorpos<=length);

  // Fix selection ranges
  adjustSelection(select,pos,del,ins);
  adjustSelection(hilite,pos,del,ins);

  // Reconcile scrollbars
  // Must be done AFTER adjusting cursor location
  placeScrollBars(width-barwidth,height);

  // Forget preferred column
  prefcol=-1;

  // Text was changed
  modified=true;
  }

/*******************************************************************************/

// Change the text in the buffer to new text
FXint FXText::setText(const FXchar* text,FXint num,FXbool notify){
  return setStyledText(text,num,0,notify);
  }


// Change all of the text
FXint FXText::setText(const FXString& text,FXbool notify){
  return setStyledText(text.text(),text.length(),0,notify);
  }


// Change the text in the buffer to new text
FXint FXText::setStyledText(const FXchar* text,FXint num,FXint style,FXbool notify){
  FXTextChange textchange;
  if(num<0){ fxerror("%s::setStyledText: bad argument.\n",getClassName()); }
  if(!resizeElms(buffer,num+MINSIZE)){
    fxerror("%s::setStyledText: out of memory.\n",getClassName());
    }
  copyElms(buffer,text,num);
  if(sbuffer){
    if(!resizeElms(sbuffer,num+MINSIZE)){
      fxerror("%s::setStyledText: out of memory.\n",getClassName());
      }
    fillElms(sbuffer,style,num);
    }
  gapstart=num;
  gapend=gapstart+MINSIZE;
  length=num;
  toppos=0;
  toprow=0;
  keeppos=0;
  select.startpos=0;
  select.endpos=-1;
  select.startcol=0;
  select.endcol=-1;
  hilite.startpos=0;
  hilite.endpos=-1;
  hilite.startcol=0;
  hilite.endcol=-1;
  anchorpos=0;
  anchorrow=0;
  anchorcol=0;
  anchorvcol=0;
  cursorpos=0;
  cursorrow=0;
  cursorcol=0;
  cursorvcol=0;
  prefcol=-1;
  pos_x=0;
  pos_y=0;
  modified=false;
  textchange.pos=0;
  textchange.ndel=0;
  textchange.nins=num;
  textchange.ins=(FXchar*)text;
  textchange.del=(FXchar*)"";
  if(notify && target){
    target->tryHandle(this,FXSEL(SEL_INSERTED,message),(void*)&textchange);
    target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)cursorpos);
    }
  recalc();
  layout();
  update();
  return num;
  }


// Change all of the text
FXint FXText::setStyledText(const FXString& text,FXint style,FXbool notify){
  return setStyledText(text.text(),text.length(),style,notify);
  }

/*******************************************************************************/

// Replace text by other text
FXint FXText::replaceText(FXint pos,FXint del,const FXchar *text,FXint ins,FXbool notify){
  return replaceStyledText(pos,del,text,ins,0,notify);
  }


// Replace text by other text
FXint FXText::replaceText(FXint pos,FXint del,const FXString& text,FXbool notify){
  return replaceStyledText(pos,del,text.text(),text.length(),0,notify);
  }


// Replace m characters at pos by n characters
FXint FXText::replaceStyledText(FXint pos,FXint del,const FXchar *text,FXint ins,FXint style,FXbool notify){
  if(0<=pos && 0<=del && 0<=ins && pos+del<=length && text){
    FXTextChange textchange;
    textchange.pos=pos;
    textchange.ndel=del;
    textchange.nins=ins;
    textchange.ins=(FXchar*)text;
    allocElms(textchange.del,del);
    extractText(textchange.del,pos,del);
    replace(pos,del,text,ins,style);
    if(notify && target){
      target->tryHandle(this,FXSEL(SEL_REPLACED,message),(void*)&textchange);
      target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)cursorpos);
      }
    freeElms(textchange.del);
    return ins;
    }
  return 0;
  }


// Replace m characters at pos by n characters
FXint FXText::replaceStyledText(FXint pos,FXint del,const FXString& text,FXint style,FXbool notify){
  return replaceStyledText(pos,del,text.text(),text.length(),style,notify);
  }

/*******************************************************************************/

// Maximum number of columns in a string
static FXint maxColumns(const FXString& text,FXint tabcols){
  FXint result=0,cols=0,p=0; FXuchar c;
  while(p<text.length()){
    c=text[p++];
    if(c=='\n'){                                // End of the line; keep track of the longest
      result=Math::imax(result,cols);
      cols=0;
      continue;
      }
    if(c=='\t'){                                // Advance by number of tab columns
      cols+=tabcols-cols%tabcols;
      continue;
      }
    cols++;
    if(c<0xC0) continue;
    p++;
    if(c<0xE0) continue;
    p++;
    if(c<0xF0) continue;
    p++;
    }
  result=Math::imax(result,cols);               // In case of unterminated last line
  return result;
  }


// Copy one utf8 character
static inline void wccopy(FXchar*& dst,const FXchar*& src){
  FXuchar c=*dst++=*src++;
  if(c>=0xC0){
    *dst++=*src++;
    if(c>=0xE0){
      *dst++=*src++;
      if(c>=0xF0){
        *dst++=*src++;
        }
      }
    }
  }


// Skip one utf8 character
static inline void wcskip(const FXchar*& src){
  FXuchar c=*src++;
  if(c>=0xC0){
    src++;
    if(c>=0xE0){
      src++;
      if(c>=0xF0){
        src++;
        }
      }
    }
  }


// Copy columns up from col to endcol
static FXint copycols(FXchar*& dst,FXchar* dstend,const FXchar*& src,const FXchar* srcend,FXint ncols=2147483647){
  FXint nc=ncols;
  while(dst<dstend && 0<nc && src<srcend && *src!='\n'){
    wccopy(dst,src);
    nc--;
    }
  return ncols-nc;
  }


// Skip columns from col to endcol
static FXint skipcols(const FXchar*& src,const FXchar* srcend,FXint ncols=2147483647){
  FXint nc=ncols;
  while(0<nc && src<srcend && *src!='\n'){
    wcskip(src);
    nc--;
    }
  return ncols-nc;
  }


// Padd output until endcol
static FXint padcols(FXchar*& dst,FXchar* dstend,FXint ncols=0){
  FXint nc=ncols;
  while(dst<dstend && 0<nc){
    *dst++=' ';
    nc--;
    }
  return ncols-nc;
  }


// Remove columns startcol up to endcol from src; assume input has been detabbed.
// For each line, copy up to startcol; then skip characters up to endcol,
// and copy the remainder of the line, up to and including newline, if any.
static FXchar* removecolumns(FXchar* dst,FXchar* dstend,const FXchar* src,const FXchar* srcend,FXint startcol,FXint endcol){
  while(dst<dstend && src<srcend){
    copycols(dst,dstend,src,srcend,startcol);                   // Copy up to startcol
    skipcols(src,srcend,endcol-startcol);                       // Skip to endcol
    copycols(dst,dstend,src,srcend);                            // Copy to line end
    if(dst<dstend && src<srcend && *src=='\n'){                 // Copy newline
      *dst++=*src++;
      }
    }
  FXASSERT(src<=srcend);
  FXASSERT(dst<=dstend);
  return dst;
  }


// Replicate text at src n times to dst
static FXchar* replicatecolumns(FXchar* dst,FXchar* dstend,const FXchar* src,const FXchar* srcend,FXint nr){
  while(dst<dstend && 0<nr){
    const FXchar *ptr=src;
    while(dst<dstend && ptr<srcend && *ptr!='\n'){
      wccopy(dst,ptr);
      }
    if(dst<dstend && --nr>0){
      *dst++='\n';
      }
    }
  FXASSERT(dst<=dstend);
  return dst;
  }


// Extract block of columns of text from input; assume input has been detabbed.
// For each line, scan to startcol, then copy characters up to endcol to the
// destination. If there are fewer than startcol columns on the line, just
// copy a newline to indicate an empty column on that particular line.
static FXchar* extractcolumns(FXchar* dst,FXchar* dstend,const FXchar* src,const FXchar* srcend,FXint startcol,FXint endcol){
  while(dst<dstend && src<srcend){
    skipcols(src,srcend,startcol);                              // Skip to startcol
    copycols(dst,dstend,src,srcend,endcol-startcol);            // Copy up to endcol
    skipcols(src,srcend);                                       // Skip to line end
    if(dst<dstend && src<srcend && *src=='\n'){                 // Copy newline
      *dst++=*src++;
      }
    }
  FXASSERT(src<=srcend);
  FXASSERT(dst<=dstend);
  return dst;
  }


// Insert same text at given column on each line.
static FXchar* insertcolumns(FXchar* dst,FXchar* dstend,const FXchar* src,const FXchar* srcend,const FXchar *ins,const FXchar* insend,FXint startcol,FXint inscols){
  FXint sc,c;
  while(dst<dstend && (src<srcend || ins<insend)){
    sc=copycols(dst,dstend,src,srcend,startcol);                // Copy to startcol
    if(ins<insend && *ins!='\n'){                               // Inserted block non-empty
      sc+=padcols(dst,dstend,startcol-sc);                      // Pad up to startcol
      sc+=copycols(dst,dstend,ins,insend,inscols);              // Copy inserted block, up to inscols
      }
    if(src<srcend && *src!='\n'){                               // Stuff past endcol
      padcols(dst,dstend,startcol+inscols-sc);                  // Pad to startcol+ninscols
      copycols(dst,dstend,src,srcend);                          // Copy the rest
      }
    c=0;
    if(dst<dstend && ins<insend && *ins=='\n'){                 // Advance over line end
      *dst=*ins++; c=1;
      }
    if(dst<dstend && src<srcend && *src=='\n'){
      *dst=*src++; c=1;
      }
    dst+=c;
    }
  FXASSERT(src<=srcend);
  FXASSERT(ins<=insend);
  FXASSERT(dst<=dstend);
  return dst;
  }


// Replace block of columns of text with new ones; assume both source text and inserted text has been detabbed.
// Copies up to inscols of new text into the destination column
static FXchar* replacecolumns(FXchar* dst,FXchar* dstend,const FXchar* src,const FXchar* srcend,const FXchar *ins,const FXchar* insend,FXint startcol,FXint endcol,FXint inscols){
  FXint sc,c;
  while(dst<dstend && (src<srcend || ins<insend)){
    sc=copycols(dst,dstend,src,srcend,startcol);                // Copy to startcol
    skipcols(src,srcend,endcol-startcol);                       // Skip to endcol
    if(ins<insend && *ins!='\n'){                               // Inserted block non-empty
      sc+=padcols(dst,dstend,startcol-sc);                      // Pad up to startcol
      sc+=copycols(dst,dstend,ins,insend,inscols);              // Copy inserted block, up to inscols
      }
    if(src<srcend && *src!='\n'){                               // Stuff past endcol
      padcols(dst,dstend,startcol+inscols-sc);                  // Pad to startcol+ninscols
      copycols(dst,dstend,src,srcend);                          // Copy the rest
      }
    c=0;
    if(dst<dstend && ins<insend && *ins=='\n'){                 // Advance over line end
      *dst=*ins++; c=1;
      }
    if(dst<dstend && src<srcend && *src=='\n'){
      *dst=*src++; c=1;
      }
    dst+=c;
    }
  FXASSERT(src<=srcend);
  FXASSERT(ins<=insend);
  FXASSERT(dst<=dstend);
  return dst;
  }


// Count (maximum) columns in text
static FXint countColumns(const FXString& text,FXint tabcols=8){
  FXint result=0,indent=0,p=0;
  FXuchar c;
  while(p<text.length()){
    c=text[p++];
    if(c=='\t'){
      indent+=(tabcols-indent%tabcols);
      continue;
      }
    if(c=='\n'){
      result=Math::imax(result,indent);
      indent=0;
      continue;
      }
    indent++;
    if(c<0xC0) continue;
    p++;
    if(c<0xE0) continue;
    p++;
    if(c<0xF0) continue;
    p++;
    }
  return result;
  }


// Overstrike text at startcol
static FXString overstrikeColumns(const FXString& src,const FXString& ovr,FXint startcol){
  FXString result;
  if(result.length(src.length()+ovr.length()+startcol)){
    FXint srccol=0,ovrcol=0,skpcol=0,d=0,s=0,o=0;
    FXuchar c;
    while(s<src.length() && srccol<startcol){
      c=result[d++]=src[s++]; srccol++;
      if(c<0xC0) continue;
      result[d++]=src[s++];
      if(c<0xE0) continue;
      result[d++]=src[s++];
      if(c<0xF0) continue;
      result[d++]=src[s++];
      }
    if(0<ovr.length()){
      while(srccol<startcol){
        result[d++]=' '; srccol++;
        }
      while(o<ovr.length()){
        c=result[d++]=ovr[o++]; ovrcol++;
        if(c<0xC0) continue;
        result[d++]=ovr[o++];
        if(c<0xE0) continue;
        result[d++]=ovr[o++];
        if(c<0xF0) continue;
        result[d++]=ovr[o++];
        }
      while(s<src.length() && skpcol<ovrcol){
        c=src[s++]; skpcol++;
        if(c<0xC0) continue;
        s++;
        if(c<0xE0) continue;
        s++;
        if(c<0xF0) continue;
        s++;
        }
      }
    while(s<src.length()){
      c=result[d++]=src[s++];
      if(c<0xC0) continue;
      result[d++]=src[s++];
      if(c<0xE0) continue;
      result[d++]=src[s++];
      if(c<0xF0) continue;
      result[d++]=src[s++];
      }
    FXASSERT(d<result.length());
    result.length(d);
    }
  return result;
  }


// Insert inscols of text at startcol
static FXString insertColumns(const FXString& src,const FXString& ins,FXint startcol,FXint numcols){
  FXString result;
  if(result.length(src.length()+ins.length()+startcol+numcols)){
    FXint srccol=0,d=0,s=0,i=0;
    FXuchar c;
    while(s<src.length() && srccol<startcol){
      c=result[d++]=src[s++]; srccol++;
      if(c<0xC0) continue;
      result[d++]=src[s++];
      if(c<0xE0) continue;
      result[d++]=src[s++];
      if(c<0xF0) continue;
      result[d++]=src[s++];
      }
    if(0<ins.length()){
      while(srccol<startcol){
        result[d++]=' '; srccol++;
        }
      while(i<ins.length() && srccol<startcol+numcols){
        c=result[d++]=ins[i++]; srccol++;
        if(c<0xC0) continue;
        result[d++]=ins[i++];
        if(c<0xE0) continue;
        result[d++]=ins[i++];
        if(c<0xF0) continue;
        result[d++]=ins[i++];
        }
      }
    if(s<src.length()){
      while(srccol<startcol+numcols){
        result[d++]=' '; srccol++;
        }
      while(s<src.length()){
        c=result[d++]=src[s++];
        if(c<0xC0) continue;
        result[d++]=src[s++];
        if(c<0xE0) continue;
        result[d++]=src[s++];
        if(c<0xF0) continue;
        result[d++]=src[s++];
        }
      }
    FXASSERT(d<result.length());
    result.length(d);
    }
  return result;
  }


// Replace columns from text at startcol
static FXString replaceColumns(const FXString& src,const FXString& ins,FXint startcol,FXint endcol,FXint numcols){
  FXString result;
  if(result.length(src.length()+ins.length()+startcol+numcols)){
    FXint srccol=0,inscol=0,d=0,s=0,i=0;
    FXuchar c;
    while(s<src.length() && srccol<startcol){
      c=result[d++]=src[s++];
      srccol++;
      if(c<0xC0) continue;
      result[d++]=src[s++];
      if(c<0xE0) continue;
      result[d++]=src[s++];
      if(c<0xF0) continue;
      result[d++]=src[s++];
      }
    while(s<src.length() && srccol<endcol){
      c=src[s++];
      srccol++;
      if(c<0xC0) continue;
      s++;
      if(c<0xE0) continue;
      s++;
      if(c<0xF0) continue;
      s++;
      }
    if(0<ins.length()){
      while(srccol<startcol){
        result[d++]=' ';
        srccol++;
        }
      while(i<ins.length()){
        c=result[d++]=ins[i++];
        inscol++;
        if(c<0xC0) continue;
        result[d++]=ins[i++];
        if(c<0xE0) continue;
        result[d++]=ins[i++];
        if(c<0xF0) continue;
        result[d++]=ins[i++];
        }
      }
    if(s<src.length()){
      while(inscol<numcols){
        result[d++]=' ';
        inscol++;
        }
      while(s<src.length()){
        c=result[d++]=src[s++];
        if(c<0xC0) continue;
        result[d++]=src[s++];
        if(c<0xE0) continue;
        result[d++]=src[s++];
        if(c<0xF0) continue;
        result[d++]=src[s++];
        }
      }
    FXASSERT(d<result.length());
    result.length(d);
    }
  return result;
  }


// Remove remcols of text at startcol
static FXString removeColumns(const FXString& src,FXint startcol,FXint numcols){
  FXString result;
  if(result.length(src.length())){
    FXint srccol=0,remcol=0,d=0,s=0;
    FXuchar c;
    while(s<src.length() && srccol<startcol){
      c=result[d++]=src[s++]; srccol++;
      if(c<0xC0) continue;
      result[d++]=src[s++];
      if(c<0xE0) continue;
      result[d++]=src[s++];
      if(c<0xF0) continue;
      result[d++]=src[s++];
      }
    while(s<src.length() && remcol<numcols){
      c=src[s++]; remcol++;
      if(c<0xC0) continue;
      s++;
      if(c<0xE0) continue;
      s++;
      if(c<0xF0) continue;
      s++;
      }
    while(s<src.length()){
      c=result[d++]=src[s++];
      if(c<0xC0) continue;
      result[d++]=src[s++];
      if(c<0xE0) continue;
      result[d++]=src[s++];
      if(c<0xF0) continue;
      result[d++]=src[s++];
      }
    FXASSERT(d<result.length());
    result.length(d);
    }
  return result;
  }


// Extract columns of text at startcol
static FXString extractColumns(const FXString& src,FXint startcol,FXint numcols){
  FXString result;
  if(result.length(src.length())){
    FXint srccol=0,dstcol=0,s=0,d=0;
    FXuchar c;
    while(s<src.length() && srccol<startcol){
      c=src[s++]; srccol++;
      if(c<0xC0) continue;
      s++;
      if(c<0xE0) continue;
      s++;
      if(c<0xF0) continue;
      s++;
      }
    while(s<src.length() && dstcol<numcols){
      c=result[d++]=src[s++]; dstcol++;
      if(c<0xC0) continue;
      result[d++]=src[s++];
      if(c<0xE0) continue;
      result[d++]=src[s++];
      if(c<0xF0) continue;
      result[d++]=src[s++];
      }
    FXASSERT(d<result.length());
    result.length(d);
    }
  return result;
  }


// Overstrike columns starting at startcol with new text; assume inputs have been detabbed.
static FXchar* overstrikecolumns(FXchar* dst,FXchar* dstend,const FXchar* src,const FXchar* srcend,const FXchar* ovr,const FXchar* ovrend,FXint startcol){
  FXint sc,ec; FXuchar c;
  while(dst<dstend && (src<srcend || ovr<ovrend)){
    sc=ec=copycols(dst,dstend,src,srcend,startcol);             // Copy up to startcol
    if(ovr<ovrend && *ovr!='\n'){                               // Overstrike block is non-empty
      ec+=padcols(dst,dstend,startcol-ec);                      // Pad up to column where overstrike starts
      ec+=copycols(dst,dstend,ovr,ovrend);                      // Copy new overstruck block
      }
    if(src<srcend && *src!='\n'){                               // More stuff past startcol
      sc+=skipcols(src,srcend,ec-sc);                           // Skip past overstruck text
      copycols(dst,dstend,src,srcend);                          // Copy the rest
      }
    c=0;
    if(dst<dstend && src<srcend && *src=='\n'){                 // Advance over line end
      *dst=*src++; c=1;
      }
    if(dst<dstend && ovr<ovrend && *ovr=='\n'){
      *dst=*ovr++; c=1;
      }
    dst+=c;
    }
  FXASSERT(src<=srcend);
  FXASSERT(ovr<=ovrend);
  FXASSERT(dst<=dstend);
  return dst;
  }

/*******************************************************************************/

// Replace block of columns with text
FXint FXText::replaceTextBlock(FXint startpos,FXint endpos,FXint startcol,FXint endcol,const FXchar *text,FXint num,FXbool notify){
  return replaceStyledTextBlock(startpos,endpos,startcol,endcol,text,num,0,notify);
  }


// Replace block of columns with text
FXint FXText::replaceTextBlock(FXint startpos,FXint endpos,FXint startcol,FXint endcol,const FXString& text,FXbool notify){
  return replaceStyledTextBlock(startpos,endpos,startcol,endcol,text.text(),text.length(),0,notify);
  }


// Replace block of columns with text
// Calculating the size of the scratch array to assemble the replacing text is a bit
// complicated; it is best understood graphically:
//
//             col-0          startcol
//             |              |
//             |              | endcol
//             |              | |
//             V              v v
// startpos--->X--------------+-+----+-------------+  ^          ^
//             |              |      |             |  |          |
//             | A       A'   | R R' |  B          |  |norgrows  |
//             |              |      |             |  |          |
//             |              |      |             |  |          |
//             +--------------+      +-------------X  v          |
//             | C            |      |             ^             |
//             |              |      |             |             |ninsrows
// endpos- - - |- - - - - - - |- - - |- - - - - - -+             |
//             |              |      |                           |
//             |              |      |                           |
//             +--------------+-+----+                           v
//
//                            <------>
//                            ninscols
//
// Here A, B are the parts of the original text, A being the part before the selected
// block and B the part after (or inside) the selected block.  R is the newly added
// text, which may be more or fewer lines than the selected block.  C is any additional
// lines added in case the newly added text includes more lines than the selection.
// Note that A, B, and R may have lines of varying lengths [some lines may have no
// part in section B, for example].
//
// The total amount of allocated space should account for:
//
//   1) Original text (A + B), plus possibly expanded tabs,
//   2) Inserted text (R), plus possibly expanded tabs,
//   3) Extra padding (A') after some lines in (A), up to startcol,
//   4) Padding of empty lines (C), if any, up to startcol,
//   5) Padding of (R), (R') up to startcol+ninscols.
//   6) The block being removed
//
// Some lines in A, B, and R are longer than others. Rather than calculating the exact
// amount of padding needed, its simpler just to over-estimate in a way which is guaranteed
// to be enough; this is done by just addding the whole rectangle; so we just add an extra
// (startcol+ninscols)*max(ninsrows,norgrows) as total padding for A,C, and R.
FXint FXText::replaceStyledTextBlock(FXint startpos,FXint endpos,FXint startcol,FXint endcol,const FXchar *text,FXint num,FXint style,FXbool notify){
  if(0<=startpos && startpos<=endpos && endpos<=length && 0<=startcol && startcol<=endcol){
    FXString org=FXString::detab(extractText(startpos,endpos-startpos),tabcolumns);
    FXString ins=FXString::detab(text,num,tabcolumns);
    FXint norgrows=org.contains('\n')+1;
    FXint ninsrows=ins.contains('\n')+1;
    FXint ninscols=maxColumns(ins,tabcolumns);
    FXString rep(' ',org.length()+ins.length()+(startcol+ninscols+1)*Math::imax(ninsrows,norgrows));
    FXchar* repend=replacecolumns(rep.text(),rep.text()+rep.length(),org.text(),org.text()+org.length(),ins.text(),ins.text()+ins.length(),startcol,endcol,ninscols);
    rep.trunc(repend-rep.text());
    if(!(options&TEXT_NO_TABS)){ rep=FXString::entab(rep,tabcolumns); }
    return replaceStyledText(startpos,endpos-startpos,rep,style,notify);
    }
  return 0;
  }


// Replace block of columns with text
FXint FXText::replaceStyledTextBlock(FXint startpos,FXint endpos,FXint startcol,FXint endcol,const FXString& text,FXint style,FXbool notify){
  return replaceStyledTextBlock(startpos,endpos,startcol,endcol,text.text(),text.length(),style,notify);
  }

/*******************************************************************************/

// Overstrike text block
FXint FXText::overstrikeTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXchar *text,FXint num,FXbool notify){
  return overstrikeStyledTextBlock(startpos,endpos,startcol,text,num,0,notify);
  }


// Overstrike text block
FXint FXText::overstrikeTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXString& text,FXbool notify){
  return overstrikeStyledTextBlock(startpos,endpos,startcol,text.text(),text.length(),0,notify);
  }


#if 0
// Overstrike styled text block (version only affecting overstruck rows)
FXint FXText::overstrikeStyledTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXchar *text,FXint num,FXint style,FXbool notify){
  if(0<=startpos && startpos<=endpos && endpos<=length && 0<=startcol){
    FXString ovr=FXString::detab(text,num,tabcolumns);
    FXint novrcols=maxColumns(ovr,tabcolumns);
    FXint novrrows=ovr.contains('\n')+1;
    FXint end=lineEnd(nextLine(startpos,novrrows-1));
    FXString org=FXString::detab(extractText(startpos,end-startpos),tabcolumns);
    FXString rep(' ',org.length()+ovr.length()+(startcol+novrcols+1)*novrrows);
    FXchar* repend=overstrikecolumns(rep.text(),rep.text()+rep.length(),org.text(),org.text()+org.length(),ovr.text(),ovr.text()+ovr.length(),startcol);
    rep.trunc(repend-rep.text());
    if(!(options&TEXT_NO_TABS)){ rep=FXString::entab(rep,tabcolumns); }
    return replaceStyledText(startpos,end-startpos,rep,style,notify);
    }
  return 0;
  }
#endif

// Overstrike styled text block
FXint FXText::overstrikeStyledTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXchar *text,FXint num,FXint style,FXbool notify){
  if(0<=startpos && startpos<=endpos && endpos<=length && 0<=startcol){
    FXString org=FXString::detab(extractText(startpos,endpos-startpos),tabcolumns);
    FXString ovr=FXString::detab(text,num,tabcolumns);
    FXint norgrows=org.contains('\n')+1;
    FXint novrrows=ovr.contains('\n')+1;
    FXint novrcols=maxColumns(ovr,tabcolumns);
    FXString rep(' ',org.length()+ovr.length()+(startcol+novrcols+1)*Math::imax(novrrows,norgrows));
    FXchar* repend=overstrikecolumns(rep.text(),rep.text()+rep.length(),org.text(),org.text()+org.length(),ovr.text(),ovr.text()+ovr.length(),startcol);
    rep.trunc(repend-rep.text());
    if(!(options&TEXT_NO_TABS)){ rep=FXString::entab(rep,tabcolumns); }
    return replaceStyledText(startpos,endpos-startpos,rep,style,notify);
    }
  return 0;
  }

// Overstrike styled text block
FXint FXText::overstrikeStyledTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXString& text,FXint style,FXbool notify){
  return overstrikeStyledTextBlock(startpos,endpos,startcol,text.text(),text.length(),style,notify);
  }

/*******************************************************************************/

// Add text at the end
FXint FXText::appendText(const FXchar *text,FXint num,FXbool notify){
  return appendStyledText(text,num,0,notify);
  }


// Add text at the end
FXint FXText::appendText(const FXString& text,FXbool notify){
  return appendStyledText(text.text(),text.length(),0,notify);
  }


// Add text at the end
FXint FXText::appendStyledText(const FXchar *text,FXint num,FXint style,FXbool notify){
  FXTextChange textchange;
  if(num<0){ fxerror("%s::appendStyledText: bad argument.\n",getClassName()); }
  textchange.pos=length;
  textchange.ndel=0;
  textchange.nins=num;
  textchange.ins=(FXchar*)text;
  textchange.del=(FXchar*)"";
  replace(length,0,text,num,style);
  if(notify && target){
    target->tryHandle(this,FXSEL(SEL_INSERTED,message),(void*)&textchange);
    target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)cursorpos);
    }
  return num;
  }


// Add text at the end
FXint FXText::appendStyledText(const FXString& text,FXint style,FXbool notify){
  return appendStyledText(text.text(),text.length(),style,notify);
  }

/*******************************************************************************/

// Insert some text at pos
FXint FXText::insertText(FXint pos,const FXchar *text,FXint num,FXbool notify){
  return insertStyledText(pos,text,num,0,notify);
  }


// Insert some text at pos
FXint FXText::insertText(FXint pos,const FXString& text,FXbool notify){
  return insertStyledText(pos,text.text(),text.length(),0,notify);
  }


// Insert some text at pos
FXint FXText::insertStyledText(FXint pos,const FXchar *text,FXint num,FXint style,FXbool notify){
  if(0<=pos && pos<=length && 0<=num && text){
    FXTextChange textchange;
    textchange.pos=pos;
    textchange.ndel=0;
    textchange.nins=num;
    textchange.ins=(FXchar*)text;
    textchange.del=(FXchar*)"";
    replace(pos,0,text,num,style);
    if(notify && target){
      target->tryHandle(this,FXSEL(SEL_INSERTED,message),(void*)&textchange);
      target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)cursorpos);
      }
    return num;
    }
  return 0;
  }


// Insert some text at pos
FXint FXText::insertStyledText(FXint pos,const FXString& text,FXint style,FXbool notify){
  return insertStyledText(pos,text.text(),text.length(),style,notify);
  }

/*******************************************************************************/

// Insert text columns at startcol in line starting at startpos to endpos
FXint FXText::insertTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXchar *text,FXint num,FXbool notify){
  return insertStyledTextBlock(startpos,endpos,startcol,text,num,0,notify);
  }


// Insert text columns at startcol in line starting at startpos to endpos
FXint FXText::insertTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXString& text,FXbool notify){
  return insertStyledTextBlock(startpos,endpos,startcol,text.text(),text.length(),0,notify);
  }


#if 0
// Insert text columns at startcol in line starting at startpos to endpos with given style (version only affecting inserted rows)
FXint FXText::insertStyledTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXchar *text,FXint num,FXint style,FXbool notify){
  if(0<=startpos && startpos<=endpos && endpos<=length && 0<=startcol){
    FXString ins=FXString::detab(text,num,tabcolumns);
    FXint ninscols=maxColumns(ins,tabcolumns);
    FXint ninsrows=ins.contains('\n')+1;
    FXint end=lineEnd(nextLine(startpos,ninsrows-1));
    FXString org=FXString::detab(extractText(startpos,end-startpos),tabcolumns);
    FXString rep(' ',org.length()+ins.length()+(startcol+ninscols+1)*ninsrows);
    FXchar *repend=insertcolumns(&rep[0],&rep[rep.length()],org.text(),org.text()+org.length(),ins.text(),ins.text()+ins.length(),startcol,ninscols);
    rep.trunc(repend-rep.text());
    if(!(options&TEXT_NO_TABS)){ rep=FXString::entab(rep,tabcolumns); }
    return replaceStyledText(startpos,end-startpos,rep,style,notify);
    }
  return 0;
  }
#endif

// Insert text columns at startcol in line starting at startpos to endpos with given style
FXint FXText::insertStyledTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXchar *text,FXint num,FXint style,FXbool notify){
  if(0<=startpos && startpos<=endpos && endpos<=length && 0<=startcol){
    FXString org=FXString::detab(extractText(startpos,endpos-startpos),tabcolumns);
    FXString ins=FXString::detab(text,num,tabcolumns);
    FXint norgrows=org.contains('\n')+1;
    FXint ninsrows=ins.contains('\n')+1;
    FXint ninscols=maxColumns(ins,tabcolumns);
    FXString rep(' ',org.length()+ins.length()+(startcol+ninscols+1)*Math::imax(ninsrows,norgrows));
    FXchar *repend=insertcolumns(&rep[0],&rep[rep.length()],org.text(),org.text()+org.length(),ins.text(),ins.text()+ins.length(),startcol,ninscols);
    rep.trunc(repend-rep.text());
    if(!(options&TEXT_NO_TABS)){ rep=FXString::entab(rep,tabcolumns); }
    return replaceStyledText(startpos,endpos-startpos,rep,style,notify);
    }
  return 0;
  }

// Insert text columns at startcol in line starting at startpos to endpos with given style
FXint FXText::insertStyledTextBlock(FXint startpos,FXint endpos,FXint startcol,const FXString& text,FXint style,FXbool notify){
  return insertStyledTextBlock(startpos,endpos,startcol,text.text(),text.length(),style,notify);
  }

/*******************************************************************************/

// Change style of text range
FXint FXText::changeStyle(FXint pos,FXint num,FXint style){
  if(0<=pos && 0<=num && pos+num<=length){
    if(sbuffer){
      if(pos+num<=gapstart){
        fillElms(sbuffer+pos,style,num);
        }
      else if(gapstart<=pos){
        fillElms(sbuffer+pos-gapstart+gapend,style,num);
        }
      else{
        fillElms(sbuffer+pos,style,gapstart-pos);
        fillElms(sbuffer+gapend,style,pos+num-gapstart);
        }
      updateRange(pos,pos+num);
      }
    return num;
    }
  return 0;
  }


// Change style of text range from style-array
FXint FXText::changeStyle(FXint pos,const FXchar* style,FXint num){
  if(0<=pos && 0<=num && pos+num<=length){
    if(sbuffer && style){
      if(pos+num<=gapstart){
        copyElms(sbuffer+pos,style,num);
        }
      else if(gapstart<=pos){
        copyElms(sbuffer+gapend-gapstart+pos,style,num);
        }
      else{
        copyElms(sbuffer+pos,style,gapstart-pos);
        copyElms(sbuffer+gapend,style+gapstart-pos,pos+num-gapstart);
        }
      updateRange(pos,pos+num);
      }
    return num;
    }
  return 0;
  }


// Change style of text range from style-array
FXint FXText::changeStyle(FXint pos,const FXString& style){
  return changeStyle(pos,style.text(),style.length());
  }

/*******************************************************************************/

// Remove some text at pos
FXint FXText::removeText(FXint pos,FXint num,FXbool notify){
  if(0<=pos && 0<=num && pos+num<=length){
    FXTextChange textchange;
    textchange.pos=pos;
    textchange.ndel=num;
    textchange.nins=0;
    textchange.ins=(FXchar*)"";
    allocElms(textchange.del,num);
    extractText(textchange.del,pos,num);
    replace(pos,num,nullptr,0,0);
    if(notify && target){
      target->tryHandle(this,FXSEL(SEL_DELETED,message),(void*)&textchange);
      target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)cursorpos);
      }
    freeElms(textchange.del);
    return num;
    }
  return 0;
  }


// Remove columns startcol to endcol from lines starting at startpos to endpos
FXint FXText::removeTextBlock(FXint startpos,FXint endpos,FXint startcol,FXint endcol,FXbool notify){
  if(0<=startpos && startpos<=endpos && endpos<=length && 0<=startcol && startcol<=endcol){
    FXString org=FXString::detab(extractText(startpos,endpos-startpos),tabcolumns);
    FXString rep(' ',org.length());
    FXchar* repend=removecolumns(rep.text(),rep.text()+rep.length(),org.text(),org.text()+org.length(),startcol,endcol);
    rep.trunc(repend-rep.text());
    if(!(options&TEXT_NO_TABS)){ rep=FXString::entab(rep,tabcolumns); }
    return replaceStyledText(startpos,endpos-startpos,rep,0,notify);
    }
  return 0;
  }


/*******************************************************************************/

// Remove all text from the buffer
FXint FXText::clearText(FXbool notify){
  return removeText(0,length,notify);
  }

/*******************************************************************************/

// Grab range of text
void FXText::extractText(FXchar *text,FXint pos,FXint num) const {
  if(0<=pos && 0<=num && pos+num<=length && text){
    if(pos+num<=gapstart){
      copyElms(text,buffer+pos,num);
      }
    else if(gapstart<=pos){
      copyElms(text,buffer+gapend-gapstart+pos,num);
      }
    else{
      copyElms(text,buffer+pos,gapstart-pos);
      copyElms(text+gapstart-pos,buffer+gapend,pos+num-gapstart);
      }
    }
  }


// Return n bytes of contents of text buffer from position pos
FXString FXText::extractText(FXint pos,FXint num) const {
  FXString result;
  if(0<=pos && 0<=num && pos+num<=length && result.length(num)){
    if(pos+num<=gapstart){
      copyElms(&result[0],buffer+pos,num);
      }
    else if(gapstart<=pos){
      copyElms(&result[0],buffer+gapend-gapstart+pos,num);
      }
    else{
      copyElms(&result[0],buffer+pos,gapstart-pos);
      copyElms(&result[gapstart-pos],buffer+gapend,pos+num-gapstart);
      }
    }
  return result;
  }


// Grab range of style
void FXText::extractText(FXString& text,FXint pos,FXint num) const {
  if(0<=pos && 0<=num && pos+num<=length && text.length(num)){
    if(pos+num<=gapstart){
      copyElms(&text[0],buffer+pos,num);
      }
    else if(gapstart<=pos){
      copyElms(&text[0],buffer+gapend-gapstart+pos,num);
      }
    else{
      copyElms(&text[0],buffer+pos,gapstart-pos);
      copyElms(&text[gapstart-pos],buffer+gapend,pos+num-gapstart);
      }
    }
  }


// Grab range of style
void FXText::extractStyle(FXchar *style,FXint pos,FXint num) const {
  if(0<=pos && 0<=num && pos+num<=length && style && sbuffer){
    if(pos+num<=gapstart){
      copyElms(style,sbuffer+pos,num);
      }
    else if(gapstart<=pos){
      copyElms(style,sbuffer+gapend-gapstart+pos,num);
      }
    else{
      copyElms(style,sbuffer+pos,gapstart-pos);
      copyElms(style+gapstart-pos,sbuffer+gapend,pos+num-gapstart);
      }
    }
  }


// Return n bytes of style info from buffer from position pos
FXString FXText::extractStyle(FXint pos,FXint num) const {
  FXString result;
  if(0<=pos && 0<=num && pos+num<=length && sbuffer && result.length(num)){
    if(pos+num<=gapstart){
      copyElms(&result[0],sbuffer+pos,num);
      }
    else if(gapstart<=pos){
      copyElms(&result[0],sbuffer+gapend-gapstart+pos,num);
      }
    else{
      copyElms(&result[0],sbuffer+pos,gapstart-pos);
      copyElms(&result[gapstart-pos],sbuffer+gapend,pos+num-gapstart);
      }
    }
  return result;
  }


// Grab range of style
void FXText::extractStyle(FXString& style,FXint pos,FXint num) const {
  if(0<=pos && 0<=num && pos+num<=length && sbuffer && style.length(num)){
    if(pos+num<=gapstart){
      copyElms(&style[0],sbuffer+pos,num);
      }
    else if(gapstart<=pos){
      copyElms(&style[0],sbuffer+gapend-gapstart+pos,num);
      }
    else{
      copyElms(&style[0],sbuffer+pos,gapstart-pos);
      copyElms(&style[gapstart-pos],sbuffer+gapend,pos+num-gapstart);
      }
    }
  }


// Extract block of columns
void FXText::extractTextBlock(FXString& text,FXint startpos,FXint endpos,FXint startcol,FXint endcol) const {
  if(startpos<endpos && startcol<=endcol){
    FXString org=FXString::detab(extractText(startpos,endpos-startpos),tabcolumns);
    text.length(org.length());
    FXchar* textend=extractcolumns(text.text(),text.text()+text.length(),org.text(),org.text()+org.length(),startcol,endcol);
    text.trunc(textend-text.text());
    if(!(options&TEXT_NO_TABS)){ text=FXString::entab(text,tabcolumns); }
    }
  else{
    text.clear();
    }
  }


// Extract block of columns
FXString FXText::extractTextBlock(FXint startpos,FXint endpos,FXint startcol,FXint endcol) const {
  FXString text;
  extractTextBlock(text,startpos,endpos,startcol,endcol);
  return text;
  }

/*******************************************************************************/

// Retrieve text into buffer
void FXText::getText(FXchar* text,FXint num) const {
  extractText(text,0,num);
  }


// Retrieve text into buffer
void FXText::getText(FXString& text) const {
  extractText(text,0,getLength());
  }


// We return a constant copy of the buffer
FXString FXText::getText() const {
  return extractText(0,getLength());
  }

/*******************************************************************************/

// End of overstruck character range
FXint FXText::overstruck(FXint start,FXint end,const FXchar *text,FXint num){
  if(!memchr(text,'\n',num)){
    FXint sindent,nindent,oindent,p;
    const FXchar *ptr;
    FXwchar ch;

    // Measure indent at pos
    sindent=columnFromPos(lineStart(start),start);

    // Measure indent at end of (first line of the) new text
    for(ptr=text,nindent=sindent; ptr<text+num; ptr=wcinc(ptr)){
      nindent+=CC(*ptr,nindent);
      }

    // Now figure out how much text to replace
    for(p=start,oindent=sindent; p<length; p+=getCharLen(p)){
      ch=getChar(p);
      if(ch=='\n') break;                // Stuff past the newline just gets inserted
      oindent+=CC(ch,oindent);
      if(oindent>=nindent){              // Replace string fits inside here
        if(oindent==nindent) p+=getCharLen(p);
        break;
        }
      }
    end=p;
    }
  return end;
  }

/*******************************************************************************/

// Select all text
FXbool FXText::selectAll(FXbool notify){
  return setSelection(0,length,notify);
  }


// Select range of len characters starting at given position pos
FXbool FXText::setSelection(FXint pos,FXint len,FXbool notify){
  FXDragType types[4]={stringType,textType,utf8Type,utf16Type};
  FXint spos=validPos(pos);
  FXint epos=validPos(pos+len);
  if(select.startpos!=spos || select.endpos!=epos || select.startcol<=select.endcol){
    FXint what[4];

    // Now a range select
    if(select.startcol<=select.endcol){
      updateRange(select.startpos,select.endpos);
      select.startcol=0;
      select.endcol=-1;
      }

    // Update affected areas
    if((epos<=select.startpos) || (select.endpos<=spos)){
      updateRange(select.startpos,select.endpos);
      updateRange(spos,epos);
      }
    else{
      updateRange(select.startpos,spos);
      updateRange(select.endpos,epos);
      }

    // Release selection
    if(spos>=epos){
      if(hasSelection()) releaseSelection();
      if(notify && target){
        what[0]=select.startpos;
        what[1]=select.endpos-select.startpos;
        what[2]=select.startcol;
        what[3]=select.endcol-select.startcol;
        target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)what);
        }
      select.startpos=0;
      select.startcol=0;
      }

    // Acquire selection
    else{
      if(!hasSelection()) acquireSelection(types,ARRAYNUMBER(types));
      if(notify && target){
        what[0]=select.startpos;
        what[1]=select.endpos-select.startpos;
        what[2]=select.startcol;
        what[3]=select.endcol-select.startcol;
        target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)what);
        }
      select.startpos=spos;
      select.endpos=epos;
      }
    return true;
    }
  return false;
  }


// Extend the primary selection from the anchor to the given position
FXbool FXText::extendSelection(FXint pos,FXuint sel,FXbool notify){
  FXint p=validPos(pos),ss=0,se=0;
  switch(sel){
    case SelectChars:                   // Selecting characters
      if(p<=anchorpos){
        ss=p;
        se=anchorpos;
        }
      else{
        ss=anchorpos;
        se=p;
        }
      break;
    case SelectWords:                   // Selecting words
      if(p<=anchorpos){
        ss=wordStart(p);
        se=wordEnd(anchorpos);
        }
      else{
        ss=wordStart(anchorpos);
        se=wordEnd(p);
        }
      break;
    case SelectRows:                    // Selecting rows
      if(p<=anchorpos){
        ss=rowStart(p);
        se=nextRow(anchorpos);
        }
      else{
        ss=rowStart(anchorpos);
        se=nextRow(p);
        }
      break;
    case SelectLines:                   // Selecting lines
      if(p<=anchorpos){
        ss=lineStart(p);
        se=nextLine(anchorpos);
        }
      else{
        ss=lineStart(anchorpos);
        se=nextLine(p);
        }
      break;
    }
  return setSelection(ss,se-ss,notify);
  }


// Select block of characters within given box
FXbool FXText::setBlockSelection(FXint trow,FXint lcol,FXint brow,FXint rcol,FXbool notify){
  FXDragType types[4]={stringType,textType,utf8Type,utf16Type};
  FXint spos=lineStart(posFromRow(trow));
  FXint epos=lineEnd(posFromRow(brow));
  if(select.startpos!=spos || select.endpos!=epos || select.startcol!=lcol || select.endcol!=rcol){
    FXint what[4];

    // Update affected areas
    updateLines(select.startpos,select.endpos);
    updateLines(spos,epos);

    // Release selection
    if(spos>epos || lcol>rcol){
      if(hasSelection()) releaseSelection();
      if(notify && target){
        what[0]=select.startpos;
        what[1]=select.endpos-select.startpos;
        what[2]=select.startcol;
        what[3]=select.endcol-select.startcol;
        target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)what);
        }
      select.startpos=0;
      select.endpos=-1;
      select.startcol=0;
      select.endcol=-1;
      }

    // Acquire selection
    else{
      if(!hasSelection()) acquireSelection(types,ARRAYNUMBER(types));
      if(notify && target){
        what[0]=select.startpos;
        what[1]=select.endpos-select.startpos;
        what[2]=select.startcol;
        what[3]=select.endcol-select.startcol;
        target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)what);
        }
      select.startpos=spos;
      select.endpos=epos;
      select.startcol=lcol;
      select.endcol=rcol;
      }
    FXTRACE((TOPIC_TEXT,"select: startpos=%d endpos=%d startcol=%d endcol=%d\n",select.startpos,select.endpos,select.startcol,select.endcol));
    return true;
    }
  return false;
  }


// Extend primary selection from anchor to given row, column
FXbool FXText::extendBlockSelection(FXint row,FXint col,FXbool notify){
  FXint trow,brow,lcol,rcol;
  FXMINMAX(trow,brow,anchorrow,row);
  FXMINMAX(lcol,rcol,anchorvcol,col);
  return setBlockSelection(trow,lcol,brow,rcol,notify);
  }


// Kill the selection
FXbool FXText::killSelection(FXbool notify){
  if(select.startpos<=select.endpos){
    FXint what[4];
    if(hasSelection()) releaseSelection();
    if(notify && target){
      what[0]=select.startpos;
      what[1]=select.endpos-select.startpos;
      what[2]=select.startcol;
      what[3]=select.endcol-select.startcol;
      target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)what);
      }
    updateRange(select.startpos,select.endpos);
    select.startpos=0;
    select.endpos=-1;
    select.startcol=0;
    select.endcol=-1;
    return true;
    }
  return false;
  }


// Position is selected if inside character range AND character range non-empty AND NOT column-selection,
// OR inside character range AND inside column range AND non-empty column-range.
FXbool FXText::isPosSelected(FXint pos,FXint col) const {
  return select.startpos<=pos && pos<=select.endpos && ((select.startpos<select.endpos && select.startcol>select.endcol) || (select.startcol<=col && col<=select.endcol && select.startcol<select.endcol));
  }


// Position is range-selected if inside character range AND character range non-empty AND NOT column-selection
FXbool FXText::isPosSelected(FXint pos) const {
  return select.startpos<=pos && pos<=select.endpos && select.startpos<select.endpos && select.startcol>select.endcol;
  }

/*******************************************************************************/

// Get selected text
FXString FXText::getSelectedText() const {
  if((select.startcol<select.endcol) && (select.startpos<=select.endpos)){
    return extractTextBlock(select.startpos,select.endpos,select.startcol,select.endcol);
    }
  if((select.startcol>select.endcol) && (select.startpos<select.endpos)){
    return extractText(select.startpos,select.endpos-select.startpos);
    }
  return FXString::null;
  }


// Copy selection to clipboard
FXbool FXText::copySelection(){
  FXDragType types[4]={stringType,textType,utf8Type,utf16Type};
  if(select.startpos<=select.endpos){
    if(acquireClipboard(types,ARRAYNUMBER(types))){
      clipped=getSelectedText();
      return true;
      }
    }
  return false;
  }


// Copy selection to clipboard and delete it
FXbool FXText::cutSelection(FXbool notify){
  if(copySelection()){
    return deleteSelection(notify);
    }
  return false;
  }


// Replace selection by other text
FXbool FXText::replaceSelection(const FXString& text,FXbool notify){
  if((select.startcol<select.endcol) && (select.startpos<=select.endpos)){
    FXint cols=maxColumns(text,tabcolumns);
    FXint ins=replaceTextBlock(select.startpos,select.endpos,select.startcol,select.endcol,text,notify);
    FXint pos=posFromColumn(lineStart(select.startpos+ins),select.startcol+cols);
    moveCursor(pos,notify);
    return true;
    }
  if((select.startcol>select.endcol) && (select.startpos<select.endpos)){
    FXint ins=replaceText(select.startpos,select.endpos-select.startpos,text,notify);
    FXint pos=select.startpos+ins;
    moveCursor(pos,notify);
    return true;
    }
  return false;
  }


// Delete selection
FXbool FXText::deleteSelection(FXbool notify){
  if((select.startcol<select.endcol) && (select.startpos<=select.endpos)){
    FXint ins=removeTextBlock(select.startpos,select.endpos,select.startcol,select.endcol,notify);
    FXint pos=posFromColumn(lineStart(select.startpos+ins),select.startcol);
    moveCursor(pos,notify);
    return true;
    }
  if((select.startcol>select.endcol) && (select.startpos<select.endpos)){
    removeText(select.startpos,select.endpos-select.startpos,notify);
    moveCursor(select.startpos,notify);
    return true;
    }
  return false;
  }


// Delete pending selection
FXbool FXText::deletePendingSelection(FXbool notify){
  return isPosSelected(cursorpos,cursorvcol) && deleteSelection(notify);
  }


// Paste primary ("middle-mouse") selection
FXbool FXText::pasteSelection(FXbool notify){

  // Don't paste inside selection
  if(!isPosSelected(cursorpos,cursorvcol)){
    FXString string;

    // Try UTF-8, then UTF-16, then 8859-1
    if(getDNDData(FROM_SELECTION,utf8Type,string) || getDNDData(FROM_SELECTION,utf16Type,string) || getDNDData(FROM_SELECTION,stringType,string)){
      FXint pos=cursorpos,ins;

      // Insert at vertical cursor
      if((select.startcol<=select.endcol) && (select.startpos<=select.endpos)){
        FXint cols=maxColumns(string,tabcolumns);
        if(isOverstrike()){
          ins=overstrikeTextBlock(select.startpos,select.endpos,select.startcol,string,notify);
          }
        else{
          ins=insertTextBlock(select.startpos,select.endpos,select.startcol,string,notify);
          }
        pos=posFromColumn(lineStart(select.startpos+ins),select.startcol+cols);
        makePositionVisible(pos);
        setCursorPos(pos,notify);
        setAnchorPos(pos);
        flashMatching();
        return true;
        }

      // Overstrike mode, extent
      if(isOverstrike()){       // FIXME should overstrike always be block-mode?
        ins=overstruck(pos,pos,string.text(),string.length());
        ins=replaceText(pos,ins-pos,string,notify);
        makePositionVisible(pos+ins);
        setCursorPos(pos+ins,notify);
        setAnchorPos(pos+ins);
        flashMatching();
        return true;
        }

      // Replace text and move cursor
      ins=insertText(pos,string,notify);
      makePositionVisible(pos+ins);
      setCursorPos(pos+ins,notify);
      setAnchorPos(pos+ins);
      flashMatching();
      return true;
      }
    }
  return false;
  }


// Paste clipboard
FXbool FXText::pasteClipboard(FXbool notify){
  FXString string;

  // Try UTF-8, then UTF-16, then 8859-1
  if(getDNDData(FROM_CLIPBOARD,utf8Type,string) || getDNDData(FROM_CLIPBOARD,utf16Type,string) || getDNDData(FROM_CLIPBOARD,stringType,string)){
    FXint pos=cursorpos,ins;

    // De-DOS
#ifdef WIN32
    dosToUnix(string);
#endif

    // If there's a selection, replace it
    if(replaceSelection(string,notify)){
      return true;
      }

    // Insert at vertical cursor
    if((select.startcol<=select.endcol) && (select.startpos<=select.endpos)){
      FXint cols=maxColumns(string,tabcolumns);
      if(isOverstrike()){
        ins=overstrikeTextBlock(select.startpos,select.endpos,select.startcol,string,notify);
        }
      else{
        ins=insertTextBlock(select.startpos,select.endpos,select.startcol,string,notify);
        }
      pos=posFromColumn(lineStart(select.startpos+ins),select.startcol+cols);
      moveCursor(pos,notify);
      return true;
      }

    // Overstrike
    if(isOverstrike()){         // FIXME should overstrike always be block-mode?
      ins=overstruck(pos,pos,string.text(),string.length());
      ins=replaceText(pos,ins-pos,string,notify);
      moveCursor(pos+ins,notify);
      return true;
      }

    // Default is to insert at cursor
    ins=insertText(pos,string,notify);
    moveCursor(pos+ins,notify);
    return true;
    }
  return false;
  }

/*******************************************************************************/

// Set highlight
FXbool FXText::setHighlight(FXint pos,FXint len){
  FXint spos=validPos(pos);
  FXint epos=validPos(pos+len);
  if(spos!=hilite.startpos || epos!=hilite.endpos){
    if(epos<=hilite.startpos || hilite.endpos<=spos){
      updateRange(hilite.startpos,hilite.endpos);
      updateRange(spos,epos);
      }
    else{
      updateRange(hilite.startpos,spos);
      updateRange(hilite.endpos,epos);
      }
    hilite.startpos=spos;
    hilite.endpos=epos;
    hilite.startcol=0;
    hilite.endcol=-1;
    return true;
    }
  return false;
  }


// Unhighlight the text
FXbool FXText::killHighlight(){
  if(hilite.startpos<=hilite.endpos){
    updateRange(hilite.startpos,hilite.endpos);
    hilite.startpos=0;
    hilite.endpos=-1;
    hilite.startcol=0;
    hilite.endcol=-1;
    return true;
    }
  return false;
  }

/*******************************************************************************/

// Draw the cursor
void FXText::drawCursor(FXuint state){
  if((state^flags)&FLAG_CARET){
    if(xid){
      FXDCWindow dc(this);
      if(state&FLAG_CARET)
        paintCursor(dc);
      else
        eraseCursor(dc);
      }
    flags^=FLAG_CARET;
    }
  }


// Paint cursor glyph
void FXText::paintCursor(FXDCWindow& dc) const {
  FXint th,tw,cursorx,cursory;
  FXwchar c;
  th=font->getFontHeight();
  cursory=getVisibleY()+margintop+pos_y+cursorrow*th;
  if(getVisibleY()+margintop<cursory+th && cursory<=getVisibleY()+getVisibleHeight()-marginbottom){
    FXASSERT(toprow<=cursorrow && cursorrow<toprow+nvisrows);
    FXASSERT(0<=visrows[cursorrow-toprow] && visrows[cursorrow-toprow]<=cursorpos && cursorpos<=length);
    tw=font->getCharWidth((cursorpos<length) && ((c=getChar(cursorpos))>=' ')?c:' ');
    cursorx=getVisibleX()+marginleft+pos_x+xoffset(visrows[cursorrow-toprow],cursorpos)-1;
    if(getVisibleX()<=cursorx+tw+2 && cursorx-2<=getVisibleX()+getVisibleWidth()){
      dc.setClipRectangle(getVisibleX(),getVisibleY(),getVisibleWidth(),getVisibleHeight());
      if(0<dc.getClipWidth() && 0<dc.getClipHeight()){
        dc.setForeground(cursorColor);
        if(options&TEXT_OVERSTRIKE){
          dc.drawRectangle(cursorx,cursory,tw,th-1);
          }
        else{
          dc.fillRectangle(cursorx,cursory,2,th);
          dc.fillRectangle(cursorx-2,cursory,6,1);
          dc.fillRectangle(cursorx-2,cursory+th-1,6,1);
          }
        }
      }
    }
  }


// Erase cursor glyph
void FXText::eraseCursor(FXDCWindow& dc) const {
  FXint th,tw,cursorx,cursory,cx,cy,ch,cw;
  FXwchar c;
  th=font->getFontHeight();
  cursory=getVisibleY()+margintop+pos_y+cursorrow*th;
  if(getVisibleY()+margintop<cursory+th && cursory<=getVisibleY()+getVisibleHeight()-marginbottom){
    FXASSERT(toprow<=cursorrow && cursorrow<toprow+nvisrows);
    FXASSERT(0<=visrows[cursorrow-toprow] && visrows[cursorrow-toprow]<=cursorpos && cursorpos<=length);
    tw=font->getCharWidth((cursorpos<length) && ((c=getChar(cursorpos))>=' ')?c:' ');
    cursorx=getVisibleX()+marginleft+pos_x+xoffset(visrows[cursorrow-toprow],cursorpos)-1;
    if(getVisibleX()<=cursorx+tw+2 && cursorx-2<=getVisibleX()+getVisibleWidth()){
      dc.setClipRectangle(getVisibleX(),getVisibleY(),getVisibleWidth(),getVisibleHeight());
      if(0<dc.getClipWidth() && 0<dc.getClipHeight()){
        dc.setFont(font);
        dc.setForeground(backColor);
        dc.fillRectangle(cursorx-2,cursory,tw+4,th);
        cx=Math::imax(cursorx-2,getVisibleX()+marginleft);
        cy=getVisibleY()+margintop;
        cw=Math::imin(cursorx+tw+2,getVisibleX()+getVisibleWidth()-marginright)-cx;
        ch=getVisibleHeight()-margintop-marginbottom;
        dc.setClipRectangle(cx,cy,cw,ch);
        FXASSERT(toprow<=cursorrow && cursorrow<toprow+nvisrows);
        drawTextRow(dc,cursorrow);
        }
      }
    }
  }


// Erase cursor overhang outside of margins
void FXText::eraseCursorOverhang(){
  FXint th,tw,cursorx,cursory;
  FXwchar c;
  th=font->getFontHeight();
  cursory=getVisibleY()+margintop+pos_y+cursorrow*th;
  if(getVisibleY()+margintop<cursory+th && cursory<=getVisibleY()+getVisibleHeight()-marginbottom){
    FXASSERT(0<=cursorrow-toprow && cursorrow-toprow<nvisrows);
    tw=font->getCharWidth((cursorpos<length) && ((c=getChar(cursorpos))>=' ')?c:' ');
    cursorx=getVisibleX()+marginleft+pos_x+xoffset(visrows[cursorrow-toprow],cursorpos)-1;
    if(getVisibleX()<=cursorx+tw+2 && cursorx-2<=getVisibleX()+getVisibleWidth()){
      FXDCWindow dc(this);
      if(cursorx-2<=getVisibleX()+marginleft && getVisibleX()<=cursorx+tw+2){
        dc.setForeground(backColor);
        dc.fillRectangle(getVisibleX(),cursory,marginleft,th);
        }
      if(getVisibleX()+getVisibleWidth()-marginright<=cursorx+tw+2 && cursorx-2<=getVisibleX()+getVisibleWidth()){
        dc.setForeground(backColor);
        dc.fillRectangle(getVisibleX()+getVisibleWidth()-marginright,cursory,marginright,th);
        }
      if(cursory<=getVisibleY()+margintop && getVisibleY()<=cursory+th){
        dc.setForeground(backColor);
        dc.fillRectangle(cursorx-2,getVisibleY(),tw+4,margintop);
        }
      if(getVisibleY()+getVisibleHeight()-marginbottom<=cursory+th && cursory<getVisibleY()+getVisibleHeight()){
        dc.setForeground(backColor);
        dc.fillRectangle(cursorx-2,getVisibleY()+getVisibleHeight()-marginbottom,tw+4,marginbottom);
        }
      }
    }
  }

/*******************************************************************************/

// Draw fragment of text in given style
void FXText::drawBufferText(FXDCWindow& dc,FXint x,FXint y,FXint,FXint,FXint pos,FXint n,FXuint style) const {
  FXuint index=(style&STYLE_MASK);
  FXuint usedstyle=style;                                              // Style flags from style buffer
  FXColor color;
  FXchar str[2];
  color=0;
  if(hilitestyles && index){                                                    // Get colors from style table
    usedstyle=hilitestyles[index-1].style;                                      // Style flags now from style table
    if(style&STYLE_SELECTED) color=hilitestyles[index-1].selectForeColor;
    else if(style&STYLE_HILITE) color=hilitestyles[index-1].hiliteForeColor;
    if(color==0) color=hilitestyles[index-1].normalForeColor;                   // Fall back on normal foreground color
    }
  if(color==0){                                                                 // Fall back to default style
    if(style&STYLE_SELECTED) color=seltextColor;
    else if(style&STYLE_HILITE) color=hilitetextColor;
    if(color==0) color=textColor;                                               // Fall back to normal text color
    }
  dc.setForeground(color);
  if(style&STYLE_CONTROL){
    y+=font->getFontAscent();
    str[0]='^';
    while(pos<gapstart && 0<n){
      str[1]=buffer[pos]|0x40;
      dc.drawText(x,y,str,2);
      if(usedstyle&STYLE_BOLD) dc.drawText(x+1,y,str,2);
      x+=font->getTextWidth(str,2);
      pos++;
      n--;
      }
    while(0<n){
      str[1]=buffer[pos-gapstart+gapend]|0x40;
      dc.drawText(x,y,str,2);
      if(usedstyle&STYLE_BOLD) dc.drawText(x+1,y,str,2);
      x+=font->getTextWidth(str,2);
      pos++;
      n--;
      }
    }
  else{
    y+=font->getFontAscent();
    if(pos+n<=gapstart){
      dc.drawText(x,y,&buffer[pos],n);
      if(usedstyle&STYLE_BOLD) dc.drawText(x+1,y,&buffer[pos],n);
      }
    else if(pos>=gapstart){
      dc.drawText(x,y,&buffer[pos-gapstart+gapend],n);
      if(usedstyle&STYLE_BOLD) dc.drawText(x+1,y,&buffer[pos-gapstart+gapend],n);
      }
    else{
      dc.drawText(x,y,&buffer[pos],gapstart-pos);
      if(usedstyle&STYLE_BOLD) dc.drawText(x+1,y,&buffer[pos],gapstart-pos);
      x+=font->getTextWidth(&buffer[pos],gapstart-pos);
      dc.drawText(x,y,&buffer[gapend],pos+n-gapstart);
      if(usedstyle&STYLE_BOLD) dc.drawText(x+1,y,&buffer[gapend],pos+n-gapstart);
      }
    }
  }


// Fill fragment of background in given style
void FXText::fillBufferRect(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,FXuint style) const {
  FXuint index=(style&STYLE_MASK);
  FXuint usedstyle=style;                              // Style flags from style buffer
  FXColor bgcolor,fgcolor;
  bgcolor=fgcolor=0;
  if(hilitestyles && index){                                    // Get colors from style table
    usedstyle=hilitestyles[index-1].style;                      // Style flags now from style table
    if(style&STYLE_SELECTED){
      bgcolor=hilitestyles[index-1].selectBackColor;
      fgcolor=hilitestyles[index-1].selectForeColor;
      }
    else if(style&STYLE_HILITE){
      bgcolor=hilitestyles[index-1].hiliteBackColor;
      fgcolor=hilitestyles[index-1].hiliteForeColor;
      }
    else if(style&STYLE_ACTIVE){
      bgcolor=hilitestyles[index-1].activeBackColor;
      }
    else{
      bgcolor=hilitestyles[index-1].normalBackColor;
      }
    if(fgcolor==0){                                             // Fall back to normal foreground color
      fgcolor=hilitestyles[index-1].normalForeColor;
      }
    }
  if(bgcolor==0){                                               // Fall back to default background colors
    if(style&STYLE_SELECTED) bgcolor=selbackColor;
    else if(style&STYLE_HILITE) bgcolor=hilitebackColor;
    else if(style&STYLE_ACTIVE) bgcolor=activebackColor;
    else bgcolor=backColor;
    }
  if(fgcolor==0){                                               // Fall back to default foreground colors
    if(style&STYLE_SELECTED) fgcolor=seltextColor;
    else if(style&STYLE_HILITE) fgcolor=hilitetextColor;
    if(fgcolor==0) fgcolor=textColor;                           // Fall back to text color
    }
  dc.setForeground(bgcolor);
  dc.fillRectangle(x,y,w,h);
  if(style&STYLE_INSERT){                                       // Vertical insertion point
    dc.setForeground(cursorColor);                              // Use cursor color for now
    dc.fillRectangle(x,y,1,h);
    }
  if(usedstyle&STYLE_UNDERLINE){
    dc.setForeground(fgcolor);
    dc.fillRectangle(x,y+font->getFontAscent()+1,w,1);
    }
  if(usedstyle&STYLE_STRIKEOUT){
    dc.setForeground(fgcolor);
    dc.fillRectangle(x,y+font->getFontAscent()/2,w,1);
    }
  }


// Obtain text style given line range, row, column, and position
// Note that for block selections, the column may be outside the text, but need
// to be on non-empty lines.
FXuint FXText::styleOf(FXint beg,FXint end,FXint row,FXint col,FXint pos) const {
  FXuint style=0;

  // Current active line
  if((row==cursorrow) && (options&TEXT_SHOWACTIVE)) style|=STYLE_ACTIVE;

  // Need non-empty line
  if(beg<end){

    // Selected range or block
    if(select.startpos<=pos){
      if(select.startcol>select.endcol){
        if(pos<select.endpos) style|=STYLE_SELECTED;
        }
      else if(pos<=select.endpos){
        if(select.startcol<=col && col<select.endcol) style|=STYLE_SELECTED;
        if(select.startcol==col && select.endcol==col) style|=STYLE_INSERT;
        }
      }

    // Highlighted range or block
    if(hilite.startpos<=pos){
      if(hilite.startcol>hilite.endcol){
        if(pos<hilite.endpos) style|=STYLE_HILITE;
        }
      else if(pos<=hilite.endpos){
        if(hilite.startcol<=col && col<hilite.endcol) style|=STYLE_HILITE;
        }
      }

    // Inside text
    if(pos<end){

      // Get character
      FXuchar c=getByte(pos);

      // Get value from style buffer
      if(sbuffer) style|=getStyle(pos);

      // Tab or whitespace
      if(c=='\t') return style;
      if(c==' ') return style;

      // Control codes
      if(c<' ') style|=STYLE_CONTROL;

      // Normal character
      style|=STYLE_TEXT;
      }
    }

  return style;
  }


// Draw line of text from the buffer, skipping over the parts outside
// of the current clip rectangle.
void FXText::drawTextRow(FXDCWindow& dc,FXint row) const {
  FXint spacew=font->getCharWidth(' ');
  FXint caretw=font->getCharWidth('^');
  FXint th=font->getFontHeight();
  FXint tx=getVisibleX()+marginleft+pos_x;
  FXint ty=getVisibleY()+margintop+pos_y+row*th;
  FXint leftclip=dc.getClipX();
  FXint riteclip=dc.getClipX()+dc.getClipWidth();
  FXint linebeg=visrows[row-toprow];
  FXint lineend=visrows[row-toprow+1];
  FXint linebreak=lineend;
  FXint tcol=0,twid=0,tadj=0;
  FXint cw,cc,pc,cx,px,cp,pp;
  FXuint curstyle,newstyle;
  FXwchar c;

  FXASSERT(toprow<=row && row<toprow+nvisrows);
  FXASSERT(0<=linebeg && lineend<=length);

  // If last character on a line is newline or space, back off by one
  // character position, and interpret all subsequent columns as spaces.
  if(linebeg<lineend){
    linebreak=dec(lineend);
    FXASSERT(linebeg<=linebreak);
    if(!Unicode::isSpace(getChar(linebreak))){
      linebreak=lineend;
      }
    }

  // Reset running variables
  cc=0;
  cx=tx;
  cp=linebeg;

  // Scan forward to get past left edge
  do{
    px=cx;
    pc=cc;
    pp=cp;
    if(cp>=linebreak){                          // Character past end of line
      cx+=spacew;
      cc+=1;
      continue;
      }
    c=getChar(cp);
    if(' '<=c){                                 // Normal character
      cx+=font->getCharWidth(c);
      cc+=1;
      cp+=getCharLen(cp);
      continue;
      }
    if(c=='\t'){                                // Tab character
      cx+=tabwidth-(cx-tx)%tabwidth;
      cc+=tabcolumns-cc%tabcolumns;
      cp+=1;
      continue;
      }
    cx+=caretw+font->getCharWidth(c|0x40);      // Control character
    cc+=1;
    cp+=1;
    }
  while(cx<leftclip);

  // Roll back to just before edge
  cx=px;
  cc=pc;
  cp=pp;

  // First style to display
  curstyle=styleOf(linebeg,lineend,row,cc,cp);

  // Draw segments of uniformly styled text
  do{
    newstyle=styleOf(linebeg,lineend,row,cc,cp);
    if(newstyle!=curstyle){                     // Found a style change!
      fillBufferRect(dc,px,ty,cx-px,th,curstyle);
      if(curstyle&STYLE_TEXT) drawBufferText(dc,px,ty,cx-px,th,pp,cp-pp,curstyle);
      curstyle=newstyle;
      pp=cp;
      pc=cc;
      px=cx;
      }
    if(cp>=linebreak){                          // Character past end of line
      cx+=spacew;
      cc+=1;
      continue;
      }
    c=getChar(cp);
    if(' '<=c){                                 // Normal character
      cx+=font->getCharWidth(c);
      cc+=1;
      cp+=getCharLen(cp);
      continue;
      }
    if(c=='\t'){                                // Tab character
      if(tcol==0){
        cw=tabwidth-(cx-tx)%tabwidth;
        tcol=tabcolumns-cc%tabcolumns;
        twid=cw/tcol;
        tadj=cw-twid*tcol;
        }
      cx+=twid+(tadj>0);                        // Mete out columns comprising the tab character
      tcol-=1;
      tadj-=1;
      cc+=1;
      cp+=(tcol==0);
      continue;
      }
    cx+=caretw+font->getCharWidth(c|0x40);      // Control character
    cc+=1;
    cp+=1;
    }
  while(cx<riteclip);

  // Draw unfinished fragment
  fillBufferRect(dc,px,ty,cx-px,th,curstyle);
  if(curstyle&STYLE_TEXT) drawBufferText(dc,px,ty,cx-px,th,pp,cp-pp,curstyle);
  }


// Repaint lines of text
// Erase margins, then draw text one line at a time to reduce flicker.
// Only draw if intersection of bar area and dirty rectangle is non-empty
void FXText::drawContents(FXDCWindow& dc) const {
  FXint vx=getVisibleX();
  FXint vy=getVisibleY();
  FXint vw=getVisibleWidth();
  FXint vh=getVisibleHeight();
  dc.setClipRectangle(vx,vy,vw,vh);
  if(0<dc.getClipWidth() && 0<dc.getClipHeight()){
    FXint th,row,trow,brow;
    dc.setForeground(backColor);
    if(dc.getClipY()<=vy+margintop){
      dc.fillRectangle(vx,vy,vw,margintop);
      }
    if(dc.getClipY()+dc.getClipHeight()>=vy+vh-marginbottom){
      dc.fillRectangle(vx,vy+vh-marginbottom,vw,marginbottom);
      }
    if(dc.getClipX()<vx+marginleft){
      dc.fillRectangle(vx,vy+margintop,marginleft,vh-margintop-marginbottom);
      }
    if(dc.getClipX()+dc.getClipWidth()>=vx+vw-marginright){
      dc.fillRectangle(vx+vw-marginright,vy+margintop,marginright,vh-margintop-marginbottom);
      }
    th=font->getFontHeight();
    trow=(dc.getClipY()-pos_y-vy-margintop)/th;
    brow=(dc.getClipY()+dc.getClipHeight()-pos_y-vy-margintop)/th;
    if(trow<=toprow) trow=toprow;
    if(brow>=toprow+nvisrows) brow=toprow+nvisrows-1;
    dc.setClipRectangle(vx+marginleft,vy+margintop,vw-marginright-marginleft,vh-margintop-marginbottom);
    for(row=trow; row<=brow; row++){
      drawTextRow(dc,row);
      }
    }
  }


// Repaint line numbers
// Erase and redraw number one at a time, instead of erasing all background
// and then drawing numbers on top; this leads to less flicker.
// Only draw if intersection of bar area and dirty rectangle is non-empty
void FXText::drawNumbers(FXDCWindow& dc) const {
  FXint vx=getVisibleX();
  FXint vy=getVisibleY();
  FXint vh=getVisibleHeight();
  dc.setClipRectangle(0,vy,vx,vh);
  if(0<dc.getClipWidth() && 0<dc.getClipHeight()){
    FXint tw,th,trow,brow,row,n;
    FXchar number[20];
    dc.setForeground(barColor);
    if(dc.getClipY()<=vy+margintop){
      dc.fillRectangle(0,vy,vx,margintop);
      }
    if(dc.getClipY()+dc.getClipHeight()>=vy+vh-marginbottom){
      dc.fillRectangle(0,vy+vh-marginbottom,vx,marginbottom);
      }
    th=font->getFontHeight();
    trow=(dc.getClipY()-pos_y-vy-margintop)/th;
    brow=(dc.getClipY()+dc.getClipHeight()-pos_y-vy-margintop)/th;
    if(trow<=toprow) trow=toprow;
    if(brow>=toprow+nvisrows) brow=toprow+nvisrows;
    dc.setClipRectangle(0,vy+margintop,vx,vh-margintop-marginbottom);
    for(row=trow; row<=brow; row++){
      n=__snprintf(number,sizeof(number),"%d",row+1);
      tw=font->getTextWidth(number,n);
      dc.setForeground(barColor);
      dc.fillRectangle(0,pos_y+vy+margintop+row*th,vx,th);
      dc.setForeground(numberColor);
      dc.drawText(vx-tw,pos_y+vy+margintop+row*th+font->getFontAscent(),number,n);
      }
    }
  }


// Repaint the row
void FXText::updateRow(FXint row) const {
  if(toprow<=row && row<=toprow+nvisrows){
    update(getVisibleX(),getVisibleY()+margintop+pos_y+row*font->getFontHeight(),getVisibleWidth(),font->getFontHeight());
    }
  }


// Update whole lines
void FXText::updateLines(FXint startpos,FXint endpos) const {
  FXint b,e,tr,br,ty,by;
  FXMINMAX(b,e,startpos,endpos);
  if(b<=visrows[nvisrows] && visrows[0]<e){
    if(b<visrows[0]) b=visrows[0];
    if(e>visrows[nvisrows-1]) e=visrows[nvisrows-1];
    tr=rowFromPos(b);
    br=rowFromPos(e);
    ty=getVisibleY()+margintop+pos_y+tr*font->getFontHeight();
    by=getVisibleY()+margintop+pos_y+br*font->getFontHeight()+font->getFontHeight();
    update(getVisibleX(),ty,getVisibleWidth(),by-ty);
    }
  }


// Repaint text range
void FXText::updateRange(FXint startpos,FXint endpos) const {
  FXint b,e,vx,vy,vw,tr,br,lx,rx,ty,by;
  FXMINMAX(b,e,startpos,endpos);
  if(b<=visrows[nvisrows] && visrows[0]<e){
    if(b<visrows[0]) b=visrows[0];
    if(e>visrows[nvisrows-1]) e=visrows[nvisrows-1];
    vx=getVisibleX();
    vy=getVisibleY();
    vw=getVisibleWidth();
    tr=rowFromPos(b);
    br=rowFromPos(e);
    if(tr==br){
      ty=pos_y+vy+margintop+tr*font->getFontHeight();
      by=ty+font->getFontHeight();
      lx=vx+pos_x+marginleft+xoffset(visrows[tr-toprow],b);
      if(e<=(visrows[tr-toprow+1]-1))
        rx=vx+pos_x+marginleft+xoffset(visrows[tr-toprow],e);
      else
        rx=vx+vw;
      }
    else{
      ty=vy+pos_y+margintop+tr*font->getFontHeight();
      by=vy+pos_y+margintop+br*font->getFontHeight()+font->getFontHeight();
      lx=vx;
      rx=lx+vw;
      }
    update(lx,ty,rx-lx,by-ty);
    }
  }


// Draw the text
long FXText::onPaint(FXObject*,FXSelector,void* ptr){
  FXDCWindow dc(this,(FXEvent*)ptr);

  // Set font
  dc.setFont(font);

//dc.setForeground(FXRGB(255,0,0));
//dc.fillRectangle(0,0,width,height);

  // Paint text
  drawContents(dc);

  // Paint line numbers if turned on
  if(barwidth){
    drawNumbers(dc);
    }

  // Paint cursor
  if(flags&FLAG_CARET){
    paintCursor(dc);
    }
  return 1;
  }

/*******************************************************************************/

// Blink the cursor
long FXText::onBlink(FXObject*,FXSelector,void*){
  drawCursor(blink);
  blink^=FLAG_CARET;
  getApp()->addTimeout(this,ID_BLINK,getApp()->getBlinkSpeed());
  return 0;
  }


// Flash matching brace
long FXText::onFlash(FXObject*,FXSelector,void*){
  killHighlight();
  return 0;
  }


// Start motion timer while in this window
long FXText::onEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onEnter(sender,sel,ptr);
  getApp()->addTimeout(this,ID_TIPTIMER,getApp()->getMenuPause());
  return 1;
  }


// Stop motion timer when leaving window
long FXText::onLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onLeave(sender,sel,ptr);
  getApp()->removeTimeout(this,ID_TIPTIMER);
  return 1;
  }


// Gained focus
long FXText::onFocusIn(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onFocusIn(sender,sel,ptr);
  if(isEditable()){
    getApp()->addTimeout(this,ID_BLINK,getApp()->getBlinkSpeed());
    drawCursor(FLAG_CARET);
    }
  return 1;
  }


// Lost focus
long FXText::onFocusOut(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onFocusOut(sender,sel,ptr);
  if(isEditable()){
    getApp()->removeTimeout(this,ID_BLINK);
    drawCursor(0);
    }
  flags|=FLAG_UPDATE;
  return 1;
  }

/*******************************************************************************/

// Update value from a message
long FXText::onCmdSetStringValue(FXObject*,FXSelector,void* ptr){
  setText(*((FXString*)ptr));
  return 1;
  }


// Obtain value from text
long FXText::onCmdGetStringValue(FXObject*,FXSelector,void* ptr){
  getText(*((FXString*)ptr));
  return 1;
  }

/*******************************************************************************/

// Set tip using a message
long FXText::onCmdSetTip(FXObject*,FXSelector,void* ptr){
  setTipText(*((FXString*)ptr));
  return 1;
  }


// Get tip using a message
long FXText::onCmdGetTip(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getTipText();
  return 1;
  }


// Set help using a message
long FXText::onCmdSetHelp(FXObject*,FXSelector,void* ptr){
  setHelpText(*((FXString*)ptr));
  return 1;
  }


// Get help using a message
long FXText::onCmdGetHelp(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getHelpText();
  return 1;
  }


// We were asked about tip text
long FXText::onQueryTip(FXObject* sender,FXSelector sel,void* ptr){
  if(FXScrollArea::onQueryTip(sender,sel,ptr)) return 1;
  if((flags&FLAG_TIP) && !tip.empty()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&tip);
    return 1;
    }
  return 0;
  }


// We were asked about status text
long FXText::onQueryHelp(FXObject* sender,FXSelector sel,void* ptr){
  if(FXScrollArea::onQueryHelp(sender,sel,ptr)) return 1;
  if((flags&FLAG_HELP) && !help.empty()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&help);
    return 1;
    }
  return 0;
  }


// Update somebody who wants to change the text
long FXText::onUpdIsEditable(FXObject* sender,FXSelector,void*){
  sender->handle(this,isEditable()?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Update somebody who works on the selection
long FXText::onUpdHaveSelection(FXObject* sender,FXSelector,void*){
  FXbool selection=((select.startcol<select.endcol) && (select.startpos<=select.endpos)) || ((select.startcol>select.endcol) && (select.startpos<select.endpos));
  sender->handle(this,selection?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Update somebody who works on the selection and change the text
long FXText::onUpdHaveEditableSelection(FXObject* sender,FXSelector,void*){
  FXbool selection=((select.startcol<select.endcol) && (select.startpos<=select.endpos)) || ((select.startcol>select.endcol) && (select.startpos<select.endpos));
  sender->handle(this,isEditable() && selection?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Start input method editor
long FXText::onIMEStart(FXObject*,FXSelector,void*){
  if(isEditable()){
    if(getComposeContext()){
      FXint th=font->getFontHeight();
      FXint cursory=getVisibleY()+margintop+pos_y+(cursorrow*th)+th;
      if(getVisibleY()<=cursory+th && cursory<=getVisibleY()+getVisibleHeight()){
        FXASSERT(0<=cursorrow-toprow && cursorrow-toprow<nvisrows);
        FXint cursorstart=visrows[cursorrow-toprow];
        FXint cursorx=getVisibleX()+marginleft+pos_x+xoffset(cursorstart,cursorpos)-1;
        getComposeContext()->setSpot(cursorx,cursory);
        }
      }
    return 1;
    }
  return 0;
  }

/*******************************************************************************/

// Start a drag operation
long FXText::onBeginDrag(FXObject* sender,FXSelector sel,void* ptr){
  FXDragType types[4]={stringType,textType,utf8Type,utf16Type};
  if(!FXScrollArea::onBeginDrag(sender,sel,ptr)){
    beginDrag(types,ARRAYNUMBER(types));
    setDragCursor(getApp()->getDefaultCursor(DEF_DNDSTOP_CURSOR));
    }
  return 1;
  }


// End drag operation
long FXText::onEndDrag(FXObject* sender,FXSelector sel,void* ptr){
  if(!FXScrollArea::onEndDrag(sender,sel,ptr)){
    endDrag((didAccept()!=DRAG_REJECT));
    setDragCursor(getApp()->getDefaultCursor(DEF_TEXT_CURSOR));
    }
  return 1;
  }


// Dragged stuff around
long FXText::onDragged(FXObject* sender,FXSelector sel,void* ptr){
  if(!FXScrollArea::onDragged(sender,sel,ptr)){
    FXDragAction action=DRAG_COPY;
    if(isEditable()){
      if(isDropTarget()) action=DRAG_MOVE;
      if(((FXEvent*)ptr)->state&CONTROLMASK) action=DRAG_COPY;
      if(((FXEvent*)ptr)->state&SHIFTMASK) action=DRAG_MOVE;
      }
    handleDrag(((FXEvent*)ptr)->root_x,((FXEvent*)ptr)->root_y,action);
    action=didAccept();
    switch(action){
      case DRAG_MOVE:
        setDragCursor(getApp()->getDefaultCursor(DEF_DNDMOVE_CURSOR));
        break;
      case DRAG_COPY:
        setDragCursor(getApp()->getDefaultCursor(DEF_DNDCOPY_CURSOR));
        break;
      default:
        setDragCursor(getApp()->getDefaultCursor(DEF_DNDSTOP_CURSOR));
        break;
      }
    }
  return 1;
  }


// Handle drag-and-drop enter
long FXText::onDNDEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onDNDEnter(sender,sel,ptr);
  if(isEditable()){
    drawCursor(FLAG_CARET);
    }
  return 1;
  }


// Handle drag-and-drop leave
long FXText::onDNDLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onDNDLeave(sender,sel,ptr);
  stopAutoScroll();
  if(isEditable()){
    drawCursor(0);
    }
  return 1;
  }


// Handle drag-and-drop motion
long FXText::onDNDMotion(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;

  // Scroll into view
  if(startAutoScroll(event,true)) return 1;

  // Handled elsewhere
  if(FXScrollArea::onDNDMotion(sender,sel,ptr)) return 1;

  // Correct drop type
  if(offeredDNDType(FROM_DRAGNDROP,textType) || offeredDNDType(FROM_DRAGNDROP,stringType) || offeredDNDType(FROM_DRAGNDROP,utf8Type) || offeredDNDType(FROM_DRAGNDROP,utf16Type)){

    // Is target editable?
    if(isEditable()){
      FXDragAction action=inquireDNDAction();

      // Check for legal DND action
      if(action==DRAG_COPY || action==DRAG_MOVE){
        FXint pos,row,col;

        // Get the suggested drop position
        pos=getRowColumnAt(event->win_x,event->win_y,row,col);

        // Move cursor to new position
        setCursorPos(pos,true);

        // We don't accept a drop on the selection
        if(!isPosSelected(pos,col)){
          acceptDrop(DRAG_ACCEPT);
          }
        }
      }
    return 1;
    }

  // Didn't handle it here
  return 0;
  }


// Handle drag-and-drop drop
long FXText::onDNDDrop(FXObject* sender,FXSelector sel,void* ptr){

  // Stop scrolling
  stopAutoScroll();
  drawCursor(0);

  // Try handling it in base class first
  if(FXScrollArea::onDNDDrop(sender,sel,ptr)) return 1;

  // Should really not have gotten this if non-editable
  if(isEditable()){
    FXString string;
    FXString junk;

    // First, try UTF-8
    if(getDNDData(FROM_DRAGNDROP,utf8Type,string)){
      if(inquireDNDAction()==DRAG_MOVE){
        getDNDData(FROM_DRAGNDROP,deleteType,junk);
        }
      replaceText(cursorpos,0,string,true);
      setCursorPos(cursorpos,true);
      return 1;
      }

    // Next, try UTF-16
    if(getDNDData(FROM_DRAGNDROP,utf16Type,string)){
      if(inquireDNDAction()==DRAG_MOVE){
        getDNDData(FROM_DRAGNDROP,deleteType,junk);
        }
      replaceText(cursorpos,0,string,true);
      setCursorPos(cursorpos,true);
      return 1;
      }

    // Next, try good old Latin-1
    if(getDNDData(FROM_DRAGNDROP,textType,string)){
      if(inquireDNDAction()==DRAG_MOVE){
        getDNDData(FROM_DRAGNDROP,deleteType,junk);
        }
      replaceText(cursorpos,0,string,true);
      setCursorPos(cursorpos,true);
      return 1;
      }
    return 1;
    }
  return 0;
  }


// Service requested DND data
long FXText::onDNDRequest(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent *event=(FXEvent*)ptr;

  // Perhaps the target wants to supply its own data
  if(FXScrollArea::onDNDRequest(sender,sel,ptr)) return 1;

  // Recognize the request?
  if(event->target==stringType || event->target==textType || event->target==utf8Type || event->target==utf16Type){
    FXString string;

    // Get selected fragment
    string=getSelectedText();

    // Return text of the selection as UTF-8
    if(event->target==utf8Type){
      setDNDData(FROM_DRAGNDROP,event->target,string);
      return 1;
      }

    // Return text of the selection translated to 8859-1
    if(event->target==stringType || event->target==textType){
      setDNDData(FROM_DRAGNDROP,event->target,string);
      return 1;
      }

    // Return text of the selection translated to UTF-16
    if(event->target==utf16Type){
      setDNDData(FROM_DRAGNDROP,event->target,string);
      return 1;
      }
    }

  // Delete dragged text, if editable
  if(event->target==deleteType){
    if(isEditable()){
      if(select.startcol<=select.endcol){
        removeTextBlock(select.startpos,select.endpos,select.startcol,select.endcol,true);
        }
      else{
        removeText(select.startpos,select.endpos-select.startpos,true);
        }
      }
    return 1;
    }

  return 0;
  }

/*******************************************************************************/

// We now really do have the selection
long FXText::onSelectionGained(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onSelectionGained(sender,sel,ptr);
  return 1;
  }


// We lost the selection somehow
long FXText::onSelectionLost(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onSelectionLost(sender,sel,ptr);
  if(target){
    FXint what[4];
    what[0]=select.startpos;
    what[1]=select.endpos-select.startpos;
    what[2]=select.startcol;
    what[3]=select.endcol-select.startcol;
    target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)what);
    }
  updateRange(select.startpos,select.endpos);
  select.startpos=0;
  select.endpos=-1;
  select.startcol=0;
  select.endcol=-1;
  return 1;
  }


// Somebody wants our selection
long FXText::onSelectionRequest(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent *event=(FXEvent*)ptr;

  // Perhaps the target wants to supply its own data for the selection
  if(FXScrollArea::onSelectionRequest(sender,sel,ptr)) return 1;

  // Recognize the request?
  if(event->target==stringType || event->target==textType || event->target==utf8Type || event->target==utf16Type){

    // Get selected fragment
    FXString string=getSelectedText();

    // Return text of the selection as UTF-8
    if(event->target==utf8Type){
      setDNDData(FROM_SELECTION,event->target,string);
      return 1;
      }

    // Return text of the selection translated to 8859-1
    if(event->target==stringType || event->target==textType){
      setDNDData(FROM_SELECTION,event->target,string);
      return 1;
      }

    // Return text of the selection translated to UTF-16
    if(event->target==utf16Type){
      setDNDData(FROM_SELECTION,event->target,string);
      return 1;
      }
    }
  return 0;
  }

/*******************************************************************************/

// We now really do have the selection
long FXText::onClipboardGained(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onClipboardGained(sender,sel,ptr);
  return 1;
  }


// We lost the selection somehow
long FXText::onClipboardLost(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onClipboardLost(sender,sel,ptr);
  clipped.clear();
  return 1;
  }


// Somebody wants our selection
long FXText::onClipboardRequest(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXString string=clipped;

  // Try handling it in base class first
  if(FXScrollArea::onClipboardRequest(sender,sel,ptr)) return 1;

  // Requested data from clipboard
  if(event->target==stringType || event->target==textType || event->target==utf8Type || event->target==utf16Type){

    // Expand newlines to CRLF on Windows
#ifdef WIN32
    unixToDos(string);
#endif

    // Return clipped text as as UTF-8
    if(event->target==utf8Type){
      setDNDData(FROM_CLIPBOARD,event->target,string);
      return 1;
      }

    // Return clipped text translated to 8859-1
    if(event->target==stringType || event->target==textType){
      setDNDData(FROM_CLIPBOARD,event->target,string);
      return 1;
      }

    // Return text of the selection translated to UTF-16
    if(event->target==utf16Type){
      setDNDData(FROM_CLIPBOARD,event->target,string);
      return 1;
      }
    }
  return 0;
  }

/*******************************************************************************/

// Pressed left button
long FXText::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint pos,row,col;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    grabx=event->win_x-pos_x;
    graby=event->win_y-pos_y;
    if(event->click_count==1){
      pos=getRowColumnAt(event->win_x,event->win_y,row,col);
      if((event->state&CONTROLMASK) && !(options&TEXT_WORDWRAP)){
        if(event->state&SHIFTMASK){                     // Shift-select block
          moveCursorRowColumnAndSelect(row,col,true);
          }
        else{                                           // Drag select block
          moveCursorRowColumn(row,col,true);
          }
        mode=MOUSE_BLOCK;
        }
      else{
        if(event->state&SHIFTMASK){                     // Shift-select range
          moveCursorAndSelect(pos,SelectChars,true);
          }
        else{                                           // Drag select range
          moveCursor(pos,true);
          }
        mode=MOUSE_CHARS;
        }
      }
    else if(event->click_count==2){     // Drag select words
      pos=getPosContaining(event->win_x,event->win_y);
      setAnchorPos(pos);
      moveCursorAndSelect(pos,SelectWords,true);
      mode=MOUSE_WORDS;
      }
    else{                               // Drag select lines
      pos=getPosAt(event->win_x,event->win_y);
      moveCursorAndSelect(pos,SelectLines,true);
      mode=MOUSE_LINES;
      }
    flags&=~FLAG_UPDATE;
    return 1;
    }
  return 0;
  }


// Released left button
long FXText::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    mode=MOUSE_NONE;
    stopAutoScroll();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;
    return 1;
    }
  return 0;
  }


// Pressed middle button
long FXText::onMiddleBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint pos,row,col;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_MIDDLEBUTTONPRESS,message),ptr)) return 1;
    pos=getRowColumnAt(event->win_x,event->win_y,row,col);
    setCursorPos(pos,true);
    setAnchorPos(cursorpos);
    if(isPosSelected(cursorpos,col)){
      mode=MOUSE_TRYDRAG;
      }
    flags&=~FLAG_UPDATE;
    return 1;
    }
  return 0;
  }


// Released middle button
long FXText::onMiddleBtnRelease(FXObject*,FXSelector,void* ptr){
  FXuint md=mode;
  if(isEnabled()){
    ungrab();
    stopAutoScroll();
    mode=MOUSE_NONE;
    if(target && target->tryHandle(this,FXSEL(SEL_MIDDLEBUTTONRELEASE,message),ptr)) return 1;
    if(md==MOUSE_DRAG){
      handle(this,FXSEL(SEL_ENDDRAG,0),ptr);
      }
    else{
      handle(this,FXSEL(SEL_COMMAND,ID_PASTE_MIDDLE),nullptr);
      }
    return 1;
    }
  return 0;
  }


// Pressed right button
long FXText::onRightBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONPRESS,message),ptr)) return 1;
    grabx=event->win_x-pos_x;
    graby=event->win_y-pos_y;
    mode=MOUSE_SCROLL;
    flags&=~FLAG_UPDATE;
    return 1;
    }
  return 0;
  }


// Released right button
long FXText::onRightBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    mode=MOUSE_NONE;
    if(target && target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONRELEASE,message),ptr)) return 1;
    return 1;
    }
  return 0;
  }


// Handle real or simulated mouse motion
long FXText::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint pos,row,col;
  flags&=~FLAG_TIP;
  getApp()->removeTimeout(this,ID_TIPTIMER);
  switch(mode){
    case MOUSE_NONE:
      getApp()->addTimeout(this,ID_TIPTIMER,getApp()->getMenuPause());
      return 1;
    case MOUSE_CHARS:
      if(startAutoScroll(event,false)) return 1;
      if((Math::iabs(event->win_x-grabx-pos_x)>getApp()->getDragDelta())||(Math::iabs(event->win_y-graby-pos_y)>getApp()->getDragDelta())){
        killHighlight();
        pos=getPosAt(event->win_x,event->win_y);
        setCursorPos(pos,true);
        extendSelection(cursorpos,SelectChars,true);
        }
      return 1;
    case MOUSE_WORDS:
      if(startAutoScroll(event,false)) return 1;
      if((Math::iabs(event->win_x-grabx-pos_x)>getApp()->getDragDelta())||(Math::iabs(event->win_y-graby-pos_y)>getApp()->getDragDelta())){
        killHighlight();
        pos=getPosContaining(event->win_x,event->win_y);
        setCursorPos(pos,true);
        extendSelection(cursorpos,SelectWords,true);
        }
      return 1;
    case MOUSE_LINES:
      if(startAutoScroll(event,false)) return 1;
      if((Math::iabs(event->win_x-grabx-pos_x)>getApp()->getDragDelta())||(Math::iabs(event->win_y-graby-pos_y)>getApp()->getDragDelta())){
        killHighlight();
        pos=getPosAt(event->win_x,event->win_y);
        setCursorPos(pos,true);
        extendSelection(cursorpos,SelectLines,true);
        }
      return 1;
    case MOUSE_BLOCK:
      if(startAutoScroll(event,false)) return 1;
      if((Math::iabs(event->win_x-grabx-pos_x)>getApp()->getDragDelta())||(Math::iabs(event->win_y-graby-pos_y)>getApp()->getDragDelta())){
        killHighlight();
        getRowColumnAt(event->win_x,event->win_y,row,col);
        setCursorRowColumn(row,col,true);
        extendBlockSelection(row,col,true);
        }
      return 1;
    case MOUSE_SCROLL:
      setPosition(event->win_x-grabx,event->win_y-graby);
      return 1;
    case MOUSE_DRAG:
      handle(this,FXSEL(SEL_DRAGGED,0),ptr);
      return 1;
    case MOUSE_TRYDRAG:
      if(event->moved){
        mode=MOUSE_NONE;
        if(handle(this,FXSEL(SEL_BEGINDRAG,0),ptr)){
          mode=MOUSE_DRAG;
          }
        }
      return 1;
    }
  return 0;
  }


// Autoscroll timer fired; autoscrolling hysteresis is based on movement
// relative to the original document position of the click, in case the
// click-position is close to the autoscrolling fudge-border.
long FXText::onAutoScroll(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint pos,row,col;
  FXScrollArea::onAutoScroll(sender,sel,ptr);
  switch(mode){
    case MOUSE_CHARS:
      if((Math::iabs(event->win_x-grabx-pos_x)>getApp()->getDragDelta())||(Math::iabs(event->win_y-graby-pos_y)>getApp()->getDragDelta())){
        killHighlight();
        pos=getPosAt(event->win_x,event->win_y);
        extendSelection(pos,SelectChars,true);
        setCursorPos(pos,true);
        }
      return 1;
    case MOUSE_WORDS:
      if((Math::iabs(event->win_x-grabx-pos_x)>getApp()->getDragDelta())||(Math::iabs(event->win_y-graby-pos_y)>getApp()->getDragDelta())){
        killHighlight();
        pos=getPosContaining(event->win_x,event->win_y);
        extendSelection(pos,SelectWords,true);
        setCursorPos(pos,true);
        }
      return 1;
    case MOUSE_LINES:
      if((Math::iabs(event->win_x-grabx-pos_x)>getApp()->getDragDelta())||(Math::iabs(event->win_y-graby-pos_y)>getApp()->getDragDelta())){
        killHighlight();
        pos=getPosAt(event->win_x,event->win_y);
        extendSelection(pos,SelectLines,true);
        setCursorPos(pos,true);
        }
      return 1;
    case MOUSE_BLOCK:
      if((Math::iabs(event->win_x-grabx-pos_x)>getApp()->getDragDelta())||(Math::iabs(event->win_y-graby-pos_y)>getApp()->getDragDelta())){
        killHighlight();
        getRowColumnAt(event->win_x,event->win_y,row,col);
        extendBlockSelection(row,col,true);
        setCursorRowColumn(row,col,true);
        }
      return 1;
    }
  return 0;
  }


// The widget lost the grab for some reason
long FXText::onUngrabbed(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onUngrabbed(sender,sel,ptr);
  mode=MOUSE_NONE;
  flags|=FLAG_UPDATE;
  stopAutoScroll();
  return 1;
  }


// Mouse hovered a while
long FXText::onTipTimer(FXObject*,FXSelector,void*){
  FXTRACE((250,"%s::onTipTimer %p\n",getClassName(),this));
  flags|=FLAG_TIP;
  return 1;
  }

/*******************************************************************************/

// Keyboard press
long FXText::onKeyPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  if(isEnabled()){
    FXEvent* event=(FXEvent*)ptr;
    FXTRACE((TOPIC_KEYBOARD,"%s::onKeyPress keysym=0x%04x state=%04x\n",getClassName(),event->code,event->state));
    if(target && target->tryHandle(this,FXSEL(SEL_KEYPRESS,message),ptr)) return 1;
    flags&=~FLAG_UPDATE;
    switch(event->code){
      case KEY_Shift_L:
      case KEY_Shift_R:
      case KEY_Control_L:
      case KEY_Control_R:
        if(mode==MOUSE_DRAG){handle(this,FXSEL(SEL_DRAGGED,0),ptr);}
        return 1;
      case KEY_Up:
      case KEY_KP_Up:
        if(event->state&CONTROLMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_SCROLL_UP),nullptr);
          }
        else if(event->state&SHIFTMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_SHIFT_UP),nullptr);
          }
        else if(event->state&ALTMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_ALT_UP),nullptr);
          }
        else{
          handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_UP),nullptr);
          }
        break;
      case KEY_Down:
      case KEY_KP_Down:
        if(event->state&CONTROLMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_SCROLL_DOWN),nullptr);
          }
        else if(event->state&SHIFTMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_SHIFT_DOWN),nullptr);
          }
        else if(event->state&ALTMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_ALT_DOWN),nullptr);
          }
        else{
          handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_DOWN),nullptr);
          }
        break;
      case KEY_Left:
      case KEY_KP_Left:
        if(event->state&CONTROLMASK){
          if(event->state&SHIFTMASK){
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_SHIFT_WORD_LEFT),nullptr);
            }
          else{
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_WORD_LEFT),nullptr);
            }
          }
        else{
          if(event->state&SHIFTMASK){
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_SHIFT_LEFT),nullptr);
            }
          else if(event->state&ALTMASK){
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_ALT_LEFT),nullptr);
            }
          else{
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_LEFT),nullptr);
            }
          }
        break;
      case KEY_Right:
      case KEY_KP_Right:
        if(event->state&CONTROLMASK){
          if(event->state&SHIFTMASK){
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_SHIFT_WORD_RIGHT),nullptr);
            }
          else{
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_WORD_RIGHT),nullptr);
            }
          }
        else{
          if(event->state&SHIFTMASK){
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_SHIFT_RIGHT),nullptr);
            }
          else if(event->state&ALTMASK){
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_ALT_RIGHT),nullptr);
            }
          else{
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_RIGHT),nullptr);
            }
          }
        break;
      case KEY_Home:
      case KEY_KP_Home:
        if(event->state&CONTROLMASK){
          if(event->state&SHIFTMASK){
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_SHIFT_TOP),nullptr);
            }
          else{
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_TOP),nullptr);
            }
          }
        else{
          if(event->state&SHIFTMASK){
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_SHIFT_HOME),nullptr);
            }
          else{
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_HOME),nullptr);
            }
          }
        break;
      case KEY_End:
      case KEY_KP_End:
        if(event->state&CONTROLMASK){
          if(event->state&SHIFTMASK){
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_SHIFT_BOTTOM),nullptr);
            }
          else{
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_BOTTOM),nullptr);
            }
          }
        else{
          if(event->state&SHIFTMASK){
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_SHIFT_END),nullptr);
            }
          else{
            handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_END),nullptr);
            }
          }
        break;
      case KEY_Page_Up:
      case KEY_KP_Page_Up:
        if(event->state&SHIFTMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_SHIFT_PAGEUP),nullptr);
          }
        else{
          handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_PAGEUP),nullptr);
          }
        break;
      case KEY_Page_Down:
      case KEY_KP_Page_Down:
        if(event->state&SHIFTMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_SHIFT_PAGEDOWN),nullptr);
          }
        else{
          handle(this,FXSEL(SEL_COMMAND,ID_CURSOR_PAGEDOWN),nullptr);
          }
        break;
      case KEY_Insert:
      case KEY_KP_Insert:
        if(event->state&CONTROLMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_COPY_SEL),nullptr);
          }
        else if(event->state&SHIFTMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_PASTE_SEL),nullptr);
          }
        else{
          handle(this,FXSEL(SEL_COMMAND,ID_TOGGLE_OVERSTRIKE),nullptr);
          }
        break;
      case KEY_Delete:
      case KEY_KP_Delete:
        if(event->state&CONTROLMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_DELETE_WORD),nullptr);
          }
        else if(event->state&SHIFTMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_DELETE_EOL),nullptr);
          }
        else{
          handle(this,FXSEL(SEL_COMMAND,ID_DELETE_CHAR),nullptr);
          }
        break;
      case KEY_BackSpace:
        if(event->state&CONTROLMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_BACKSPACE_WORD),nullptr);
          }
        else if(event->state&SHIFTMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_BACKSPACE_BOL),nullptr);
          }
        else{
          handle(this,FXSEL(SEL_COMMAND,ID_BACKSPACE_CHAR),nullptr);
          }
        break;
      case KEY_Return:
      case KEY_KP_Enter:
        if(event->state&CONTROLMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_INSERT_NEWLINE_ONLY),nullptr);
          }
        else if(event->state&SHIFTMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_INSERT_NEWLINE_INDENT),nullptr);
          }
        else{
          handle(this,FXSEL(SEL_COMMAND,ID_INSERT_NEWLINE),nullptr);
          }
        break;
      case KEY_Tab:
      case KEY_KP_Tab:
        if(event->state&CONTROLMASK){
          handle(this,FXSEL(SEL_COMMAND,ID_INSERT_HARDTAB),nullptr);
          }
        else{
          handle(this,FXSEL(SEL_COMMAND,ID_INSERT_TAB),nullptr);
          }
        break;
      case KEY_a:
        if(!(event->state&CONTROLMASK)) goto ins;
        handle(this,FXSEL(SEL_COMMAND,ID_SELECT_ALL),nullptr);
        break;
      case KEY_x:
        if(!(event->state&CONTROLMASK)) goto ins;
      case KEY_F20:                               // Sun Cut key
        handle(this,FXSEL(SEL_COMMAND,ID_CUT_SEL),nullptr);
        break;
      case KEY_c:
        if(!(event->state&CONTROLMASK)) goto ins;
      case KEY_F16:                               // Sun Copy key
        handle(this,FXSEL(SEL_COMMAND,ID_COPY_SEL),nullptr);
        break;
      case KEY_v:
        if(!(event->state&CONTROLMASK)) goto ins;
      case KEY_F18:                               // Sun Paste key
        handle(this,FXSEL(SEL_COMMAND,ID_PASTE_SEL),nullptr);
        break;
      case KEY_k:
        if(!(event->state&CONTROLMASK)) goto ins;
        handle(this,FXSEL(SEL_COMMAND,ID_DELETE_LINE),nullptr);
        break;
      case KEY_j:
        if(!(event->state&CONTROLMASK)) goto ins;
        handle(this,FXSEL(SEL_COMMAND,ID_JOIN_LINES),nullptr);
        break;
      default:
ins:    if((event->state&(CONTROLMASK|ALTMASK)) || ((FXuchar)event->text[0]<32)) return 0;
        handle(this,FXSEL(SEL_COMMAND,ID_INSERT_STRING),(void*)event->text.text());
        break;
      }
    return 1;
    }
  return 0;
  }


// Keyboard release
long FXText::onKeyRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    FXEvent* event=(FXEvent*)ptr;
    FXTRACE((TOPIC_KEYBOARD,"%s::onKeyRelease keysym=0x%04x state=%04x\n",getClassName(),event->code,event->state));
    if(target && target->tryHandle(this,FXSEL(SEL_KEYRELEASE,message),ptr)) return 1;
    switch(event->code){
      case KEY_Shift_L:
      case KEY_Shift_R:
      case KEY_Control_L:
      case KEY_Control_R:
        if(mode==MOUSE_DRAG){handle(this,FXSEL(SEL_DRAGGED,0),ptr);}
        return 1;
      }
    }
  return 0;
  }

/*******************************************************************************/

// Move cursor to top of buffer
long FXText::onCmdCursorTop(FXObject*,FXSelector,void*){
  moveCursor(0,true);
  return 1;
  }


// Move cursor to bottom of buffer
long FXText::onCmdCursorBottom(FXObject*,FXSelector,void*){
  moveCursor(length,true);
  return 1;
  }


// Move cursor to begin of line
long FXText::onCmdCursorHome(FXObject*,FXSelector,void*){
  moveCursor(lineStart(cursorpos),true);
  return 1;
  }


// Move cursor to end of line
long FXText::onCmdCursorEnd(FXObject*,FXSelector,void*){
  moveCursor(lineEnd(cursorpos),true);
  return 1;
  }


// Process cursor right
long FXText::onCmdCursorRight(FXObject*,FXSelector,void*){
  moveCursor(cursorpos<length?inc(cursorpos):length,true);
  return 1;
  }


// Process cursor left
long FXText::onCmdCursorLeft(FXObject*,FXSelector,void*){
  moveCursor(0<cursorpos?dec(cursorpos):0,true);
  return 1;
  }


// Process cursor up
long FXText::onCmdCursorUp(FXObject*,FXSelector,void*){
  FXint col=(0<=prefcol) ? prefcol : cursorcol;
  moveCursor(posFromColumn(prevRow(cursorpos),col),true);
  prefcol=col;
  return 1;
  }


// Process cursor down
long FXText::onCmdCursorDown(FXObject*,FXSelector,void*){
  FXint col=(0<=prefcol) ? prefcol : cursorcol;
  moveCursor(posFromColumn(nextRow(cursorpos),col),true);
  prefcol=col;
  return 1;
  }


// Page up
long FXText::onCmdCursorPageUp(FXObject*,FXSelector,void*){
  FXint col=(0<=prefcol) ? prefcol : cursorcol;
  FXint lines=Math::imax(nvisrows-2,1);
  setTopLine(prevRow(toppos,lines));
  moveCursor(posFromColumn(prevRow(cursorpos,lines),col),true);
  prefcol=col;
  return 1;
  }


// Page down
long FXText::onCmdCursorPageDown(FXObject*,FXSelector,void*){
  FXint col=(0<=prefcol) ? prefcol : cursorcol;
  FXint lines=Math::imax(nvisrows-2,1);
  setTopLine(nextRow(toppos,lines));
  moveCursor(posFromColumn(nextRow(cursorpos,lines),col),true);
  prefcol=col;
  return 1;
  }


// Process cursor word left
long FXText::onCmdCursorWordLeft(FXObject*,FXSelector,void*){
  moveCursor(leftWord(cursorpos),true);
  return 1;
  }


// Process cursor word right
long FXText::onCmdCursorWordRight(FXObject*,FXSelector,void*){
  moveCursor(rightWord(cursorpos),true);
  return 1;
  }


// Process cursor shift+top
long FXText::onCmdCursorShiftTop(FXObject*,FXSelector,void*){
  moveCursorAndSelect(0,SelectChars,true);
  return 1;
  }


// Process cursor shift+bottom
long FXText::onCmdCursorShiftBottom(FXObject*,FXSelector,void*){
  moveCursorAndSelect(length,SelectChars,true);
  return 1;
  }


// Process cursor shift+home
long FXText::onCmdCursorShiftHome(FXObject*,FXSelector,void*){
  moveCursorAndSelect(lineStart(cursorpos),SelectChars,true);
  return 1;
  }


// Process cursor shift+end
long FXText::onCmdCursorShiftEnd(FXObject*,FXSelector,void*){
  moveCursorAndSelect(lineEnd(cursorpos),SelectChars,true);
  return 1;
  }


// Process cursor shift+right
long FXText::onCmdCursorShiftRight(FXObject*,FXSelector,void*){
  moveCursorAndSelect(cursorpos<length?inc(cursorpos):length,SelectChars,true);
  return 1;
  }


// Process cursor shift+left
long FXText::onCmdCursorShiftLeft(FXObject*,FXSelector,void*){
  moveCursorAndSelect(0<cursorpos?dec(cursorpos):0,SelectChars,true);
  return 1;
  }


// Process cursor shift+up
long FXText::onCmdCursorShiftUp(FXObject*,FXSelector,void*){
  FXint col=(0<=prefcol) ? prefcol : cursorcol;
  moveCursorAndSelect(posFromColumn(prevRow(cursorpos),col),SelectChars,true);
  prefcol=col;
  return 1;
  }


// Process cursor shift+down
long FXText::onCmdCursorShiftDown(FXObject*,FXSelector,void*){
  FXint col=(0<=prefcol) ? prefcol : cursorcol;
  moveCursorAndSelect(posFromColumn(nextRow(cursorpos),col),SelectChars,true);
  prefcol=col;
  return 1;
  }


// Process cursor shift+page up
long FXText::onCmdCursorShiftPageUp(FXObject*,FXSelector,void*){
  FXint col=(0<=prefcol) ? prefcol : cursorcol;
  FXint lines=Math::imax(nvisrows-2,1);
  setTopLine(prevRow(toppos,lines));
  moveCursorAndSelect(posFromColumn(prevRow(cursorpos,lines),col),SelectChars,true);
  prefcol=col;
  return 1;
  }


// Process cursor shift+page down
long FXText::onCmdCursorShiftPageDown(FXObject*,FXSelector,void*){
  FXint col=(0<=prefcol) ? prefcol : cursorcol;
  FXint lines=Math::imax(nvisrows-2,1);
  setTopLine(nextRow(toppos,lines));
  moveCursorAndSelect(posFromColumn(nextRow(cursorpos,lines),col),SelectChars,true);
  prefcol=col;
  return 1;
  }


// Process cursor shift+word left
long FXText::onCmdCursorShiftWordLeft(FXObject*,FXSelector,void*){
  moveCursorAndSelect(leftWord(cursorpos),SelectChars,true);
  return 1;
  }


// Process cursor shift+word right
long FXText::onCmdCursorShiftWordRight(FXObject*,FXSelector,void*){
  moveCursorAndSelect(rightWord(cursorpos),SelectChars,true);
  return 1;
  }


// Process cursor alt-up
long FXText::onCmdCursorAltUp(FXObject*,FXSelector,void*){      // FIXME
  FXint col=(0<=prefcol) ? prefcol : cursorvcol;
  moveCursorRowColumnAndSelect(cursorrow-1,col,true);
  prefcol=col;
  return 1;
  }


// Process cursor alt-down
long FXText::onCmdCursorAltDown(FXObject*,FXSelector,void*){
  FXint col=(0<=prefcol) ? prefcol : cursorvcol;
  moveCursorRowColumnAndSelect(cursorrow+1,col,true);
  prefcol=col;
  return 1;
  }


// Process cursor alt-left
long FXText::onCmdCursorAltLeft(FXObject*,FXSelector,void*){
  moveCursorRowColumnAndSelect(cursorrow,cursorvcol-1,true);
  return 1;
  }


// Process cursor alt-right
long FXText::onCmdCursorAltRight(FXObject*,FXSelector,void*){
  moveCursorRowColumnAndSelect(cursorrow,cursorvcol+1,true);
  return 1;
  }


// Scroll up one line
long FXText::onCmdScrollUp(FXObject*,FXSelector,void*){
  setTopLine(prevRow(toppos));
  return 1;
  }


// Scroll down one line
long FXText::onCmdScrollDown(FXObject*,FXSelector,void*){
  setTopLine(nextRow(toppos));
  return 1;
  }


// Scroll to move cursor to top of screen
long FXText::onCmdScrollTop(FXObject*,FXSelector,void*){
  setTopLine(cursorpos);
  return 1;
  }


// Scroll to move cursor to bottom of screen
long FXText::onCmdScrollBottom(FXObject*,FXSelector,void*){
  setBottomLine(cursorpos);
  return 1;
  }


// Scroll to move cursor to center of screen
long FXText::onCmdScrollCenter(FXObject*,FXSelector,void*){
  setCenterLine(cursorpos);
  return 1;
  }


// Insert a string as if typed
long FXText::onCmdInsertString(FXObject*,FXSelector,void* ptr){
  if(isEditable()){
    FXchar* txt=(FXchar*)ptr;
    FXint len=strlen(txt);
    FXint beg=cursorpos;
    FXint end=cursorpos;
    FXint rows,ins;
    FXString rep;

    // Position is selected
    if(isPosSelected(cursorpos,cursorvcol)){
      beg=select.startpos;
      end=select.endpos;

      // Block selection
      if(select.startcol<select.endcol){
        rows=countLines(beg,end);
        if(0<len && txt[len-1]=='\n'){ --len; }
        for(FXint i=0; i<rows; ++i){
          rep.append(txt,len);
          rep.append('\n');
          }
        rep.append(txt,len);
        ins=replaceTextBlock(select.startpos,select.endpos,select.startcol,select.endcol,rep,true);
        beg=posFromColumn(lineStart(beg+ins),select.startcol)+len;
        moveCursor(beg,true);
        return 1;
        }

      // Range selection
      if(select.startpos<select.endpos){
        ins=replaceText(beg,end-beg,txt,len,true);
        moveCursor(beg+ins,true);
        return 1;
        }
      }

    // Overstrike
    if(isOverstrike()){         // FIXME should overstrike always be block-mode?
      ins=overstruck(beg,beg,txt,len);
      ins=replaceText(beg,ins-beg,txt,len,true);
      moveCursor(beg+ins,true);
      return 1;
      }

    // Default is insert
    ins=insertText(beg,txt,len,true);
    moveCursor(beg+ins,true);
    return 1;
    }

  getApp()->beep();
  return 1;
  }


// Insert newline with optional autoindent
long FXText::onCmdInsertNewline(FXObject*,FXSelector,void*){
  if(options&TEXT_AUTOINDENT){
    return onCmdInsertNewlineIndent(this,FXSEL(SEL_COMMAND,ID_INSERT_NEWLINE_INDENT),nullptr);
    }
  return onCmdInsertNewlineOnly(this,FXSEL(SEL_COMMAND,ID_INSERT_NEWLINE_ONLY),nullptr);
  }


// Insert newline only
long FXText::onCmdInsertNewlineOnly(FXObject*,FXSelector,void*){
  return onCmdInsertString(this,FXSEL(SEL_COMMAND,ID_INSERT_STRING),(void*)"\n");
  }


// Create indent string
static FXString makeIndentString(FXint indent,FXint tabcols){
  FXString result(' ',indent+1);
  result[0]='\n';
  if(0<tabcols){
    FXint i=1;
    while(tabcols<=indent){
      result[i++]='\t';
      indent-=tabcols;
      }
    while(0<indent){
      result[i++]=' ';
      indent-=1;
      }
    result.trunc(i);
    }
  return result;
  }


// Insert newline with optional indent
// If block-selection in effect, delete the block selection; if range-selection in effect,
// replace it with indented empty line; otherwise, append indented empty line.
long FXText::onCmdInsertNewlineIndent(FXObject*,FXSelector,void*){
  FXString string("\n");
  if(select.startcol>select.endcol){
    FXint pos,start,indent;
    pos=cursorpos;
    if(isPosSelected(cursorpos)){
      pos=select.endpos;
      }
    start=lineStart(pos);
    indent=indentOfLine(start,pos);     // Indent to up to position, or first non-blank
    string=makeIndentString(indent,(options&TEXT_NO_TABS)?0:tabcolumns);
    }
  return onCmdInsertString(this,FXSEL(SEL_COMMAND,ID_INSERT_STRING),(void*)string.text());
  }


// Insert optional soft-tab
long FXText::onCmdInsertTab(FXObject*,FXSelector,void*){
  if(options&TEXT_NO_TABS){
    return onCmdInsertSoftTab(this,FXSEL(SEL_COMMAND,ID_INSERT_SOFTTAB),nullptr);
    }
  return onCmdInsertHardTab(this,FXSEL(SEL_COMMAND,ID_INSERT_HARDTAB),nullptr);
  }


// Insert hard-tab
long FXText::onCmdInsertHardTab(FXObject*,FXSelector,void*){
  return onCmdInsertString(this,FXSEL(SEL_COMMAND,ID_INSERT_STRING),(void*)"\t");
  }


// Insert soft-tab
// The number of spaces to insert depends on the indentation level.
//   No selection:      use indent at the cursor insert-position
//   Range-selection:   use indent at the start of the selection
//   Block-selection:   use the start column of the selection
long FXText::onCmdInsertSoftTab(FXObject*,FXSelector,void*){
  FXint indent;
  if(isPosSelected(cursorpos,cursorvcol)){
    if(select.startcol<select.endcol){
      indent=select.startcol;
      }
    else{
      indent=columnFromPos(lineStart(select.startpos),select.startpos);
      }
    }
  else{
    indent=columnFromPos(lineStart(cursorpos),cursorpos);
    }
  FXASSERT(0<tabcolumns && tabcolumns<MAXTABCOLUMNS);
  return onCmdInsertString(this,FXSEL(SEL_COMMAND,ID_INSERT_STRING),(void*)(spaces+MAXTABCOLUMNS+indent%tabcolumns-tabcolumns));
  }

/*******************************************************************************/

// Cut
long FXText::onCmdCutSel(FXObject*,FXSelector,void*){
  if(isEditable() && cutSelection(true)) return 1;
  getApp()->beep();
  return 1;
  }


// Copy
long FXText::onCmdCopySel(FXObject*,FXSelector,void*){
  copySelection();
  return 1;
  }


// Paste clipboard
long FXText::onCmdPasteSel(FXObject*,FXSelector,void*){
  if(isEditable() && pasteClipboard(true)) return 1;
  getApp()->beep();
  return 1;
  }


// Paste selection
long FXText::onCmdPasteMiddle(FXObject*,FXSelector,void*){
  if(isEditable() && pasteSelection(true)) return 1;
  getApp()->beep();
  return 1;
  }


// Delete selection
long FXText::onCmdDeleteSel(FXObject*,FXSelector,void*){
  if(isEditable() && deleteSelection(true)) return 1;
  getApp()->beep();
  return 1;
  }


// Select character
long FXText::onCmdSelectChar(FXObject*,FXSelector,void*){
  setAnchorPos(cursorpos);
  extendSelection(inc(cursorpos),SelectChars,true);
  return 1;
  }


// Select Word
long FXText::onCmdSelectWord(FXObject*,FXSelector,void*){
  setAnchorPos(cursorpos);
  extendSelection(cursorpos,SelectWords,true);
  return 1;
  }


// Select Line
long FXText::onCmdSelectLine(FXObject*,FXSelector,void*){
  setAnchorPos(cursorpos);
  extendSelection(cursorpos,SelectLines,true);
  return 1;
  }


// Select text till matching character
long FXText::onCmdSelectMatching(FXObject*,FXSelector,void*){
  if(0<cursorpos){
    FXchar ch=getByte(cursorpos-1);
    FXint pos=findMatching(cursorpos-1,0,length,ch,1);
    if(0<=pos){
      if(cursorpos<=pos){
        setSelection(cursorpos-1,pos-cursorpos+2,true);
        setAnchorPos(cursorpos-1);
        setCursorPos(pos+1,true);
        }
      else{
        setSelection(pos,cursorpos-pos,true);
        setAnchorPos(cursorpos);
        setCursorPos(pos+1,true);
        }
      makePositionVisible(cursorpos);
      flashMatching();
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }


// Select entire enclosing block
long FXText::onCmdSelectEnclosing(FXObject*,FXSelector sel,void*){
  FXint what=FXSELID(sel)-ID_SELECT_BRACE;
  FXint level=1;
  FXint beg,end;
  while(1){
    beg=matchBackward(cursorpos-1,0,lefthand[what],righthand[what],level);
    end=matchForward(cursorpos,length,lefthand[what],righthand[what],level);
    if(0<=beg && beg<end){
      if(isPosSelected(beg) && isPosSelected(end+1)){ level++; continue; }
      setAnchorPos(beg);
      extendSelection(end+1,SelectChars,true);
      return 1;
      }
    getApp()->beep();
    break;
    }
  return 1;
  }


// Select All
long FXText::onCmdSelectAll(FXObject*,FXSelector,void*){
  setAnchorPos(0);
  extendSelection(length,SelectChars,true);
  return 1;
  }


// Deselect All
long FXText::onCmdDeselectAll(FXObject*,FXSelector,void*){
  killSelection(true);
  return 1;
  }

/*******************************************************************************/

// Backspace character
long FXText::onCmdBackspaceChar(FXObject*,FXSelector,void*){
  if(isEditable()){
    if(deletePendingSelection(true)) return 1;
    if(0<cursorpos){
      FXint pos=dec(cursorpos);
      removeText(pos,cursorpos-pos,true);
      moveCursor(pos,true);
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }


// Backspace word
long FXText::onCmdBackspaceWord(FXObject*,FXSelector,void*){
  if(isEditable()){
    if(deletePendingSelection(true)) return 1;
    FXint pos=leftWord(cursorpos);
    if(pos<cursorpos){
      removeText(pos,cursorpos-pos,true);
      moveCursor(pos,true);
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }


// Backspace bol
long FXText::onCmdBackspaceBol(FXObject*,FXSelector,void*){
  if(isEditable()){
    if(deletePendingSelection(true)) return 1;
    FXint pos=lineStart(cursorpos);
    if(pos<cursorpos){
      removeText(pos,cursorpos-pos,true);
      moveCursor(pos,true);
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }


// Delete character
long FXText::onCmdDeleteChar(FXObject*,FXSelector,void*){
  if(isEditable()){
    if(deletePendingSelection(true)) return 1;
    if(cursorpos<length){
      FXint pos=inc(cursorpos);
      removeText(cursorpos,pos-cursorpos,true);
      moveCursor(cursorpos,true);
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }


// Delete word
long FXText::onCmdDeleteWord(FXObject*,FXSelector,void*){
  if(isEditable()){
    if(deletePendingSelection(true)) return 1;
    FXint pos=rightWord(cursorpos);
    if(pos<length){
      removeText(cursorpos,pos-cursorpos,true);
      moveCursor(cursorpos,true);
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }


// Delete to end of line
long FXText::onCmdDeleteEol(FXObject*,FXSelector,void*){
  if(isEditable()){
    if(deletePendingSelection(true)) return 1;
    FXint pos=lineEnd(cursorpos);
    if(pos<length){
      removeText(cursorpos,pos-cursorpos,true);
      moveCursor(cursorpos,true);
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }


// Delete line
long FXText::onCmdDeleteLine(FXObject*,FXSelector,void*){
  if(isEditable()){
    if(deletePendingSelection(true)) return 1;
    FXint beg=lineStart(cursorpos);
    FXint end=nextLine(cursorpos);
    if(beg<end){
      removeText(beg,end-beg,true);
      moveCursor(beg,true);
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }


// Delete all text
long FXText::onCmdDeleteAll(FXObject*,FXSelector,void*){
  if(isEditable()){
    if(0<length){
      removeText(0,length,true);
      moveCursor(0,true);
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }

/*******************************************************************************/

// Shift block of lines from position start up to end by given indent
FXint FXText::shiftText(FXint startpos,FXint endpos,FXint shift,FXbool notify){
  if(0<=startpos && startpos<endpos && endpos<=length){
    FXString org=extractText(startpos,endpos-startpos);
    FXString rep=FXString::tabbify(org,tabcolumns,0,0,shift,!(options&TEXT_NO_TABS));
    return replaceStyledText(startpos,endpos-startpos,rep,0,notify);
    }
  return 0;
  }


// Shift selected lines left or right, or clean indent
// Try keep the cursor on same row and (adjusted) column as before
long FXText::onCmdShiftText(FXObject*,FXSelector sel,void*){
  if(isEditable()){
    FXint startpos,endpos,len;
    FXint currow=getCursorRow();
    FXint cursta=rowStart(getCursorPos());
    FXint curcol=columnFromPos(cursta,getCursorPos());
    FXint curind=indentOfLine(cursta,getCursorPos());
    FXint indent=0;
    FXint newcol;
    switch(FXSELID(sel)){
      case ID_SHIFT_LEFT: indent=-1; break;
      case ID_SHIFT_RIGHT: indent=1; break;
      case ID_SHIFT_TABLEFT: indent=-tabcolumns; break;
      case ID_SHIFT_TABRIGHT: indent=tabcolumns; break;
      }
    newcol=curcol-curind+Math::imax(curind+indent,0);
    if(select.startpos<=select.endpos){
      startpos=lineStart(select.startpos);
      endpos=nextLine(select.endpos-1);
      }
    else{
      startpos=lineStart(cursorpos);
      endpos=lineEnd(cursorpos);
      if(endpos<length) endpos++;
      }
    len=shiftText(startpos,endpos,indent,true);
    setSelection(startpos,len,true);
    setAnchorRowColumn(currow,newcol);
    setCursorRowColumn(currow,newcol,true);
    }
  else{
    getApp()->beep();
    }
  return 1;
  }

/*******************************************************************************/

// Make selected text upper case
long FXText::onCmdChangeCase(FXObject*,FXSelector sel,void*){
  if(isEditable()){
    FXint curc=getCursorColumn();
    FXint curr=getCursorRow();
    if((select.startcol<select.endcol) && (select.startpos<=select.endpos)){
      FXString string=extractTextBlock(select.startpos,select.endpos,select.startcol,select.endcol);
      if(FXSELID(sel)==ID_UPPER_CASE){string.upper();} else{string.lower();}
      replaceTextBlock(select.startpos,select.endpos,select.startcol,select.endcol,string,true);
      }
    if((select.startcol>select.endcol) && (select.startpos<select.endpos)){
      FXString string=extractText(select.startpos,select.endpos-select.startpos);
      if(FXSELID(sel)==ID_UPPER_CASE){string.upper();} else{string.lower();}
      replaceText(select.startpos,select.endpos-select.startpos,string,true);
      }
    setAnchorRowColumn(curr,curc);
    setCursorRowColumn(curr,curc,true);
    return 1;
    }
  getApp()->beep();
  return 1;
  }

/*******************************************************************************/

// Copy current line to the line below; leave it selected with cursor at the end
long FXText::onCmdCopyLine(FXObject*,FXSelector,void*){
  if(isEditable()){
    FXString text;
    FXint start,end;
    if(select.startpos<=select.endpos){
      start=lineStart(select.startpos);
      end=lineEnd(select.endpos-1);
      }
    else{
      start=lineStart(cursorpos);
      end=lineEnd(cursorpos);
      }
    text=extractText(start,end-start);
    text+='\n';
    insertText(start,text,true);
    setSelection(start+text.length(),text.length(),true);
    setAnchorPos(cursorpos);
    makePositionVisible(cursorpos);
    return 1;
    }
  getApp()->beep();
  return 1;
  }

/*******************************************************************************/

// Move the current line up, if there is a line above it.
// More tricky than it looks; current line may be non-terminated by a newline.
// However, previous line *is* newline terminated by definition.
// Solution is to snip the lines without the newline, and then place the
// newline at the appropriate spot.
long FXText::onCmdMoveLineUp(FXObject*,FXSelector,void*){
  if(isEditable()){
    FXint curbeg,curend,prvbeg,pos;
    if(select.startpos<=select.endpos){
      curbeg=lineStart(select.startpos);
      curend=lineEnd(select.endpos-1);
      }
    else{
      curbeg=lineStart(cursorpos);
      curend=lineEnd(cursorpos);
      }
    FXASSERT(curbeg<=curend);
    prvbeg=prevLine(curbeg);
    if(0<curbeg){
      FXString text('\0',curend-prvbeg);
      pos=prvbeg+cursorpos-curbeg;
      extractText(&text[0],curbeg,curend-curbeg);
      text[curend-curbeg]='\n';
      extractText(&text[curend-curbeg+1],prvbeg,curbeg-prvbeg-1);
      replaceText(prvbeg,curend-prvbeg,text,true);
      setSelection(prvbeg,curend-curbeg+1,true);
      setAnchorPos(prvbeg);
      setCursorPos(pos,true);
      makePositionVisible(cursorpos);
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }


// Move current line down, if there is a line below it.
// Similar logic as above; the line to be moved up may be non-terminated by a newline.
// The current line *is* newline terminated, by definition.
// Thus we snip the lines w/o including the newline, and place the missing
// newline at the proper place in the middle.
long FXText::onCmdMoveLineDown(FXObject*,FXSelector,void*){
  if(isEditable()){
    FXint curbeg,curend,nxtend,pos;
    if(select.startpos<=select.endpos){
      curbeg=lineStart(select.startpos);
      curend=nextLine(select.endpos-1);
      }
    else{
      curbeg=lineStart(cursorpos);
      curend=nextLine(cursorpos);
      }
    nxtend=lineEnd(curend);
    if(curend<length){
      FXString text('\0',nxtend-curbeg);
      pos=nxtend-curend+cursorpos;
      extractText(&text[0],curend,nxtend-curend);
      text[nxtend-curend]='\n';
      extractText(&text[nxtend-curend+1],curbeg,curend-curbeg-1);
      replaceText(curbeg,nxtend-curbeg,text,true);
      setSelection(curbeg+nxtend-curend+1,curend-curbeg,true);
      setAnchorPos(curbeg+nxtend-curend+1);
      setCursorPos(pos,true);
      makePositionVisible(cursorpos);
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }

/*******************************************************************************/

// Join lines
long FXText::onCmdJoinLines(FXObject*,FXSelector,void*){
  if(isEditable()){
    FXint pos=lineEnd(cursorpos);
    if(pos<length){
      removeText(pos,1,true);
      return 1;
      }
    }
  else{
    getApp()->beep();
    }
  return 1;
  }


// Goto start of enclosing block
long FXText::onCmdBlockBeg(FXObject*,FXSelector sel,void*){
  FXint what=FXSELID(sel)-ID_LEFT_BRACE;
  FXint beg=cursorpos-1;
  if(0<beg){
    if(getByte(beg)==lefthand[what]) beg--;
    FXint pos=matchBackward(beg,0,lefthand[what],righthand[what],1);
    if(0<=pos){
      moveCursor(pos+1,true);
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }


// Goto end of enclosing block
long FXText::onCmdBlockEnd(FXObject*,FXSelector sel,void*){
  FXint what=FXSELID(sel)-ID_RIGHT_BRACE;
  FXint start=cursorpos;
  if(start<length){
    if(getByte(start)==righthand[what]) start++;
    FXint pos=matchForward(start,length,lefthand[what],righthand[what],1);
    if(0<=pos){
      moveCursor(pos,true);
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }


// Goto matching character
long FXText::onCmdGotoMatching(FXObject*,FXSelector,void*){
  if(0<cursorpos){
    FXchar ch=getByte(cursorpos-1);
    FXint pos=findMatching(cursorpos-1,0,length,ch,1);
    if(0<=pos){
      moveCursor(pos+1,true);
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }


// Move cursor to indicated row
long FXText::onCmdCursorRow(FXObject* sender,FXSelector,void*){
  FXint row=cursorrow+1;
  sender->handle(this,FXSEL(SEL_COMMAND,ID_GETINTVALUE),(void*)&row);
  setCursorRow(row-1,true);
  return 1;
  }


// Being asked about current row number
long FXText::onUpdCursorRow(FXObject* sender,FXSelector,void*){
  FXint row=cursorrow+1;
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SETINTVALUE),(void*)&row);
  return 1;
  }


// Move cursor to indicated column
long FXText::onCmdCursorColumn(FXObject* sender,FXSelector,void*){
  FXint col=cursorcol;
  sender->handle(this,FXSEL(SEL_COMMAND,ID_GETINTVALUE),(void*)&col);
  setCursorColumn(col,true);
  return 1;
  }


// Being asked about current column
long FXText::onUpdCursorColumn(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETINTVALUE),(void*)&cursorcol);
  return 1;
  }


// Editable toggle
long FXText::onCmdToggleEditable(FXObject*,FXSelector,void*){
  setEditable(!isEditable());
  return 1;
  }


// Update editable toggle
long FXText::onUpdToggleEditable(FXObject* sender,FXSelector,void*){
  sender->handle(this,isEditable()?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SHOW),nullptr);
  sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  return 1;
  }


// Overstrike toggle
long FXText::onCmdToggleOverstrike(FXObject*,FXSelector,void*){
  setOverstrike(!isOverstrike());
  return 1;
  }


// Update overstrike toggle
long FXText::onUpdToggleOverstrike(FXObject* sender,FXSelector,void*){
  sender->handle(this,isOverstrike()?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SHOW),nullptr);
  sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  return 1;
  }

/*******************************************************************************/

// Change text style
void FXText::setTextStyle(FXuint style){
  FXuint opts=((style^options)&TEXT_MASK)^options;
  if(options!=opts){
    options=opts;
    recalc();
    update();
    }
  }


// Get text style
FXuint FXText::getTextStyle() const {
  return (options&TEXT_MASK);
  }


// Change the font
void FXText::setFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setFont: NULL font specified.\n",getClassName()); }
  if(font!=fnt){
    font=fnt;
    tabwidth=tabcolumns*font->getTextWidth(" ",1);
    barwidth=barcolumns*font->getTextWidth("8",1);
    if(getComposeContext()) getComposeContext()->setFont(font);
    recalc();
    update();
    }
  }


// Change number of visible rows
void FXText::setVisibleRows(FXint rows){
  if(rows<0) rows=0;
  if(vrows!=rows){
    vrows=rows;
    recalc();
    }
  }


// Change number of visible columns
void FXText::setVisibleColumns(FXint cols){
  if(cols<0) cols=0;
  if(vcols!=cols){
    vcols=cols;
    recalc();
    }
  }


// Change top margin
void FXText::setMarginTop(FXint mt){
  if(margintop!=mt){
    margintop=mt;
    recalc();
    update();
    }
  }


// Change bottom margin
void FXText::setMarginBottom(FXint mb){
  if(marginbottom!=mb){
    marginbottom=mb;
    recalc();
    update();
    }
  }


// Change left margin
void FXText::setMarginLeft(FXint ml){
  if(marginleft!=ml){
    marginleft=ml;
    recalc();
    update();
    }
  }


// Change right margin
void FXText::setMarginRight(FXint mr){
  if(marginright!=mr){
    marginright=mr;
    recalc();
    update();
    }
  }


// Change number of columns used for line numbers
void FXText::setBarColumns(FXint cols){
  if(cols<=0) cols=0;
  if(cols!=barcolumns){
    barcolumns=cols;
    barwidth=barcolumns*font->getTextWidth("8",1);
    recalc();
    update();
    }
  }


// Set wrap columns
void FXText::setWrapColumns(FXint cols){
  if(cols<=0) cols=1;
  if(cols!=wrapcolumns){
    wrapcolumns=cols;
    recalc();
    update();
    }
  }


// Set tab columns
void FXText::setTabColumns(FXint cols){
  cols=Math::iclamp(1,cols,MAXTABCOLUMNS);
  if(cols!=tabcolumns){
    tabcolumns=cols;
    tabwidth=tabcolumns*font->getTextWidth(" ",1);
    recalc();
    update();
    }
  }


// Set text color
void FXText::setTextColor(FXColor clr){
  if(clr!=textColor){
    textColor=clr;
    update();
    }
  }


// Set select background color
void FXText::setSelBackColor(FXColor clr){
  if(clr!=selbackColor){
    selbackColor=clr;
    update();
    }
  }


// Set selected text color
void FXText::setSelTextColor(FXColor clr){
  if(clr!=seltextColor){
    seltextColor=clr;
    update();
    }
  }


// Change highlighted text color
void FXText::setHiliteTextColor(FXColor clr){
  if(clr!=hilitetextColor){
    hilitetextColor=clr;
    update();
    }
  }


// Change highlighted background color
void FXText::setHiliteBackColor(FXColor clr){
  if(clr!=hilitebackColor){
    hilitebackColor=clr;
    update();
    }
  }


// Change active background color
void FXText::setActiveBackColor(FXColor clr){
  if(clr!=activebackColor){
    activebackColor=clr;
    update();
    }
  }

// Set cursor color
void FXText::setCursorColor(FXColor clr){
  if(clr!=cursorColor){
    cursorColor=clr;
    update();
    }
  }


// Change line number color
void FXText::setNumberColor(FXColor clr){
  if(clr!=numberColor){
    numberColor=clr;
    update();
    }
  }

// Change bar color
void FXText::setBarColor(FXColor clr){
  if(clr!=barColor){
    barColor=clr;
    update();
    }
  }


// Set styled text mode
FXbool FXText::setStyled(FXbool styled){
  if(styled && !sbuffer){
    if(!callocElms(sbuffer,length+gapend-gapstart)) return false;
    update();
    }
  if(!styled && sbuffer){
    freeElms(sbuffer);
    update();
    }
  return true;
  }


// Set highlight styles
void FXText::setHiliteStyles(FXHiliteStyle* styles){
  hilitestyles=styles;
  update();
  }


// Save object to stream
void FXText::save(FXStream& store) const {
  FXScrollArea::save(store);
  store << length;
  store.save(buffer,gapstart);
  store.save(buffer+gapend,length-gapstart);
  store << nvisrows;
  store.save(visrows,nvisrows+1);
  store << margintop;
  store << marginbottom;
  store << marginleft;
  store << marginright;
  store << wrapcolumns;
  store << tabcolumns;
  store << barcolumns;
  store << font;
  store << textColor;
  store << selbackColor;
  store << seltextColor;
  store << hilitebackColor;
  store << hilitetextColor;
  store << activebackColor;
  store << numberColor;
  store << cursorColor;
  store << barColor;
  store << vrows;
  store << vcols;
  store << help;
  store << tip;
  store << matchtime;
  }


// Load object from stream
void FXText::load(FXStream& store){
  FXScrollArea::load(store);
  store >> length;
  allocElms(buffer,length+MINSIZE);
  store.load(buffer,length);
  gapstart=length;
  gapend=length+MINSIZE;
  store >> nvisrows;
  allocElms(visrows,nvisrows+1);
  store.load(visrows,nvisrows+1);
  store >> margintop;
  store >> marginbottom;
  store >> marginleft;
  store >> marginright;
  store >> wrapcolumns;
  store >> tabcolumns;
  store >> barcolumns;
  store >> font;
  store >> textColor;
  store >> selbackColor;
  store >> seltextColor;
  store >> hilitebackColor;
  store >> hilitetextColor;
  store >> activebackColor;
  store >> numberColor;
  store >> cursorColor;
  store >> barColor;
  store >> vrows;
  store >> vcols;
  store >> help;
  store >> tip;
  store >> matchtime;
  }


// Clean up
FXText::~FXText(){
  getApp()->removeTimeout(this,ID_BLINK);
  getApp()->removeTimeout(this,ID_FLASH);
  getApp()->removeTimeout(this,ID_TIPTIMER);
  freeElms(buffer);
  freeElms(sbuffer);
  freeElms(visrows);
  buffer=(FXchar*)-1L;
  sbuffer=(FXchar*)-1L;
  visrows=(FXint*)-1L;
  font=(FXFont*)-1L;
  delimiters=(const FXchar*)-1L;
  hilitestyles=(FXHiliteStyle*)-1L;
  }

}
