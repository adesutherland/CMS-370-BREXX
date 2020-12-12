/*
 * $Id: expr.c,v 1.8 2008/07/15 07:40:25 bnv Exp $
 * $Log: expr.c,v $
 * Revision 1.8  2008/07/15 07:40:25  bnv
 * #include changed from <> to ""
 *
 * Revision 1.7  2004/08/16 15:28:54  bnv
 * Changed: name of mnemonic operands from xxx_mn to O_XXX
 *
 * Revision 1.6  2004/03/27 08:34:07  bnv
 * Nothing
 *
 * Revision 1.5  2003/10/30 13:16:28  bnv
 * Variable name change
 *
 * Revision 1.4  2002/06/11 12:37:38  bnv
 * Added: CDECL
 *
 * Revision 1.3  2001/06/25 18:51:48  bnv
 * Header -> Id
 *
 * Revision 1.2  1999/11/26 13:13:47  bnv
 * Some spaces changed to tabs.
 *
 * Revision 1.1  1998/07/02 17:34:50  bnv
 * Initial revision
 *
 */

#include <cmssys.h>
#include "lerror.h"
#include "lstring.h"

#include "rexx.h"
#include "trace.h"
#include "compile.h"

/*-----------------* local function prototypes *------------ */
static void __CDECL Exp0(void);

static void __CDECL Exp1(void);

static void __CDECL Exp2(void);

static void __CDECL Exp3(void);

static void __CDECL Exp4(void);

static void __CDECL Exp5(void);

static void __CDECL Exp6(void);

static void __CDECL Exp7(void);

static void __CDECL Exp8(void);

static void __CDECL C_function(void);

/* ========================= C_expr ========================== */
/* return if it had exited with another code than OP_COPY */
/* so something is left in stack */
int __CDECL
C_expr(int calltype) {
    Context *context = (Context *) CMSGetPG();
    (context->expr_exp_ct) = calltype;
    (context->expr_exp_pos) = (context->compileCompileCodeLen);
    Exp0();

    /* If nothing was processed in the expr then push a Null string */
    if ((context->expr_exp_pos) == (context->compileCompileCodeLen)) {
        _CodeAddByte(OP_PUSH);
        _CodeAddPtr(&((context->rexxnullStr)->key));
        TraceByte(nothing_middle);
    }
    switch ((context->expr_exp_ct)) {
        case exp_assign:
            _CodeAddByte(OP_COPY);
            return FALSE;

        case exp_tmp:
            _CodeAddByte(OP_COPY2TMP);
            break;

        case exp_normal:
            /* do nothing */
            break;

        default:
            (context->lstring_Lerror)(ERR_INTERPRETER_FAILURE, 0);
    }
    return TRUE;
} /* C_expr */

/* ---------------- InsTmp ---------------- */
static int __CDECL
InsTmp(size_t pos, int pushtmp) {
    Context *context = (Context *) CMSGetPG();
    /* do we need to push a tmp value */
    if (((context->expr_exp_ct) == exp_assign) &&
        ((context->expr_exp_pos) == pos)) {
        /* because we have return the value in a temporary var */
        /* change the type to exp_normal   */
        if (pushtmp)
            (context->expr_exp_ct) = exp_normal;
    } else {
        if (pushtmp)
            _CodeInsByte(pos, OP_PUSHTMP);
        else
            return TRUE;
    }
    return FALSE;
} /* InsTmp */

/* ----------------- Exp0 ----------------- */
static void __CDECL
Exp0(void) {
    enum mnemonic_type orxor;
    CTYPE pos, pos2;
    Context *context = (Context *) CMSGetPG();

    pos = (context->compileCompileCodeLen);
    Exp1();
    orxor = ((context->nextsymbsymbol) == xor_sy) ? OP_XOR : OP_OR;
    while (((context->nextsymbsymbol) == or_sy) ||
           ((context->nextsymbsymbol) == xor_sy)) {  /* Logical OR; XOR */
        if ((context->compileCompileCodeLen) == pos)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
        nextsymbol();
        pos2 = (context->compileCompileCodeLen);
        Exp1();
        if ((context->compileCompileCodeLen) == pos2)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
        InsTmp(pos, TRUE);
        _CodeAddByte(orxor);
        TraceByte(operator_middle);
        orxor = ((context->nextsymbsymbol) == xor_sy) ? OP_XOR : OP_OR;
    }
} /* Exp0 */

