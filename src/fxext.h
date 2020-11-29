/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2021 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#ifndef FXEXT_H
#define FXEXT_H

enum {
  WINDOWTYPE_NORMAL,
  WINDOWTYPE_DIALOG,
  WINDOWTYPE_COMBO,
  WINDOWTYPE_POPUP_MENU,
  WINDOWTYPE_DROPDOWN_MENU,
  WINDOWTYPE_TOOLTIP,
  };

extern void ewmh_change_window_type(const FXWindow *,FXuint);
extern void ewmh_set_window_icon(const FXWindow *,FXImage *);
extern void ewmh_activate_window(const FXWindow*);
extern void fix_wm_properties(const FXWindow*);


class GMThreadDialog : public FXTopWindow {
  FXDECLARE(GMThreadDialog)
protected:
  FXMutex     mutex;
  FXCondition condition;
  FXuint      code = 0;
protected:
  GMThreadDialog(){}
private:
  GMThreadDialog(const GMThreadDialog&);
  GMThreadDialog &operator=(const GMThreadDialog&);
public:
  long onCmdAccept(FXObject*,FXSelector,void*);
  long onCmdCancel(FXObject*,FXSelector,void*);
  long onThreadExec(FXObject*,FXSelector,void*);
  long onKeyPress(FXObject*,FXSelector,void*);
  long onKeyRelease(FXObject*,FXSelector,void*);
public:
  enum {
    ID_CANCEL=FXTopWindow::ID_LAST,     /// Closes the dialog, cancel the entry
    ID_ACCEPT,                          /// Closes the dialog, accept the entry
    ID_THREAD_EXEC,
    ID_LAST
    };
public:

  /// Construct free-floating dialog
  GMThreadDialog(FXApp* a,const FXString& name,FXuint opts=DECOR_TITLE|DECOR_BORDER,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=10,FXint pr=10,FXint pt=10,FXint pb=10,FXint hs=4,FXint vs=4);

  /// Construct dialog which will always float over the owner window
  GMThreadDialog(FXWindow* owner,const FXString& name,FXuint opts=DECOR_TITLE|DECOR_BORDER,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=10,FXint pr=10,FXint pt=10,FXint pb=10,FXint hs=4,FXint vs=4);

  /// Execute from thread using message channel
  FXuint execute(FXMessageChannel*);
  };





class GMListBox : public FXListBox {
  FXDECLARE(GMListBox)
protected:
  GMListBox();
private:
  GMListBox(const GMListBox&);
  GMListBox& operator=(const GMListBox&);
public:
  GMListBox(FXComposite*,FXObject*tgt=nullptr,FXSelector sel=0,FXuint opts=FRAME_SUNKEN|FRAME_THICK|LISTBOX_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);
  virtual void create();
  };

class GMComboBox : public FXComboBox {
  FXDECLARE(GMComboBox)
protected:
  GMComboBox();
private:
  GMComboBox(const GMComboBox&);
  GMComboBox& operator=(const GMComboBox&);
public:
  GMComboBox(FXComposite *p,FXint cols,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=COMBOBOX_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);
  virtual void create();
  };

class GMScrollArea : public FXScrollArea {
  FXDECLARE(GMScrollArea)
protected:
  GMScrollArea(){}
private:
  GMScrollArea(const GMScrollArea&);
  GMScrollArea& operator=(const GMScrollArea&);
public:
  static void replaceScrollbars(FXScrollArea*);
  };



class GMTreeListBox : public FXTreeListBox {
  FXDECLARE(GMTreeListBox)
protected:
  GMTreeListBox(){}
private:
  GMTreeListBox(const GMTreeListBox&);
  GMTreeListBox& operator=(const GMTreeListBox&);
public:
  static void replace(FXTreeListBox*);
  };


