/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2015-2017 by Sander Jansen. All Rights Reserved      *
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

/*
  GMRulerEditor: Implements UI for one rule
*/
class GMRuleEditor : public FXObject {
  FXDECLARE(GMRuleEditor)
public:
  GMComboBox  * columns         = nullptr;
  GMComboBox  * operators       = nullptr;
  GMComboBox  * periods         = nullptr;
  GMComboBox  * options         = nullptr;
  FXSwitcher  * valueswitcher   = nullptr;
  GMTextField * text            = nullptr;
  GMSpinner   * spinner         = nullptr;
  GMSpinner   * datetimespinner = nullptr;
  GMButton    * closebutton     = nullptr;
  GMSpinner   * time_minutes    = nullptr;
  GMSpinner   * time_seconds    = nullptr;
public:
  enum {
    ID_COLUMN = 1,  // Column List
    ID_DELETE,      // Delete Button
    ID_TEXT,        // Text Input
    ID_OPERATOR,    // Operator List
    ID_LAST,
    };
protected:
  // Create UI
  void create(FXMatrix *,FXWindow * before);

  // Validate Text Input
  FXbool validateText();

  // Set Input Type
  void setInputType(FXint);

  // Set Column
  void setColumn(FXint);

  // Get Column
  FXint getColumn() const;

  // Set Operator
  void setOperator(FXint);

  // Get Operator
  FXint getOperator() const;

  // Get Time Period Multiplier used by date input
  FXint getPeriodMultiplier() const;

  // Set Time Period Multiplier used by date input
  void setPeriodMultiplier(FXint);

  // Get Option Value
  FXint getOptionValue() const;

  // Set Option Value
  void setOptionValue(FXint);

  // Set Time Value
  void setTimeValue(FXint);

  // Get Time Value
  FXint getTimeValue() const;
public:
  long onCmdDelete(FXObject*,FXSelector,void*);
  long onCmdColumn(FXObject*,FXSelector,void*);
  long onCmdText(FXObject*,FXSelector,void*);
public:
  GMRuleEditor(){}
public:
  // Construct Editor for new rule
  GMRuleEditor(FXMatrix * rules,FXWindow * lastrow);

  // Construct Editor for existing rule
  GMRuleEditor(FXMatrix * rules,FXWindow * lastrow,const Rule &);

  // Set Rule
  void setRule(const Rule & rule);

  // Get Rule
  void getRule(Rule & rule);

  // Clear any timers
  void clearTimeout();

  // Destructor
  ~GMRuleEditor();
  };



/*
  GMSortLimitEditor: Implements UI for one sort colun
*/
class GMSortLimitEditor : public FXObject {
  FXDECLARE(GMSortLimitEditor)
public:
  GMComboBox  * columns     = nullptr;
  GMComboBox  * order       = nullptr;
  GMButton    * closebutton = nullptr;
  FXFrame     * filler1     = nullptr;
  FXFrame     * filler2     = nullptr;
public:
  enum {
    ID_COLUMN = 1,    // Column List
    ID_DELETE,        // Delete Button
    ID_LAST,
    };
protected:
  // Create UI
  void create(FXMatrix *,FXWindow * before);

  // Set Column
  void setColumn(FXint);

  // Get Column
  FXint getColumn() const;

  // Set Sort Order
  void setOrder(FXbool);

  // Get Sort Order
  FXbool getOrder() const;
public:
  long onCmdDelete(FXObject*,FXSelector,void*);
public:
  GMSortLimitEditor(){}
public:
  // Construct Editor for new column
  GMSortLimitEditor(FXMatrix * rules,FXWindow * lastrow);

  // Construct Editor for existing column
  GMSortLimitEditor(FXMatrix * rules,FXWindow * lastrow,const SortLimit&);

  // Set Sort Limit
  void setSortLimit(const SortLimit &);

  // Get Sort Limit
  void getSortLimit(SortLimit &);

  // Destructor
  ~GMSortLimitEditor();
  };


/*
  GMFilterEditor: Implements full filter editor
*/
class GMFilterEditor : public FXDialogBox {
FXDECLARE(GMFilterEditor)
protected:
  FXMatrix    * rules         = nullptr;
  FXWindow    * rules_offset  = nullptr;
  FXMatrix    * limits        = nullptr;
  FXWindow    * limits_offset = nullptr;
  FXTextField * namefield     = nullptr;
  FXSpinner   * limitspinner  = nullptr;
  FXComboBox  * match         = nullptr;
protected:
  GMFilterEditor(){}
private:
  GMFilterEditor(const GMFilterEditor&);
  GMFilterEditor &operator=(const GMFilterEditor&);
protected:
  // Get Rule Editor for given index
  GMRuleEditor * getRuleEditor(FXint index) const;

  // Get Sort Limit Editor for given index
  GMSortLimitEditor * getSortLimitEditor(FXint index) const;

  // Get Number of Rules
  FXint getNumRules() const;

  // Get Number of Sort Limits
  FXint getNumSortLimits() const;

  // Set Match Mode
  void setMatch(FXint);

  // Get Match Mode
  FXint getMatch() const;
public:
  enum {
    ID_ADD_RULE = FXDialogBox::ID_LAST, // Add Rule Button
    ID_ADD_LIMIT,                       // Add Limit Button
    ID_LIMIT,                           // Limit Input
    ID_MATCH,                           // Match Input
    };
public:
  long onCmdAddRule(FXObject*,FXSelector,void*);
  long onCmdAddLimit(FXObject*,FXSelector,void*);
  long onUpdLimit(FXObject*,FXSelector,void*);
  long onUpdMatch(FXObject*,FXSelector,void*);
public:
  // Construct Filter Editor for given filter
  GMFilterEditor(FXWindow * p,const GMFilter & filter);

  // Show Editor Dialog
  void show(FXuint placement);

  // Set Filter
  void setFilter(const GMFilter &);

  // Get Filter
  void getFilter(GMFilter & query) const;

  // Destructor
  virtual ~GMFilterEditor();
  };

#endif
