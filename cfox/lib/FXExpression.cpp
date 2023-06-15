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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "fxascii.h"
#include "fxunicode.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXElement.h"
#include "FXString.h"
#include "FXExpression.h"


/*
  Notes:
  - Old as night, but recently rediscovered ;-).
  - Better treatment of identifiers needed [user-supplied names].
  - Maintain stack-depth during compile phase for possible limit check.
  - The new return code ErrMore means not all the input for the expression
    was consumed; It could treated it as ErrOK.  However, we should add a
    way to find out where the parse stopped.
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_DETAIL    1001
//#define TOPIC_EXPDUMP   1015          // Debugging expression code

#define MAXSTACKDEPTH   128

// Access to argument
#if defined(__i386__) || defined(__x86_64__)            // No alignment limits on shorts
#define SETARG(p,val) (*((FXshort*)(p))=(val))
#define GETARG(p)     (*((const FXshort*)(p)))
#elif (FOX_BIGENDIAN == 1)                              // Big-endian machines
#define SETARG(p,val) (*((p)+0)=(val)>>8,*((p)+1)=(val))
#define GETARG(p)     ((FXshort)((*((p)+0)<<8)+(*((p)+1))))
#else                                                   // Little-endian machines
#define SETARG(p,val) (*((p)+0)=(val),*((p)+1)=(val)>>8)
#define GETARG(p)     ((FXshort)((*((p)+0))+(*((p)+1)<<8)))
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {


// Furnish our own versions
extern FXAPI FXlong __strtoll(const FXchar *beg,const FXchar** end=nullptr,FXint base=0,FXbool* ok=nullptr);
extern FXAPI FXdouble __strtod(const FXchar *beg,const FXchar** end=nullptr,FXbool* ok=nullptr);


namespace {

// Tokens
enum {
  TK_EOF        = 0U,
  TK_INT        = 1U,
  TK_INT_HEX    = 2U,
  TK_INT_BIN    = 3U,
  TK_INT_OCT    = 4U,
  TK_REAL       = 5U,
  TK_PLUS       = 6U,
  TK_MINUS      = 7U,
  TK_TIMES      = 8U,
  TK_DIVIDE     = 9U,
  TK_MODULO     = 10U,
  TK_POWER      = 11U,
  TK_LPAR       = 12U,
  TK_RPAR       = 13U,
  TK_LESS       = 14U,
  TK_GREATER    = 15U,
  TK_LESSEQ     = 16U,
  TK_GREATEREQ  = 17U,
  TK_EQUAL      = 18U,
  TK_NOTEQUAL   = 19U,
  TK_AND        = 20U,
  TK_OR         = 21U,
  TK_XOR        = 22U,
  TK_NOT        = 23U,
  TK_SHIFTLEFT  = 24U,
  TK_SHIFTRIGHT = 25U,
  TK_COMMA      = 26U,
  TK_QUEST      = 27U,
  TK_COLON      = 28U,
  TK_ERROR      = 29U,
  TK_PI         = 2585U,
  TK_EULER      = 69U,
  TK_RTOD       = 3005613U,
  TK_DTOR       = 2389741U,
  TK_ABS        = 108848U,
  TK_ACOS       = 3592862U,
  TK_ACOSH      = 118564406U,
  TK_ASIN       = 3610325U,
  TK_ASINH      = 119140637U,
  TK_ATAN       = 3615258U,
  TK_ATANH      = 119303474U,
  TK_CBRT       = 3520359U,
  TK_CEIL       = 3523203U,
  TK_COS        = 107103U,
  TK_COSH       = 3534423U,
  TK_CUB        = 107284U,
  TK_ERF        = 113713U,
  TK_ERFC       = 3752498U,
  TK_EXP        = 114029U,
  TK_EXP2       = 3763007U,
  TK_EXP10      = 124179084U,
  TK_FLOOR      = 122360152U,
  TK_INVERF     = 4219871328U,
  TK_INVERFC    = 1816800259U,
  TK_ISFIN      = 128618299U,
  TK_ISINF      = 128611099U,
  TK_ISNAN      = 128609851U,
  TK_LOG        = 114052U,
  TK_LOG2       = 3763766U,
  TK_LOG10      = 124204261U,
  TK_NEAR       = 3985784U,
  TK_SIN        = 124308U,
  TK_SINH       = 4102268U,
  TK_SQR        = 123536U,
  TK_SQRT       = 4076772U,
  TK_ROUND      = 136616002U,
  TK_TRUNC      = 133596670U,
  TK_TAN        = 123227U,
  TK_TANH       = 4066515U,
  TK_WRAP       = 4221012U,
  TK_WRAP4      = 139293408U,
  TK_MAX        = 121748U,
  TK_MIN        = 121482U,
  TK_POW        = 119176U,
  TK_ATAN2      = 119303528U,
  };


// Opcodes
enum {
  OP_END,

  OP_BRA,
  OP_BRT,
  OP_BRF,

  OP_NUM,
  OP_VAR,

  OP_NOT,
  OP_NEG,

  OP_MUL,
  OP_DIV,
  OP_MOD,
  OP_ADD,
  OP_SUB,
  OP_AND,
  OP_OR,
  OP_XOR,
  OP_SHL,
  OP_SHR,
  OP_LT,
  OP_GT,
  OP_LE,
  OP_GE,
  OP_EQ,
  OP_NE,

  OP_ABS,
  OP_ACOS,
  OP_ACOSH,
  OP_ASIN,
  OP_ASINH,
  OP_ATAN,
  OP_ATANH,
  OP_CEIL,
  OP_COS,
  OP_COSH,
  OP_ERF,
  OP_ERFC,
  OP_EXP,
  OP_EXP2,
  OP_EXP10,
  OP_FLOOR,
  OP_INVERF,
  OP_INVERFC,
  OP_ISFIN,
  OP_ISINF,
  OP_ISNAN,
  OP_LOG,
  OP_LOG2,
  OP_LOG10,
  OP_NEAR,
  OP_SIN,
  OP_SINH,
  OP_SQR,
  OP_CUB,
  OP_SQRT,
  OP_CBRT,
  OP_ROUND,
  OP_TRUNC,
  OP_TAN,
  OP_TANH,
  OP_WRAP,
  OP_WRAP4,

  OP_MAX,
  OP_MIN,
  OP_POW,
  OP_ATAN2,

  OP_LAST
  };


// Compile class
class FXCompile {
  const FXchar *head;           // Start of token
  const FXchar *tail;           // End of token + 1
  const FXchar *vars;           // Variables
  FXuchar      *code;           // Program code
  FXuchar      *pc;             // Program counter
  FXuint        token;          // Token
public:

  // Create compile engine
  FXCompile(FXuchar *prog,const FXchar* ex,const FXchar* vs);

  // Return size of generated code
  FXival size() const { return pc-code; }

  // Get token
  void gettok();

  // Parsing
  FXExpression::Error compile();
  FXExpression::Error expression();
  FXExpression::Error altex();
  FXExpression::Error compex();
  FXExpression::Error shiftexp();
  FXExpression::Error bitexp();
  FXExpression::Error addexp();
  FXExpression::Error mulexp();
  FXExpression::Error powexp();
  FXExpression::Error primary();
  FXExpression::Error element();

  // Variable lookup
  FXint lookup(const FXchar* sym,FXival len) const;

  // Code generation
  FXuchar* opcode(FXuchar op);
  FXuchar* offset(FXival n);
  FXuchar* number(FXdouble num);

  // Backpatch
  void fix(FXuchar *ptr,FXival val);
  };


/*******************************************************************************/

