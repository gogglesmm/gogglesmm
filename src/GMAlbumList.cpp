#include "gmdefs.h"
#include "gmutils.h"
#include <fxkeys.h>

#include "GMTrack.h"
#include "GMApp.h"
#include "GMTrackList.h"
#include "GMSource.h"
#include "GMPlayerManager.h"
#include "GMTrackDatabase.h"
#include "GMTrackView.h"

#include "GMCoverCache.h"
#include "GMAlbumList.h"


#define COVER_TEXT_SPACING 2
#define SIDE_SPACING             4    // Left or right spacing between items

#define LIST_LINE_SPACING 4
#define LIST_ICON_SPACING 4
#define LIST_SIDE_SPACING 6

#define SELECT_MASK   (ALBUMLIST_EXTENDEDSELECT|ALBUMLIST_SINGLESELECT|ALBUMLIST_BROWSESELECT|ALBUMLIST_MULTIPLESELECT)
#define ALBUMLIST_MASK (SELECT_MASK|ALBUMLIST_COLUMNS|ALBUMLIST_BROWSER|ALBUMLIST_YEAR)





#define GET_ARTIST_STRING(x)  GMPlayerManager::instance()->getTrackDatabase()->getArtist(x)


// return true if string starts with configured keyword
static inline FXbool begins_with_keyword(const FXString & t){
  for (FXint i=0;i<GMPlayerManager::instance()->getPreferences().gui_sort_keywords.no();i++){
    if (comparecase(t,GMPlayerManager::instance()->getPreferences().gui_sort_keywords[i],GMPlayerManager::instance()->getPreferences().gui_sort_keywords[i].length())==0) return true;
    }
  return false;
  }

// return true if string starts with configured keyword
static inline FXbool begins_with_keyword_ptr(const FXString * t){
  for (FXint i=0;i<GMPlayerManager::instance()->getPreferences().gui_sort_keywords.no();i++){
    if (comparecase(*t,GMPlayerManager::instance()->getPreferences().gui_sort_keywords[i],GMPlayerManager::instance()->getPreferences().gui_sort_keywords[i].length())==0) return true;
    }
  return false;
  }

// compare two string taking into account the configured keywords it needs to ignore
static inline FXint keywordcompare(const FXString *a,const FXString *b) {
  FXint pa,pb;

  if (a==b) return 0;

  if (begins_with_keyword_ptr(a))
    pa=FXMIN(a->length()-1,a->find(' ')+1);
  else
    pa=0;

  if (begins_with_keyword_ptr(b))
    pb=FXMIN(b->length()-1,b->find(' ')+1);
  else
    pb=0;
  return comparecase(&((*a)[pa]),&((*b)[pb]));
  }

// compare two string taking into account the configured keywords it needs to ignore
static inline FXint keywordcompare(const FXString & a,const FXString & b) {
  FXint pa,pb;
  if (begins_with_keyword(a))
    pa=FXMIN(a.length()-1,a.find(' ')+1);
  else
    pa=0;

  if (begins_with_keyword(b))
    pb=FXMIN(b.length()-1,b.find(' ')+1);
  else
    pb=0;
  return comparecase(&((a)[pa]),&((b)[pb]));
  }



FXint GMAlbumListItem::album_list_sort(const GMAlbumListItem* pa,const GMAlbumListItem* pb){
  FXint a=0,b=0;
  if (GMTrackView::album_by_year) {
    if (pa->year>pb->year) return 1;
    else if (pa->year<pb->year) return -1;
    }
  if (begins_with_keyword(pa->title)) a=FXMIN(pa->title.length()-1,pa->title.find(' ')+1);
  if (begins_with_keyword(pb->title)) b=FXMIN(pb->title.length()-1,pb->title.find(' ')+1);
  return comparecase(&pa->title[a],&pb->title[b]);
  }

FXint GMAlbumListItem::album_list_sort_reverse(const GMAlbumListItem* pa,const GMAlbumListItem* pb){
  FXint a=0,b=0;
  if (GMTrackView::album_by_year) {
    if (pa->year>pb->year) return -1;
    else if (pa->year<pb->year) return 1;
    }
  if (begins_with_keyword(pa->title)) a=FXMIN(pa->title.length()-1,pa->title.find(' ')+1);
  if (begins_with_keyword(pb->title)) b=FXMIN(pb->title.length()-1,pb->title.find(' ')+1);
  return -comparecase(&pa->title[a],&pb->title[b]);
  }



FXint GMAlbumListItem::album_browser_sort(const GMAlbumListItem* pa,const GMAlbumListItem* pb){
  FXint x = keywordcompare(GET_ARTIST_STRING(pa->artist),GET_ARTIST_STRING(pb->artist));
  if (x!=0) return GMTrackView::reverse_artist ? -x : x;
  if (GMTrackView::album_by_year) {
    if (pa->year>pb->year) return 1;
    else if (pa->year<pb->year) return -1;
    }
  return keywordcompare(pa->title,pb->title);
  }

FXint GMAlbumListItem::album_browser_sort_reverse(const GMAlbumListItem* pa,const GMAlbumListItem* pb){
  FXint x = keywordcompare(GET_ARTIST_STRING(pb->artist),GET_ARTIST_STRING(pa->artist));
  if (x!=0) return GMTrackView::reverse_artist ? -x : x;
  if (GMTrackView::album_by_year) {
    if (pa->year>pb->year) return -1;
    else if (pa->year<pb->year) return 1;
    }
  return keywordcompare(pa->title,pb->title);
  }


// Object implementation
FXIMPLEMENT(GMAlbumListItem,FXObject,nullptr,0)

static void drawTextLimited(FXDC & dc,FXFont * font,FXint x,FXint y,FXint space,const FXString & text) {
  FXint dw,len=text.length();
  FXint tw = font->getTextWidth(text.text(),text.length());
  FXint th = font->getFontHeight();
  if (tw>space) {
    dw = font->getTextWidth("...",3);
    while((tw=font->getTextWidth(text.text(),len))+dw>space && len>1) len=text.dec(len);
    dc.setClipRectangle(x,y,space,th);
    dc.drawText(x,y+font->getFontAscent(),text.text(),len);
    dc.drawText(x+tw,y+font->getFontAscent(),"...",3);
    dc.clearClipRectangle();
    }
  else {
    dc.drawText(x,y+font->getFontAscent(),text.text(),len);
    }
  }

void GMAlbumListItem::prepare(GMAlbumList * list) {
  list->getCoverRender().markCover(id);
  }

void GMAlbumListItem::drawList(const GMAlbumList* list,FXDC& dc,FXint xx,FXint yy,FXint ww,FXint hh) const {

  FXFont *basefont = (id==-1) ? list->getListHeadFont() : list->getListBaseFont();
  FXFont *tailfont = list->getListTailFont();
  FXIcon *icon     = list->getListIcon();

  const FXint hb=basefont->getFontHeight();
  const FXint hi=tailfont->getFontHeight();
  FXint baseline=yy;

  FXint displayyear = 0;
  if (list->getListStyle()&ALBUMLIST_YEAR)
    displayyear = year;


  if (hb>=hi)
    baseline+=basefont->getFontAscent()+(hh-hb)/2;
  else
    baseline+=tailfont->getFontAscent()+(hi-hb)/2;

  if(isSelected())
    dc.setForeground(list->getSelBackColor());

  dc.fillRectangle(xx,yy,ww,hh);
  if(hasFocus()){
    dc.drawFocusRectangle(xx+1,yy+1,ww-2,hh-2);
    }
  xx+=LIST_SIDE_SPACING/2;
  if(icon){
    dc.drawIcon(icon,xx,yy+(hh-icon->getHeight())/2);
    xx+=LIST_ICON_SPACING+icon->getWidth();
    }
  if(!title.empty()){
    if(isSelected())
      dc.setForeground(list->getSelTextColor());
    else
      dc.setForeground(list->getTextColor());

    dc.setFont(basefont);
    dc.drawText(xx,baseline,title);

    if (displayyear || audioproperty.length() || state&SHOW_ARTIST) {
      FXint len=basefont->getTextWidth(title.text(),title.length());
      dc.setFont(tailfont);
      if (state&SHOW_ARTIST){
        if (displayyear) {
          if (audioproperty.length())
            dc.drawText(xx+len+LIST_SIDE_SPACING,baseline,FXString::value("(%s %d %s)",GMPlayerManager::instance()->getTrackDatabase()->getArtist(artist)->text(),displayyear,audioproperty.text()));
          else
            dc.drawText(xx+len+LIST_SIDE_SPACING,baseline,FXString::value("(%s %d)",GMPlayerManager::instance()->getTrackDatabase()->getArtist(artist)->text(),year));
          }
        else {
          if (audioproperty.length())
            dc.drawText(xx+len+LIST_SIDE_SPACING,baseline,FXString::value("(%s %s)",GMPlayerManager::instance()->getTrackDatabase()->getArtist(artist)->text(),audioproperty.text()));
          else
            dc.drawText(xx+len+LIST_SIDE_SPACING,baseline,FXString::value("(%s)",GMPlayerManager::instance()->getTrackDatabase()->getArtist(artist)->text()));
          }
        }
      else {
        if (displayyear) {
          if (audioproperty.length())
            dc.drawText(xx+len+LIST_SIDE_SPACING,baseline,FXString::value("(%d %s)",displayyear,audioproperty.text()));
          else
            dc.drawText(xx+len+LIST_SIDE_SPACING,baseline,FXString::value("(%d)",displayyear));
          }
        else {
          dc.drawText(xx+len+LIST_SIDE_SPACING,baseline,FXString::value("(%s)",audioproperty.text()));
          }
        }
      }
    }
  }


