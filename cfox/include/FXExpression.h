/********************************************************************************
*                                                                               *
*                      E x p r e s s i o n   E v a l u a t o r                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2023 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXEXPRESSION_H
#define FXEXPRESSION_H

namespace FX {


/**
* Expression compiles a simple numerical expression into efficient byte-code
* for repeated evaluation.
* When an expression is compiled, an optional comma-separated list of variable
* names may be passed; during evaluation, values may be provided for these variable
* by passing an array of values.
* For instance, when compiling an expression:
*
*       b**2-4*a*c
*
* with the list of variables:
*
*       a,b,c
*
* then a subsequent evaluation can pass an array of three numbers:
*
*       [1 4 2]
*
* which will then evaluate the expression:
*
*       4**2-4*1*2
*
* which should yield:
*
*       8
*
* The expression syntax can be build from the following operators,
* in order of increasing precedence:
*
*       ? :                     alternate expression
*       <, <=, >, >=, ==, !=    comparisons
*       <<, >>                  bit-shifts
*       &, |, ^                 bit-wise operations
*       +, -                    addition and subtraction
*       *, /, %                 multiply, divide, modulo
*       **                      power
*       -, ~                    unary minus, bitwise not
*       ( )                     parenthesized subexpressions
*
* The following functions are available:
*
*       abs, acos, acosh, asin, asinh, atan, atanh,
*       cbrt, ceil, cos, cosh, cub, erf, erfc, exp, exp2,
*       exp10, floor, inverf, inverfc, isfin, isinf, isnan,
*       log, log2, log10, near, round, sin, sinh, sqr, sqrt,
*       tan, tanh, trunc, wrap, wrap4, max, min, atan2
*
* The expression engine also contains the following constants:
*
*       PI, E, DTOR, RTOD
*
* Variables should not use any of these reserved names.
*
* Initially, FXExpression will be set to a dummy program which
* will return 0 when evaluated.
*/
class FXAPI FXExpression {
private:
  FXuchar *code;
private:
  static const FXuchar initial[];
  static const FXchar *const errors[];
public:

  /// Expression error codes
  enum Error {
    ErrOK,              /// No errors
    ErrEmpty,           /// Empty input
    ErrMore,            /// More characters after input
    ErrMemory,          /// Out of memory
    ErrParent,          /// Unmatched parentheses
    ErrToken,           /// Illegal token
    ErrComma,           /// Expected comma
    ErrIdent,           /// Unknown identifier
    ErrColon,           /// Expected colon
    ErrLong             /// Expression too long
    };

public:

  /// Construct empty expression object
  FXExpression();

  /// Copy expression object
  FXExpression(const FXExpression& orig);

  /// Compile expression; if error is not NULL, error code is returned
  FXExpression(const FXchar* expression,const FXchar* variables=nullptr,Error* error=nullptr);

  /// Compile expression; if error is not NULL, error code is returned
  FXExpression(const FXString& expression,const FXString& variables=FXString::null,Error* error=nullptr);

  /// Assign another expression to this one
  FXExpression& operator=(const FXExpression& orig);

  /// See if expression is empty
  FXbool empty() const { return (code==initial); }

  /**
  * Evaluate expression with given arguments, if any.
  * The array of arguments should match the number of variables
  * passed when compiling the expression.
  */
  FXdouble evaluate(const FXdouble *args=nullptr) const;

  /**
  * Parse expression, return error code if syntax error is found.
  * If a comma-separated list of variables is passed, then an array of
  * equal number of doubles must be passed when calling evaluate.
  * The values of the array will be read in place of the variable names
  * in the expression.
  */
  Error parse(const FXchar* expression,const FXchar* variables=nullptr);
  Error parse(const FXString& expression,const FXString& variables=FXString::null);

  /// Returns error code for given error
  static const FXchar* getError(Error err){ return errors[err]; }

  /// Saving and loading
  friend FXAPI FXStream& operator<<(FXStream& store,const FXExpression& s);
  friend FXAPI FXStream& operator>>(FXStream& store,FXExpression& s);

  /// Clear the expression
  void clear();

  /// Delete
 ~FXExpression();
  };


// Serialization
extern FXAPI FXStream& operator<<(FXStream& store,const FXExpression& s);
extern FXAPI FXStream& operator>>(FXStream& store,FXExpression& s);

}

#endif