// Construct compile engine
FXCompile::FXCompile(FXuchar *prog,const FXchar* ex,const FXchar* vs):head(ex),tail(ex),vars(vs),code(prog),pc(prog),token(TK_EOF){
  }


// Compile expression
FXExpression::Error FXCompile::compile(){
  FXExpression::Error err;
  FXuchar* at=pc;
  gettok();
  pc+=2;

  FXASSERT_STATIC(OP_LAST<=256);

  // Empty expression
  if(token==TK_EOF) return FXExpression::ErrEmpty;

  // Parse expression
  err=expression();

  // Error occurred
  if(err!=FXExpression::ErrOK) return err;

  // Return from evaluator
  opcode(OP_END);
  FXTRACE((TOPIC_DETAIL,"SIZE=%ld\n",pc-code));

  // Code is too long
  if(code+32767<pc) return FXExpression::ErrLong;

  // Fix up compiled code size
  fix(at,pc-code);

  FXTRACE((TOPIC_DETAIL,"OP_LAST=%d\n",OP_LAST));

  // Not at the end of the expression
  if(token!=TK_EOF) return FXExpression::ErrMore;

  return FXExpression::ErrOK;
  }


// Expression
FXExpression::Error FXCompile::expression(){
  FXExpression::Error err=altex();
  if(err!=FXExpression::ErrOK) return err;
  return FXExpression::ErrOK;
  }


// Parse x?y:z
FXExpression::Error FXCompile::altex(){
  FXExpression::Error err=compex();
  if(err!=FXExpression::ErrOK) return err;
  if(token==TK_QUEST){
    gettok();
    opcode(OP_BRF);
    FXuchar* piff=offset(0);
    err=altex();
    if(err!=FXExpression::ErrOK) return err;
    if(token!=TK_COLON) return FXExpression::ErrColon;
    opcode(OP_BRA);
    FXuchar* pels=offset(0);
    gettok();
    fix(piff,pc-piff);
    err=altex();
    if(err!=FXExpression::ErrOK) return err;
    fix(pels,pc-pels);
    }
  return FXExpression::ErrOK;
  }