// Draw item
void GMAlbumListItem::draw(GMAlbumList* list,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const {
  if(isSelected())
    dc.setForeground(list->getSelBackColor());
  dc.fillRectangle(x,y,w,h);

  FXFont *font=list->getCoverBaseFont();
  FXFont *cfont=list->getCoverHeadFont();
  const FXint h1=cfont->getFontHeight();
  const FXint xx=x+SIDE_SPACING;
  const FXint is=list->getCoverRender().getSize();
  FXint yy=y+SIDE_SPACING;

  list->getCoverRender().drawCover(id,dc,xx,yy);

  if (audioproperty.length()){
    FXint fudge = 4;
    FXint ql = cfont->getTextWidth(audioproperty);
    dc.setForeground(FXRGB(0,200,0));
    dc.fillRectangle(xx+is-ql-fudge-1,yy+is-h1-fudge-1,ql+fudge,h1+fudge);
    dc.setFont(cfont);
    dc.setForeground(list->getSelTextColor());
    dc.drawText(xx+is-ql-(fudge>>1)-1,yy+is+cfont->getFontAscent()-h1-(fudge>>1)-1,audioproperty.text(),audioproperty.length());
    }

  if(isSelected())
    dc.setForeground(list->getSelTextColor());
  else
    dc.setForeground(list->getApp()->getShadowColor());

  dc.drawRectangle(xx,yy,is-1,is-1);


  if(isSelected())
    dc.setForeground(list->getSelTextColor());
  else
    dc.setForeground(list->getTextColor());

  /// Start of Text
  yy+=is+COVER_TEXT_SPACING;

  if(isSelected())
    dc.setForeground(list->getSelTextColor());
  else
    dc.setForeground(list->getTextColor());

  const FXString * str = GMPlayerManager::instance()->getTrackDatabase()->getArtist(artist);
  if (!str->empty()) {
    dc.setFont(cfont);
    drawTextLimited(dc,cfont,xx,yy,is,*str);
    dc.setFont(font);
    drawTextLimited(dc,font,xx,yy+h1,is,title);
    }
  else {
    dc.setFont(cfont);
    drawTextLimited(dc,font,xx,yy,is,title);
    }
  }




// See if item got hit and where: 0 is outside, 1 is icon, 2 is text
FXint GMAlbumListItem::hitItem(const GMAlbumList* list,FXint rx,FXint ry,FXint rw,FXint rh) const {
  FXFont *font=list->getCoverBaseFont();
  FXFont *cfont=list->getCoverHeadFont();

  //register FXuint options=list->getListStyle();
  FXint tw=0,th=0,ix,iy,tx,ty;

  FXint h1=cfont->getFontHeight();
  FXint h2=font->getFontHeight();

  FXint iw,ih;
  iw=ih=list->getCoverRender().getSize();


  ix=SIDE_SPACING;
  iy=SIDE_SPACING;

  ty=iy+ih+COVER_TEXT_SPACING;
  tx=ix;
  tw=iw;
  th=h1+h2;

  // In icon?
  if(ix<=rx+rw && iy<=ry+rh && rx<ix+iw && ry<iy+ih) return 1;

  // In text?
  if(tx<=rx+rw && ty<=ry+rh && rx<tx+tw && ry<ty+th) return 2;

  // Outside
  return 0;
  }


// Set or kill focus
void GMAlbumListItem::setFocus(FXbool focus){
  state^=((0-focus)^state)&FOCUS;
  }

// Select or deselect item
void GMAlbumListItem::setSelected(FXbool selected){
  state^=((0-selected)^state)&SELECTED;
  }

// Icon is draggable
void GMAlbumListItem::setDraggable(FXbool draggable){
  state^=((0-draggable)^state)&DRAGGABLE;
  }

// Icon is draggable
void GMAlbumListItem::setShowArtist(FXbool showartist){
  state^=((0-showartist)^state)&SHOW_ARTIST;
  }


FXint GMAlbumListItem::getWidth(const GMAlbumList * list){
  FXint w=0;

  if (list->getListIcon())
    w+=list->getListIcon()->getWidth();

  if (id==-1)
    w+=list->getListTailFont()->getTextWidth(title.text(),title.length());
  else
    w+=list->getListBaseFont()->getTextWidth(title.text(),title.length());

  if (year && list->getListStyle()&ALBUMLIST_YEAR)
    w+=LIST_SIDE_SPACING+list->getListTailFont()->getTextWidth("(8888)",6);

  return w+LIST_SIDE_SPACING;
  }

FXString GMAlbumListItem::getTipText() const {
  const FXString * artistname = GMPlayerManager::instance()->getTrackDatabase()->getArtist(artist);
  if (year>0) {
    if (!artistname->empty())
      return *artistname+"\n"+title+"\n"+FXString::value(year);
    else
      return title+"\n"+FXString::value(year);
    }
  else {
    if (!artistname->empty())
      return *artistname+"\n"+title;
    else
      return title;
    }
  }

// Delete icons if owned
GMAlbumListItem::~GMAlbumListItem(){
  }

/*******************************************************************************/

// Map
FXDEFMAP(GMAlbumList) GMAlbumListMap[]={
  FXMAPFUNC(SEL_PAINT,0,GMAlbumList::onPaint),
  FXMAPFUNC(SEL_MOTION,0,GMAlbumList::onMotion),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,GMAlbumList::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,GMAlbumList::onLeftBtnRelease),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS,0,GMAlbumList::onRightBtnPress),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,0,GMAlbumList::onRightBtnRelease),
  FXMAPFUNC(SEL_TIMEOUT,FXScrollArea::ID_AUTOSCROLL,GMAlbumList::onAutoScroll),
  FXMAPFUNC(SEL_TIMEOUT,GMAlbumList::ID_TIPTIMER,GMAlbumList::onTipTimer),
  FXMAPFUNC(SEL_TIMEOUT,GMAlbumList::ID_LOOKUPTIMER,GMAlbumList::onLookupTimer),
  FXMAPFUNC(SEL_UNGRABBED,0,GMAlbumList::onUngrabbed),
  FXMAPFUNC(SEL_KEYPRESS,0,GMAlbumList::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,GMAlbumList::onKeyRelease),
  FXMAPFUNC(SEL_ENTER,0,GMAlbumList::onEnter),
  FXMAPFUNC(SEL_LEAVE,0,GMAlbumList::onLeave),
  FXMAPFUNC(SEL_FOCUSIN,0,GMAlbumList::onFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT,0,GMAlbumList::onFocusOut),
  FXMAPFUNC(SEL_CLICKED,0,GMAlbumList::onClicked),
  FXMAPFUNC(SEL_DOUBLECLICKED,0,GMAlbumList::onDoubleClicked),
  FXMAPFUNC(SEL_TRIPLECLICKED,0,GMAlbumList::onTripleClicked),
  FXMAPFUNC(SEL_COMMAND,0,GMAlbumList::onCommand),
  FXMAPFUNC(SEL_QUERY_TIP,0,GMAlbumList::onQueryTip),
  FXMAPFUNC(SEL_QUERY_HELP,0,GMAlbumList::onQueryHelp),
  FXMAPFUNC(SEL_UPDATE,GMAlbumList::ID_YEAR,GMAlbumList::onUpdShowYear),
  FXMAPFUNC(SEL_UPDATE,GMAlbumList::ID_ARRANGE_BY_ROWS,GMAlbumList::onUpdArrangeByRows),
  FXMAPFUNC(SEL_UPDATE,GMAlbumList::ID_ARRANGE_BY_COLUMNS,GMAlbumList::onUpdArrangeByColumns),
  FXMAPFUNC(SEL_COMMAND,GMAlbumList::ID_YEAR,GMAlbumList::onCmdShowYear),
  FXMAPFUNC(SEL_COMMAND,GMAlbumList::ID_ARRANGE_BY_ROWS,GMAlbumList::onCmdArrangeByRows),
  FXMAPFUNC(SEL_COMMAND,GMAlbumList::ID_ARRANGE_BY_COLUMNS,GMAlbumList::onCmdArrangeByColumns),
  FXMAPFUNC(SEL_COMMAND,GMAlbumList::ID_SELECT_ALL,GMAlbumList::onCmdSelectAll),
  FXMAPFUNC(SEL_COMMAND,GMAlbumList::ID_DESELECT_ALL,GMAlbumList::onCmdDeselectAll),
  FXMAPFUNC(SEL_COMMAND,GMAlbumList::ID_SELECT_INVERSE,GMAlbumList::onCmdSelectInverse),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETVALUE,GMAlbumList::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETINTVALUE,GMAlbumList::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_GETINTVALUE,GMAlbumList::onCmdGetIntValue),
  };


// Object implementation
FXIMPLEMENT(GMAlbumList,FXScrollArea,GMAlbumListMap,ARRAYNUMBER(GMAlbumListMap))


/*******************************************************************************/