class GMTabBook : public FXTabBook {
  FXDECLARE(GMTabBook)
protected:
  GMTabBook(){}
private:
  GMTabBook(const GMTabBook&);
  GMTabBook& operator=(const GMTabBook&);
public:
  GMTabBook(FXComposite* p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=TABBOOK_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_SPACING,FXint pr=DEFAULT_SPACING,FXint pt=DEFAULT_SPACING,FXint pb=DEFAULT_SPACING);
	virtual void setCurrent(FXint panel,FXbool notify=false);
	};


class GMTabItem : public FXTabItem {
  FXDECLARE(GMTabItem)
protected:
  GMTabItem(){}
private:
  GMTabItem(const GMTabItem&);
  GMTabItem& operator=(const GMTabItem&);
public:
  long onPaint(FXObject*,FXSelector,void*);
public:
  /// Construct a tab item
  GMTabItem(FXTabBar* p,const FXString& text,FXIcon* ic=0,FXuint opts=TAB_TOP_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);

//  virtual void raise();
  };



class GMHeaderItem : public FXHeaderItem {
  FXDECLARE(GMHeaderItem)
  friend class GMHeader;
protected:
  GMHeaderItem(){}
public:
  /// Construct new item with given text, icon, size, and user-data
  GMHeaderItem(const FXString& text,FXIcon* ic=nullptr,FXint s=0,void* ptr=nullptr): FXHeaderItem(text,ic,s,ptr) {}
  };

class GMHeader : public FXHeader {
  FXDECLARE(GMHeader)
protected:
  GMHeader();
private:
  GMHeader(const GMHeader&);
  GMHeader &operator=(const GMHeader&);
public:
  long onPaint(FXObject*,FXSelector,void*);
public:
  GMHeader(FXComposite* p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=HEADER_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);
  };


class GMHeaderButton : public FXButton {
FXDECLARE(GMHeaderButton)
protected:
	FXuint arrowstate;
protected:
  GMHeaderButton();
private:
  GMHeaderButton(const GMHeaderButton&);
  GMHeaderButton& operator=(const GMHeaderButton&);
public:
  long onPaint(FXObject*,FXSelector,void*);
public:
  /// Construct button with text and icon
  GMHeaderButton(FXComposite* p,const FXString& text,FXIcon* ic=nullptr,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=BUTTON_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);

	void setArrowState(FXuint s);

	FXuint getArrowState() const;

  };



class GMMenuCommand : public FXMenuCommand {
FXDECLARE(GMMenuCommand)
protected:
  GMMenuCommand(){}
private:
  GMMenuCommand(const GMMenuCommand&);
  GMMenuCommand& operator=(const GMMenuCommand&);
public:
  GMMenuCommand(FXComposite* p,const FXString& text,FXIcon* ic=nullptr,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=0);
  };


class GMMenuCheck : public FXMenuCheck {
FXDECLARE(GMMenuCheck)
protected:
  GMMenuCheck(){}
private:
  GMMenuCheck(const GMMenuCheck&);
  GMMenuCheck& operator=(const GMMenuCheck&);
public:
  GMMenuCheck(FXComposite* p,const FXString& text,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=0);
  };


class GMMenuRadio : public FXMenuRadio {
FXDECLARE(GMMenuRadio)
protected:
  GMMenuRadio(){}
private:
  GMMenuRadio(const GMMenuRadio&);
  GMMenuRadio& operator=(const GMMenuRadio&);
public:
  GMMenuRadio(FXComposite* p,const FXString& text,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=0);
  };


class GMMenuCascade : public FXMenuCascade {
FXDECLARE(GMMenuCascade)
protected:
  GMMenuCascade(){}
private:
  GMMenuCascade(const GMMenuCascade&);
  GMMenuCascade& operator=(const GMMenuCascade&);
public:
  GMMenuCascade(FXComposite* p,const FXString& text,FXIcon* ic=nullptr,FXPopup* pup=nullptr,FXuint opts=0);
  };