// Compare expression
FXExpression::Error FXCompile::compex(){
  FXExpression::Error err=shiftexp();
  if(err!=FXExpression::ErrOK) return err;
  if(TK_LESS<=token && token<=TK_NOTEQUAL){
    FXuint t=token;
    gettok();
    err=shiftexp();
    if(err!=FXExpression::ErrOK) return err;
    if(t==TK_LESS) opcode(OP_LT);
    else if(t==TK_LESSEQ) opcode(OP_LE);
    else if(t==TK_GREATER) opcode(OP_GT);
    else if(t==TK_GREATEREQ) opcode(OP_GE);
    else if(t==TK_EQUAL) opcode(OP_EQ);
    else opcode(OP_NE);
    }
  return FXExpression::ErrOK;
  }


// Shift expression
FXExpression::Error FXCompile::shiftexp(){
  FXExpression::Error err=bitexp();
  if(err!=FXExpression::ErrOK) return err;
  while(TK_SHIFTLEFT<=token && token<=TK_SHIFTRIGHT){
    FXuint t=token;
    gettok();
    err=bitexp();
    if(err!=FXExpression::ErrOK) return err;
    if(t==TK_SHIFTLEFT) opcode(OP_SHL);
    else opcode(OP_SHR);
    }
  return FXExpression::ErrOK;
  }


// Bit expression
FXExpression::Error FXCompile::bitexp(){
  FXExpression::Error err=addexp();
  if(err!=FXExpression::ErrOK) return err;
  while(TK_AND<=token && token<=TK_XOR){
    FXuint t=token;
    gettok();
    err=addexp();
    if(err!=FXExpression::ErrOK) return err;
    if(t==TK_AND) opcode(OP_AND);
    else if(t==TK_OR) opcode(OP_OR);
    else opcode(OP_XOR);
    }
  return FXExpression::ErrOK;
  }


// Add expression
FXExpression::Error FXCompile::addexp(){
  FXExpression::Error err=mulexp();
  if(err!=FXExpression::ErrOK) return err;
  while(TK_PLUS<=token && token<=TK_MINUS){
    FXuint t=token;
    gettok();
    err=mulexp();
    if(err!=FXExpression::ErrOK) return err;
    if(t==TK_MINUS) opcode(OP_SUB);
    else opcode(OP_ADD);
    }
  return FXExpression::ErrOK;
  }


// Mul expression
FXExpression::Error FXCompile::mulexp(){
  FXExpression::Error err=powexp();
  if(err!=FXExpression::ErrOK) return err;
  while(TK_TIMES<=token && token<=TK_MODULO){
    FXuint t=token;
    gettok();
    err=powexp();
    if(err!=FXExpression::ErrOK) return err;
    if(t==TK_TIMES) opcode(OP_MUL);
    else if(t==TK_DIVIDE) opcode(OP_DIV);
    else opcode(OP_MOD);
    }
  return FXExpression::ErrOK;
  }


// Power expression
FXExpression::Error FXCompile::powexp(){
  FXExpression::Error err=primary();
  if(err!=FXExpression::ErrOK) return err;
  if(token==TK_POWER){
    gettok();
    err=powexp();
    if(err!=FXExpression::ErrOK) return err;
    opcode(OP_POW);
    }
  return FXExpression::ErrOK;
  }


// Primary
FXExpression::Error FXCompile::primary(){
  FXExpression::Error err;
  if(token==TK_PLUS || token==TK_MINUS || token==TK_NOT){
    FXuint t=token;
    gettok();
    err=primary();
    if(err!=FXExpression::ErrOK) return err;
    if(t==TK_MINUS) opcode(OP_NEG);
    else if(t==TK_NOT) opcode(OP_NOT);
    }
  else{
    err=element();
    if(err!=FXExpression::ErrOK) return err;
    }
  return FXExpression::ErrOK;
  }


