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
#ifndef GMFILTEREDITOR_H
#define GMFILTEREDITOR_H

class GMRuleEditor : public FXObject {
  FXDECLARE(GMRuleEditor)
public:
  GMComboBox  * columns;
  GMComboBox  * operators;
  GMComboBox  * periods;
  GMComboBox  * options;
  FXSwitcher  * valueswitcher;
  GMTextField * text;
  GMSpinner   * spinner;
  GMSpinner   * datetimespinner;
  GMButton    * closebutton;
  GMSpinner   * time_minutes;
  GMSpinner   * time_seconds;
public:
  enum {
    ID_COLUMN = 1,
    ID_DELETE,
    ID_TEXT,
    ID_OPERATOR,
    ID_LAST,
    };
protected:
  void create(FXMatrix *,FXWindow * before);
  void setInputType(FXint);
  void setColumn(FXint);
  FXint getColumn() const;
  void setOperator(FXint);
  FXint getOperator() const;
  FXint getPeriodMultiplier() const;
  void setPeriodMultiplier(FXint);
  FXbool validateText();
  FXint getOptionValue() const;
  void setOptionValue(FXint);
  void setTimeValue(FXint);
  FXint getTimeValue() const;
public:
  long onCmdDelete(FXObject*,FXSelector,void*);
  long onCmdColumn(FXObject*,FXSelector,void*);
  long onCmdText(FXObject*,FXSelector,void*);
public:
  GMRuleEditor(){}
public:
  GMRuleEditor(FXMatrix * rules,FXWindow * lastrow);
  GMRuleEditor(FXMatrix * rules,FXWindow * lastrow,const Rule &);

  // Set Rule
  void setRule(const Rule & rule);

  // Get Rule
  void getRule(Rule & rule);

  // Clear any timers
  void clearTimeout();

  ~GMRuleEditor();
  };


class GMSortLimitEditor : public FXObject {
  FXDECLARE(GMSortLimitEditor)
public:
  GMComboBox  * columns;
  GMComboBox  * order;
  GMButton    * closebutton;
  FXFrame     * filler1;
  FXFrame     * filler2;
public:
  enum {
    ID_COLUMN = 1,
    ID_DELETE,
    ID_LAST,
    };
protected:
  void create(FXMatrix *,FXWindow * before);
  void setColumn(FXint);
  FXint getColumn() const;
  void setOrder(FXbool);
  FXbool getOrder() const;
public:
  long onCmdDelete(FXObject*,FXSelector,void*);
public:
  GMSortLimitEditor(){}
public:
  GMSortLimitEditor(FXMatrix * rules,FXWindow * lastrow);
  GMSortLimitEditor(FXMatrix * rules,FXWindow * lastrow,const SortLimit&);

  void setSortLimit(const SortLimit &);

  void getSortLimit(SortLimit &);

  ~GMSortLimitEditor();
  };


class GMFilterEditor : public FXDialogBox {
FXDECLARE(GMFilterEditor)
protected:
  FXMatrix    * rules;
  FXWindow    * rules_offset;
  FXMatrix    * limits;
  FXWindow    * limits_offset;
  FXTextField * namefield;
  FXSpinner   * limitspinner;
  FXComboBox  * match;
  FXButton    * addremovelimit;
protected:
  GMFilterEditor(){}
private:
  GMFilterEditor(const GMFilterEditor&);
  GMFilterEditor &operator=(const GMFilterEditor&);
public:
  enum {
    ID_ADD_RULE = FXDialogBox::ID_LAST,
    ID_ADD_LIMIT,
    ID_LIMIT,
    ID_MATCH,
    };
public:
  long onCmdAccept(FXObject*,FXSelector,void*);
  long onUpdAccept(FXObject*,FXSelector,void*);
  long onCmdAddRule(FXObject*,FXSelector,void*);
  long onCmdAddLimit(FXObject*,FXSelector,void*);
  long onUpdLimit(FXObject*,FXSelector,void*);
  long onUpdMatch(FXObject*,FXSelector,void*);
public:
  GMFilterEditor(FXWindow * p,const GMFilter & query);

  void show(FXuint placement);

  void setFilter(const GMFilter &);

  void getFilter(GMFilter & query) const;

  GMRuleEditor * getRuleEditor(FXint index) const;

  GMSortLimitEditor * getSortLimitEditor(FXint index) const;

  FXint getNumRules() const;

  FXint getNumSortLimits() const;

  void setMatch(FXint);

  FXint getMatch() const;

  void createRule();

  virtual ~GMFilterEditor();
  };

#endif