// Serialization
GMAlbumList::GMAlbumList(){
  flags|=FLAG_ENABLED;
  }


// Icon List
GMAlbumList::GMAlbumList(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXScrollArea(p,opts,x,y,w,h){
  GMScrollArea::replaceScrollbars(this);
  flags|=FLAG_ENABLED;
  target=tgt;
  message=sel;
  textColor=getApp()->getForeColor();
  selbackColor=getApp()->getSelbackColor();
  seltextColor=getApp()->getSelforeColor();
  listbasefont=getApp()->getNormalFont();
  listheadfont=getApp()->getNormalFont();
  listtailfont=((GMApp*)(getApp()))->getListTailFont();
  coverheadfont=((GMApp*)(getApp()))->getCoverHeadFont();
  coverbasefont=((GMApp*)(getApp()))->getCoverBaseFont();
  altbackColor=GMPlayerManager::instance()->getPreferences().gui_row_color;
  }


void GMAlbumList::setCoverCache(GMCoverCache* cache){
  covers.setCache(cache);
  recalc();
  }


// Create window
void GMAlbumList::create(){
  FXScrollArea::create();
  listbasefont->create();
  listheadfont->create();
  listtailfont->create();
  coverheadfont->create();
  coverbasefont->create();
  }


// Detach window
void GMAlbumList::detach(){
  FXScrollArea::detach();
  listbasefont->detach();
  listheadfont->detach();
  listtailfont->detach();
  coverheadfont->detach();
  coverbasefont->detach();
  }


// If window can have focus
FXbool GMAlbumList::canFocus() const { return true; }

// Into focus chain
void GMAlbumList::setFocus(){
  FXScrollArea::setFocus();
  setDefault(true);
  }


// Out of focus chain
void GMAlbumList::killFocus(){
  FXScrollArea::killFocus();
  setDefault(maybe);
  }


// Propagate size change
void GMAlbumList::recalc(){
  FXScrollArea::recalc();
  flags|=FLAG_RECALC;
  cursor=-1;
  }



// Return visible area y position
FXint GMAlbumList::getVisibleY() const {
  return 0;
  }


// Return visible area height
FXint GMAlbumList::getVisibleHeight() const {
  return height-horizontal->getHeight();
  }

// Determine number of columns and number of rows
void GMAlbumList::getrowscols(FXint& nr,FXint& nc,FXint w,FXint h) const {
  if(options&ALBUMLIST_BROWSER){
    if(options&ALBUMLIST_COLUMNS){
      nc=w/itemWidth;
      if(nc<1) nc=1;
      nr=(items.no()+nc-1)/nc;
      if(nr*itemHeight > h){
        nc=(w-vertical->getDefaultWidth())/itemWidth;
        if(nc<1) nc=1;
        nr=(items.no()+nc-1)/nc;
        }
      if(nr<1) nr=1;
      }
    else{
      nr=h/itemHeight;
      if(nr<1) nr=1;
      nc=(items.no()+nr-1)/nr;
      if(nc*itemWidth > w){
        nr=(h-horizontal->getDefaultHeight())/itemHeight;
        if(nr<1) nr=1;
        nc=(items.no()+nr-1)/nr;
        }
      if(nc<1) nc=1;
      }
    }
  else{
    nr=items.no();
    nc=1;
    }
  }


// Recompute interior
void GMAlbumList::recompute(){
  if (options&ALBUMLIST_BROWSER) {
    itemWidth=covers.getSize()+SIDE_SPACING+SIDE_SPACING;
    itemHeight=covers.getSize()+SIDE_SPACING+SIDE_SPACING+coverheadfont->getFontHeight()+coverbasefont->getFontHeight()+COVER_TEXT_SPACING;
    }
  else {
    FXint w;
    FXint ih=(listicon) ? listicon->getHeight() : 0;
    itemWidth=1;
    itemHeight=LIST_LINE_SPACING+FXMAX3(listtailfont->getFontHeight(),listbasefont->getFontHeight(),ih);
    for(FXint i=0;i<items.no();i++){
      w=items[i]->getWidth(this);
      if (w>itemWidth) itemWidth=w;
      }
    }

  // Get number of rows or columns
  getrowscols(nrows,ncols,width,height);

  // Done
  flags&=~FLAG_RECALC;
  }


// Determine content width of icon list
FXint GMAlbumList::getContentWidth(){
  if(flags&FLAG_RECALC) recompute();
  return ncols*itemWidth;
  }


// Determine content height of icon list
FXint GMAlbumList::getContentHeight(){
  if(flags&FLAG_RECALC) recompute();
  return nrows*itemHeight;
  }


// Recalculate layout
void GMAlbumList::layout(){
  // Place scroll bars
  placeScrollBars(width,height);

  // Set line size
  vertical->setLine(itemHeight);
  horizontal->setLine(itemWidth);

  // We were supposed to make this item viewable
  if(0<=viewable){
    makeItemVisible(viewable);
    }

  // Force repaint
  update();

  // Clean
  flags&=~FLAG_DIRTY;
  }


// Changed size:- this is a bit tricky, because
// we don't want to re-measure the items, but the content
// size has changed because the number of rows/columns has...
void GMAlbumList::resize(FXint w,FXint h){
  FXint nr=nrows;
  FXint nc=ncols;
  if(w!=width || h!=height){
    getrowscols(nrows,ncols,w,h);
    if(nr!=nrows || nc!=ncols) update();
    }
  FXScrollArea::resize(w,h);
  }


// Changed size and/or pos:- this is a bit tricky, because
// we don't want to re-measure the items, but the content
// size has changed because the number of rows/columns has...
void GMAlbumList::position(FXint x,FXint y,FXint w,FXint h){
  FXint nr=nrows;
  FXint nc=ncols;
  if(w!=width || h!=height){
    getrowscols(nrows,ncols,w,h);
    if(nr!=nrows || nc!=ncols) update();
    }
  FXScrollArea::position(x,y,w,h);
  }

// Get item text
FXint GMAlbumList::getItemId(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::getItemText: index out of range.\n",getClassName()); }
  return items[index]->getId();
  }


// True if item is selected
FXbool GMAlbumList::isItemSelected(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::isItemSelected: index out of range.\n",getClassName()); }
  return items[index]->isSelected();
  }


// True if item is current
FXbool GMAlbumList::isItemCurrent(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::isItemCurrent: index out of range.\n",getClassName()); }
  return index==current;
  }


// True if item (partially) visible
FXbool GMAlbumList::isItemVisible(FXint index) const {
  FXbool vis=false;
  FXint x,y;
  if(index<0 || items.no()<=index){ fxerror("%s::isItemVisible: index out of range.\n",getClassName()); }
  if(options&(ALBUMLIST_BROWSER)){
    if(options&ALBUMLIST_COLUMNS){
      FXASSERT(ncols>0);
      x=pos_x+itemWidth*(index%ncols);
      y=pos_y+itemHeight*(index/ncols);
      }
    else{
      FXASSERT(nrows>0);
      x=pos_x+itemWidth*(index/nrows);
      y=pos_y+itemHeight*(index%nrows);
      }
    if(0<x+itemWidth && x<getVisibleWidth() && 0<y+itemHeight && y<getVisibleHeight()) vis=true;
    }
  else{
    y=pos_y+index*itemHeight;
    if(0<y+itemHeight && y<getVisibleHeight()) vis=true;
    }
  return vis;
  }


// Make item fully visible
void GMAlbumList::makeItemVisible(FXint index){
  if(0<=index && index<items.no()){

    // Remember for later
    viewable=index;

    // Was realized
    if(xid){
      FXint x,y,px,py,vw,vh;

      // Force layout if dirty
      if(flags&FLAG_RECALC) layout();

      px=pos_x;
      py=pos_y;

      vw=getVisibleWidth();
      vh=getVisibleHeight();

      // Showing icon view
      if(options&(ALBUMLIST_BROWSER)){
        if(options&ALBUMLIST_COLUMNS){
          FXASSERT(ncols>0);
          x=itemWidth*(index%ncols);
          y=itemHeight*(index/ncols);
          }
        else{
          FXASSERT(nrows>0);
          x=itemWidth*(index/nrows);
          y=itemHeight*(index%nrows);
          }
        if(px+x+itemWidth >= vw) px=vw-x-itemWidth;
        if(px+x <= 0) px=-x;
        if(py+y+itemHeight >= vh) py=vh-y-itemHeight;
        if(py+y <= 0) py=-y;
        }

      // Showing list view
      else{
        y=index*itemHeight;
        if(py+y+itemHeight >= vh) py=vh-y-itemHeight;
        if(py+y<=0) py=y;
        }

      // Scroll into view
      setPosition(px,py);

      // Done it
      viewable=-1;
      }
    }
  }

// Get item at position x,y
FXint GMAlbumList::getItemAt(FXint x,FXint y) const {
  FXint ix,iy;
  FXint r,c,index;
  y-=pos_y;
  x-=pos_x;
  if(options&(ALBUMLIST_BROWSER)){
    c=x/itemWidth;
    r=y/itemHeight;
    if(c<0 || c>=ncols || r<0 || r>=nrows) return -1;
    index=(options&ALBUMLIST_COLUMNS) ? ncols*r+c : nrows*c+r;
    if(index<0 || index>=items.no()) return -1;
    ix=itemWidth*c;
    iy=itemHeight*r;
    if(items[index]->hitItem(this,x-ix,y-iy)==0) return -1;
    }
  else{
    index=y/itemHeight;
    if(index<0 || index>=items.no()) return -1;
    }
  return index;
  }


typedef FXint (*FXCompareFunc)(const FXString&,const FXString&,FXint);


// Did we hit the item, and which part of it did we hit
FXint GMAlbumList::hitItem(FXint index,FXint x,FXint y,FXint ww,FXint hh) const {
  FXint ix,iy,r,c,hit=0;
  if(0<=index && index<items.no()){
    x-=pos_x;
    y-=pos_y;
    if(options&ALBUMLIST_BROWSER){
      if(options&ALBUMLIST_COLUMNS){
        r=index/ncols;
        c=index%ncols;
        }
      else{
        c=index/nrows;
        r=index%nrows;
        }
      }
    else{
      r=index;
      c=0;
      }
    ix=itemWidth*c;
    iy=itemHeight*r;
    hit=items[index]->hitItem(this,x-ix,y-iy,ww,hh);
    }
  return hit;
  }


// Repaint
void GMAlbumList::updateItem(FXint index) const {
  if(xid && 0<=index && index<items.no()){
    if(options&ALBUMLIST_BROWSER){
      if(options&ALBUMLIST_COLUMNS){
        FXASSERT(ncols>0);
        update(pos_x+itemWidth*(index%ncols),pos_y+itemHeight*(index/ncols),itemWidth,itemHeight);
        }
      else{
        FXASSERT(nrows>0);
        update(pos_x+itemWidth*(index/nrows),pos_y+itemHeight*(index%nrows),itemWidth,itemHeight);
        }
      }
    else{
      update(0,pos_y+index*itemHeight,width,itemHeight);
      }
    }
  }


// Select one item
FXbool GMAlbumList::selectItem(FXint index,FXbool notify){
  if(index<0 || items.no()<=index){ fxerror("%s::selectItem: index out of range.\n",getClassName()); }
  if(!items[index]->isSelected()){
    switch(options&SELECT_MASK){
      case ALBUMLIST_SINGLESELECT:
      case ALBUMLIST_BROWSESELECT:
        killSelection(notify);
      case ALBUMLIST_EXTENDEDSELECT:
      case ALBUMLIST_MULTIPLESELECT:
        items[index]->setSelected(true);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)index);}
        break;
      }
    return true;
    }
  return false;
  }