// Element
FXExpression::Error FXCompile::element(){
  FXExpression::Error err;
  FXdouble num;
  FXuchar op;
  FXbool ok;
  FXint v;
  switch(token){
    case TK_LPAR:
      gettok();
      err=expression();
      if(err!=FXExpression::ErrOK) return err;
      if(token!=TK_RPAR) return FXExpression::ErrParent;
      break;
    case TK_INT_HEX:
      num=(FXdouble)__strtoll(head+2,nullptr,16,&ok);
      if(!ok) return FXExpression::ErrToken;
      opcode(OP_NUM);
      number(num);
      break;
    case TK_INT_BIN:
      num=(FXdouble)__strtoll(head+2,nullptr,2,&ok);
      if(!ok) return FXExpression::ErrToken;
      opcode(OP_NUM);
      number(num);
      break;
    case TK_INT_OCT:
      num=(FXdouble)__strtoll(head+1,nullptr,8,&ok);
      if(!ok) return FXExpression::ErrToken;
      opcode(OP_NUM);
      number(num);
      break;
    case TK_INT:
      num=(FXdouble)__strtoll(head,nullptr,10,&ok);
      opcode(OP_NUM);
      if(!ok) return FXExpression::ErrToken;
      number(num);
      break;
    case TK_REAL:
      num=__strtod(head,nullptr,&ok);
      if(!ok) return FXExpression::ErrToken;
      opcode(OP_NUM);
      number(num);
      break;
    case TK_PI:
      opcode(OP_NUM);
      number(PI);
      break;
    case TK_EULER:
      opcode(OP_NUM);
      number(EULER);
      break;
    case TK_RTOD:
      opcode(OP_NUM);
      number(RTOD);
      break;
    case TK_DTOR:
      opcode(OP_NUM);
      number(DTOR);
      break;
    case TK_MAX:
      op=OP_MAX;
      goto dyad;
    case TK_MIN:
      op=OP_MIN;
      goto dyad;
    case TK_POW:
      op=OP_POW;
      goto dyad;
    case TK_ATAN2:
      op=OP_ATAN2;
dyad: gettok();
      if(token!=TK_LPAR) return FXExpression::ErrParent;
      gettok();
      err=expression();
      if(err!=FXExpression::ErrOK) return err;
      if(token!=TK_COMMA) return FXExpression::ErrComma;
      gettok();
      err=expression();
      if(err!=FXExpression::ErrOK) return err;
      if(token!=TK_RPAR) return FXExpression::ErrParent;
      opcode(op);
      break;
    case TK_ABS:
      op=OP_ABS;
      goto mono;
    case TK_ACOS:
      op=OP_ACOS;
      goto mono;
    case TK_ACOSH:
      op=OP_ACOS;
      goto mono;
    case TK_ASIN:
      op=OP_ASIN;
      goto mono;
    case TK_ASINH:
      op=OP_ASINH;
      goto mono;
    case TK_ATAN:
      op=OP_ATAN;
      goto mono;
    case TK_ATANH:
      op=OP_ATANH;
      goto mono;
    case TK_CEIL:
      op=OP_CEIL;
      goto mono;
    case TK_COS:
      op=OP_COS;
      goto mono;
    case TK_COSH:
      op=OP_COSH;
      goto mono;
    case TK_ERF:
      op=OP_ERF;
      goto mono;
    case TK_ERFC:
      op=OP_ERFC;
      goto mono;
    case TK_EXP:
      op=OP_EXP;
      goto mono;
    case TK_EXP2:
      op=OP_EXP2;
      goto mono;
    case TK_EXP10:
      op=OP_EXP10;
      goto mono;
    case TK_FLOOR:
      op=OP_FLOOR;
      goto mono;
    case TK_LOG:
      op=OP_LOG;
      goto mono;
    case TK_LOG2:
      op=OP_LOG2;
      goto mono;
    case TK_LOG10:
      op=OP_LOG10;
      goto mono;
    case TK_INVERF:
      op=OP_INVERF;
      goto mono;
    case TK_INVERFC:
      op=OP_INVERFC;
      goto mono;
    case TK_ISFIN:
      op=OP_ISFIN;
      goto mono;
    case TK_ISINF:
      op=OP_ISINF;
      goto mono;
    case TK_ISNAN:
      op=OP_ISNAN;
      goto mono;
    case TK_NEAR:
      op=OP_NEAR;
      goto mono;
    case TK_SIN:
      op=OP_SIN;
      goto mono;
    case TK_SINH:
      op=OP_SINH;
      goto mono;
    case TK_SQR:
      op=OP_SQR;
      goto mono;
    case TK_CUB:
      op=OP_CUB;
      goto mono;
    case TK_SQRT:
      op=OP_SQRT;
      goto mono;
    case TK_CBRT:
      op=OP_CBRT;
      goto mono;
    case TK_ROUND:
      op=OP_ROUND;
      goto mono;
    case TK_TRUNC:
      op=OP_TRUNC;
      goto mono;
    case TK_TAN:
      op=OP_TAN;
      goto mono;
    case TK_TANH:
      op=OP_TANH;
mono: gettok();
      if(token!=TK_LPAR) return FXExpression::ErrParent;
      gettok();
      err=expression();
      if(err!=FXExpression::ErrOK) return err;
      if(token!=TK_RPAR) return FXExpression::ErrParent;
      opcode(op);
      break;
    case TK_WRAP:
      op=OP_WRAP;
      goto mono;
    case TK_WRAP4:
      op=OP_WRAP4;
      goto mono;
    default:
      v=lookup(head,tail-head);
      if(v<0) return FXExpression::ErrIdent;
      opcode(OP_VAR);
      opcode(v);
      break;
    case TK_EOF:
    case TK_TIMES:
    case TK_DIVIDE:
    case TK_MODULO:
    case TK_POWER:
    case TK_RPAR:
    case TK_LESS:
    case TK_GREATER:
    case TK_LESSEQ:
    case TK_GREATEREQ:
    case TK_EQUAL:
    case TK_NOTEQUAL:
    case TK_AND:
    case TK_OR:
    case TK_XOR:
    case TK_SHIFTLEFT:
    case TK_SHIFTRIGHT:
    case TK_COMMA:
    case TK_ERROR:
    case TK_QUEST:
    case TK_COLON:
      return FXExpression::ErrToken;
    }
  gettok();
  return FXExpression::ErrOK;
  }