/* ----------------- Exp1 ----------------- */
static void __CDECL
Exp1(void) {
    CTYPE pos, pos2;
    Context *context = (Context *) CMSGetPG();

    pos = (context->compileCompileCodeLen);
    Exp2();
    while ((context->nextsymbsymbol) == and_sy) { /* Logical AND  */
        if ((context->compileCompileCodeLen) == pos)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
        nextsymbol();
        pos2 = (context->compileCompileCodeLen);
        Exp2();
        if ((context->compileCompileCodeLen) == pos2)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
        InsTmp(pos, TRUE);
        _CodeAddByte(OP_AND);
        TraceByte(operator_middle);
    }
} /* Exp1 */

/* ----------------- Exp2 ----------------- */
static void __CDECL
Exp2(void) {
    enum symboltype _symbol;
    CTYPE pos, pos2;
    Context *context = (Context *) CMSGetPG();

    pos = (context->compileCompileCodeLen);
    Exp3();
    _symbol = (context->nextsymbsymbol);

    /* DO NOT CHANGE THE ORDER OF THIS SYMBOLS */
    if (((context->nextsymbsymbol) >= eq_sy) &&
        ((context->nextsymbsymbol) <= dgt_sy)) {
        if ((context->compileCompileCodeLen) == pos)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
        nextsymbol();
        pos2 = (context->compileCompileCodeLen);
        Exp3();
        if ((context->compileCompileCodeLen) == pos2)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
/****
//  if (!InsTmp(pos,FALSE)) { * do we need to add a pushtmp *
*****/
        InsTmp(pos, TRUE); /* add the pushtmp byte */
        switch (_symbol) {
            case eq_sy  :
                _CodeAddByte(OP_EQ);
                break;
            case ne_sy  :
                _CodeAddByte(OP_NE);
                break;
            case gt_sy  :
                _CodeAddByte(OP_GT);
                break;
            case ge_sy  :
                _CodeAddByte(OP_GE);
                break;
            case lt_sy  :
                _CodeAddByte(OP_LT);
                break;
            case le_sy  :
                _CodeAddByte(OP_LE);
                break;
            case deq_sy :
                _CodeAddByte(OP_DEQ);
                break;
            case dne_sy :
                _CodeAddByte(OP_DNE);
                break;
            case dgt_sy :
                _CodeAddByte(OP_DGT);
                break;
            case dge_sy :
                _CodeAddByte(OP_DGE);
                break;
            case dlt_sy :
                _CodeAddByte(OP_DLT);
                break;
            case dle_sy :
                _CodeAddByte(OP_DLE);
                break;
            default:
                (context->lstring_Lerror)(ERR_INTERPRETER_FAILURE, 0);
        }
/*****
//  } else {
//   switch (_symbol) {
//    case eq_sy  : _CodeAddByte(OP_TEQ);   break;
//    case ne_sy  : _CodeAddByte(OP_TNE);   break;
//    case gt_sy  : _CodeAddByte(OP_TGT);   break;
//    case ge_sy  : _CodeAddByte(OP_TGE);   break;
//    case lt_sy  : _CodeAddByte(OP_TLT);   break;
//    case le_sy  : _CodeAddByte(OP_TLE);   break;
//    case deq_sy : _CodeAddByte(OP_TDEQ);  break;
//    case dne_sy : _CodeAddByte(OP_TDNE);  break;
//    case dgt_sy : _CodeAddByte(OP_TDGT);  break;
//    case dge_sy : _CodeAddByte(OP_TDGE);  break;
//    case dlt_sy : _CodeAddByte(OP_TDLT);  break;
//    case dle_sy : _CodeAddByte(OP_TDLE);  break;
//    default:
//     (context->lstring_Lerror)(ERR_INTERPRETER_FAILURE,0);
//   }
//  }
*****/
        TraceByte(operator_middle);
    }
} /* Exp2 */