// Deselect one item
FXbool GMAlbumList::deselectItem(FXint index,FXbool notify){
  if(index<0 || items.no()<=index){ fxerror("%s::deselectItem: index out of range.\n",getClassName()); }
  if(items[index]->isSelected()){
    switch(options&SELECT_MASK){
      case ALBUMLIST_EXTENDEDSELECT:
      case ALBUMLIST_MULTIPLESELECT:
      case ALBUMLIST_SINGLESELECT:
        items[index]->setSelected(false);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)index);}
        break;
      }
    return true;
    }
  return false;
  }


// Toggle one item
FXbool GMAlbumList::toggleItem(FXint index,FXbool notify){
  if(index<0 || items.no()<=index){ fxerror("%s::toggleItem: index out of range.\n",getClassName()); }
  switch(options&SELECT_MASK){
    case ALBUMLIST_BROWSESELECT:
      if(!items[index]->isSelected()){
        killSelection(notify);
        items[index]->setSelected(true);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)index);}
        }
      break;
    case ALBUMLIST_SINGLESELECT:
      if(!items[index]->isSelected()){
        killSelection(notify);
        items[index]->setSelected(true);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)index);}
        }
      else{
        items[index]->setSelected(false);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)index);}
        }
      break;
    case ALBUMLIST_EXTENDEDSELECT:
    case ALBUMLIST_MULTIPLESELECT:
      if(!items[index]->isSelected()){
        items[index]->setSelected(true);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)index);}
        }
      else{
        items[index]->setSelected(false);
        updateItem(index);
        if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)index);}
        }
      break;
    }
  return true;
  }


// Select items in rectangle
FXbool GMAlbumList::selectInRectangle(FXint x,FXint y,FXint w,FXint h,FXbool notify){
  FXint r,c,index;
  FXbool changed=false;
  if(options&ALBUMLIST_BROWSER){
    for(r=0; r<nrows; r++){
      for(c=0; c<ncols; c++){
        index=(options&ALBUMLIST_COLUMNS) ? ncols*r+c : nrows*c+r;
        if(index<items.no()){
          if(hitItem(index,x,y,w,h)){
            changed|=selectItem(index,notify);
            }
          }
        }
      }
    }
  else{
    for(index=0; index<items.no(); index++){
      if(hitItem(index,x,y,w,h)){
        changed|=selectItem(index,notify);
        }
      }
    }
  return changed;
  }


// Extend selection
FXbool GMAlbumList::extendSelection(FXint index,FXbool notify){
  FXbool changes=false;
  FXint i1,i2,i3,i;
  if(0<=index && 0<=anchor && 0<=extent){

    // Find segments
    i1=index;
    if(anchor<i1){i2=i1;i1=anchor;}
    else{i2=anchor;}
    if(extent<i1){i3=i2;i2=i1;i1=extent;}
    else if(extent<i2){i3=i2;i2=extent;}
    else{i3=extent;}

    // First segment
    for(i=i1; i<i2; i++){

      // item===extent---anchor
      // item===anchor---extent
      if(i1==index){
        if(!items[i]->isSelected()){
          items[i]->setSelected(true);
          updateItem(i);
          changes=true;
          if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)i);}
          }
        }

      // extent===anchor---item
      // extent===item-----anchor
      else if(i1==extent){
        if(items[i]->isSelected()){
          items[i]->setSelected(false);
          updateItem(i);
          changes=true;
          if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)i);}
          }
        }
      }

    // Second segment
    for(i=i2+1; i<=i3; i++){

      // extent---anchor===item
      // anchor---extent===item
      if(i3==index){
        if(!items[i]->isSelected()){
          items[i]->setSelected(true);
          updateItem(i);
          changes=true;
          if(notify && target){target->tryHandle(this,FXSEL(SEL_SELECTED,message),(void*)(FXival)i);}
          }
        }

      // item-----anchor===extent
      // anchor---item=====extent
      else if(i3==extent){
        if(items[i]->isSelected()){
          items[i]->setSelected(false);
          updateItem(i);
          changes=true;
          if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)i);}
          }
        }
      }
    extent=index;
    }
  return changes;
  }


// Kill selection
FXbool GMAlbumList::killSelection(FXbool notify){
  FXbool changes=false;
  FXint i;
  for(i=0; i<items.no(); i++){
    if(items[i]->isSelected()){
      items[i]->setSelected(false);
      updateItem(i);
      changes=true;
      if(notify && target){target->tryHandle(this,FXSEL(SEL_DESELECTED,message),(void*)(FXival)i);}
      }
    }
  return changes;
  }


// Lasso changed, so select/unselect items based on difference between new and old lasso box
void GMAlbumList::lassoChanged(FXint ox,FXint oy,FXint ow,FXint oh,FXint nx,FXint ny,FXint nw,FXint nh,FXbool notify){
  FXint r,c;
  FXint ohit,nhit,index;
  if(options&ALBUMLIST_BROWSER){
    for(r=0; r<nrows; r++){
      for(c=0; c<ncols; c++){
        index=(options&ALBUMLIST_COLUMNS) ? ncols*r+c : nrows*c+r;
        if(index<items.no()){
          ohit=hitItem(index,ox,oy,ow,oh);
          nhit=hitItem(index,nx,ny,nw,nh);
          if(ohit && !nhit){      // In old rectangle and not in new rectangle
            deselectItem(index,notify);
            }
          else if(!ohit && nhit){ // Not in old rectangle and in new rectangle
            selectItem(index,notify);
            }
          }
        }
      }
    }
  else{
    for(index=0; index<items.no(); index++){
      ohit=hitItem(index,ox,oy,ow,oh);
      nhit=hitItem(index,nx,ny,nw,nh);
      if(ohit && !nhit){          // Was in old, not in new
        deselectItem(index,notify);
        }
      else if(!ohit && nhit){     // Not in old, but in new
        selectItem(index,notify);
        }
      }
    }
  }


// Update value from a message
long GMAlbumList::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setCurrentItem((FXint)(FXival)ptr);
  return 1;
  }


// Obtain value from list
long GMAlbumList::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXint*)ptr)=getCurrentItem();
  return 1;
  }


// Update value from a message
long GMAlbumList::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setCurrentItem(*((FXint*)ptr));
  return 1;
  }


// Start motion timer while in this window
long GMAlbumList::onEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onEnter(sender,sel,ptr);
  getApp()->addTimeout(this,ID_TIPTIMER,getApp()->getMenuPause());
  cursor=-1;
  return 1;
  }


// Stop motion timer when leaving window
long GMAlbumList::onLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onLeave(sender,sel,ptr);
  getApp()->removeTimeout(this,ID_TIPTIMER);
  cursor=-1;
  return 1;
  }


// We timed out, i.e. the user didn't move for a while
long GMAlbumList::onTipTimer(FXObject*,FXSelector,void*){
  FXTRACE((250,"%s::onTipTimer %p\n",getClassName(),this));
  flags|=FLAG_TIP;
  return 1;
  }