// Lookup symbol
FXint FXCompile::lookup(const FXchar* sym,FXival len) const{
  if(vars){
    const FXchar* beg=vars;
    const FXchar* end=vars;
    FXint which=0;
    while(*end){
      beg=end;
      while(*end && *end!=',') end++;
      if((end-beg)==len){
        if(equalElms(sym,beg,len)) return which;
        }
      if(*end==',') end++;
      which++;
      }
    }
  return -1;
  }


// Obtain next token from input
void FXCompile::gettok(){
  FXchar c;
  head=tail;
  while((c=*tail)!='\0'){
    switch(c){
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        head=++tail;
        break;
      case '=':
        token=TK_ERROR; tail++;
        if(*tail=='='){ token=TK_EQUAL; tail++; }
        return;
      case '<':
        token=TK_LESS; tail++;
        if(*tail=='='){ token=TK_LESSEQ; tail++; }
        else if(*tail=='<'){ token=TK_SHIFTLEFT; tail++; }
        return;
      case '>':
        token=TK_GREATER;
        tail++;
        if(*tail=='='){ token=TK_GREATEREQ; tail++; }
        else if(*tail=='>'){ token=TK_SHIFTRIGHT; tail++; }
        return;
      case '|':
        token=TK_OR; tail++;
        return;
      case '&':
        token=TK_AND; tail++;
        return;
      case '^':
        token=TK_XOR; tail++;
        return;
      case '~':
        token=TK_NOT; tail++;
        return;
      case '-':
        token=TK_MINUS; tail++;
        return;
      case '+':
        token=TK_PLUS; tail++;
        return;
      case '*':
        token=TK_TIMES; tail++;
        if(*tail=='*'){ token=TK_POWER; tail++; }
        return;
      case '/':
        token=TK_DIVIDE; tail++;
        return;
      case '%':
        token=TK_MODULO; tail++;
        return;
      case '!':
        token=TK_ERROR; tail++;
        if(*tail=='='){ token=TK_NOTEQUAL; tail++; }
        return;
      case '(':
        token=TK_LPAR; tail++;
        return;
      case ')':
        token=TK_RPAR; tail++;
        return;
      case ',':
        token=TK_COMMA; tail++;
        return;
      case '?':
        token=TK_QUEST; tail++;
        return;
      case ':':
        token=TK_COLON; tail++;
        return;
      case '.':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        token=TK_INT;
        if(c=='0'){
          tail++;
          if(*tail=='x' || *tail=='X'){
            tail++;
            if(!Ascii::isHexDigit(*tail)){ token=TK_ERROR; return; }
            tail++;
            while(Ascii::isHexDigit(*tail)) tail++;
            token=TK_INT_HEX;
            return;
            }
          if(*tail=='b' || *tail=='B'){
            tail++;
            if(*tail!='0' && *tail!='1'){ token=TK_ERROR; return; }
            tail++;
            while(*tail=='0' || *tail=='1') tail++;
            token=TK_INT_BIN;
            return;
            }
          if('0'<=*tail && *tail<='7'){
            tail++;
            while('0'<=*tail && *tail<='7') tail++;
            if('7'<=*tail && *tail<='9'){
              token=TK_ERROR;
              return;
              }
            token=TK_INT_OCT;
            return;
            }
          }
        while(Ascii::isDigit(*tail)) tail++;
        if(*tail=='.'){
          token=TK_REAL;
          tail++;
          while(Ascii::isDigit(*tail)) tail++;
          }
        if(*tail=='e' || *tail=='E'){
          token=TK_REAL;
          tail++;
          if(*tail=='-' || *tail=='+') tail++;
          if(!Ascii::isDigit(*tail)){ token=TK_ERROR; return; }
          tail++;
          while(Ascii::isDigit(*tail)) tail++;
          }
        return;
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
      case 'g':
      case 'h':
      case 'i':
      case 'j':
      case 'k':
      case 'l':
      case 'm':
      case 'n':
      case 'o':
      case 'p':
      case 'q':
      case 'r':
      case 's':
      case 't':
      case 'u':
      case 'v':
      case 'w':
      case 'x':
      case 'y':
      case 'z':
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
      case 'G':
      case 'H':
      case 'I':
      case 'J':
      case 'K':
      case 'L':
      case 'M':
      case 'N':
      case 'O':
      case 'P':
      case 'Q':
      case 'R':
      case 'S':
      case 'T':
      case 'U':
      case 'V':
      case 'W':
      case 'X':
      case 'Y':
      case 'Z':
      case '_':
        token=(FXuchar)*tail++;
        while(Ascii::isWord(*tail)){
          token=((token<<5)+token) ^ (FXuchar)*tail++;
          }
        //FXTRACE((1,"token=%u\n",token));
        return;
      default:
        token=TK_ERROR;
        return;
      }
    }
  token=TK_EOF;
  }


