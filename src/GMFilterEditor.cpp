/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2015-2015 by Sander Jansen. All Rights Reserved      *
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
#include "gmdefs.h"
#include "GMIconTheme.h"
#include "GMFilter.h"
#include "GMFilterEditor.h"

#define add_column(str,column) columns->appendItem(str,(void*)(FXival)column)
#define add_period(str,period) periods->appendItem(str,(void*)(FXival)(period))
#define add_limit_column(str,column) limitcolumn->appendItem(str,NULL,(void*)(FXival)(column))
#define add_operator(str,opcode) operators->appendItem(str,(void*)(FXival)opcode)

#define PERIOD_MINUTES (60)
#define PERIOD_HOURS (60*60)
#define PERIOD_DAYS (60*60*24)
#define PERIOD_WEEKS (60*60*24*7)

static const FXint column_types[]={
  Rule::ColumnTitle,
  Rule::ColumnArtist,
  Rule::ColumnAlbumArtist,
  Rule::ColumnComposer,
  Rule::ColumnConductor,
  Rule::ColumnAlbum,
  Rule::ColumnYear,
  Rule::ColumnPlaycount,
  Rule::ColumnPlaydate,
  Rule::ColumnImportdate
  };

static const FXchar * const column_names[]={
  "Title",
  "Artist",
  "Album Artist",
  "Composer",
  "Conductor",
  "Album",
  "Year",
  "Play Count",
  "Last Played",
  "Last Updated"
  };

enum {
  InputText    = 1,
  InputYear    = 2,
  InputDate    = 3,
  InputInteger = 4,
  };

static void fillColumns(GMComboBox * combobox) {
  FXASSERT(ARRAYNUMBER(column_types)==ARRAYNUMBER(column_names));
  for (FXuint i=0;i<ARRAYNUMBER(column_types);i++){
    combobox->appendItem(column_names[i],(void*)(FXival)column_types[i]);
    }
  combobox->setNumVisible(FXMIN(9,combobox->getNumItems()));
  }


FXDEFMAP(GMRuleEditor) GMRuleEditorMap[]={
  FXMAPFUNC(SEL_COMMAND,GMRuleEditor::ID_COLUMN,GMRuleEditor::onCmdColumn),
  FXMAPFUNC(SEL_COMMAND,GMRuleEditor::ID_OPERATOR,GMRuleEditor::onCmdText),
  FXMAPFUNC(SEL_COMMAND,GMRuleEditor::ID_DELETE,GMRuleEditor::onCmdDelete),
  FXMAPFUNC(SEL_COMMAND,GMRuleEditor::ID_TEXT,GMRuleEditor::onCmdText),
  FXMAPFUNC(SEL_TIMEOUT,GMRuleEditor::ID_TEXT,GMRuleEditor::onCmdText),
  FXMAPFUNC(SEL_CHANGED,GMRuleEditor::ID_TEXT,GMRuleEditor::onCmdText),
  };

FXIMPLEMENT(GMRuleEditor,FXObject,GMRuleEditorMap,ARRAYNUMBER(GMRuleEditorMap))


GMRuleEditor::GMRuleEditor(FXMatrix * parent,FXWindow * before,const Rule & rule) {
  create(parent,before);
  setRule(rule);
  }


GMRuleEditor::GMRuleEditor(FXMatrix * parent,FXWindow * before) {
  create(parent,before);
  }


GMRuleEditor::~GMRuleEditor(){
  delete valueswitcher;
  delete operators;
  delete columns;
  delete closebutton;
  }


void GMRuleEditor::clearTimeout() {
  text->getApp()->removeTimeout(this,ID_TEXT);
  }