// We were asked about tip text
long GMAlbumList::onQueryTip(FXObject* sender,FXSelector sel,void* ptr){
  if(FXScrollArea::onQueryTip(sender,sel,ptr)) return 1;
  if((flags&FLAG_TIP) && (0<=cursor)){
    FXString string=items[cursor]->getTipText();
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&string);
    return 1;
    }
  return 0;
  }


// We were asked about status text
long GMAlbumList::onQueryHelp(FXObject* sender,FXSelector sel,void* ptr){
  if(FXScrollArea::onQueryHelp(sender,sel,ptr)) return 1;
  if((flags&FLAG_HELP) && !help.empty()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&help);
    return 1;
    }
  return 0;
  }


// Gained focus
long GMAlbumList::onFocusIn(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onFocusIn(sender,sel,ptr);
  if(0<=current){
    FXASSERT(current<items.no());
    items[current]->setFocus(true);
    updateItem(current);
    }
  return 1;
  }


// Lost focus
long GMAlbumList::onFocusOut(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onFocusOut(sender,sel,ptr);
  if(0<=current){
    FXASSERT(current<items.no());
    items[current]->setFocus(false);
    updateItem(current);
    }
  return 1;
  }


// Draw item list
long GMAlbumList::onPaint(FXObject*,FXSelector,void* ptr){
  FXint rlo,rhi,clo,chi,x,y,w,h,r,c,index;
  FXEvent* event=(FXEvent*)ptr;
  FXDCWindow dc(this,event);

  if (options&ALBUMLIST_BROWSER) {

  // Set font
//dc.setFont(font);

  // Exposed rows
  rlo=(0-pos_y)/itemHeight;
  rhi=(height-pos_y)/itemHeight;
  if(rlo<0) rlo=0;
  if(rhi>=nrows) rhi=nrows-1;

  // Exposed columns
  clo=(0-pos_x)/itemWidth;
  chi=(width-pos_x)/itemWidth;
  if(clo<0) clo=0;
  if(chi>=ncols) chi=ncols-1;

  covers.reset();
  for(r=rlo; r<=rhi; r++){
    //y=pos_y+r*itemHeight;
    for(c=clo; c<=chi; c++){
      //x=pos_x+c*itemWidth;
      index=(options&ALBUMLIST_COLUMNS) ? ncols*r+c : nrows*c+r;
      if(index<items.no()){
        items[index]->prepare(this);
        }
      }
    }

  // Exposed rows
  rlo=(event->rect.y-pos_y)/itemHeight;
  rhi=(event->rect.y+event->rect.h-pos_y)/itemHeight;
  if(rlo<0) rlo=0;
  if(rhi>=nrows) rhi=nrows-1;

  // Exposed columns
  clo=(event->rect.x-pos_x)/itemWidth;
  chi=(event->rect.x+event->rect.w-pos_x)/itemWidth;
  if(clo<0) clo=0;
  if(chi>=ncols) chi=ncols-1;

  //

  for(r=rlo; r<=rhi; r++){
    y=pos_y+r*itemHeight;
    for(c=clo; c<=chi; c++){
      x=pos_x+c*itemWidth;
      index=(options&ALBUMLIST_COLUMNS) ? ncols*r+c : nrows*c+r;

//      if ((index%2)==0)
        dc.setForeground(backColor);
//      else
//        dc.setForeground(altbackColor);

      if(index<items.no()){
        items[index]->draw(this,dc,x,y,itemWidth,itemHeight);
        }
      else{
        dc.fillRectangle(x,y,itemWidth,itemHeight);
        }
      }
    }

  // Background below
  y=pos_y+(rhi+1)*itemHeight;
  if(y<event->rect.y+event->rect.h){
    dc.setForeground(backColor);
    dc.fillRectangle(event->rect.x,y,event->rect.w,event->rect.y+event->rect.h-y);
    }

  // Background to the right
  x=pos_x+(chi+1)*itemWidth;
  if(x<event->rect.x+event->rect.w){
    dc.setForeground(backColor);
    dc.fillRectangle(x,event->rect.y,event->rect.x+event->rect.w-x,event->rect.h);
    }


  // Gray selection rectangle; look ma, no blending!
  if(flags&FLAG_LASSO){
    if(anchorx!=currentx && anchory!=currenty){
      FXMINMAX(x,w,anchorx,currentx); w-=x;
      FXMINMAX(y,h,anchory,currenty); h-=y;
      dc.setFunction(BLT_SRC_AND_DST);
      dc.setForeground(FXRGB(0xD5,0xD5,0xD5));
      dc.fillRectangle(x+pos_x,y+pos_y,w,h);
      dc.setForeground(FXRGB(0x55,0x55,0x55));
      dc.drawRectangle(x+pos_x,y+pos_y,w-1,h-1);
      }
    }
  }
  else {
  FXint i;

  // Set font
//  dc.setFont(font);

  // Paint items
  y=pos_y;
  for(i=0; i<items.no(); i++){
    h=itemHeight;
//    w=items[i]->getWidth(this);
    if(event->rect.y<=y+h && y<event->rect.y+event->rect.h){
      if (i%2)
        dc.setForeground(altbackColor);
      else
        dc.setForeground(backColor);


//      if (items[i]->getData()==(void*)(FXival)-1)
//        dc.setFont(thickfont);
//      else
//        dc.setFont(font);

      items[i]->drawList(this,dc,pos_x,y,FXMAX(itemWidth,getVisibleWidth()),h);
      }
    y+=h;
    }

  // Paint blank area below items
  if(y<event->rect.y+event->rect.h){
    dc.setForeground(backColor);
    dc.fillRectangle(event->rect.x,y,event->rect.w,event->rect.y+event->rect.h-y);
    }





    }
  return 1;
  }


// Start lasso operation
void GMAlbumList::startLasso(FXint ax,FXint ay){
  anchorx=currentx=ax;
  anchory=currenty=ay;
  flags|=FLAG_LASSO;
  }


// Update lasso area
void GMAlbumList::updateLasso(FXint cx,FXint cy){
  FXint slx,shx,sly,shy,lx,hx,ly,hy;
  FXMINMAX(slx,shx,currentx,cx);
  FXMINMAX(sly,shy,currenty,cy);
  lx=FXMIN(anchorx,slx);
  ly=FXMIN(anchory,sly);
  hx=FXMAX(anchorx,shx);
  hy=FXMAX(anchory,shy);
  currentx=cx;
  currenty=cy;
  update(pos_x+lx,pos_y+sly-1,hx-lx,shy-sly+2);
#ifdef _WIN32
  repaint(pos_x+lx,pos_y+sly-1,hx-lx,shy-sly+2);
#endif
  update(pos_x+slx-1,pos_y+ly,shx-slx+2,hy-ly);
#ifdef _WIN32
  repaint(pos_x+slx-1,pos_y+ly,shx-slx+2,hy-ly);
#endif
  }


// End lasso operation
void GMAlbumList::endLasso(){
  FXint lx,ly,hx,hy;
  FXMINMAX(lx,hx,anchorx,currentx);
  FXMINMAX(ly,hy,anchory,currenty);
  update(pos_x+lx,pos_y+ly,hx-lx,hy-ly);
  flags&=~FLAG_LASSO;
  }



// Arrange by rows
long GMAlbumList::onCmdShowYear(FXObject*,FXSelector,void*){
  if (options&ALBUMLIST_YEAR)
    options&=~ALBUMLIST_YEAR;
  else
    options|=ALBUMLIST_YEAR;
  recalc();
  return 1;
  }