// Emit opcode
FXuchar* FXCompile::opcode(FXuchar op){
  FXuchar* result=pc;
  if(code){
    pc[0]=op;
    }
  pc++;
  return result;
  }


// Emit offset
FXuchar* FXCompile::offset(FXival n){
  FXuchar* result=pc;
  if(code){
    SETARG(pc,n);
    }
  pc+=2;
  return result;
  }


// Emit double
FXuchar* FXCompile::number(FXdouble n){
  FXuchar* result=pc;
  if(code){
#if defined(__i386__) || defined(__x86_64__) || defined(WIN32) || defined(__minix)
    ((FXdouble*)pc)[0]=n;
#else
    pc[0]=((const FXuchar*)&n)[0];
    pc[1]=((const FXuchar*)&n)[1];
    pc[2]=((const FXuchar*)&n)[2];
    pc[3]=((const FXuchar*)&n)[3];
    pc[4]=((const FXuchar*)&n)[4];
    pc[5]=((const FXuchar*)&n)[5];
    pc[6]=((const FXuchar*)&n)[6];
    pc[7]=((const FXuchar*)&n)[7];
#endif
    }
  pc+=8;
  return result;
  }


// Fix value
void FXCompile::fix(FXuchar *ptr,FXival val){
  if(code && ptr){
    SETARG(ptr,val);
    }
  }

}

/*******************************************************************************/

// Default expression (returns 0)
#if (FOX_BIGENDIAN == 1)
const FXuchar FXExpression::initial[]={0,14,OP_NUM,0,0,0,0,0,0,0,0,OP_END};
#endif
#if (FOX_BIGENDIAN == 0)
const FXuchar FXExpression::initial[]={14,0,OP_NUM,0,0,0,0,0,0,0,0,OP_END};
#endif


// Empty expression
#define EMPTY (const_cast<FXuchar*>(FXExpression::initial))


// Error messages
const FXchar *const FXExpression::errors[]={
  "OK",
  "Empty expression",
  "Extra characters at end",
  "Out of memory",
  "Unmatched parenthesis",
  "Illegal token",
  "Expected comma",
  "Unknown identifier",
  "Expected colon",
  "Expression too long",
  };


// Construct empty expression object
FXExpression::FXExpression():code(EMPTY){
  FXTRACE((TOPIC_CONSTRUCT,"FXExpression::FXExpression()\n"));
  }


// Copy regex object
FXExpression::FXExpression(const FXExpression& orig):code(EMPTY){
  FXTRACE((TOPIC_CONSTRUCT,"FXExpression::FXExpression(FXExpression)\n"));
  if(orig.code!=initial){
    dupElms(code,orig.code,GETARG(orig.code));
    }
  }


// Compile expression from pattern; fail if error
FXExpression::FXExpression(const FXchar* expression,const FXchar* variables,FXExpression::Error* error):code(EMPTY){
  FXTRACE((TOPIC_CONSTRUCT,"FXExpression::FXExpression(%s,%s,%p)\n",expression,variables,error));
  FXExpression::Error err=parse(expression,variables);
  if(error){ *error=err; }
  }


// Compile expression from pattern; fail if error
FXExpression::FXExpression(const FXString& expression,const FXString& variables,FXExpression::Error* error):code(EMPTY){
  FXTRACE((TOPIC_CONSTRUCT,"FXExpression::FXExpression(%s,%s,%p)\n",expression.text(),variables.text(),error));
  FXExpression::Error err=parse(expression.text(),variables.text());
  if(error){ *error=err; }
  }


// Assignment
FXExpression& FXExpression::operator=(const FXExpression& orig){
  if(code!=orig.code){
    if(code!=initial) freeElms(code);
    code=EMPTY;
    if(orig.code!=initial){
      dupElms(code,orig.code,GETARG(orig.code));
      }
    }
  return *this;
  }