void GMRuleEditor::create(FXMatrix * rules,FXWindow * lastrow) {

  // List of Columns
  columns = new GMComboBox(rules,10,this,ID_COLUMN,LAYOUT_FILL_X|COMBOBOX_STATIC);
  columns->setUserData(this);
  fillColumns(columns);

  // Available Operators
  operators = new GMComboBox(rules,12,this,ID_OPERATOR,LAYOUT_FILL_X|COMBOBOX_STATIC);
  operators->setNumVisible(FXMIN(9,operators->getNumItems()));

  // Switcher for each input type
  valueswitcher = new FXSwitcher(rules,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);

  // Delete button
  closebutton = new GMButton(rules,fxtr("\tRemove Rule\tRemove Rule"),GMIconTheme::instance()->icon_remove,this,ID_DELETE,BUTTON_TOOLBAR|FRAME_RAISED);

  // #0 Simple Text Input
  text = new GMTextField(valueswitcher,20,this,ID_TEXT,LAYOUT_FILL_X);

  // #1 Year Input / #3 Number
  FXHorizontalFrame * spinnerframe = new FXHorizontalFrame(valueswitcher,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
  spinner = new GMSpinner(spinnerframe,6,NULL,0);
  spinner->setRange(0,2020);
  spinner->setValue(2015);

  // #2 Date Input
  FXHorizontalFrame * dateframe = new FXHorizontalFrame(valueswitcher,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
  datetimespinner = new GMSpinner(dateframe,6,NULL,0);
  periods = new GMComboBox(dateframe,0,NULL,0,LAYOUT_FILL_X|COMBOBOX_STATIC);
  add_period("Minutes",PERIOD_MINUTES);
  add_period("Hours",PERIOD_HOURS);
  add_period("Days",PERIOD_DAYS);
  add_period("Weeks",PERIOD_WEEKS);
  periods->setNumVisible(FXMIN(9,periods->getNumItems()));

  // Put into correct position of the matrxi
  if (rules->id()) rules->create();
  columns->reparent(rules,lastrow);
  operators->reparent(rules,lastrow);
  valueswitcher->reparent(rules,lastrow);
  closebutton->reparent(rules,lastrow);

  // Initialize all fields
  updateColumn();
  }


void GMRuleEditor::setRule(const Rule & rule) {
  setColumn(rule.column);
  setOperator(rule.opcode);
  switch(rule.column) {
    case Rule::ColumnTitle  :
    case Rule::ColumnArtist :
    case Rule::ColumnAlbum  :
    case Rule::ColumnAlbumArtist  :
    case Rule::ColumnComposer  :
    case Rule::ColumnConductor  :
      text->setText(rule.text,true);
      break;
    case Rule::ColumnYear   :
      spinner->setValue(rule.value);
      break;
    case Rule::ColumnPlaydate :
    case Rule::ColumnImportdate :
      if (rule.value>=PERIOD_WEEKS) {
        setPeriodMultiplier(PERIOD_WEEKS);
        datetimespinner->setValue(rule.value/PERIOD_WEEKS);
        }
      else if (rule.value>=PERIOD_DAYS){
        setPeriodMultiplier(PERIOD_DAYS);
        datetimespinner->setValue(rule.value/PERIOD_DAYS);
        }
      else if (rule.value>=PERIOD_HOURS){
        setPeriodMultiplier(PERIOD_HOURS);
        datetimespinner->setValue(rule.value/PERIOD_HOURS);
        }
      else {
        setPeriodMultiplier(PERIOD_MINUTES);
        datetimespinner->setValue(rule.value/PERIOD_MINUTES);
        }
      break;
    }
  }


void GMRuleEditor::getRule(Rule & rule) {
  rule.column = getColumn();
  rule.opcode = getOperator();
  switch(getColumn()) {
    case Rule::ColumnTitle  :
    case Rule::ColumnArtist :
    case Rule::ColumnAlbum  :
    case Rule::ColumnAlbumArtist  :
    case Rule::ColumnComposer  :
    case Rule::ColumnConductor  :
      rule.text = text->getText();
      rule.value = 0;
      break;
    case Rule::ColumnYear   :
      rule.value = spinner->getValue();
      rule.text.clear();
      break;
    case Rule::ColumnPlaydate:
    case Rule::ColumnImportdate:
      rule.value = datetimespinner->getValue() * getPeriodMultiplier();
      rule.text.clear();
      break;
    default: FXASSERT(0); break;
    }
  }


void GMRuleEditor::setColumn(FXint column) {
  FXint item = columns->findItemByData((void*)(FXival)column);
  FXASSERT(item>=0);
  columns->setCurrentItem(item,true);
  }


FXint GMRuleEditor::getColumn() const {
  return (FXint)(FXival)columns->getItemData(columns->getCurrentItem());
  }


void GMRuleEditor::setOperator(FXint opcode) {
  FXint item = operators->findItemByData((void*)(FXival)opcode);
  FXASSERT(item>=0);
  operators->setCurrentItem(item);
  }


FXint GMRuleEditor::getOperator() const {
  return (FXint)(FXival)operators->getItemData(operators->getCurrentItem());
  }


void GMRuleEditor::setPeriodMultiplier(FXint value){
  FXint item = periods->findItemByData((void*)(FXival)value);
  periods->setCurrentItem(item,true);
  }


FXint GMRuleEditor::getPeriodMultiplier() const {
  return (FXint)(FXival)periods->getItemData(periods->getCurrentItem());
  }


void GMRuleEditor::setInputType(FXint type) {
  FXint current = (FXint)(FXival)operators->getUserData();
  if (current!=type) {
    switch(type) {
      case InputText:
        operators->clearItems();
        add_operator("contains",Rule::OperatorLike);
        add_operator("does not contain",Rule::OperatorNotLike);
        add_operator("equals",Rule::OperatorEquals);
        add_operator("not equal to",Rule::OperatorNotEqual);
        add_operator("starts with",Rule::OperatorPrefix);
        add_operator("ends with",Rule::OperatorSuffix);
        add_operator("matches",Rule::OperatorMatch);
        validateText();
        valueswitcher->setCurrent(0);
        break;
      case InputYear:
        operators->clearItems();
        add_operator("in",Rule::OperatorEquals);
        add_operator("not in",Rule::OperatorNotEqual);
        add_operator("before",Rule::OperatorLess);
        add_operator("after",Rule::OperatorGreater);
        valueswitcher->setCurrent(1);
        spinner->setRange(0,2100);
        break;
      case InputDate:
        operators->clearItems();
        add_operator("in the last",Rule::OperatorGreater);
        add_operator("not in the last",Rule::OperatorLess);
        valueswitcher->setCurrent(2);
        break;
      case InputInteger:
        add_operator("equals",Rule::OperatorEquals);
        add_operator("not equal to",Rule::OperatorNotEqual);
        add_operator("at least",Rule::OperatorGreater);
        add_operator("at most",Rule::OperatorLess);
        valueswitcher->setCurrent(1);
        spinner->setRange(0,9999);
        break;
      }
    operators->setUserData((void*)(FXival)type);
    operators->setNumVisible(FXMIN(9,operators->getNumItems()));
    }
  }


void GMRuleEditor::updateColumn() {
  switch(getColumn()) {
    case Rule::ColumnTitle        :
    case Rule::ColumnArtist       :
    case Rule::ColumnAlbum        :
    case Rule::ColumnAlbumArtist  :
    case Rule::ColumnComposer     :
    case Rule::ColumnConductor    :
      setInputType(InputText);
      break;
    case Rule::ColumnYear:
      setInputType(InputYear);
      break;
    case Rule::ColumnPlaydate:
    case Rule::ColumnImportdate:
      setInputType(InputDate);
      break;
    case Rule::ColumnPlaycount:
      setInputType(InputInteger);
      break;
    }
  }


long GMRuleEditor::onCmdColumn(FXObject*,FXSelector,void*) {
  updateColumn();
  return 1;
  }


long GMRuleEditor::onCmdDelete(FXObject*,FXSelector,void*) {
  delete this;
  return 1;
  }


FXbool GMRuleEditor::validateText(){
  if (getOperator()==Rule::OperatorMatch) {
    if (!text->getText().empty()){
      FXRex rex;
      FXRex::Error code = rex.parse(text->getText(),FXRex::Syntax);
      if (code) {
        text->setTextColor(FXRGB(255,0,0));
        text->setTipText(FXRex::getError(code));
        return false;
        }
      }
    }
  text->setTextColor(text->getApp()->getForeColor());
  text->setTipText(FXString::null);
  return true;
  }


long GMRuleEditor::onCmdText(FXObject*,FXSelector sel,void*) {
  if (getOperator()==Rule::OperatorMatch) {
    if (FXSELTYPE(sel)==SEL_CHANGED) {
      text->getApp()->addTimeout(this,ID_TEXT);
      return 1;
      }
    }
  validateText();
  return 1;
  }


FXDEFMAP(GMSortLimitEditor) GMSortLimitEditorMap[]={
  FXMAPFUNC(SEL_COMMAND,GMSortLimitEditor::ID_DELETE,GMSortLimitEditor::onCmdDelete),
  };

FXIMPLEMENT(GMSortLimitEditor,FXObject,GMSortLimitEditorMap,ARRAYNUMBER(GMSortLimitEditorMap))


GMSortLimitEditor::GMSortLimitEditor(FXMatrix * parent,FXWindow * before,const SortLimit & limit) {
  create(parent,before);
  setSortLimit(limit);
  }


GMSortLimitEditor::GMSortLimitEditor(FXMatrix * parent,FXWindow * before) {
  create(parent,before);
  }


GMSortLimitEditor::~GMSortLimitEditor(){
  delete order;
  delete columns;
  delete closebutton;
  delete filler1;
  delete filler2;
  }


void GMSortLimitEditor::setColumn(FXint column) {
  FXint item = columns->findItemByData((void*)(FXival)column);
  FXASSERT(item>=0);
  columns->setCurrentItem(item);
  }


FXint GMSortLimitEditor::getColumn() const {
  return (FXint)(FXival)columns->getItemData(columns->getCurrentItem());
  }


void GMSortLimitEditor::setOrder(FXbool ascending) {
  if (ascending)
    order->setCurrentItem(0);
  else
    order->setCurrentItem(1);
  }


FXbool GMSortLimitEditor::getOrder() const {
  return order->getCurrentItem()==0;
  }


void GMSortLimitEditor::setSortLimit(const SortLimit & limit) {
  setColumn(limit.column);
  if (limit.ascending)
    order->setCurrentItem(0);
  else
    order->setCurrentItem(1);
  }


void GMSortLimitEditor::getSortLimit(SortLimit & limit){
  limit.column = getColumn();
  limit.ascending = getOrder();
  }


void GMSortLimitEditor::create(FXMatrix * rules,FXWindow * lastrow) {
  // List of Columns
  columns = new GMComboBox(rules,12,this,ID_COLUMN,LAYOUT_FILL_X|COMBOBOX_STATIC);
  columns->setUserData(this);
  fillColumns(columns);

  order = new GMComboBox(rules,10,NULL,0,LAYOUT_FILL_X|COMBOBOX_STATIC);
  order->appendItem("Ascending");
  order->appendItem("Descending");
  order->setNumVisible(2);

  closebutton = new GMButton(rules,fxtr("\tRemove Column\tRemove Column"),GMIconTheme::instance()->icon_remove,this,ID_DELETE,BUTTON_TOOLBAR|FRAME_RAISED);

  filler1 = new FXFrame(rules,FRAME_NONE);
  filler2 = new FXFrame(rules,FRAME_NONE);

  if (rules->id()) rules->create();
  columns->reparent(rules,lastrow);
  order->reparent(rules,lastrow);
  closebutton->reparent(rules,lastrow);
  filler1->reparent(rules,lastrow);
  filler2->reparent(rules,lastrow);

  columns->setUserData(this);
  }


long GMSortLimitEditor::onCmdDelete(FXObject*,FXSelector,void*) {
  delete this;
  return 1;
  }


FXDEFMAP(GMFilterEditor) GMFilterEditorMap[]={
  FXMAPFUNC(SEL_UPDATE,FXDialogBox::ID_ACCEPT,GMFilterEditor::onUpdAccept),
  FXMAPFUNC(SEL_UPDATE,GMFilterEditor::ID_LIMIT,GMFilterEditor::onUpdLimit),
  FXMAPFUNC(SEL_UPDATE,GMFilterEditor::ID_MATCH,GMFilterEditor::onUpdMatch),
  FXMAPFUNC(SEL_COMMAND,GMFilterEditor::ID_ADD_RULE,GMFilterEditor::onCmdAddRule),
  FXMAPFUNC(SEL_COMMAND,GMFilterEditor::ID_ADD_LIMIT,GMFilterEditor::onCmdAddLimit),
  };

FXIMPLEMENT(GMFilterEditor,FXDialogBox,GMFilterEditorMap,ARRAYNUMBER(GMFilterEditorMap))


GMFilterEditor::GMFilterEditor(FXWindow *p,const GMFilter & query) : FXDialogBox(p,"Edit Filter",DECOR_BORDER|DECOR_TITLE|DECOR_STRETCHABLE,0,0,600,250,0,0,0,0,0,0) {

  FXWindow * filler;

  FXHorizontalFrame *closebox=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  new GMButton(closebox,tr("&Update"),NULL,this,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  new GMButton(closebox,tr("&Cancel"),NULL,this,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  new FXSeparator(this,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);

  FXVerticalFrame * main=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y);

  // Filter Name
  FXMatrix  * header = new FXMatrix(main,2,LAYOUT_FILL|MATRIX_BY_COLUMNS);
  new FXLabel(header,"Name:",NULL,LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  namefield = new GMTextField(header,30,NULL,0,LAYOUT_FILL_COLUMN);

  // Match All or Any
  new FXLabel(header,"Match:",NULL,LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  match = new GMComboBox(header,3,this,ID_MATCH,LAYOUT_FILL_COLUMN|COMBOBOX_STATIC);
  match->appendItem("All",(void*)(FXival)GMFilter::MatchAll);
  match->appendItem("Any",(void*)(FXival)GMFilter::MatchAny);
  match->setNumVisible(2);

  // Filter Rules
  new FXLabel(header,"Filter:",NULL,LAYOUT_RIGHT);
  rules = new FXMatrix(header,4,LAYOUT_FILL|MATRIX_BY_COLUMNS|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);

  filler = new GMComboBox(rules,12,NULL,0,LAYOUT_FILL_X|COMBOBOX_STATIC);
  filler->disable();
  rules_offset = filler;

  filler = new GMComboBox(rules,12,NULL,0,LAYOUT_FILL_X|COMBOBOX_STATIC);
  filler->disable();
  filler = new GMTextField(rules,20,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN);
  filler->disable();
  new GMButton(rules,fxtr("\tAdd Filter\tAdd Filter"),GMIconTheme::instance()->icon_add,this,ID_ADD_RULE,BUTTON_TOOLBAR|FRAME_RAISED);

  // Limits
  new FXLabel(header,"Limit:",NULL,LAYOUT_RIGHT);
  limits = new FXMatrix(header,5,LAYOUT_FILL|MATRIX_BY_COLUMNS|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
  limitspinner = new GMSpinner(limits,6,this,ID_LIMIT,SPIN_NOMAX);
  limitspinner->setRange(0,0);
  new FXLabel(limits,"songs sorted by:",NULL,LAYOUT_CENTER_Y);
  limitspinner->disable();
  filler = new GMComboBox(limits,12,NULL,0,LAYOUT_FILL_X|COMBOBOX_STATIC);
  filler->disable();
  limits_offset = filler;
  filler = new GMComboBox(limits,10,NULL,0,LAYOUT_FILL_X|COMBOBOX_STATIC);
  filler->disable();
  new GMButton(limits,fxtr("\tAdd Column\tAdd Column"),GMIconTheme::instance()->icon_add,this,ID_ADD_LIMIT,BUTTON_TOOLBAR|FRAME_RAISED);

  setFilter(query);
  }

GMFilterEditor::~GMFilterEditor(){
  for (FXint i=0;i<getNumRules();i++){
    getRuleEditor(i)->clearTimeout();
    }
  }


void GMFilterEditor::show(FXuint placement){
  resize(getWidth(),getDefaultHeight());
  FXDialogBox::show(placement);
  }


void GMFilterEditor::setMatch(FXint matchrule) {
  if (matchrule==GMFilter::MatchAll)
    match->setCurrentItem(0);
  else
    match->setCurrentItem(1);
  }


FXint GMFilterEditor::getMatch() const {
  return (FXint)(FXival)match->getItemData(match->getCurrentItem());
  }


FXint GMFilterEditor::getNumRules() const {
  return (rules->numChildren() / 4) - 1;
  }


FXint GMFilterEditor::getNumSortLimits() const {
  return (limits->numChildren() / 5) - 1;
  }


GMRuleEditor * GMFilterEditor::getRuleEditor(FXint index) const {
  return static_cast<GMRuleEditor*>(rules->childAtIndex((index*4))->getUserData());
  }


GMSortLimitEditor * GMFilterEditor::getSortLimitEditor(FXint index) const {
  return static_cast<GMSortLimitEditor*>(limits->childAtIndex((index*5)+2)->getUserData());
  }


void GMFilterEditor::setFilter(const GMFilter & query) {
  namefield->setText(query.name);

  for (FXint i=0;i<query.rules.no();i++) {
    new GMRuleEditor(rules,rules_offset,query.rules[i]);
    }
  for (FXint i=0;i<query.order.no();i++) {
    new GMSortLimitEditor(limits,limits_offset,query.order[i]);
    }

  if (query.limit>0)
    limitspinner->setRange(1,99999);
  limitspinner->setValue(query.limit);

  setMatch(query.match);
  }


void GMFilterEditor::getFilter(GMFilter & query) const {
  query.name  = namefield->getText();
  query.limit = limitspinner->getValue();

  query.rules.no(getNumRules());
  for (FXint i=0;i<getNumRules();i++) {
    getRuleEditor(i)->getRule(query.rules[i]);
    }
  query.order.no(getNumSortLimits());
  for (FXint i=0;i<getNumSortLimits();i++) {
    getSortLimitEditor(i)->getSortLimit(query.order[i]);
    }

  query.match = getMatch();
  }


long GMFilterEditor::onCmdAddRule(FXObject*,FXSelector,void*){
  new GMRuleEditor(rules,rules_offset);
  resize(getWidth(),FXMAX(getHeight(),getDefaultHeight()));
  return 1;
  }


long GMFilterEditor::onCmdAddLimit(FXObject*,FXSelector,void*){
  new GMSortLimitEditor(limits,limits_offset);
  resize(getWidth(),FXMAX(getHeight(),getDefaultHeight()));
  return 1;
  }


long GMFilterEditor::onCmdAccept(FXObject*sender,FXSelector sel,void*ptr){
  return FXDialogBox::onCmdAccept(sender,sel,ptr);
  }


long GMFilterEditor::onUpdAccept(FXObject*sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
  return 1;
  }


long GMFilterEditor::onUpdLimit(FXObject*sender,FXSelector,void*){
  if (limits->numChildren()>5) {
    limitspinner->setRange(1,99999);
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
    }
  else {
    limitspinner->setRange(0,0);
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
    }
  return 1;
  }


long GMFilterEditor::onUpdMatch(FXObject*sender,FXSelector,void*){
  if (getNumRules()>1) {
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
    }
  else {
    match->setCurrentItem(0);
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
    }
  return 1;
  }