// Update sender
long GMAlbumList::onUpdShowYear(FXObject* sender,FXSelector,void*){
  sender->handle(this,(options&ALBUMLIST_YEAR)?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  sender->handle(this,(options&ALBUMLIST_BROWSER)?FXSEL(SEL_COMMAND,ID_HIDE):FXSEL(SEL_COMMAND,ID_SHOW),nullptr);
  return 1;
  }

// Arrange by rows
long GMAlbumList::onCmdArrangeByRows(FXObject*,FXSelector,void*){
  options&=~ALBUMLIST_COLUMNS;
  recalc();
  return 1;
  }


// Update sender
long GMAlbumList::onUpdArrangeByRows(FXObject* sender,FXSelector,void*){
  sender->handle(this,(options&ALBUMLIST_COLUMNS)?FXSEL(SEL_COMMAND,ID_UNCHECK):FXSEL(SEL_COMMAND,ID_CHECK),nullptr);
  sender->handle(this,(options&ALBUMLIST_BROWSER)?FXSEL(SEL_COMMAND,ID_SHOW):FXSEL(SEL_COMMAND,ID_HIDE),nullptr);
  return 1;
  }


// Arrange by columns
long GMAlbumList::onCmdArrangeByColumns(FXObject*,FXSelector,void*){
  options|=ALBUMLIST_COLUMNS;
  recalc();
  return 1;
  }


// Update sender
long GMAlbumList::onUpdArrangeByColumns(FXObject* sender,FXSelector,void*){
  sender->handle(this,(options&ALBUMLIST_COLUMNS)?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  sender->handle(this,(options&ALBUMLIST_BROWSER)?FXSEL(SEL_COMMAND,ID_SHOW):FXSEL(SEL_COMMAND,ID_HIDE),nullptr);
  return 1;
  }

// Select all items
long GMAlbumList::onCmdSelectAll(FXObject*,FXSelector,void*){
  for(int i=0; i<items.no(); i++) selectItem(i,true);
  return 1;
  }


// Deselect all items
long GMAlbumList::onCmdDeselectAll(FXObject*,FXSelector,void*){
  for(int i=0; i<items.no(); i++) deselectItem(i,true);
  return 1;
  }


// Select inverse of current selection
long GMAlbumList::onCmdSelectInverse(FXObject*,FXSelector,void*){
  for(int i=0; i<items.no(); i++) toggleItem(i,true);
  return 1;
  }



// Sort the items based on the sort function
void GMAlbumList::sortItems(){
  GMAlbumListItem *v,*c=0;
  FXbool exch=false;
  FXint i,j,h;
  if(sortfunc){
    if(0<=current){
      c=items[current];
      }
    for(h=1; h<=items.no()/9; h=3*h+1){}
    for(; h>0; h/=3){
      for(i=h+1;i<=items.no();i++){
        v=items[i-1];
        j=i;
        while(j>h && sortfunc(items[j-h-1],v)>0){
          items[j-1]=items[j-h-1];
          exch=true;
          j-=h;
          }
        items[j-1]=v;
        }
      }
    if(0<=current){
      for(i=0; i<items.no(); i++){
        if(items[i]==c){ current=i; break; }
        }
      }
    if(exch) recalc();
    }
  }


// Set current item
void GMAlbumList::setCurrentItem(FXint index,FXbool notify){
  if(index<-1 || items.no()<=index){ fxerror("%s::setCurrentItem: index out of range.\n",getClassName()); }
  if(index!=current){

    // Deactivate old item
    if(0<=current){

      // No visible change if it doen't have the focus
      if(hasFocus()){
        items[current]->setFocus(false);
        updateItem(current);
        }
      }

    current=index;

    // Activate new item
    if(0<=current){

      // No visible change if it doen't have the focus
      if(hasFocus()){
        items[current]->setFocus(true);
        updateItem(current);
        }
      }

    // Notify item change
    if(notify && target){target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);}
    }

  // In browse selection mode, select item
  if((options&SELECT_MASK)==ALBUMLIST_BROWSESELECT && 0<=current){
    selectItem(current,notify);
    }
  }


// Set anchor item
void GMAlbumList::setAnchorItem(FXint index){
  if(index<-1 || items.no()<=index){ fxerror("%s::setAnchorItem: index out of range.\n",getClassName()); }
  anchor=index;
  extent=index;
  }


// Zero out lookup string
long GMAlbumList::onLookupTimer(FXObject*,FXSelector,void*){
  lookup=FXString::null;
  return 1;
  }


// Key Press
long GMAlbumList::onKeyPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint index=current;
  flags&=~FLAG_TIP;
  if(!isEnabled()) return 0;
  if(target && target->tryHandle(this,FXSEL(SEL_KEYPRESS,message),ptr)) return 1;
  switch(event->code){
    case KEY_Control_L:
    case KEY_Control_R:
    case KEY_Shift_L:
    case KEY_Shift_R:
    case KEY_Alt_L:
    case KEY_Alt_R:
      if(flags&FLAG_DODRAG){handle(this,FXSEL(SEL_DRAGGED,0),ptr);}
      return 1;
    case KEY_Page_Up:
    case KEY_KP_Page_Up:
      lookup=FXString::null;
      setPosition(pos_x,pos_y+verticalScrollBar()->getPage());
      return 1;
    case KEY_Page_Down:
    case KEY_KP_Page_Down:
      lookup=FXString::null;
      setPosition(pos_x,pos_y-verticalScrollBar()->getPage());
      return 1;
    case KEY_Right:
    case KEY_KP_Right:
      if(!(options&ALBUMLIST_BROWSER)){
        setPosition(pos_x-10,pos_y);
        return 1;
        }
      if(options&ALBUMLIST_COLUMNS) index+=1; else index+=nrows;
      goto hop;
    case KEY_Left:
    case KEY_KP_Left:
      if(!(options&ALBUMLIST_BROWSER)){
        setPosition(pos_x+10,pos_y);
        return 1;
        }
      if(options&ALBUMLIST_COLUMNS) index-=1; else index-=nrows;
      goto hop;
    case KEY_Up:
    case KEY_KP_Up:
      if(options&ALBUMLIST_COLUMNS) index-=ncols; else index-=1;
      goto hop;
    case KEY_Down:
    case KEY_KP_Down:
      if(options&ALBUMLIST_COLUMNS) index+=ncols; else index+=1;
      goto hop;
    case KEY_Home:
    case KEY_KP_Home:
      index=0;
      goto hop;
    case KEY_End:
    case KEY_KP_End:
      index=items.no()-1;
hop:  lookup=FXString::null;
      if(0<=index && index<items.no()){
        setCurrentItem(index,true);
        makeItemVisible(index);
        if((options&SELECT_MASK)==ALBUMLIST_EXTENDEDSELECT){
          if(event->state&SHIFTMASK){
            if(0<=anchor){
              selectItem(anchor,true);
              extendSelection(index,true);
              }
            else{
              selectItem(index,true);
              }
            }
          else if(!(event->state&CONTROLMASK)){
            killSelection(true);
            selectItem(index,true);
            setAnchorItem(index);
            }
          }
        }
      handle(this,FXSEL(SEL_CLICKED,0),(void*)(FXival)current);
      if(0<=current){
        handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXival)current);
        }
      return 1;
    case KEY_space:
    case KEY_KP_Space:
      lookup=FXString::null;
      if(0<=current){
        switch(options&SELECT_MASK){
          case ALBUMLIST_EXTENDEDSELECT:
            if(event->state&SHIFTMASK){
              if(0<=anchor){
                selectItem(anchor,true);
                extendSelection(current,true);
                }
              else{
                selectItem(current,true);
                }
              }
            else if(event->state&CONTROLMASK){
              toggleItem(current,true);
              }
            else{
              killSelection(true);
              selectItem(current,true);
              }
            break;
          case ALBUMLIST_MULTIPLESELECT:
          case ALBUMLIST_SINGLESELECT:
            toggleItem(current,true);
            break;
          }
        setAnchorItem(current);
        }
      handle(this,FXSEL(SEL_CLICKED,0),(void*)(FXival)current);
      if(0<=current){
        handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXival)current);
        }
      return 1;
    case KEY_Return:
    case KEY_KP_Enter:
      lookup=FXString::null;
      handle(this,FXSEL(SEL_DOUBLECLICKED,0),(void*)(FXival)current);
      if(0<=current){
        handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXival)current);
        }
      return 1;
    default:
      if((FXuchar)event->text[0]<' ') return 0;
      if(event->state&(CONTROLMASK|ALTMASK)) return 0;
      if(!Ascii::isPrint(event->text[0])) return 0;
#if 0
      lookup.append(event->text);
      getApp()->addTimeout(this,ID_LOOKUPTIMER,getApp()->getTypingSpeed());
      index=findItem(lookup,current,SEARCH_FORWARD|SEARCH_WRAP|SEARCH_PREFIX);
      if(0<=index){
	setCurrentItem(index,true);
	makeItemVisible(index);
	if(items[index]->isEnabled()){
	  if((options&SELECT_MASK)==ALBUMLIST_EXTENDEDSELECT){
	    killSelection(true);
	    selectItem(index,true);
	    }
	  setAnchorItem(index);
	  }
        }
#endif
      handle(this,FXSEL(SEL_CLICKED,0),(void*)(FXival)current);
      if(0<=current){
        handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXival)current);
        }
      return 1;
    }
  return 0;
  }


// Key Release
long GMAlbumList::onKeyRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(!isEnabled()) return 0;
  if(target && target->tryHandle(this,FXSEL(SEL_KEYRELEASE,message),ptr)) return 1;
  switch(event->code){
    case KEY_Shift_L:
    case KEY_Shift_R:
    case KEY_Control_L:
    case KEY_Control_R:
    case KEY_Alt_L:
    case KEY_Alt_R:
      if(flags&FLAG_DODRAG){handle(this,FXSEL(SEL_DRAGGED,0),ptr);}
      return 1;
    }
  return 0;
  }


// Autoscrolling timer
long GMAlbumList::onAutoScroll(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint olx,orx,oty,oby,nlx,nrx,nty,nby;

  // Scroll the content
  FXScrollArea::onAutoScroll(sender,sel,ptr);

  // Lasso mode
  if(flags&FLAG_LASSO){

    // Select items in lasso
    FXMINMAX(olx,orx,anchorx,currentx);
    FXMINMAX(oty,oby,anchory,currenty);
    updateLasso(event->win_x-pos_x,event->win_y-pos_y);
    FXMINMAX(nlx,nrx,anchorx,currentx);
    FXMINMAX(nty,nby,anchory,currenty);
    lassoChanged(pos_x+olx,pos_y+oty,orx-olx+1,oby-oty+1,pos_x+nlx,pos_y+nty,nrx-nlx+1,nby-nty+1,true);
    return 1;
    }

  // Content scrolled, so perhaps something else under cursor
  if(flags&FLAG_DODRAG){
    handle(this,FXSEL(SEL_DRAGGED,0),ptr);
    return 1;
    }

  return 0;
  }