/*******************************************************************************/

#ifdef TOPIC_EXPDUMP
#include "fxexprdbg.h"
#endif


// Parse expression, return error code if syntax error is found
FXExpression::Error FXExpression::parse(const FXchar* expression,const FXchar* variables){
  FXExpression::Error err=FXExpression::ErrEmpty;

  FXTRACE((TOPIC_DETAIL,"Expression::parse(%s,%p)\n",expression,variables));

  // Free old code, if any
  clear();

  // If not empty, parse expression
  if(expression){

    // Create compile engine
    FXCompile cs(nullptr,expression,variables);

    // Parse to check syntax and determine size
    if((err=cs.compile())==ErrOK){
      FXuchar *prog;

      // Allocate new code
      if(allocElms(prog,cs.size())){

        // Create compile engine
        FXCompile gs(prog,expression,variables);

        // Now generate code for expression
        if((err=gs.compile())==ErrOK){

          // Size still checking out?
          FXASSERT(gs.size()==cs.size());

          // Install new program
          code=prog;

#ifdef TOPIC_EXPDUMP
          if(getTraceTopic(TOPIC_EXPDUMP)){ dump(code); }
#endif

          // Report success
          return ErrOK;
          }
        freeElms(prog);
        }
      }
    }
  return err;
  }


// Parse expression, return error code if syntax error is found
FXExpression::Error FXExpression::parse(const FXString& expression,const FXString& variables){
  return parse(expression.text(),variables.text());
  }