class GMMenuTitle : public FXMenuTitle {
FXDECLARE(GMMenuTitle)
protected:
  GMMenuTitle(){}
private:
  GMMenuTitle(const GMMenuTitle&);
  GMMenuTitle& operator=(const GMMenuTitle&);
public:
  long onPaint(FXObject*,FXSelector,void*);
  long onCmdPost(FXObject*,FXSelector,void*);
public:
  GMMenuTitle(FXComposite* p,const FXString& text,FXIcon* ic=nullptr,FXPopup* pup=nullptr,FXuint opts=0);
  };


/// Popup menu pane
class GMMenuPane : public FXMenuPane {
  FXDECLARE(GMMenuPane)
protected:
  GMMenuPane(){}
private:
  GMMenuPane(const GMMenuPane&);
  GMMenuPane &operator=(const GMMenuPane&);
public:
  /// Construct menu pane
  GMMenuPane(FXWindow* owner,FXuint opts=0);
  };


class GMButton : public FXButton {
FXDECLARE(GMButton)
protected:
  GMButton();
private:
  GMButton(const GMButton&);
  GMButton& operator=(const GMButton&);
public:
  long onPaint(FXObject*,FXSelector,void*);
public:
  GMButton(FXComposite* p,const FXString& text,FXIcon* ic=nullptr,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=BUTTON_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);
  };


class GMToggleButton : public FXToggleButton {
FXDECLARE(GMToggleButton)
protected:
  GMToggleButton();
private:
  GMToggleButton(const GMToggleButton&);
  GMToggleButton& operator=(const GMToggleButton&);
public:
  long onPaint(FXObject*,FXSelector,void*);
public:
  GMToggleButton(FXComposite* p,const FXString& text1,const FXString& text2,FXIcon* icon1=nullptr,FXIcon* icon2=nullptr,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=TOGGLEBUTTON_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);
  };

class GMRadioButton : public FXRadioButton {
FXDECLARE(GMRadioButton)
protected:
  GMRadioButton();
private:
  GMRadioButton(const GMRadioButton&);
  GMRadioButton& operator=(const GMRadioButton&);
public:
  GMRadioButton(FXComposite* p,const FXString& text,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=RADIOBUTTON_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);
  };

class GMCheckButton : public FXCheckButton {
FXDECLARE(GMCheckButton)
protected:
  GMCheckButton();
private:
  GMCheckButton(const GMCheckButton&);
  GMCheckButton& operator=(const GMCheckButton&);
public:
  long onPaint(FXObject*,FXSelector,void*);
public:
  GMCheckButton(FXComposite* p,const FXString& text,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=CHECKBUTTON_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);
  };

class GMMenuButton : public FXMenuButton {
FXDECLARE(GMMenuButton)
protected:
  GMMenuButton();
private:
  GMMenuButton(const GMMenuButton&);
  GMMenuButton& operator=(const GMMenuButton&);
public:
  long onPaint(FXObject*,FXSelector,void*);
public:
  GMMenuButton(FXComposite* p,const FXString& text,FXIcon* ic=nullptr,FXPopup* pup=nullptr,FXuint opts=JUSTIFY_NORMAL|ICON_BEFORE_TEXT|MENUBUTTON_DOWN,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);

  virtual FXint getDefaultWidth();
  virtual FXint getDefaultHeight();
  };


class GMTextField : public FXTextField {
  FXDECLARE(GMTextField)
protected:
  GMTextField(){}
private:
  GMTextField(const GMTextField&);
  GMTextField& operator=(const GMTextField&);
public:
  long onLeftBtnPress(FXObject*,FXSelector,void*);
public:
  GMTextField(FXComposite* p,FXint ncols,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=TEXTFIELD_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);

  FXbool extendWordSelection(FXint p,FXbool notify=false);
  };



class GMScrollFrame : public FXVerticalFrame {
FXDECLARE(GMScrollFrame)
protected:
  GMScrollFrame();
private:
  GMScrollFrame(const GMScrollFrame&);
  GMScrollFrame& operator=(const GMScrollFrame&);
public:
  GMScrollFrame(FXComposite*p);
  };

