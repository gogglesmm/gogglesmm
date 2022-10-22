/********************************************************************************
*                                                                               *
*                         F O X   E v e n t   S t u f f                         *
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
#ifndef FXEVENT_H
#define FXEVENT_H

namespace FX {


/// FOX Keyboard and Button states
enum {
  SHIFTMASK        = 0x001,           /// Shift key is down
  CAPSLOCKMASK     = 0x002,           /// Caps Lock key is down
  CONTROLMASK      = 0x004,           /// Ctrl key is down
#ifdef __APPLE__
  ALTMASK          = 0x2000,          /// Alt key is down
  METAMASK         = 0x10,            /// Meta key is down
#else
  ALTMASK          = 0x008,           /// Alt key is down
  METAMASK         = 0x040,           /// Meta key is down
#endif
  NUMLOCKMASK      = 0x010,           /// Num Lock key is down
  SCROLLLOCKMASK   = 0x0E0,           /// Scroll Lock key is down (seems to vary)
  LEFTBUTTONMASK   = 0x100,           /// Left mouse button is down
  MIDDLEBUTTONMASK = 0x200,           /// Middle mouse button is down
  RIGHTBUTTONMASK  = 0x400            /// Right mouse button is down
  };


/// FOX Mouse buttons
enum {
  LEFTBUTTON       = 1,
  MIDDLEBUTTON     = 2,
  RIGHTBUTTON      = 3
  };


/// FOX window crossing modes
enum {
  CROSSINGNORMAL,		     /// Normal crossing event
  CROSSINGGRAB,			     /// Crossing due to mouse grab
  CROSSINGUNGRAB		     /// Crossing due to mouse ungrab
  };


/// FOX window visibility modes
enum {
  VISIBILITYTOTAL,
  VISIBILITYPARTIAL,
  VISIBILITYNONE
  };


/// FOX Event Types
enum {
  SEL_NONE,
  SEL_KEYPRESS,                 /// Key pressed
  SEL_KEYRELEASE,               /// Key released
  SEL_LEFTBUTTONPRESS,          /// Left mouse button pressed
  SEL_LEFTBUTTONRELEASE,        /// Left mouse button released
  SEL_MIDDLEBUTTONPRESS,        /// Middle mouse button pressed
  SEL_MIDDLEBUTTONRELEASE,      /// Middle mouse button released
  SEL_RIGHTBUTTONPRESS,         /// Right mouse button pressed
  SEL_RIGHTBUTTONRELEASE,       /// Right mouse button released
  SEL_MOTION,                   /// Mouse motion
  SEL_ENTER,                    /// Mouse entered window
  SEL_LEAVE,                    /// Mouse left window
  SEL_FOCUSIN,                  /// Focus into window
  SEL_FOCUSOUT,                 /// Focus out of window
  SEL_KEYMAP,
  SEL_UNGRABBED,                /// Lost the grab (Windows)
  SEL_PAINT,                    /// Must repaint window
  SEL_CREATE,
  SEL_DESTROY,
  SEL_UNMAP,                    /// Window was hidden
  SEL_MAP,                      /// Window was shown
  SEL_CONFIGURE,                /// Resize
  SEL_SELECTION_LOST,           /// Widget lost selection
  SEL_SELECTION_GAINED,         /// Widget gained selection
  SEL_SELECTION_REQUEST,        /// Inquire selection data
  SEL_RAISED,                   /// Window to top of stack
  SEL_LOWERED,                  /// Window to bottom of stack
  SEL_CLOSE,                    /// Close window
  SEL_DELETE,                   /// Delete window
  SEL_MINIMIZE,                 /// Iconified
  SEL_RESTORE,                  /// No longer iconified or maximized
  SEL_MAXIMIZE,                 /// Maximized
  SEL_UPDATE,                   /// GUI update
  SEL_COMMAND,                  /// GUI command
  SEL_CLICKED,                  /// Clicked
  SEL_DOUBLECLICKED,            /// Double-clicked
  SEL_TRIPLECLICKED,            /// Triple-clicked
  SEL_MOUSEWHEEL,               /// Mouse wheel
  SEL_CHANGED,                  /// GUI has changed
  SEL_VERIFY,                   /// Verify change
  SEL_DESELECTED,               /// Deselected
  SEL_SELECTED,                 /// Selected
  SEL_INSERTED,                 /// Inserted
  SEL_REPLACED,                 /// Replaced
  SEL_DELETED,                  /// Deleted
  SEL_OPENED,                   /// Opened
  SEL_CLOSED,                   /// Closed
  SEL_EXPANDED,                 /// Expanded
  SEL_COLLAPSED,                /// Collapsed
  SEL_BEGINDRAG,                /// Start a drag
  SEL_ENDDRAG,                  /// End a drag
  SEL_DRAGGED,                  /// Dragged
  SEL_LASSOED,                  /// Lassoed
  SEL_TIMEOUT,                  /// Timeout occurred
  SEL_SIGNAL,                   /// Signal received
  SEL_CLIPBOARD_LOST,           /// Widget lost clipboard
  SEL_CLIPBOARD_GAINED,         /// Widget gained clipboard
  SEL_CLIPBOARD_REQUEST,        /// Inquire clipboard data
  SEL_CHORE,                    /// Background chore
  SEL_FOCUS_SELF,               /// Focus on widget itself
  SEL_FOCUS_RIGHT,              /// Focus moved right
  SEL_FOCUS_LEFT,               /// Focus moved left
  SEL_FOCUS_DOWN,               /// Focus moved down
  SEL_FOCUS_UP,                 /// Focus moved up
  SEL_FOCUS_NEXT,               /// Focus moved to next widget
  SEL_FOCUS_PREV,               /// Focus moved to previous widget
  SEL_DND_ENTER,                /// Drag action entering potential drop target
  SEL_DND_LEAVE,                /// Drag action leaving potential drop target
  SEL_DND_DROP,                 /// Drop on drop target
  SEL_DND_MOTION,               /// Drag position changed over potential drop target
  SEL_DND_REQUEST,              /// Inquire drag and drop data
  SEL_IO_READ,                  /// Read activity on a pipe
  SEL_IO_WRITE,                 /// Write activity on a pipe
  SEL_IO_EXCEPT,                /// Except activity on a pipe
  SEL_PICKED,                   /// Picked some location
  SEL_QUERY_TIP,                /// Message inquiring about tooltip
  SEL_QUERY_HELP,               /// Message inquiring about statusline help
  SEL_DOCKED,                   /// Toolbar docked
  SEL_FLOATED,                  /// Toolbar floated
  SEL_SPACEBALLMOTION,          /// Moved space ball puck
  SEL_SPACEBALLBUTTONPRESS,     /// Pressed space ball button
  SEL_SPACEBALLBUTTONRELEASE,   /// Released space ball button
  SEL_SESSION_NOTIFY,           /// Session is about to close
  SEL_SESSION_CLOSED,           /// Session is closed
  SEL_COPYDATA,                 /// Copy data message
  SEL_IME_START,                /// IME mode
  SEL_IME_END,                  /// IME mode
  SEL_LAST
  };


/// FOX Event
struct FXAPI FXEvent {
  FXuint      type;           /// Event type
  FXuint      time;           /// Time of last event
  FXint       win_x;          /// Window-relative x-coord
  FXint       win_y;          /// Window-relative y-coord
  FXint       root_x;         /// Root x-coord
  FXint       root_y;         /// Root y-coord
  FXint       state;          /// Mouse button and modifier key state
  FXint       code;           /// Button, Keysym, or mode; DDE Source
  FXString    text;           /// Text of keyboard event
  FXint       last_x;         /// Window-relative x-coord of previous mouse location
  FXint       last_y;         /// Window-relative y-coord of previous mouse location
  FXint       click_x;        /// Window-relative x-coord of mouse press
  FXint       click_y;        /// Window-relative y-coord of mouse press
  FXint       rootclick_x;    /// Root-relative x-coord of mouse press
  FXint       rootclick_y;    /// Root-relative y-coord of mouse press
  FXuint      click_time;     /// Time of mouse button press
  FXint       click_button;   /// Mouse button pressed
  FXint       click_count;    /// Click-count
  FXint       values[6];      /// Valuators from space ball
  FXbool      moved;          /// Moved cursor since press
  FXRectangle rect;           /// Rectangle
  FXbool      synthetic;      /// True if synthetic expose event
  FXDragType  target;         /// Target drag type being requested

  /// Initialize empty event
  FXEvent(FXuint t=SEL_NONE){
    type=t;
    time=0;
    win_x=0;
    win_y=0;
    root_x=0;
    root_y=0;
    state=0;
    code=0;
    last_x=0;
    last_y=0;
    click_x=0;
    click_y=0;
    rootclick_x=0;
    rootclick_y=0;
    click_time=0;
    click_button=0;
    click_count=0;
    values[0]=0;
    values[1]=0;
    values[2]=0;
    values[3]=0;
    values[4]=0;
    values[5]=0;
    moved=false;
    rect.x=0;
    rect.y=0;
    rect.w=0;
    rect.h=0;
    synthetic=false;
    target=0;
    }
  };


}

#endif