// Evaluate expression
FXdouble FXExpression::evaluate(const FXdouble *args) const {
  FXdouble stack[MAXSTACKDEPTH];
  const FXuchar *pc=code+2;
  FXdouble *sp=stack;
  stack[0]=0.0;
  while(1){
    switch(*pc++){
      case OP_END:
        return *sp;
      case OP_BRA:
        pc+=GETARG(pc);
        break;
      case OP_BRF:
        pc+=*sp-- ? 2 : GETARG(pc);
        break;
      case OP_BRT:
        pc+=*sp-- ? GETARG(pc) : 2;
        break;
#if defined(__i386__) || defined(__x86_64__) || defined(WIN32) || defined(__minix)
      case OP_NUM:
        *++sp=*((const FXdouble*)pc);
        pc+=8;
        break;
#else
      case OP_NUM:
        ++sp;
        ((FXuchar*)sp)[0]=*pc++;
        ((FXuchar*)sp)[1]=*pc++;
        ((FXuchar*)sp)[2]=*pc++;
        ((FXuchar*)sp)[3]=*pc++;
        ((FXuchar*)sp)[4]=*pc++;
        ((FXuchar*)sp)[5]=*pc++;
        ((FXuchar*)sp)[6]=*pc++;
        ((FXuchar*)sp)[7]=*pc++;
        break;
#endif
      case OP_VAR:
        *++sp=args[*pc++];
        break;
      case OP_NOT:
        *sp=(FXdouble)(~((FXlong)*sp));
        break;
      case OP_NEG:
        *sp=-*sp;
        break;
      case OP_ABS:
        *sp=Math::fabs(*sp);
        break;
      case OP_SIN:
        *sp=Math::sin(*sp);
        break;
      case OP_COS:
        *sp=Math::cos(*sp);
        break;
      case OP_TAN:
        *sp=Math::tan(*sp);
        break;
      case OP_ASIN:
        *sp=Math::asin(*sp);
        break;
      case OP_ACOS:
        *sp=Math::acos(*sp);
        break;
      case OP_ATAN:
        *sp=Math::atan(*sp);
        break;
      case OP_SINH:
        *sp=Math::sinh(*sp);
        break;
      case OP_COSH:
        *sp=Math::cosh(*sp);
        break;
      case OP_TANH:
        *sp=Math::tanh(*sp);
        break;
      case OP_ASINH:
        *sp=Math::asinh(*sp);
        break;
      case OP_ACOSH:
        *sp=Math::acosh(*sp);
        break;
      case OP_ATANH:
        *sp=Math::atanh(*sp);
        break;
      case OP_SQR:
        *sp=Math::sqr(*sp);
        break;
      case OP_CUB:
        *sp=Math::cub(*sp);
        break;
      case OP_SQRT:
        *sp=Math::sqrt(*sp);
        break;
      case OP_CBRT:
        *sp=Math::cbrt(*sp);
        break;
      case OP_CEIL:
        *sp=Math::ceil(*sp);
        break;
      case OP_FLOOR:
        *sp=Math::floor(*sp);
        break;
      case OP_NEAR:
        *sp=Math::nearbyint(*sp);
        break;
      case OP_ROUND:
        *sp=Math::round(*sp);
        break;
      case OP_TRUNC:
        *sp=Math::trunc(*sp);
        break;
      case OP_ERF:
        *sp=Math::erf(*sp);
        break;
      case OP_ERFC:
        *sp=Math::erfc(*sp);
        break;
      case OP_INVERF:
        *sp=Math::inverf(*sp);
        break;
      case OP_INVERFC:
        *sp=Math::inverfc(*sp);
        break;
      case OP_EXP:
        *sp=Math::exp(*sp);
        break;
      case OP_EXP2:
        *sp=Math::exp2(*sp);
        break;
      case OP_EXP10:
        *sp=Math::exp10(*sp);
        break;
      case OP_LOG:
        *sp=Math::log(*sp);
        break;
      case OP_LOG2:
        *sp=Math::log2(*sp);
        break;
      case OP_LOG10:
        *sp=Math::log10(*sp);
        break;
      case OP_ISFIN:
        *sp=(FXdouble)Math::fpFinite(*sp);
        break;
      case OP_ISINF:
        *sp=(FXdouble)Math::fpInfinite(*sp);
        break;
      case OP_ISNAN:
        *sp=(FXdouble)Math::fpNan(*sp);
        break;
      case OP_WRAP:
        *sp=Math::wrap(*sp);
        break;
      case OP_WRAP4:
        *sp=Math::wrap4(*sp);
        break;
      case OP_MUL:
        *(sp-1)=*(sp-1) * *sp;
        --sp;
        break;
      case OP_DIV:
        *(sp-1)=*(sp-1) / *sp;
        --sp;
        break;
      case OP_MOD:
        *(sp-1)=Math::fmod(*(sp-1),*sp);
        --sp;
        break;
      case OP_ADD:
        *(sp-1)=*(sp-1) + *sp;
        --sp;
        break;
      case OP_SUB:
        *(sp-1)=*(sp-1) - *sp;
        --sp;
        break;
      case OP_AND:
        *(sp-1)=(FXdouble)(((FXlong)*(sp-1)) & ((FXlong)*sp));
        --sp;
        break;
      case OP_OR:
        *(sp-1)=(FXdouble)(((FXlong)*(sp-1)) | ((FXlong)*sp));
        --sp;
        break;
      case OP_XOR:
        *(sp-1)=(FXdouble)(((FXlong)*(sp-1)) ^ ((FXlong)*sp));
        --sp;
        break;
      case OP_SHL:
         *(sp-1)=(FXdouble)(((FXlong)*(sp-1)) << ((FXlong)*sp));
         --sp;
         break;
      case OP_SHR:
        *(sp-1)=(FXdouble)(((FXlong)*(sp-1)) >> ((FXlong)*sp));
        --sp;
        break;
      case OP_LT:
         *(sp-1)=(FXdouble)(*(sp-1) < *sp);
         --sp;
         break;
      case OP_GT:
        *(sp-1)=(FXdouble)(*(sp-1) > *sp);
        --sp;
         break;
      case OP_LE:
        *(sp-1)=(FXdouble)(*(sp-1) <= *sp);
        --sp;
        break;
      case OP_GE:
        *(sp-1)=(FXdouble)(*(sp-1) >= *sp);
        --sp;
        break;
      case OP_EQ:
        *(sp-1)=(FXdouble)(*(sp-1) == *sp);
        --sp;
        break;
      case OP_NE:
        *(sp-1)=(FXdouble)(*(sp-1) != *sp);
        --sp;
        break;
      case OP_POW:
        *(sp-1)=Math::pow(*(sp-1),*sp);
        --sp;
        break;
      case OP_MAX:
        *(sp-1)=Math::fmax(*(sp-1),*sp);
        --sp;
        break;
      case OP_MIN:
        *(sp-1)=Math::fmin(*(sp-1),*sp);
        --sp;
        break;
      case OP_ATAN2:
        *(sp-1)=Math::atan2(*(sp-1),*sp);
        --sp;
        break;
      }
    }
  return 0.0;
  }


// Save
FXStream& operator<<(FXStream& store,const FXExpression& s){
  FXshort size=GETARG(s.code);
  store << size;
  store.save(s.code+2,size-2);
  return store;
  }


// Load
FXStream& operator>>(FXStream& store,FXExpression& s){
  FXshort size;
  store >> size;
  allocElms(s.code,size);
  store.load(s.code+2,size-2);
  SETARG(s.code,size);
  return store;
  }


// Clear the expression
void FXExpression::clear(){
  if(code!=EMPTY){
    freeElms(code);
    code=EMPTY;
    }
  }


// Clean up
FXExpression::~FXExpression(){
  FXTRACE((TOPIC_CONSTRUCT,"FXExpression::~FXExpression()\n"));
  clear();
  }

}