/* ----------------- Exp3 ----------------- */
static void __CDECL
Exp3(void) {
    int _Concat;
    int _Pblank;
    CTYPE pos, pos2;
    Context *context = (Context *) CMSGetPG();

    pos = (context->compileCompileCodeLen);
    Exp4();
    _Concat = ((context->nextsymbsymbol) == concat_sy);
    _Pblank = (context->nextsymbsymbolPrevBlank);

    /* UNTIL NOT_SY there must be prefix and starting '(' operators */
    while (((context->nextsymbsymbol) <= not_sy) || _Concat) {
        if ((context->compileCompileCodeLen) == pos)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
        if (_Concat) nextsymbol();
        pos2 = (context->compileCompileCodeLen);
        Exp4();
        if ((context->compileCompileCodeLen) == pos2)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
        InsTmp(pos, TRUE); /* add the pushtmp byte */
        if (_Concat || !_Pblank)
            _CodeAddByte(OP_CONCAT);
        else
            _CodeAddByte(OP_BCONCAT);

        TraceByte(operator_middle);

        _Concat = ((context->nextsymbsymbol) == concat_sy);
        _Pblank = (context->nextsymbsymbolPrevBlank);
    }
} /* Exp3 */

/* ----------------- Exp4 ----------------- */
static void __CDECL
Exp4(void) {
    enum symboltype _symbol;
    CTYPE pos, pos2;
    Context *context = (Context *) CMSGetPG();

    pos = (context->compileCompileCodeLen);
    Exp5();
    _symbol = (context->nextsymbsymbol);

    while (((context->nextsymbsymbol) == plus_sy) ||
           ((context->nextsymbsymbol) == minus_sy)) {
        if ((context->compileCompileCodeLen) == pos)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
        nextsymbol();
        pos2 = (context->compileCompileCodeLen);
        Exp5();
        if ((context->compileCompileCodeLen) == pos2)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
        InsTmp(pos, TRUE);
        if (_symbol == plus_sy)
            _CodeAddByte(OP_ADD);
        else
            _CodeAddByte(OP_SUB);
        TraceByte(operator_middle);
        _symbol = (context->nextsymbsymbol);
    }
} /* Exp4 */

/* ----------------- Exp5 ----------------- */
static void __CDECL
Exp5(void) {
    enum symboltype _symbol;
    CTYPE pos, pos2;
    Context *context = (Context *) CMSGetPG();

    pos = (context->compileCompileCodeLen);
    Exp6();
    _symbol = (context->nextsymbsymbol);

    /* ORDER IS IMPORTANT */
    while (((context->nextsymbsymbol) >= times_sy) &&
           ((context->nextsymbsymbol) <= intdiv_sy)) {
        if ((context->compileCompileCodeLen) == pos)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
        nextsymbol();
        pos2 = (context->compileCompileCodeLen);
        Exp6();
        if ((context->compileCompileCodeLen) == pos2)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
        InsTmp(pos, TRUE);
        switch (_symbol) {
            case times_sy  :
                _CodeAddByte(OP_MUL);
                break;
            case div_sy    :
                _CodeAddByte(OP_DIV);
                break;
            case intdiv_sy :
                _CodeAddByte(OP_IDIV);
                break;
            case mod_sy    :
                _CodeAddByte(OP_MOD);
                break;
            default:
                (context->lstring_Lerror)(ERR_INTERPRETER_FAILURE, 0);
        }
        TraceByte(operator_middle);
        _symbol = (context->nextsymbsymbol);
    }
}  /* Exp5 */

/* ----------------- Exp6 ----------------- */
static void __CDECL
Exp6(void) {
    CTYPE pos, pos2;
    Context *context = (Context *) CMSGetPG();

    pos = (context->compileCompileCodeLen);
    Exp7();
    while ((context->nextsymbsymbol) == power_sy) {
        if ((context->compileCompileCodeLen) == pos)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
        nextsymbol();
        pos2 = (context->compileCompileCodeLen);
        Exp7();
        if ((context->compileCompileCodeLen) == pos2)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
        InsTmp(pos, TRUE);
        _CodeAddByte(OP_POW);
        TraceByte(operator_middle);
    }
} /* Exp6 */

