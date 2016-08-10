/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2015-2016 by Sander Jansen. All Rights Reserved      *
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
#include "GMTrack.h"
#include "GMIconTheme.h"
#include "GMFilter.h"
#include "GMFilterEditor.h"

#define add(list,str,column) list->appendItem(str,(void*)(FXival)column)

#define PERIOD_MINUTES (60)
#define PERIOD_HOURS (60*60)
#define PERIOD_DAYS (60*60*24)
#define PERIOD_WEEKS (60*60*24*7)


struct ColumnDefintion {
  FXint column;
  const FXchar * const name;
  };


// Column titles and order of display
static const ColumnDefintion column_types[]={
  { Rule::ColumnTitle,notr("Title")},
  { Rule::ColumnAlbum,notr("Album")},
  { Rule::ColumnAlbumArtist,notr("Album Artist")},
  { Rule::ColumnArtist,notr("Artist")},
  { Rule::ColumnComposer,notr("Composer")},
  { Rule::ColumnConductor, notr("Conductor")},
  { Rule::ColumnTag, notr("Tag")},
  { Rule::ColumnYear,notr("Year")},
  { Rule::ColumnTime,notr("Time")},
  { Rule::ColumnRating,notr("Rating")},
  { Rule::ColumnTrackNumber,notr("Track Number")},
  { Rule::ColumnDiscNumber,notr("Disc Number")},
  { Rule::ColumnPlayCount,notr("Play Count")},
  { Rule::ColumnPlayDate,notr("Last Played")},
  { Rule::ColumnImportDate,notr("Last Updated")},
  { Rule::ColumnFileType,notr("File Type")},
  { Rule::ColumnPath,notr("Path")},
  { Rule::ColumnChannels,notr("Channels")},
  { Rule::ColumnBitRate,notr("Bit Rate")},
  { Rule::ColumnSampleRate,notr("Sample Rate")},
  { Rule::ColumnSampleSize,notr("Sample Size")},
  };


// Input Types
enum {
  InputText    = 1,
  InputYear    = 2,
  InputDate    = 3,
  InputInteger = 4,
  InputOption  = 5,
  InputTime    = 6
  };


// Input Type for each column
// Keep this in sync with GMFilter::Column* order.
static const FXint column_input_map[]{
  InputText,    /* ColumnTitle */
  InputText,    /* ColumnArtist */
  InputText,    /* ColumnAlbumArtist */
  InputText,    /* ColumnComposer */
  InputText,    /* ColumnConductor */
  InputText,    /* ColumnAlbum */
  InputText,    /* ColumnPath */
  InputText,    /* ColumnTag */
  InputYear,    /* ColumnYear */
  InputTime,    /* ColumnTime */
  InputInteger, /* ColumnTrackNumber */
  InputInteger, /* ColumnDiscNumber */
  InputInteger, /* ColumnRating */
  InputInteger, /* ColumnPlayCount  */
  InputDate,    /* ColumnPlaydate */
  InputDate,    /* ColumnImportdate */
  InputOption,  /* ColumnFileType */
  InputInteger, /* ColumnChannels */
  InputInteger, /* ColumnBitRate */
  InputInteger, /* ColumnSampleRate */
  InputInteger, /* ColumnSampleSize */
  };


// Fill Column List
static void fillColumns(GMComboBox * combobox) {
  for (FXuint i=0;i<ARRAYNUMBER(column_types);i++){
    add(combobox,fxtr(column_types[i].name),column_types[i].column);
    }
  combobox->setNumVisible(FXMIN(9,combobox->getNumItems()));
  }