// Mouse moved
long GMAlbumList::onMotion(FXObject*,FXSelector,void* ptr){
  FXint olx,orx,oty,oby,nlx,nrx,nty,nby;
  FXEvent* event=(FXEvent*)ptr;
  FXint oldcursor=cursor;
  FXuint flg=flags;

  // Kill the tip
  flags&=~FLAG_TIP;

  // Kill the tip timer
  getApp()->removeTimeout(this,ID_TIPTIMER);

  // Right mouse scrolling
  if(flags&FLAG_SCROLLING){
    setPosition(event->win_x-grabx,event->win_y-graby);
    return 1;
    }

  // Lasso selection mode
  if(flags&FLAG_LASSO){
    if(startAutoScroll(event,false)) return 1;

    // Select items in lasso
    FXMINMAX(olx,orx,anchorx,currentx);
    FXMINMAX(oty,oby,anchory,currenty);
    updateLasso(event->win_x-pos_x,event->win_y-pos_y);
    FXMINMAX(nlx,nrx,anchorx,currentx);
    FXMINMAX(nty,nby,anchory,currenty);
    lassoChanged(pos_x+olx,pos_y+oty,orx-olx+1,oby-oty+1,pos_x+nlx,pos_y+nty,nrx-nlx+1,nby-nty+1,true);
    return 1;
    }

  // Drag and drop mode
  if(flags&FLAG_DODRAG){
    if(startAutoScroll(event,true)) return 1;
    handle(this,FXSEL(SEL_DRAGGED,0),ptr);
    return 1;
    }

  // Tentative drag and drop
  if(flags&FLAG_TRYDRAG){
    if(event->moved){
      flags&=~FLAG_TRYDRAG;
      if(handle(this,FXSEL(SEL_BEGINDRAG,0),ptr)){
        flags|=FLAG_DODRAG;
        }
      }
    return 1;
    }

  // Reset tip timer if nothing's going on
  getApp()->addTimeout(this,ID_TIPTIMER,getApp()->getMenuPause());

  // Get item we're over
  cursor=getItemAt(event->win_x,event->win_y);

  // Force GUI update only when needed
  return (cursor!=oldcursor)||(flg&FLAG_TIP);
  }


// Pressed a button
long GMAlbumList::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXint index;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    flags&=~FLAG_UPDATE;

    // First change callback
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;

    // Locate item
    index=getItemAt(event->win_x,event->win_y);

    // No item
    if(index<0){

      // Start lasso
      if((options&SELECT_MASK)==ALBUMLIST_EXTENDEDSELECT){

        // Kill selection
        if(!(event->state&(SHIFTMASK|CONTROLMASK))){
          killSelection(true);
          }

        // Start lasso
        startLasso(event->win_x-pos_x,event->win_y-pos_y);
        }
      return 1;
      }

    // Previous selection state
    state=items[index]->isSelected();

    // Change current item
    setCurrentItem(index,true);

    // Change item selection
    switch(options&SELECT_MASK){
      case ALBUMLIST_EXTENDEDSELECT:
        if(event->state&SHIFTMASK){
          if(0<=anchor){
            selectItem(anchor,true);
            extendSelection(index,true);
            }
          else{
           selectItem(index,true);
            setAnchorItem(index);
            }
          }
        else if(event->state&CONTROLMASK){
          if(!state) selectItem(index,true);
          setAnchorItem(index);
          }
        else{
          if(!state){ killSelection(true); selectItem(index,true); }
          setAnchorItem(index);
          }
        break;
      case ALBUMLIST_MULTIPLESELECT:
      case ALBUMLIST_SINGLESELECT:
        if(!state) selectItem(index,true);
        break;
      }

    // Are we dragging?
    if(state && items[index]->isSelected() && items[index]->isDraggable()){
      flags|=FLAG_TRYDRAG;
      }

    flags|=FLAG_PRESSED;
    return 1;
    }
  return 0;
  }


// Released button
long GMAlbumList::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXuint flg=flags;
  if(isEnabled()){
    ungrab();
    stopAutoScroll();
    flags|=FLAG_UPDATE;
    flags&=~(FLAG_PRESSED|FLAG_TRYDRAG|FLAG_LASSO|FLAG_DODRAG);

    // First chance callback
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;

    // Was lassoing
    if(flg&FLAG_LASSO){
      endLasso();
      return 1;
      }

    // Was dragging
    if(flg&FLAG_DODRAG){
      handle(this,FXSEL(SEL_ENDDRAG,0),ptr);
      return 1;
      }

    // Must have pressed
    if(flg&FLAG_PRESSED){

      // Selection change
      switch(options&SELECT_MASK){
        case ALBUMLIST_EXTENDEDSELECT:
          if(0<=current){
            if(event->state&CONTROLMASK){
              if(state) deselectItem(current,true);
              }
            else if(!(event->state&SHIFTMASK)){
              if(state){ killSelection(true); selectItem(current,true); }
              }
            }
          break;
        case ALBUMLIST_MULTIPLESELECT:
        case ALBUMLIST_SINGLESELECT:
          if(0<=current){
            if(state) deselectItem(current,true);
            }
          break;
        }

      // Scroll to make item visibke
      makeItemVisible(current);

      // Update anchor
      setAnchorItem(current);

      // Generate clicked callbacks
      if(event->click_count==1){
        handle(this,FXSEL(SEL_CLICKED,0),(void*)(FXival)current);
        }
      else if(event->click_count==2){
        handle(this,FXSEL(SEL_DOUBLECLICKED,0),(void*)(FXival)current);
        }
      else if(event->click_count==3){
        handle(this,FXSEL(SEL_TRIPLECLICKED,0),(void*)(FXival)current);
        }

      // Command callback only when clicked on item
      if(0<=current){
        handle(this,FXSEL(SEL_COMMAND,0),(void*)(FXival)current);
        }
      }
    return 1;
    }
  return 0;
  }


// Pressed right button
long GMAlbumList::onRightBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  flags&=~FLAG_TIP;
  handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
  if(isEnabled()){
    grab();
    flags&=~FLAG_UPDATE;
    if(target && target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONPRESS,message),ptr)) return 1;
    flags|=FLAG_SCROLLING;
    grabx=event->win_x-pos_x;
    graby=event->win_y-pos_y;
    return 1;
    }
  return 0;
  }


// Released right button
long GMAlbumList::onRightBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    flags&=~FLAG_SCROLLING;
    flags|=FLAG_UPDATE;
    if(target) target->tryHandle(this,FXSEL(SEL_RIGHTBUTTONRELEASE,message),ptr);
    return 1;
    }
  return 0;
  }


// The widget lost the grab for some reason
long GMAlbumList::onUngrabbed(FXObject* sender,FXSelector sel,void* ptr){
  FXScrollArea::onUngrabbed(sender,sel,ptr);
  flags&=~(FLAG_DODRAG|FLAG_LASSO|FLAG_TRYDRAG|FLAG_PRESSED|FLAG_CHANGED|FLAG_SCROLLING);
  flags|=FLAG_UPDATE;
  stopAutoScroll();
  return 1;
  }


// Command message
long GMAlbumList::onCommand(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_COMMAND,message),ptr);
  }


// Clicked in list
long GMAlbumList::onClicked(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_CLICKED,message),ptr);
  }


// Double Clicked in list; ptr may or may not point to an item
long GMAlbumList::onDoubleClicked(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_DOUBLECLICKED,message),ptr);
  }


// Triple Clicked in list; ptr may or may not point to an item
long GMAlbumList::onTripleClicked(FXObject*,FXSelector,void* ptr){
  return target && target->tryHandle(this,FXSEL(SEL_TRIPLECLICKED,message),ptr);
  }

// Retrieve item
GMAlbumListItem *GMAlbumList::getItem(FXint index) const {
  if(index<0 || items.no()<=index){ fxerror("%s::getItem: index out of range.\n",getClassName()); }
  return items[index];
  }


// Replace item with another
FXint GMAlbumList::setItem(FXint index,GMAlbumListItem* item,FXbool notify){

  // Must have item
  if(!item){ fxerror("%s::setItem: item is nullptr.\n",getClassName()); }

  // Must be in range
  if(index<0 || items.no()<=index){ fxerror("%s::setItem: index out of range.\n",getClassName()); }

  // Notify item will be replaced
  if(notify && target){target->tryHandle(this,FXSEL(SEL_REPLACED,message),(void*)(FXival)index);}

  // Copy the state over
  item->state=items[index]->state;

  // Delete old
  delete items[index];

  // Add new
  items[index]=item;

  // Redo layout
  recalc();
  return index;
  }



// Insert item
FXint GMAlbumList::insertItem(FXint index,GMAlbumListItem* item,FXbool notify){
  FXint old=current;

  // Must have item
  if(!item){ fxerror("%s::insertItem: item is nullptr.\n",getClassName()); }

  // Must be in range
  if(index<0 || items.no()<index){ fxerror("%s::insertItem: index out of range.\n",getClassName()); }

  // Add item to list
  items.insert(index,item);

  // Adjust indices
  if(anchor>=index)  anchor++;
  if(extent>=index)  extent++;
  if(current>=index) current++;
  if(viewable>=index) viewable++;
  if(current<0 && items.no()==1) current=0;

  // Notify item has been inserted
  if(notify && target){target->tryHandle(this,FXSEL(SEL_INSERTED,message),(void*)(FXival)index);}

  // Current item may have changed
  if(old!=current){
    if(notify && target){target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);}
    }

  // Was new item
  if(0<=current && current==index){
    if(hasFocus()){
      items[current]->setFocus(true);
      }
    if((options&SELECT_MASK)==ALBUMLIST_BROWSESELECT){
      selectItem(current,notify);
      }
    }

  // Redo layout
  recalc();
  return index;
  }