/* ----------------- Exp7 ----------------- */
static void __CDECL
Exp7(void) {
    enum symboltype _symbol;
    int prefix;
    CTYPE pos;
    Context *context = (Context *) CMSGetPG();

    pos = (context->compileCompileCodeLen);
    _symbol = (context->nextsymbsymbol);

    if (((context->nextsymbsymbol) == not_sy) ||
        ((context->nextsymbsymbol) == minus_sy)) {
        nextsymbol();
        prefix = TRUE;
    } else
        prefix = FALSE;

    if (!prefix && ((context->nextsymbsymbol) == plus_sy))
        nextsymbol();

    Exp8();
    if (prefix) {
        if ((context->compileCompileCodeLen) == pos)
            (context->lstring_Lerror)(ERR_INVALID_EXPRESSION, 0);
        InsTmp(pos, TRUE);
        if (_symbol == not_sy)
            _CodeAddByte(OP_NOT);
        else
            _CodeAddByte(OP_NEG);
        TraceByte(operator_middle);
    }
} /* Exp7 */

/* ----------------- Exp8 ----------------- */
static void __CDECL
Exp8(void) {
    Context *context = (Context *) CMSGetPG();
    if ((context->nextsymbsymbol) == ident_sy) {
        _CodeAddByte(OP_LOAD);
        _CodeAddPtr(SYMBOLADD2LITS);
        TraceByte(variable_middle);
        nextsymbol();
    } else if ((context->nextsymbsymbol) == literal_sy) {
        _CodeAddByte(OP_PUSH);
        _CodeAddPtr(SYMBOLADD2LITS_KEY);
        TraceByte(litteral_middle);
        nextsymbol();
    } else if ((context->nextsymbsymbol) == function_sy) {
        C_function();
        _mustbe(ri_parent, ERR_UNMATCHED_PARAN, 0);
    } else if ((context->nextsymbsymbol) == le_parent) {
        nextsymbol();
        Exp0();
        _mustbe(ri_parent, ERR_UNMATCHED_PARAN, 0);
    }
} /* Exp8 */

/* -------------------------------------------------------------- */
/*  [Function] ::= [Identifier]([[Expression][,[Expression]]...]) */
/* -------------------------------------------------------------- */
static void __CDECL
C_function(void) {
    int ia, line, realarg = 0;
    word existarg = 0, bp = 1, lastarg; /* bit mapped, if arguments exist */
    void *lbl;
    Context *context = (Context *) CMSGetPG();

    line = (context->nextsymbsymboline); /* keep line number */

    lbl = _AddLabel(FT_FUNCTION, UNKNOWN_LABEL);

    /* add a space in stack, for the result string */
    if (((context->expr_exp_ct) == exp_assign) &&
        ((context->expr_exp_pos) == (context->compileCompileCodeLen)))
        (context->expr_exp_ct) = exp_normal;
    else
        _CodeAddByte(OP_PUSHTMP);

    nextsymbol();
    ia = lastarg = 0;

    if ((context->nextsymbsymbol) != ri_parent) {
        if ((context->nextsymbsymbol) != comma_sy) {
            Exp0();
            realarg++;
            lastarg = ia + 1;
            existarg |= bp; /* argument exist */
        }
        ia++;
        bp <<= 1; /* increase bit position */

        while ((context->nextsymbsymbol) == comma_sy) {
            nextsymbol();
            if (ia >= MAXARGS)
                (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
            if (!(((context->nextsymbsymbol) == comma_sy) ||
                  ((context->nextsymbsymbol) == ri_parent))) {
                Exp0();
                lastarg = ia + 1;
                realarg++;
                existarg |= bp;
            }
            ia++;
            bp <<= 1; /* increase bit position */
        }
    }

    _CodeAddByte(OP_CALL);
    _CodeAddPtr(lbl); /* call pointer */
    _CodeAddByte(lastarg); /* arguments */
    _CodeAddByte(realarg); /* real args */
    _CodeAddWord(existarg); /* which exist */
    _CodeAddWord(line); /* (context->nextsymbsymbol) line */
    _CodeAddByte(CT_FUNCTION); /* call type */
    TraceByte(function_middle);
} /* C_function */