class GMCoverFrame : public FXVerticalFrame {
FXDECLARE(GMCoverFrame)
protected:
  GMCoverFrame();
private:
  GMCoverFrame(const GMCoverFrame&);
  GMCoverFrame& operator=(const GMCoverFrame&);
public:
  GMCoverFrame(FXComposite*p);
  };


class GMScrollHFrame : public FXHorizontalFrame {
FXDECLARE(GMScrollHFrame)
protected:
  GMScrollHFrame();
private:
  GMScrollHFrame(const GMScrollHFrame&);
  GMScrollHFrame& operator=(const GMScrollHFrame&);
public:
  GMScrollHFrame(FXComposite*p);
  };


class GMTabFrame : public FXVerticalFrame {
FXDECLARE(GMTabFrame)
protected:
  GMTabFrame();
private:
  GMTabFrame(const GMTabFrame&);
  GMTabFrame& operator=(const GMTabFrame&);
public:
  GMTabFrame(FXComposite*p);
  };


class GMImageFrame : public FXImageFrame {
FXDECLARE(GMImageFrame)
protected:
  GMImageFrame();
private:
  GMImageFrame(const GMImageFrame&);
  GMImageFrame& operator=(const GMImageFrame&);
public:
  GMImageFrame(FXComposite*p,FXImage *img,FXuint opts=FRAME_SUNKEN|FRAME_THICK,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=0,FXint pr=0,FXint pt=0,FXint pb=0);
  };


class GMScrollBar : public FXScrollBar {
  FXDECLARE(GMScrollBar)
protected:
  GMScrollBar();
  void drawThumb(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h);
private:
  GMScrollBar(const GMScrollBar&);
  GMScrollBar &operator=(const GMScrollBar&);
public:
  long onPaint(FXObject*,FXSelector,void*);
public:
  /// Construct scroll bar
  GMScrollBar(FXComposite* p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=SCROLLBAR_VERTICAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0);
  };

class GMScrollCorner : public FXScrollCorner {
  FXDECLARE(GMScrollCorner)
protected:
  FXColor shadowColor = 0;
protected:
  GMScrollCorner() {}
private:
  GMScrollCorner(const GMScrollCorner&);
  GMScrollCorner &operator=(const GMScrollCorner&);
public:
  long onPaint(FXObject*,FXSelector,void*);
public:
  GMScrollCorner(FXComposite* p);
  };

class GMSpinner : public FXSpinner {
  FXDECLARE(GMSpinner)
protected:
  GMSpinner(){}
private:
  GMSpinner(const GMSpinner&);
  GMSpinner& operator=(const GMSpinner&);
public:
  GMSpinner(FXComposite* p,FXint ncols,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=SPIN_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);
  };


class GMProgressBar : public FXProgressBar {
  FXDECLARE(GMProgressBar)
protected:
  GMProgressBar();
private:
  GMProgressBar(const GMProgressBar&);
  GMProgressBar &operator=(const GMProgressBar&);
public:
  /// Construct progress bar
  GMProgressBar(FXComposite* p,FXObject* target=nullptr,FXSelector sel=0,FXuint opts=PROGRESSBAR_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);
  };


class GMTrackProgressBar : public FXProgressBar {
  FXDECLARE(GMTrackProgressBar)
protected:
  GMTrackProgressBar();
private:
  GMTrackProgressBar(const GMTrackProgressBar&);
  GMTrackProgressBar &operator=(const GMTrackProgressBar&);
public:
  long onMotion(FXObject*,FXSelector,void*);
  long onLeftBtnPress(FXObject*,FXSelector,void*);
  long onLeftBtnRelease(FXObject*,FXSelector,void*);
public:
  /// Construct progress bar
  GMTrackProgressBar(FXComposite* p,FXObject* target=nullptr,FXSelector sel=0,FXuint opts=PROGRESSBAR_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);
  };



#endif