// Append item
FXint GMAlbumList::appendItem(GMAlbumListItem* item,FXbool notify){
  return insertItem(items.no(),item,notify);
  }


// Prepend item
FXint GMAlbumList::prependItem(GMAlbumListItem* item,FXbool notify){
  return insertItem(0,item,notify);
  }


// Move item from oldindex to newindex
FXint GMAlbumList::moveItem(FXint newindex,FXint oldindex,FXbool notify){
  FXint old=current;
  GMAlbumListItem *item;

  // Must be in range
  if(newindex<0 || oldindex<0 || items.no()<=newindex || items.no()<=oldindex){ fxerror("%s::moveItem: index out of range.\n",getClassName()); }

  // Did it change?
  if(oldindex!=newindex){

    // Move item
    item=items[oldindex];
    items.erase(oldindex);
    items.insert(newindex,item);

    // Move item down
    if(newindex<oldindex){
      if(newindex<=anchor && anchor<oldindex) anchor++;
      if(newindex<=extent && extent<oldindex) extent++;
      if(newindex<=current && current<oldindex) current++;
      if(newindex<=viewable && viewable<oldindex) viewable++;
      }

    // Move item up
    else{
      if(oldindex<anchor && anchor<=newindex) anchor--;
      if(oldindex<extent && extent<=newindex) extent--;
      if(oldindex<current && current<=newindex) current--;
      if(oldindex<viewable && viewable<=newindex) viewable--;
      }

    // Adjust if it was equal
    if(anchor==oldindex) anchor=newindex;
    if(extent==oldindex) extent=newindex;
    if(current==oldindex) current=newindex;
    if(viewable==oldindex) viewable=newindex;

    // Current item may have changed
    if(old!=current){
      if(notify && target){target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);}
      }

    // Redo layout
    recalc();
    }
  return newindex;
  }


// Extract node from list
GMAlbumListItem* GMAlbumList::extractItem(FXint index,FXbool notify){
  GMAlbumListItem *result;
  FXint old=current;

  // Must be in range
  if(index<0 || items.no()<=index){ fxerror("%s::extractItem: index out of range.\n",getClassName()); }

  // Notify item will be deleted
  if(notify && target){target->tryHandle(this,FXSEL(SEL_DELETED,message),(void*)(FXival)index);}

  // Extract item
  result=items[index];

  // Remove from list
  items.erase(index);

  // Adjust indices
  if(anchor>index || anchor>=items.no())  anchor--;
  if(extent>index || extent>=items.no())  extent--;
  if(current>index || current>=items.no()) current--;
  if(viewable>index || viewable>=items.no())  viewable--;

  // Current item has changed
  if(index<=old){
    if(notify && target){target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);}
    }

  // Deleted current item
  if(0<=current && index==old){
    if(hasFocus()){
      items[current]->setFocus(true);
      }
    if((options&SELECT_MASK)==ALBUMLIST_BROWSESELECT){
      selectItem(current,notify);
      }
    }

  // Redo layout
  recalc();

  // Return item
  return result;
  }


// Remove node from list
void GMAlbumList::removeItem(FXint index,FXbool notify){
  FXint old=current;

  // Must be in range
  if(index<0 || items.no()<=index){ fxerror("%s::removeItem: index out of range.\n",getClassName()); }

  // Notify item will be deleted
  if(notify && target){target->tryHandle(this,FXSEL(SEL_DELETED,message),(void*)(FXival)index);}

  // Delete item
  delete items[index];

  // Remove from list
  items.erase(index);

  // Adjust indices
  if(anchor>index || anchor>=items.no())  anchor--;
  if(extent>index || extent>=items.no())  extent--;
  if(current>index || current>=items.no()) current--;
  if(viewable>index || viewable>=items.no())  viewable--;

  // Current item has changed
  if(index<=old){
    if(notify && target){target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)current);}
    }

  // Deleted current item
  if(0<=current && index==old){
    if(hasFocus()){
      items[current]->setFocus(true);
      }
    if((options&SELECT_MASK)==ALBUMLIST_BROWSESELECT){
      selectItem(current,notify);
      }
    }

  // Redo layout
  recalc();
  }


// Remove all items
void GMAlbumList::clearItems(FXbool notify){
  FXint old=current;

  // Delete items
  for(FXint index=items.no()-1; 0<=index; index--){
    if(notify && target){target->tryHandle(this,FXSEL(SEL_DELETED,message),(void*)(FXival)index);}
    delete items[index];
    }

  // Free array
  items.clear();

  // Adjust indices
  current=-1;
  anchor=-1;
  extent=-1;
  viewable=-1;

  // Current item has changed
  if(old!=-1){
    if(notify && target){target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)(FXival)-1);}
    }

  // Redo layout
  recalc();
  }

// Get item by data
FXint GMAlbumList::findItemById(const FXint needle,FXint start,FXuint flgs) const {
  FXint index;
  if(0<items.no()){
    if(flgs&SEARCH_BACKWARD){
      if(start<0) start=items.no()-1;
      for(index=start; 0<=index; index--){
        if(items[index]->getId()==needle) return index;
        }
      if(!(flgs&SEARCH_WRAP)) return -1;
      for(index=items.no()-1; start<index; index--){
        if(items[index]->getId()==needle) return index;
        }
      }
    else{
      if(start<0) start=0;
      for(index=start; index<items.no(); index++){
        if(items[index]->getId()==needle) return index;
        }
      if(!(flgs&SEARCH_WRAP)) return -1;
      for(index=0; index<start; index++){
        if(items[index]->getId()==needle) return index;
        }
      }
    }
  return -1;
  }



void GMAlbumList::setListIcon(FXIcon*ico) {
  if(listicon!=ico){
    listicon=ico;
    recalc();
    update();
    }
  }

void GMAlbumList::setListBaseFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setFont: nullptr font specified.\n",getClassName()); }
  if(listbasefont!=fnt){
    listbasefont=fnt;
    recalc();
    update();
    }
  }

void GMAlbumList::setListHeadFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setFont: nullptr font specified.\n",getClassName()); }
  if(listheadfont!=fnt){
    listheadfont=fnt;
    recalc();
    update();
    }
  }

void GMAlbumList::setListTailFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setFont: nullptr font specified.\n",getClassName()); }
  if(listtailfont!=fnt){
    listtailfont=fnt;
    recalc();
    update();
    }
  }


void GMAlbumList::setCoverBaseFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setFont: nullptr font specified.\n",getClassName()); }
  if(coverbasefont!=fnt){
    coverbasefont=fnt;
    recalc();
    update();
    }
  }

void GMAlbumList::setCoverHeadFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setFont: nullptr font specified.\n",getClassName()); }
  if(coverheadfont!=fnt){
    coverheadfont=fnt;
    recalc();
    update();
    }
  }



// Set text color
void GMAlbumList::setTextColor(FXColor clr){
  if(clr!=textColor){
    textColor=clr;
    update();
    }
  }


// Set select background color
void GMAlbumList::setAltBackColor(FXColor clr){
  if(clr!=selbackColor){
    altbackColor=clr;
    update();
    }
  }

// Set select background color
void GMAlbumList::setSelBackColor(FXColor clr){
  if(clr!=selbackColor){
    selbackColor=clr;
    update();
    }
  }

// Set selected text color
void GMAlbumList::setSelTextColor(FXColor clr){
  if(clr!=seltextColor){
    seltextColor=clr;
    update();
    }
  }



// Change list style
void GMAlbumList::setListStyle(FXuint style){
  FXuint opts=(options&~ALBUMLIST_MASK) | (style&ALBUMLIST_MASK);
  if(options!=opts){
    options=opts;
    recalc();
    }
  }


// Get list style
FXuint GMAlbumList::getListStyle() const {
  return (options&ALBUMLIST_MASK);
  }


// Change help text
void GMAlbumList::setHelpText(const FXString& text){
  help=text;
  }


// Save data
void GMAlbumList::save(FXStream& store) const {
  FXScrollArea::save(store);
  items.save(store);
  store << nrows;
  store << ncols;
  store << anchor;
  store << current;
  store << extent;
  store << textColor;
  store << selbackColor;
  store << seltextColor;
  store << itemWidth;
  store << itemHeight;
  store << help;
  }


// Load data
void GMAlbumList::load(FXStream& store){
  FXScrollArea::load(store);
  items.load(store);
  store >> nrows;
  store >> ncols;
  store >> anchor;
  store >> current;
  store >> extent;
  store >> textColor;
  store >> selbackColor;
  store >> seltextColor;
  store >> itemWidth;
  store >> itemHeight;
  store >> help;
  }


// Cleanup
GMAlbumList::~GMAlbumList(){
  getApp()->removeTimeout(this,ID_TIPTIMER);
  getApp()->removeTimeout(this,ID_LOOKUPTIMER);
  clearItems(false);
  }