// Fill Option List
static void fillOptions(GMComboBox * c,FXint column) {
  c->clearItems();
  switch(column) {
    case Rule::ColumnFileType:
      add(c,"flac",FILETYPE_FLAC);
      add(c,"vorbis",FILETYPE_OGG_VORBIS);
      add(c,"opus",FILETYPE_OGG_OPUS);
      add(c,"ogg flac",FILETYPE_OGG_FLAC);
      add(c,"mp3",FILETYPE_MP3);
      add(c,"aac",FILETYPE_MP4_AAC);
      add(c,"alac",FILETYPE_MP4_ALAC);
      add(c,"speex",FILETYPE_OGG_SPEEX);
      add(c,"unknown",FILETYPE_UNKNOWN);
      break;
    }
  c->setNumVisible(FXMIN(9,c->getNumItems()));
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


// Construct rule editor for new rule
GMRuleEditor::GMRuleEditor(FXMatrix * parent,FXWindow * before) {
  create(parent,before);
  }


// Construct rule editor for existing rule
GMRuleEditor::GMRuleEditor(FXMatrix * parent,FXWindow * before,const Rule & rule) {
  create(parent,before);
  setRule(rule);
  }


// Destructor
GMRuleEditor::~GMRuleEditor(){
  delete valueswitcher;
  delete operators;
  delete columns;
  delete closebutton;
  }


// Clear Timeout
// This needs to be called by the filter editor before it deletes itself
void GMRuleEditor::clearTimeout() {
  text->getApp()->removeTimeout(this,ID_TEXT);
  }


// Create UI
void GMRuleEditor::create(FXMatrix * rules,FXWindow * before) {

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
    spinner = new GMSpinner(spinnerframe,6,nullptr,0);
    spinner->setRange(0,2020);
    spinner->setValue(2015);

    // #2 Date Input
    FXHorizontalFrame * dateframe = new FXHorizontalFrame(valueswitcher,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
    datetimespinner = new GMSpinner(dateframe,6,nullptr,0);
    periods = new GMComboBox(dateframe,0,nullptr,0,LAYOUT_FILL_X|COMBOBOX_STATIC);
    add(periods,fxtr("Minutes"),PERIOD_MINUTES);
    add(periods,fxtr("Hours"),PERIOD_HOURS);
    add(periods,fxtr("Days"),PERIOD_DAYS);
    add(periods,fxtr("Weeks"),PERIOD_WEEKS);
    periods->setNumVisible(FXMIN(9,periods->getNumItems()));

    // #3 Filetypes
    FXHorizontalFrame * optionframe = new FXHorizontalFrame(valueswitcher,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
    options = new GMComboBox(optionframe,0,nullptr,0,COMBOBOX_STATIC);

    // #4 Time
    FXHorizontalFrame * timeframe = new FXHorizontalFrame(valueswitcher,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
    time_minutes = new GMSpinner(timeframe,3,nullptr,0);
    time_minutes->setRange(0,24*60);
    new FXLabel(timeframe,":");
    time_seconds = new GMSpinner(timeframe,3,nullptr,0);
    time_seconds->setRange(0,59);

  // Put into correct position of the matrix
  if (rules->id()) rules->create();
  columns->reparent(rules,before);
  operators->reparent(rules,before);
  valueswitcher->reparent(rules,before);
  closebutton->reparent(rules,before);

  // Initialize all fields
  setInputType(column_input_map[getColumn()]);
  }


// Set Rule
void GMRuleEditor::setRule(const Rule & rule) {
  setColumn(rule.column);
  setOperator(rule.opcode);
  switch(column_input_map[rule.column]){
    case InputText:
      text->setText(rule.text,true);
      break;
    case InputYear:
    case InputInteger:
      spinner->setValue(rule.value);
      break;
    case InputDate:
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
    case InputOption:
      setOptionValue(rule.value);
      break;
    case InputTime:
      setTimeValue(rule.value);
      break;
    }
  }


// Get rule
void GMRuleEditor::getRule(Rule & rule) {
  rule.column = getColumn();
  rule.opcode = getOperator();
  switch(column_input_map[getColumn()]){
    case InputText:
      rule.text = text->getText();
      rule.value = 0;
      break;
    case InputInteger:
    case InputYear:
      rule.value = spinner->getValue();
      rule.text.clear();
      break;
    case InputDate:
      rule.value = datetimespinner->getValue() * getPeriodMultiplier();
      rule.text.clear();
      break;
    case InputOption:
      rule.value = getOptionValue();
      rule.text.clear();
      break;
    case InputTime:
      rule.value = getTimeValue();
      rule.text.clear();
      break;
    default: FXASSERT(0); break;
    }
  }


// Set Column
void GMRuleEditor::setColumn(FXint column) {
  FXint item = columns->findItemByData((void*)(FXival)column);
  FXASSERT(item>=0);
  columns->setCurrentItem(item,true);
  }


// Get Column
FXint GMRuleEditor::getColumn() const {
  return (FXint)(FXival)columns->getItemData(columns->getCurrentItem());
  }


// Set Operator
void GMRuleEditor::setOperator(FXint opcode) {
  FXint item = operators->findItemByData((void*)(FXival)opcode);
  FXASSERT(item>=0);
  operators->setCurrentItem(item);
  }


// Get Operator
FXint GMRuleEditor::getOperator() const {
  return (FXint)(FXival)operators->getItemData(operators->getCurrentItem());
  }


// Set Option Value
void GMRuleEditor::setOptionValue(FXint option) {
  FXint item = options->findItemByData((void*)(FXival)option);
  FXASSERT(item>=0);
  options->setCurrentItem(item,true);
  }


// Get Option Value
FXint GMRuleEditor::getOptionValue() const {
  return (FXint)(FXival)options->getItemData(options->getCurrentItem());
  }


// Set Time Value
void GMRuleEditor::setTimeValue(FXint value){
  FXint minutes = value / 60;
  FXint seconds = value % 60;
  time_minutes->setValue(minutes);
  time_seconds->setValue(seconds);
  }


// Get Time Value
FXint GMRuleEditor::getTimeValue() const {
  return (time_minutes->getValue()*60)+(time_seconds->getValue());
  }


// Set Time Period Multiplier for date inputs
void GMRuleEditor::setPeriodMultiplier(FXint value){
  FXint item = periods->findItemByData((void*)(FXival)value);
  periods->setCurrentItem(item,true);
  }


// Get Time Period Multiplier for date inputs
FXint GMRuleEditor::getPeriodMultiplier() const {
  return (FXint)(FXival)periods->getItemData(periods->getCurrentItem());
  }


// Set the Input Type
void GMRuleEditor::setInputType(FXint type) {
  FXint current = (FXint)(FXival)operators->getUserData();
  if (current!=type) {
    operators->clearItems();
    switch(type) {
      case InputText:
        add(operators,fxtr("contains"),Rule::OperatorLike);
        add(operators,fxtr("does not contain"),Rule::OperatorNotLike);
        add(operators,fxtr("equals"),Rule::OperatorEquals);
        add(operators,fxtr("not equal to"),Rule::OperatorNotEqual);
        add(operators,fxtr("starts with"),Rule::OperatorPrefix);
        add(operators,fxtr("ends with"),Rule::OperatorSuffix);
        add(operators,fxtr("matches"),Rule::OperatorMatch);
        validateText();
        valueswitcher->setCurrent(0);
        break;
      case InputYear:
        add(operators,fxtr("in"),Rule::OperatorEquals);
        add(operators,fxtr("not in"),Rule::OperatorNotEqual);
        add(operators,fxtr("before"),Rule::OperatorLess);
        add(operators,fxtr("after"),Rule::OperatorGreater);
        valueswitcher->setCurrent(1);
        break;
      case InputDate:
        add(operators,fxtr("in the last"),Rule::OperatorGreater);
        add(operators,fxtr("not in the last"),Rule::OperatorLess);
        valueswitcher->setCurrent(2);
        break;
      case InputInteger:
      case InputTime:
        add(operators,fxtr("equals"),Rule::OperatorEquals);
        add(operators,fxtr("not equal to"),Rule::OperatorNotEqual);
        add(operators,fxtr("at least"),Rule::OperatorGreater);
        add(operators,fxtr("at most"),Rule::OperatorLess);
        if (type==InputTime)
          valueswitcher->setCurrent(4);
        else
          valueswitcher->setCurrent(1);
        break;
      case InputOption:
        add(operators,fxtr("equals"),Rule::OperatorEquals);
        add(operators,fxtr("not equal to"),Rule::OperatorNotEqual);
        valueswitcher->setCurrent(3);
        fillOptions(options,getColumn());
        break;
      }
    operators->setUserData((void*)(FXival)type);
    operators->setNumVisible(FXMIN(9,operators->getNumItems()));
    }

  // Set default values
  switch(getColumn()) {
    case Rule::ColumnRating    : spinner->setRange(0,5);
                                 spinner->setValue(0);
                                 spinner->setIncrement(1);
                                 break;
    case Rule::ColumnYear      : spinner->setRange(0,2100);
                                 spinner->setValue(FXDate::localDate().year());
                                 spinner->setIncrement(1);
                                 break;
    case Rule::ColumnChannels  : spinner->setRange(0,8);
                                 spinner->setValue(2);
                                 spinner->setIncrement(1);
                                 break;
    case Rule::ColumnBitRate   : spinner->setRange(0,1000);
                                 spinner->setValue(320);
                                 spinner->setIncrement(1);
                                 break;
    case Rule::ColumnSampleRate: spinner->setRange(0,192000);
                                 spinner->setValue(44100);
                                 spinner->setIncrement(1);
                                 break;
    case Rule::ColumnSampleSize: spinner->setRange(0,32);
                                 spinner->setValue(16);
                                 spinner->setIncrement(8);
                                 break;
    default                    : spinner->setRange(0,2147483647);
                                 spinner->setValue(0);
                                 spinner->setIncrement(1);
                                 break;
    }
  }


// Column Changed
long GMRuleEditor::onCmdColumn(FXObject*,FXSelector,void*) {
  setInputType(column_input_map[getColumn()]);
  return 1;
  }


// Rule Deleted
long GMRuleEditor::onCmdDelete(FXObject*,FXSelector,void*) {
  delete this;
  return 1;
  }


// Validate Text Input
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


// Text Changed
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


// Construct Editor for new sort limit
GMSortLimitEditor::GMSortLimitEditor(FXMatrix * parent,FXWindow * before) {
  create(parent,before);
  }


// Construct Editor for existing sort limit
GMSortLimitEditor::GMSortLimitEditor(FXMatrix * parent,FXWindow * before,const SortLimit & limit) {
  create(parent,before);
  setSortLimit(limit);
  }


// Destructor
GMSortLimitEditor::~GMSortLimitEditor(){
  delete order;
  delete columns;
  delete closebutton;
  delete filler1;
  delete filler2;
  }


// Set Column
void GMSortLimitEditor::setColumn(FXint column) {
  FXint item = columns->findItemByData((void*)(FXival)column);
  FXASSERT(item>=0);
  columns->setCurrentItem(item);
  }


// Get Column
FXint GMSortLimitEditor::getColumn() const {
  return (FXint)(FXival)columns->getItemData(columns->getCurrentItem());
  }


// Set Order
void GMSortLimitEditor::setOrder(FXbool ascending) {
  if (ascending)
    order->setCurrentItem(0);
  else
    order->setCurrentItem(1);
  }


// Get Order
FXbool GMSortLimitEditor::getOrder() const {
  return order->getCurrentItem()==0;
  }


// Set Sort Limit
void GMSortLimitEditor::setSortLimit(const SortLimit & limit) {
  setColumn(limit.column);
  if (limit.ascending)
    order->setCurrentItem(0);
  else
    order->setCurrentItem(1);
  }


// Get Sort Limit
void GMSortLimitEditor::getSortLimit(SortLimit & limit){
  limit.column = getColumn();
  limit.ascending = getOrder();
  }


// Create UI
void GMSortLimitEditor::create(FXMatrix * rules,FXWindow * before) {
  // List of Columns
  columns = new GMComboBox(rules,12,this,ID_COLUMN,LAYOUT_FILL_X|COMBOBOX_STATIC);
  columns->setUserData(this);
  fillColumns(columns);

  order = new GMComboBox(rules,10,nullptr,0,LAYOUT_FILL_X|COMBOBOX_STATIC);
  order->appendItem(fxtr("Ascending"));
  order->appendItem(fxtr("Descending"));
  order->setNumVisible(2);

  closebutton = new GMButton(rules,fxtr("\tRemove Column\tRemove Column"),GMIconTheme::instance()->icon_remove,this,ID_DELETE,BUTTON_TOOLBAR|FRAME_RAISED);

  filler1 = new FXFrame(rules,FRAME_NONE);
  filler2 = new FXFrame(rules,FRAME_NONE);

  if (rules->id()) rules->create();
  columns->reparent(rules,before);
  order->reparent(rules,before);
  closebutton->reparent(rules,before);
  filler1->reparent(rules,before);
  filler2->reparent(rules,before);

  columns->setUserData(this);
  }


// Limit Deleted
long GMSortLimitEditor::onCmdDelete(FXObject*,FXSelector,void*) {
  delete this;
  return 1;
  }




FXDEFMAP(GMFilterEditor) GMFilterEditorMap[]={
  FXMAPFUNC(SEL_UPDATE,GMFilterEditor::ID_LIMIT,GMFilterEditor::onUpdLimit),
  FXMAPFUNC(SEL_UPDATE,GMFilterEditor::ID_MATCH,GMFilterEditor::onUpdMatch),
  FXMAPFUNC(SEL_COMMAND,GMFilterEditor::ID_ADD_RULE,GMFilterEditor::onCmdAddRule),
  FXMAPFUNC(SEL_COMMAND,GMFilterEditor::ID_ADD_LIMIT,GMFilterEditor::onCmdAddLimit),
  };

FXIMPLEMENT(GMFilterEditor,FXDialogBox,GMFilterEditorMap,ARRAYNUMBER(GMFilterEditorMap))


// Construct Filter Editor
GMFilterEditor::GMFilterEditor(FXWindow *p,const GMFilter & query) : FXDialogBox(p,"Edit Filter",DECOR_BORDER|DECOR_TITLE|DECOR_STRETCHABLE,0,0,600,250,0,0,0,0,0,0) {

  FXWindow * filler;

  FXHorizontalFrame *closebox=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  new GMButton(closebox,tr("&Update"),nullptr,this,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  new GMButton(closebox,tr("&Cancel"),nullptr,this,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  new FXSeparator(this,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);

  FXVerticalFrame * main=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y);

  // Filter Name
  FXMatrix  * header = new FXMatrix(main,2,LAYOUT_FILL|MATRIX_BY_COLUMNS);
  new FXLabel(header,tr("Name:"),nullptr,LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  namefield = new GMTextField(header,30,nullptr,0,LAYOUT_FILL_COLUMN);

  // Match All or Any
  new FXLabel(header,tr("Match:"),nullptr,LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  match = new GMComboBox(header,3,this,ID_MATCH,LAYOUT_FILL_COLUMN|COMBOBOX_STATIC);
  add(match,tr("All"),GMFilter::MatchAll);
  add(match,tr("Any"),GMFilter::MatchAny);
  match->setNumVisible(2);

  // Filter Rules
  new FXLabel(header,tr("Filter:"),nullptr,LAYOUT_RIGHT);
  rules = new FXMatrix(header,4,LAYOUT_FILL|MATRIX_BY_COLUMNS|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
  filler = new GMComboBox(rules,12,nullptr,0,LAYOUT_FILL_X|COMBOBOX_STATIC);
  filler->disable();
  rules_offset = filler;
  filler = new GMComboBox(rules,12,nullptr,0,LAYOUT_FILL_X|COMBOBOX_STATIC);
  filler->disable();
  filler = new GMTextField(rules,20,nullptr,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN);
  filler->disable();
  new GMButton(rules,tr("\tAdd Filter\tAdd Filter"),GMIconTheme::instance()->icon_add,this,ID_ADD_RULE,BUTTON_TOOLBAR|FRAME_RAISED);

  // Limits
  new FXLabel(header,tr("Limit:"),nullptr,LAYOUT_RIGHT);
  limits = new FXMatrix(header,5,LAYOUT_FILL|MATRIX_BY_COLUMNS|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
  limitspinner = new GMSpinner(limits,6,this,ID_LIMIT,SPIN_NOMAX);
  limitspinner->setRange(0,0);
  new FXLabel(limits,tr("tracks sorted by:"),nullptr,LAYOUT_CENTER_Y);
  limitspinner->disable();
  filler = new GMComboBox(limits,12,nullptr,0,LAYOUT_FILL_X|COMBOBOX_STATIC);
  filler->disable();
  limits_offset = filler;
  filler = new GMComboBox(limits,10,nullptr,0,LAYOUT_FILL_X|COMBOBOX_STATIC);
  filler->disable();
  new GMButton(limits,tr("\tAdd Column\tAdd Column"),GMIconTheme::instance()->icon_add,this,ID_ADD_LIMIT,BUTTON_TOOLBAR|FRAME_RAISED);

  setFilter(query);
  }


// Destructor
GMFilterEditor::~GMFilterEditor(){
  for (FXint i=0;i<getNumRules();i++){
    getRuleEditor(i)->clearTimeout();
    }
  }


// Show Editor
void GMFilterEditor::show(FXuint placement){
  resize(getWidth(),getDefaultHeight());
  FXDialogBox::show(placement);
  }


// Set Match Mode
void GMFilterEditor::setMatch(FXint matchrule) {
  if (matchrule==GMFilter::MatchAll)
    match->setCurrentItem(0);
  else
    match->setCurrentItem(1);
  }


// Get Match Mode
FXint GMFilterEditor::getMatch() const {
  return (FXint)(FXival)match->getItemData(match->getCurrentItem());
  }


// Get Number of Rules
FXint GMFilterEditor::getNumRules() const {
  return (rules->numChildren() / 4) - 1;
  }


// Get Number of Limits
FXint GMFilterEditor::getNumSortLimits() const {
  return (limits->numChildren() / 5) - 1;
  }


// Get Editor at index
GMRuleEditor * GMFilterEditor::getRuleEditor(FXint index) const {
  return static_cast<GMRuleEditor*>(rules->childAtIndex((index*4))->getUserData());
  }


// Get Editor at index
GMSortLimitEditor * GMFilterEditor::getSortLimitEditor(FXint index) const {
  return static_cast<GMSortLimitEditor*>(limits->childAtIndex((index*5)+2)->getUserData());
  }


// Set Filter
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


// Get Filter
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


// Add a new rule
long GMFilterEditor::onCmdAddRule(FXObject*,FXSelector,void*){
  new GMRuleEditor(rules,rules_offset);
  resize(getWidth(),FXMAX(getHeight(),getDefaultHeight()));
  return 1;
  }


// Add a new limit
long GMFilterEditor::onCmdAddLimit(FXObject*,FXSelector,void*){
  new GMSortLimitEditor(limits,limits_offset);
  resize(getWidth(),FXMAX(getHeight(),getDefaultHeight()));
  return 1;
  }


// Update handler for limit
long GMFilterEditor::onUpdLimit(FXObject*sender,FXSelector,void*){
  if (limits->numChildren()>5) {
    limitspinner->setRange(1,99999);
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
    }
  else {
    limitspinner->setRange(0,0);
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
    }
  return 1;
  }


// Update handler for match mode
long GMFilterEditor::onUpdMatch(FXObject*sender,FXSelector,void*){
  if (getNumRules()>1) {
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
    }
  else {
    match->setCurrentItem(0);
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
    }
  return 1;
  }
