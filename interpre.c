/* Modified for VM/370 CMS and GCC by Robert O'Hara, July 2010. */
/*
 * $Id: interpre.c,v 1.22 2009/06/02 09:41:27 bnv Exp $
 * $Log: interpre.c,v $
 * Revision 1.22  2009/06/02 09:41:27  bnv
 * MVS/CMS corrections
 *
 * Revision 1.21  2009/02/02 09:25:52  bnv
 * Modifications for CMS/MVS
 *
 * Revision 1.20  2008/07/15 07:40:25  bnv
 * #include changed from <> to ""
 *
 * Revision 1.19  2008/07/14 13:08:42  bnv
 * MVS,CMS support
 *
 * Revision 1.18  2006/01/26 10:25:11  bnv
 * Added: Indirect exposure to variables
 *
 * Revision 1.17  2004/08/16 15:28:54  bnv
 * Changed: name of mnemonic operands from xxx_mn to O_XXX
 *
 * Revision 1.16  2004/04/30 15:27:19  bnv
 * Type changes
 *
 * Revision 1.15  2004/03/27 08:34:21  bnv
 * Corrected: SIGNAL VALUE was cleaning the stack before the call
 *
 * Revision 1.14  2003/10/30 13:16:28  bnv
 * Variable name change
 *
 * Revision 1.13  2002/08/22 12:31:28  bnv
 * Removed CR's
 *
 * Revision 1.12  2002/07/03 13:15:08  bnv
 * Changed: Version define
 *
 * Revision 1.11  2002/06/11 12:37:38  bnv
 * Added: CDECL
 *
 * Revision 1.10  2001/09/28 10:00:39  bnv
 * Added: Quotes arround the arguments of a system-function call
 *
 * Revision 1.9  2001/06/25 18:50:56  bnv
 * Added: Memory check in debug version at the end of Interpretation
 *
 * Revision 1.8  1999/11/26 13:13:47  bnv
 * Added: Windows CE support
 * Changed: To use the new macros.
 *
 * Revision 1.7  1999/06/10 14:08:35  bnv
 * When a called procedure with local variables returne a variable
 * the variable contents was freed first before copied to the RESULT.
 *
 * Revision 1.6  1999/05/14 12:31:22  bnv
 * Minor changes
 *
 * Revision 1.5  1999/03/15 15:21:36  bnv
 * Corrected to handle the error_trace
 *
 * Revision 1.4  1999/03/15 09:01:57  bnv
 * Corrected: error_trace
 *
 * Revision 1.3  1999/03/10 16:53:32  bnv
 * Added MSC support
 *
 * Revision 1.2  1999/03/01 10:54:37  bnv
 * Corrected: To clean correctly the RxStck after an interpret_mn
 *
 * Revision 1.1  1998/07/02 17:34:50  bnv
 * Initial revision
 *
 */

#define __INTERPRET_C__

#ifndef WCE

# include <stdio.h>
# include <signal.h>

#endif

#include <cmssys.h>

#include "lerror.h"
#include "lstring.h"

#include "rexx.h"
#include "stack.h"
#include "trace.h"
#include "bintree.h"
#include "compile.h"
#include "interpre.h"
#include "context.h"

#ifdef WCE
# define MAX_EVENT_COUNT 50
#endif

/*void ProcessInterrupt();*/

#define STACKTOP (context->interpre_RxStck)[(context->interpre_RxStckTop)]
#define STACKP(i) (context->interpre_RxStck)[(context->interpre_RxStckTop)-(i)]

#ifdef __DEBUG__
# define DEBUGDISPLAY0(a)  if ((context->rexx__debug__)) printf("\t%u\t%s\n",inst_ip,(a))
# define DEBUGDISPLAY0nl(a) if ((context->rexx__debug__)) printf("\t%u\t%s\t",inst_ip,(a))
# define DEBUGDISPLAY(a)  if ((context->rexx__debug__)) {printf("\t%u\t%s\t\"",inst_ip,(a)); \
     Lprint(STDOUT,(context->interpre_RxStck)[(context->interpre_RxStckTop)]); printf("\"\n"); }
# define DEBUGDISPLAYi(a,b) if ((context->rexx__debug__)) {printf("\t%u\t%s\t\"",inst_ip,(a)); \
     Lprint(STDOUT,(b)); printf("\"\n"); }
# define DEBUGDISPLAY2(a)  if ((context->rexx__debug__)) {printf("\t%u\t%s\t\"",inst_ip,(a));\
     Lprint(STDOUT,STACKP(1)); printf("\",\""); \
     Lprint(STDOUT,STACKTOP);printf("\"\n"); }
#else
# define DEBUGDISPLAY0(a)
# define DEBUGDISPLAY0nl(a)
# define DEBUGDISPLAY(a)
# define DEBUGDISPLAYi(a, b)
# define DEBUGDISPLAY2(a)
#endif

#define CHKERROR if ((context->interpre_RxStckTop)==STCK_SIZE-1) (context->lstring_Lerror)(ERR_STORAGE_EXHAUSTED,0)
#define INCSTACK { (context->interpre_RxStckTop)++; CHKERROR; }
#define POP_C_POP_B_PEEK_A { POP(C); POP(B); PEEK(A); }
#define PEEK(x)  x = &(STACKTOP)
#define PEEKR(x, r) x = &((context->interpre_RxStck)[(context->interpre_RxStckTop)-(r)])
#define POP(x)  x = &((context->interpre_RxStck)[(context->interpre_RxStckTop)--])
#define PUSH(x)  {x = &((context->interpre_RxStck)[++(context->interpre_RxStckTop)]); CHKERROR; }

#ifndef ALIGN
# define PLEAF(x)  { x = (PBinLeaf)(*(dword*)(context->interpreRxcip)); \
    (context->interpreRxcip) += sizeof(dword); }
# define INCWORD(x) (x) += sizeof(word)
# define INCDWORD(x) (x) += sizeof(dword)
# define CWORD  word
#else
# define PLEAF(x)  x = (PBinLeaf)(*(context->interpreRxcip)++)
# define INCWORD(x) (x)++
# define INCDWORD(x) (x)++
# define CWORD  dword
#endif

#ifdef __DEBUG__
/* -------------- DebugStackList ------------- */
static void
DebugStackList(void)
{
 int i;
 Context *context = (Context*)CMSGetPG();
 if ((context->interpre_RxStckTop)<0)
  printf("Stack is empty\n");
 else
  for (i=(context->interpre_RxStckTop); i>=0; i--) {
   printf("#%d: \"",i);
   Lprint(STDOUT,(context->interpre_RxStck)[i]);
   printf("\"\n");
  }
} /* DebugStackList */
#endif

/* ---------------- RxProcResize ---------------- */
void __CDECL
RxProcResize(void) {
    Context *context = (Context *) CMSGetPG();
    size_t oldsize = (context->rexx_proc_size);

    (context->rexx_proc_size) += PROC_INC;
    (context->rexx_proc) = (RxProc *) REALLOC((context->rexx_proc),
                                              (context->rexx_proc_size) *
                                              sizeof(RxProc));
    MEMSET((context->rexx_proc) + oldsize, 0, PROC_INC * sizeof(RxProc));
} /* RxProcResize */

/* ------------- I_trigger_space -------------- */
static void
I_trigger_space(void) {
    Context *context = (Context *) CMSGetPG();
    /* normalise to 0 .. len-1 */
    (context->interpre_DataStart) = (context->interpre_BreakEnd) - 1;

    /* skip leading spaces */
    LSKIPBLANKS(*(context->interpre_ToParse), (context->interpre_DataStart));

    /* find word */
    (context->interpre_BreakStart) = (context->interpre_DataStart);
    LSKIPWORD(*(context->interpre_ToParse), (context->interpre_BreakStart));

    /* skip trailing spaces */
    (context->interpre_BreakEnd) = (context->interpre_BreakStart);
    LSKIPBLANKS(*(context->interpre_ToParse), (context->interpre_BreakEnd));

    /* again in rexx strings 1..len */
    (context->interpre_DataStart)++;
    (context->interpre_BreakStart)++;
    (context->interpre_BreakEnd)++;
} /* I_trigger_space */

/* ------------- I_trigger_litteral -------------- */
static void
I_trigger_litteral(const PLstr lit) {
    int PatternPos;
    Context *context = (Context *) CMSGetPG();

    PatternPos = (int) Lpos(lit, (context->interpre_ToParse),
                            (context->interpre_DataStart));
    if (PatternPos > 0) {
        (context->interpre_BreakStart) = PatternPos;
        (context->interpre_BreakEnd) = PatternPos + LLEN(*lit);
    } else { /* the rest of the source is selected */
        (context->interpre_DataStart) = (context->interpre_BreakEnd);
        (context->interpre_BreakStart) = (context->interpre_SourceEnd);
        (context->interpre_BreakEnd) = (context->interpre_SourceEnd);
    }
} /* I_trigger_litteral */

/* ----------------- I_LoadOption ---------------- */
static void
I_LoadOption(const PLstr value, const int opt) {
#ifndef WCE
    char *ch;
#endif
    Context *context = (Context *) CMSGetPG();

    switch (opt) {
        case environment_opt:
            Lstrcpy(value, (context->rexx_proc)[(context->rexx_rx_proc)].env);
            break;

        case digits_opt:
            Licpy(value, (context->rexx_proc)[(context->rexx_rx_proc)].digits);
            break;

        case fuzz_opt:
            Licpy(value, (context->rexx_proc)[(context->rexx_rx_proc)].fuzz);
            break;

        case form_opt:
            Lscpy(value, ((context->rexx_proc)[(context->rexx_rx_proc)].form)
                         ? "SCIENTIFIC" : "ENGINEERING");
            break;

        case author_opt:
            Lscpy(value, AUTHOR);
            break;

        case version_opt:
            Lscpy(value, VERSIONSTR);
            break;

        case os_opt:
            Lscpy(value, OS);
            break;

        case calltype_opt:
            switch ((context->rexx_proc)[(context->rexx_rx_proc)].calltype) {
                case CT_PROCEDURE:
                    Lscpy(value, "PROCEDURE");
                    break;
                case CT_FUNCTION:
                    Lscpy(value, "FUNCTION");
                    break;
                default:
                    Lscpy(value, "COMMAND");
            }
            break;

        case filename_opt:
            Lstrcpy(value, &((context->compileCompileClause)[0].fptr)->name);
            break;

        case prgname_opt:
            Lscpy(value, (context->rexx_prgname));
            break;

        case shell_opt:
#ifndef WCE
            ch = getenv(SHELL);
            if (ch)
                Lscpy(value, ch);
            else
#endif
            LZEROSTR(*value);
            break;

        default:
            (context->lstring_Lerror)(ERR_INTERPRETER_FAILURE, 0);
    }
} /* I_LoadOption */

/* ----------------- I_StoreOption --------------- */
static void
I_StoreOption(const PLstr value, const int opt) {
    long l;
    Context *context = (Context *) CMSGetPG();

    switch (opt) {
        case environment_opt:
            if (LLEN(*value) > 250)
                (context->lstring_Lerror)(ERR_ENVIRON_TOO_LONG, 1, value);
            if ((context->rexx_proc)[(context->rexx_rx_proc)].env ==
                (context->rexx_proc)[(context->rexx_rx_proc) - 1].env) LPMALLOC(
                    (context->rexx_proc)[(context->rexx_rx_proc)].env);
            Lstrcpy((context->rexx_proc)[(context->rexx_rx_proc)].env, value);
            break;

        case trace_opt:
            TraceSet(value);
            if ((context->rexx_proc)[(context->rexx_rx_proc)].trace &
                (normal_trace | off_trace | error_trace))
                (context->interpre__trace) = FALSE;
            else
                (context->interpre__trace) = TRUE;

            break;

        case digits_opt:
            if (LLEN(*value) == 0)
                (context->rexx_proc)[(context->rexx_rx_proc)].digits = LMAXNUMERICDIGITS;
            else {
                l = Lrdint(value);
                if (l <= 0)
                    (context->lstring_Lerror)(ERR_INVALID_INTEGER, 5, value);
                (context->rexx_proc)[(context->rexx_rx_proc)].digits = (int) l;
            }
            (context->lstring_lNumericDigits) = MIN(
                    (context->rexx_proc)[(context->rexx_rx_proc)].digits,
                    LMAXNUMERICDIGITS);
            if ((context->rexx_proc)[(context->rexx_rx_proc)].digits <=
                (context->rexx_proc)[(context->rexx_rx_proc)].fuzz)
                (context->lstring_Lerror)(ERR_INVALID_RESULT, 1);
            break;

        case fuzz_opt:
            if (LLEN(*value) == 0)
                (context->rexx_proc)[(context->rexx_rx_proc)].fuzz = 0;
            else {
                l = Lrdint(value);
                if (l <= 0)
                    (context->lstring_Lerror)(ERR_INVALID_INTEGER, 6, value);
                (context->rexx_proc)[(context->rexx_rx_proc)].fuzz = (int) l;
            }
            if ((context->rexx_proc)[(context->rexx_rx_proc)].digits <=
                (context->rexx_proc)[(context->rexx_rx_proc)].fuzz)
                (context->lstring_Lerror)(ERR_INVALID_RESULT, 1);
            break;

        case form_opt:
            (context->rexx_proc)[(context->rexx_rx_proc)].form = (int) Lrdint(
                    value);
            break;

        case set_signal_opt:
        case set_signal_name_opt:
            switch (LSTR(*value)[0]) {
                case 'E':
                    (context->rexx_proc)[(context->rexx_rx_proc)].condition |= SC_ERROR;
                    if (opt == set_signal_name_opt)
                        (context->rexx_proc)[(context->rexx_rx_proc)].lbl_error = STACKP(
                                1);
                    else
                        (context->rexx_proc)[(context->rexx_rx_proc)].lbl_error = &((context->rexxerrorStr)->key);
                    break;
                case 'H':
                    (context->rexx_proc)[(context->rexx_rx_proc)].condition |= SC_HALT;
                    if (opt == set_signal_name_opt)
                        (context->rexx_proc)[(context->rexx_rx_proc)].lbl_halt = STACKP(
                                1);
                    else
                        (context->rexx_proc)[(context->rexx_rx_proc)].lbl_halt = &((context->rexxhaltStr)->key);
                    break;
                case 'N':
                    if (LSTR(*value)[2] == 'V') {
                        (context->rexx_proc)[(context->rexx_rx_proc)].condition |= SC_NOVALUE;
                        if (opt == set_signal_name_opt)
                            (context->rexx_proc)[(context->rexx_rx_proc)].lbl_novalue = STACKP(
                                    1);
                        else
                            (context->rexx_proc)[(context->rexx_rx_proc)].lbl_novalue = &((context->rexxnoValueStr)->key);
                    } else {
                        (context->rexx_proc)[(context->rexx_rx_proc)].condition |= SC_NOTREADY;
                        if (opt == set_signal_name_opt)
                            (context->rexx_proc)[(context->rexx_rx_proc)].lbl_notready = STACKP(
                                    1);
                        else
                            (context->rexx_proc)[(context->rexx_rx_proc)].lbl_notready = &((context->rexxnotReadyStr)->key);
                    }
                    break;
                case 'S':
                    (context->rexx_proc)[(context->rexx_rx_proc)].condition |= SC_SYNTAX;
                    if (opt == set_signal_name_opt)
                        (context->rexx_proc)[(context->rexx_rx_proc)].lbl_syntax = STACKP(
                                1);
                    else
                        (context->rexx_proc)[(context->rexx_rx_proc)].lbl_syntax = &((context->rexxsyntaxStr)->key);
                    break;
                default:
                    (context->lstring_Lerror)(ERR_INTERPRETER_FAILURE, 0);
            }
            break;

        case unset_signal_opt:
            switch (LSTR(*value)[0]) {
                case 'E':
                    (context->rexx_proc)[(context->rexx_rx_proc)].condition &= ~SC_ERROR;
                    break;
                case 'H':
                    (context->rexx_proc)[(context->rexx_rx_proc)].condition &= ~SC_HALT;
                    break;
                case 'N':
                    if (LSTR(*value)[2] == 'V')
                        (context->rexx_proc)[(context->rexx_rx_proc)].condition &= ~SC_NOVALUE;
                    else
                        (context->rexx_proc)[(context->rexx_rx_proc)].condition &= ~SC_NOTREADY;
                    break;
                case 'S':
                    (context->rexx_proc)[(context->rexx_rx_proc)].condition &= ~SC_SYNTAX;
                    break;
                default:
                    (context->lstring_Lerror)(ERR_INTERPRETER_FAILURE, 0);
            }
            break;

        default:
            (context->lstring_Lerror)(ERR_INTERPRETER_FAILURE, 0);
    }
} /* I_StoreOption */

/* ---------------- I_MakeIntArgs ---------------- */
/* prepare arguments for an internal function call */
/* returns stack position after call               */
#ifdef INLINE
inline
#endif

static int
I_MakeIntArgs(const int na, const int realarg, const CTYPE existarg) {
    int i, st;
    word bp; /* bit position */
    Context *context = (Context *) CMSGetPG();

    (context->rexxrxArg).n = na;
    bp = (1 << (na - 1));

    /* must doit reverse */
    MEMSET((context->rexxrxArg).a, 0, sizeof((context->rexxrxArg).a));

    (context->rexxrxArg).r = (context->interpre_RxStck)[
            (context->interpre_RxStckTop) - realarg];

    st = (context->interpre_RxStckTop); /* stack position of arguments */
    for (i = na - 1; i >= 0; i--) {
        if (existarg & bp) {
            if ((context->rexxrxArg).r == (context->interpre_RxStck)[st]) {
                Lstrcpy(&((context->interpre__tmpstr)[st]),
                        (context->interpre_RxStck)[st]);
                (context->rexxrxArg).a[i] = &(context->interpre__tmpstr)[st];
            } else
                (context->rexxrxArg).a[i] = (context->interpre_RxStck)[st];
            st--;
        }
        bp >>= 1;
    }
    return st;
} /* I_MakeIntArgs */

/* ---------------- I_MakeArgs ---------------- */
/* prepare arguments for a call to a func       */
#ifdef INLINE
inline
#endif

static void
I_MakeArgs(const int calltype, const int na, const CTYPE existarg) {
    int i, st;
    CTYPE bp; /* bit position */
    RxProc *pr;
    Args *arg;
    Context *context = (Context *) CMSGetPG();

    (context->rexx_rx_proc)++;  /* increase program items       */

    if ((context->rexx_rx_proc) == (context->rexx_proc_size)) RxProcResize();
    pr = (context->rexx_proc) + (context->rexx_rx_proc);

    /* initialise pr structure */
    /* use the old values      */
    MEMCPY(pr, (context->rexx_proc) + (context->rexx_rx_proc) - 1, sizeof(*pr));
    pr->calltype = calltype;
    pr->ip = (size_t) ((byte huge *) (context->interpreRxcip) -
                       (byte huge *) (context->interpreRxcodestart));
    pr->stacktop = (context->interpre_RxStckTop);

    /* setup arguments */
    arg = &(pr->arg);
    arg->n = na;

    bp = (1 << (na - 1));

    /* must doit reverse */
    MEMSET(arg->a, 0, sizeof(arg->a));

    st = (context->interpre_RxStckTop); /* stack position of arguments */
    for (i = na - 1; i >= 0; i--) {
        if (existarg & bp) {
            arg->a[i] = (context->interpre_RxStck)[st];
            st--;
        } else
            arg->a[i] = NULL;
        bp >>= 1;
    }
    arg->r = (context->interpre_RxStck)[st];

    if (calltype == CT_FUNCTION)
        pr->stack = st;
    else
        pr->stack = st - 1;
} /* I_MakeArgs */

/* -------------- I_CallFunction ---------------- */
static int
I_CallFunction(void) {
    PBinLeaf leaf, litleaf;
    RxFunc *func;
    int ct, nargs, realarg, i;
    char *ret_string;
    char *cmd_string;
    char **argv;
    int *lenv;
    CTYPE existarg, line;
    Lstr cmd;
    PLstr res = NULL;
    int st;
    char physical[21];
    char logical[21];
    char load_package_cmd[25];
    FILEP f;
#ifdef __DEBUG__
    size_t inst_ip;
#endif
    Context *context = (Context *) CMSGetPG();

    /* --- read the arguments here --- */
    PLEAF(leaf);   /* function number */
    nargs = *((context->interpreRxcip)++);  /* number of args */
    realarg = *((context->interpreRxcip)++);  /* real arguments */
    existarg = *(CWORD *) (context->interpreRxcip); /* existing arguments */
    INCWORD((context->interpreRxcip));
    line = *(CWORD *) (context->interpreRxcip); /* SIGL line  */
    INCWORD((context->interpreRxcip));
    ct = *((context->interpreRxcip)++);  /* call type  */
    func = (RxFunc *) (leaf->value);

#ifdef __DEBUG__
    if ((context->rexx__debug__)) {
     int i;
     for (i=0; i<LLEN(leaf->key); i++)
      putchar(LSTR(leaf->key)[i]);
     printf(" NoArgs=%d, Exist=%lX Type=%s\n",
      nargs, existarg,(func->type==FT_BUILTIN)?"builtin":"other");
    }
#endif

    if (func->type == FT_BUILTIN) {
        nargs = I_MakeIntArgs(nargs, realarg, existarg);
        (func->builtin->func)(func->builtin->opt);
        if (ct == CT_PROCEDURE) {
            RxVarSet((context->interpre_VarScope), (context->rexxresultStr),
                     (context->rexxrxArg).r);
            (context->interpre_RxStckTop) = nargs - 1; /* clear stack */
        } else
            (context->interpre_RxStckTop) = nargs;
        return TRUE;

    } else {

        /* If a function is not linked up by now treat it as a system call */
        if (func->type != FT_SYSTEM && func->label == UNKNOWN_LABEL) {
            func->type = FT_SYSTEM;
            func->systype = SYST_UNKNOWN;
        }

        if (func->type == FT_SYSTEM) {

            /* Make the calltype 5 Arguments and prepare command string */
            L2STR(&(leaf->key));
            LASCIIZ(leaf->key);
            argv = malloc(realarg * sizeof(char *));
            lenv = malloc(realarg * sizeof(int));
            st = (context->interpre_RxStckTop) - realarg;
            res = (context->interpre_RxStck)[st++];
            i = 0;
            while (st <= (context->interpre_RxStckTop)) {
                L2STR((context->interpre_RxStck)[st]);
                LASCIIZ(*((context->interpre_RxStck)[st]));
                argv[i] = LSTR(*((context->interpre_RxStck)[st]));
                lenv[i] = LLEN(*((context->interpre_RxStck)[st]));
                st++;
                i++;
            }

            switch (func->systype) {
                case SYST_UNKNOWN:
                    /* Need to go through the (rather brain dead) IBM call convention */
                    /* 1. RX is prefixed to the func name */
                    sprintf(physical, "RX%.6s", LSTR(leaf->key));
                    sprintf(logical, "RX%.6s %s", LSTR(leaf->key),
                            LSTR(leaf->key));

                    /* 2. Attempt to call function (i.e. with the RX) */
                    i = CMSfunctionDataArray(physical,
                                             logical,
                                             ct == CT_PROCEDURE,
                                             &ret_string,
                                             realarg,
                                             argv, lenv);
                    if (!(i < 0)) func->systype = SYST_RX;

                    /* 3. Function packaged loaded and attempt to call function (x3) */
                    if (i < 0 &&
                        !(context->interpre_no_user_fp)) { /* Not found (yet) */
                        sprintf(load_package_cmd, "RXUSERFN LOAD RX%.6s",
                                LSTR(leaf->key));
                        i = CMScommand(load_package_cmd, 0);
                        if (i == -3) (context->interpre_no_user_fp) = 1;
                        if (!i) {
                            i = CMSfunctionDataArray(physical,
                                                     logical,
                                                     ct == CT_PROCEDURE,
                                                     &ret_string,
                                                     realarg,
                                                     argv, lenv);
                            if (!(i < 0)) func->systype = SYST_RX;
                        }
                    }

                    if (i < 0 &&
                        !(context->interpre_no_loc_fp)) { /* Not found (yet) */
                        sprintf(load_package_cmd, "RXLOCFN LOAD RX%.6s",
                                LSTR(leaf->key));
                        i = CMScommand(load_package_cmd, 0);
                        if (i == -3) (context->interpre_no_loc_fp) = 1;
                        if (!i) {
                            i = CMSfunctionDataArray(physical,
                                                     logical,
                                                     ct == CT_PROCEDURE,
                                                     &ret_string,
                                                     realarg,
                                                     argv, lenv);
                            if (!(i < 0)) func->systype = SYST_RX;
                        }
                    }

                    if (i < 0 &&
                        !(context->interpre_no_sys_fp)) { /* Not found (yet) */
                        sprintf(load_package_cmd, "RXSYSFN LOAD RX%.6s",
                                LSTR(leaf->key));
                        i = CMScommand(load_package_cmd, 0);
                        if (i == -3) (context->interpre_no_sys_fp) = 1;
                        if (!i) {
                            i = CMSfunctionDataArray(physical,
                                                     logical,
                                                     ct == CT_PROCEDURE,
                                                     &ret_string,
                                                     realarg,
                                                     argv, lenv);
                            if (!(i < 0)) func->systype = SYST_RX;
                        }
                    }

                    /* 4. RX is removed and the REXX function (i.e. an EXEC) searched for */
                    /*    and run (without the RX) */
                    if (i < 0) {
                        /* Does the exec exist? */
                        CMSFILEINFO *existingFileState;
                        sprintf(physical, "%-8s%-8s%-2s", LSTR(leaf->key),
                                "EXEC", "*");
                        if (CMSfileState(physical, &existingFileState)) {
                            /* File does not exist (or some other error) */
                            i = -3;
                        } else {
                            sprintf(physical, "EXEC %.8s", LSTR(leaf->key));
                            i = CMSfunctionDataArray(physical,
                                                     physical,
                                                     ct == CT_PROCEDURE,
                                                     &ret_string,
                                                     realarg,
                                                     argv, lenv);
                            if (!(i < 0)) func->systype = SYST_EXEC;
                        }
                    }

                    /* 5. Final attempt to call function (without the RX) */
                    if (i < 0) {
                        i = CMSfunctionDataArray(LSTR(leaf->key),
                                                 LSTR(leaf->key),
                                                 ct == CT_PROCEDURE,
                                                 &ret_string,
                                                 realarg,
                                                 argv, lenv);
                        if (!(i < 0)) func->systype = SYST_BARE;
                    }
                    break;

                case SYST_BARE:
                    i = CMSfunctionDataArray(LSTR(leaf->key),
                                             LSTR(leaf->key),
                                             ct == CT_PROCEDURE,
                                             &ret_string,
                                             realarg,
                                             argv, lenv);
                    break;

                case SYST_RX:
                    sprintf(physical, "RX%.6s", LSTR(leaf->key));
                    sprintf(logical, "RX%.6s %s", LSTR(leaf->key),
                            LSTR(leaf->key));
                    i = CMSfunctionDataArray(physical,
                                             logical,
                                             ct == CT_PROCEDURE,
                                             &ret_string,
                                             realarg,
                                             argv, lenv);
                    break;

                case SYST_EXEC:
                    sprintf(physical, "EXEC %.8s", LSTR(leaf->key));
                    i = CMSfunctionDataArray(physical,
                                             physical,
                                             ct == CT_PROCEDURE,
                                             &ret_string,
                                             realarg,
                                             argv, lenv);
                    break;
            }

            /* Cleanup and handle result string */
            free(argv);
            free(lenv);
            if (ret_string) {
                if (i <= 0) { /* Error or zero data returned */
                    Lscpy(res, ret_string);
                } else { /* i is the length of data returned */
                    Lmcpy(res, ret_string, i);
                }
                free(ret_string);
            }

            /* Error calling function */
            if (i == -3) {
                (context->lstring_Lerror)(ERR_ROUTINE_NOT_FOUND, 0);
            } else if (i == -2) {
                (context->lstring_Lerror)(ERR_STORAGE_EXHAUSTED, 0);
            } else if (i < 0) {
                (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
            }

            if (!ret_string) {
                if (ct == CT_FUNCTION)
                    (context->lstring_Lerror)(ERR_NO_DATA_RETURNED, 0);
            }
            (context->interpre_RxStckTop) -= realarg;

            if (ct == CT_PROCEDURE) {
                RxVarSet((context->interpre_VarScope), (context->rexxresultStr),
                         res);
                (context->interpre_RxStckTop)--;
            }
            return TRUE;

        } else {
            (context->interpreRxcip)++;
            RxSetSpecialVar(SIGLVAR, line);
            I_MakeArgs(ct, nargs, existarg);
            (context->interpreRxcip) = (CIPTYPE *) (
                    (byte huge *) (context->interpreRxcodestart) + func->label);
            (context->interpreRxcip)++; /* skip the OP_NEWCLAUSE */
            if ((context->interpre__trace)) TraceClause();

            /* Handle Procedure Context */
            if (*(context->interpreRxcip) == OP_PROC) {
                int exposed;

                /* give a unique program id */
                /* we might have a problem after 2*32 routine calls!! */
                (context->rexx_procidcnt)++;
                (context->rexx_proc)[(context->rexx_rx_proc)].id = (context->rexx_procidcnt);
                (context->interpreRx_id) = (context->rexx_procidcnt);
#ifdef __DEBUG__
                if ((context->rexx__debug__))
                 inst_ip = (size_t)((byte huge *)(context->interpreRxcip) - (byte huge *)(context->interpreRxcodestart));
#endif
                DEBUGDISPLAY0nl("PROC ");
                (context->interpreRxcip)++;
                (context->rexx_proc)[(context->rexx_rx_proc)].scope = RxScopeMalloc();
                (context->interpre_VarScope) = (context->rexx_proc)[(context->rexx_rx_proc)].scope;

                /* handle exposed variables */
                exposed = *((context->interpreRxcip)++);
#ifdef __DEBUG__
                if ((context->rexx__debug__) && exposed>0)
                 printf("EXPOSE");
#endif
                for (; exposed > 0; exposed--) {
                    /* Get pointer to variable */
                    PLEAF(litleaf);
                    /* test if indirect exposure (var) */
                    if (litleaf == NULL) {
                        int found;
                        PBinLeaf leaf;

                        PLEAF(litleaf);
                        RxVarExpose((context->interpre_VarScope), litleaf);
#ifdef __DEBUG__
                        if ((context->rexx__debug__)) {
                         putchar(' ');
                         putchar('(');
                         Lprint(STDOUT,&(litleaf->key));
                         putchar(')');
                         putchar('=');
                        }
#endif
                        leaf = RxVarFind(
                                (context->rexx_proc)[(context->rexx_rx_proc) -
                                                     1].scope,
                                litleaf,
                                &found);
#ifdef __DEBUG__
                        if ((context->rexx__debug__))
                         Lprint(STDOUT,LEAFVAL(leaf));
#endif
                        if (found)
                            RxVarExposeInd((context->interpre_VarScope),
                                           LEAFVAL(leaf));
                    } else {
#ifdef __DEBUG__
                        if ((context->rexx__debug__)) {
                         putchar(' ');
                         Lprint(STDOUT,&(litleaf->key));
                        }
#endif
                        RxVarExpose((context->interpre_VarScope), litleaf);
                    }
                }
#ifdef __DEBUG__
                if ((context->rexx__debug__))
                 putchar('\n');
#endif
            }
        }
        return FALSE;
    }
} /* I_CallFunction */

/* ---------------- I_ReturnProc -------------- */
/* restore arguments after a procedure return */
static void
I_ReturnProc(void) {
    Context *context = (Context *) CMSGetPG();
    /* fix ip and stack */
    (context->interpreRxcip) = (CIPTYPE *) (
            (byte huge *) (context->interpreRxcodestart) +
            (context->rexx_proc)[(context->rexx_rx_proc)].ip);
    (context->interpre_RxStckTop) = (context->rexx_proc)[(context->rexx_rx_proc)].stack;

    if ((context->rexx_rx_proc) > 0) {
        /* free everything that it is new */
        if ((context->interpre_VarScope) !=
            (context->rexx_proc)[(context->rexx_rx_proc) - 1].scope) {
            RxScopeFree((context->interpre_VarScope));
            FREE((context->interpre_VarScope));
        }

        if ((context->rexx_proc)[(context->rexx_rx_proc)].env !=
            (context->rexx_proc)[(context->rexx_rx_proc) - 1].env) LPFREE(
                (context->rexx_proc)[(context->rexx_rx_proc)].env);
    }

    /* load previous data and exit */
    (context->rexx_rx_proc)--;
    (context->interpreRx_id) = (context->rexx_proc)[(context->rexx_rx_proc)].id;
    (context->interpre_VarScope) = (context->rexx_proc)[(context->rexx_rx_proc)].scope;
    (context->lstring_lNumericDigits) = (context->rexx_proc)[(context->rexx_rx_proc)].digits;
    if ((context->rexx_proc)[(context->rexx_rx_proc)].trace &
        (normal_trace | off_trace | error_trace))
        (context->interpre__trace) = FALSE;
    else
        (context->interpre__trace) = TRUE;
} /* I_ReturnProc */

/* ------------ RxInitInterStr -------------- */
void __CDECL
RxInitInterStr() {
    RxProc *pr;
    Context *context = (Context *) CMSGetPG();

    (context->rexx_rx_proc)++;  /* increase program items       */
    if ((context->rexx_rx_proc) == (context->rexx_proc_size)) RxProcResize();
    pr = (context->rexx_proc) + (context->rexx_rx_proc);

    /* program id is the same */
    MEMCPY(pr, (context->rexx_proc) + (context->rexx_rx_proc) - 1, sizeof(*pr));
    pr->calltype = CT_INTERPRET;
    pr->ip = (size_t) ((byte huge *) (context->interpreRxcip) -
                       (byte huge *) (context->interpreRxcodestart));
    pr->codelen = LLEN(*(context->rexx_code));
    pr->clauselen = (context->compileCompileCurClause);
    pr->stack = (context->interpre_RxStckTop) - 1;  /* before temporary str */
    pr->stacktop = (context->interpre_RxStckTop);

    /* setup arguments */
    (pr->arg).n = 0;
    MEMSET(pr->arg.a, 0, sizeof(pr->arg.a));
    (pr->arg).r = NULL;

    /* --- save state --- */
    MEMCPY((context->interpre_old_error), (context->rexx_error_trap),
           sizeof((context->rexx_error_trap)));
    SIGNAL(SIGINT, SIG_IGN);

    /* compile the program */
    RxInitCompile((context->rexxrxFileList), STACKTOP);
    RxCompile();

    /* --- restore state --- */
    MEMCPY((context->rexx_error_trap), (context->interpre_old_error),
           sizeof((context->rexx_error_trap)));
    SIGNAL(SIGINT, RxHaltTrap);

    /* might have changed position */
    (context->interpreRxcodestart) = (CIPTYPE *) LSTR(*(context->rexx_code));
    (context->interpreRxcip) = (CIPTYPE *) (
            (byte huge *) (context->interpreRxcodestart) + pr->codelen);

    /* check for an error in compilation */
    if ((context->rexxrxReturnCode)) {
        /* --- load previous data and exit ---- */
        (context->interpreRxcip) = (CIPTYPE *) (
                (byte huge *) (context->interpreRxcodestart) + pr->ip);
        (context->rexx_rx_proc)--;
        (context->interpreRx_id) = (context->rexx_proc)[(context->rexx_rx_proc)].id;
        (context->interpre_VarScope) = (context->rexx_proc)[(context->rexx_rx_proc)].scope;

        RxSetSpecialVar(RCVAR, (context->rexxrxReturnCode));
        RxSignalCondition(SC_SYNTAX);
    }
} /* RxInitInterStr */

/* ------------ RxDoneInterStr -------------- */
static void
RxDoneInterStr(void) {
    Context *context = (Context *) CMSGetPG();
    /* fix ip and stack */
    if ((context->rexx_proc)[(context->rexx_rx_proc)].calltype ==
        CT_INTERACTIVE) {
        if ((context->rexx_proc)[(context->rexx_rx_proc)].trace &
            (normal_trace | off_trace | error_trace))
            (context->interpre__trace) = FALSE;
        else
            (context->interpre__trace) = TRUE;
    }

    (context->interpreRxcip) = (CIPTYPE *) (
            (byte huge *) (context->interpreRxcodestart) +
            (context->rexx_proc)[(context->rexx_rx_proc)].ip);
    (context->interpre_RxStckTop) = (context->rexx_proc)[(context->rexx_rx_proc)].stack;

    /* fixup code length, cut the interpretation code */
    LLEN(*(context->rexx_code)) = (context->rexx_proc)[(context->rexx_rx_proc)].codelen;
    (context->compileCompileCurClause) = (context->rexx_proc)[(context->rexx_rx_proc)].clauselen;
    if ((context->rexx_proc)[(context->rexx_rx_proc)].env !=
        (context->rexx_proc)[(context->rexx_rx_proc) - 1].env) {
        Lstrcpy((context->rexx_proc)[(context->rexx_rx_proc) - 1].env,
                (context->rexx_proc)[(context->rexx_rx_proc)].env);
        LPFREE((context->rexx_proc)[(context->rexx_rx_proc)].env);
    }

    /* --- load previous data and exit ---- */
    (context->rexx_rx_proc)--;
    (context->interpreRx_id) = (context->rexx_proc)[(context->rexx_rx_proc)].id;

    (context->rexx_proc)[(context->rexx_rx_proc)].trace = (context->rexx_proc)[
            (context->rexx_rx_proc) + 1].trace;
    (context->rexx_proc)[(context->rexx_rx_proc)].interactive_trace = (context->rexx_proc)[
            (context->rexx_rx_proc) + 1].interactive_trace;
    (context->interpre_VarScope) = (context->rexx_proc)[(context->rexx_rx_proc)].scope;
} /* RxDoneInterStr */

/* ---------------- RxInitInterpret --------------- */
void __CDECL
RxInitInterpret(void) {
    int i;
    Context *context = (Context *) CMSGetPG();

#ifdef __PROFILE__
    MEMSET((context->interpre_instr_cnt),sizeof((context->interpre_instr_cnt)),0);
#endif
    MEMSET((context->interpre_RxStck), 0, (STCK_SIZE) * sizeof(PLstr));
    (context->interpre_RxStckTop) = -1;
    MEMSET((context->interpre__tmpstr), 0, (STCK_SIZE) * sizeof(Lstr));
    for (i = 0; i < STCK_SIZE; i++) {
        Lfx(&((context->interpre__tmpstr)[i]), 0);
        if (!LSTR((context->interpre__tmpstr)[i]))
            (context->lstring_Lerror)(ERR_STORAGE_EXHAUSTED, 0);
    }
} /* RxInitInterpret */

/* ---------------- RxDoneInterpret --------------- */
void __CDECL
RxDoneInterpret(void) {
    int i;
    Context *context = (Context *) CMSGetPG();
#ifdef __PROFILE__
    FILE *fout;
    Context *context = (Context*)CMSGetPG();
    fout = fopen("instr.cnt","w");
    fprintf(fout,"Instr\tCount\n");
    for (i=0; i<OP_POW; i++) /* pow is the last command */
     fprintf(fout,"%d\t%d\n",i,(context->interpre_instr_cnt[i]));
    fclose(fout);
#endif
#ifdef __DEBUG__
    mem_chk();

    if ((context->interpre_RxStckTop)>=0) {
     fprintf(STDERR,"interpret: %d items left in stack.\n", (context->interpre_RxStckTop)+1);
     DebugStackList();
    }
#endif

    /* clear stack */
    for (i = 0; i < STCK_SIZE; i++) {
#ifdef __DEBUG__
        if ((context->rexx__debug__) && LLEN((context->interpre__tmpstr)[i])) {
         fprintf(STDERR,"Freeing... %d: \"",i);
         Lprint(STDERR,&((context->interpre__tmpstr)[i]));
         fprintf(STDERR,"\"\n");
        }
#endif
        LFREESTR((context->interpre__tmpstr)[i]);
    }
} /* RxDoneInterpret */

/* ---------------- RxInterpret --------------- */
int __CDECL
RxInterpret(void) {
    PLstr a;
    IdentInfo *inf;
    CTYPE w;
    int na, nf, jc, errno, subno, found;
    PBinLeaf litleaf, leaf;
    RxFunc *func;
#ifdef __DEBUG__
    size_t inst_ip;
    char cmd='\n';
#endif
#ifdef WCE
    int event_count = 0;
#endif
    int oldTraceFlag = CMSGetFlag(TRACEFLAG);
    int newTraceFlag;
    Context *context = (Context *) CMSGetPG();

    (context->rexxrxReturnCode) = 0;
    (context->interpreRx_id) = (context->rexx_proc)[(context->rexx_rx_proc)].id;
    (context->interpreRxcodestart) = (CIPTYPE *) LSTR(*(context->rexx_code));
    (context->interpre_VarScope) = (context->rexx_proc)[(context->rexx_rx_proc)].scope;
    (context->interpreRxcip) = (CIPTYPE *) (
            (byte huge *) (context->interpreRxcodestart) +
            (context->rexx_proc)[(context->rexx_rx_proc)].ip);
    (context->rexx_proc)[(context->rexx_rx_proc)].stack = (context->interpre_RxStckTop);

    if ((context->rexx_proc)[(context->rexx_rx_proc)].trace &
        (normal_trace | off_trace | error_trace))
        (context->interpre__trace) = FALSE;
    else
        (context->interpre__trace) = TRUE;

    SIGNAL(SIGINT, RxHaltTrap);

    if ((jc = setjmp((context->rexx_error_trap))) != 0) {
        CIPTYPE *tmp_Rxcip;
        if (jc == JMP_EXIT) {
            (context->interpre_RxStckTop) = -1;
            goto interpreter_fin;
        }
        /* else if (jc==JMP_CONTINUE) .... CONTINUE code */

        /* exit from interpret, if we are in any */
        tmp_Rxcip = (context->interpreRxcip);
        while ((context->rexx_proc)[(context->rexx_rx_proc)].calltype ==
               CT_INTERPRET)
            RxDoneInterStr();
        (context->interpreRxcip) = tmp_Rxcip;

        /* clear stack */
        (context->interpre_RxStckTop) = (context->rexx_proc)[(context->rexx_rx_proc)].stacktop;
    }

    while (1) {
        main_loop:

#ifdef __DEBUG__
        if ((context->rexx__debug__)) {
         while (1) {
          cmd = getchar();
          switch ((context->lstring_l2u)[(byte)cmd]) {
           case 'M':
            printf("Memory allocated=%ld\n",
             mem_allocated());
            break;
           case 'S':
            DebugStackList();
            break;
           case 'Q':
            goto interpreter_fin;
           case '\n':
            goto outofcmd;
          }
         }
       outofcmd:
         printf("Stck:%d\t",(context->interpre_RxStckTop)+1);
         inst_ip = (size_t)((byte huge *)(context->interpreRxcip) - (byte huge *)(context->interpreRxcodestart));
        }
#endif
#ifdef __PROFILE__
        (context->interpre_instr_cnt)[(int)*(context->interpreRxcip)]++;
#endif
        switch (*((context->interpreRxcip)++)) {
            /*
             * [mnemonic] <type>[value]...
             * type: b = byte
             *  w = word
             *  p = pointer
             */
            /* START A NEW COMMAND */
            case OP_NEWCLAUSE:
                DEBUGDISPLAY0("NEWCLAUSE");
                if ((context->interpre__trace)) TraceClause();

                /* Poll Flags - HALT */
                if (CMSGetFlag(HALTFLAG)) raise(SIGINT);

                /* Poll Flag - TRACE */
                newTraceFlag = CMSGetFlag(TRACEFLAG);
                if (newTraceFlag != oldTraceFlag) {
                    oldTraceFlag = newTraceFlag;
                    if (newTraceFlag) (context->interpre__trace) = TRUE;
                    else (context->interpre__trace) = FALSE;
                }

#ifdef WCE
                /* Check for messages in the event queue */
                if (++event_count == MAX_EVENT_COUNT) {
                 event_count = 0;
                 /* Peek the stacked events */
                 WKeyPressed();
             /*//    if (_interrupt) ProcessInterrupt(); */
                }
#endif
                goto main_loop;

                /* POP = NO OPERATION */
            case OP_NOP:
                DEBUGDISPLAY0("NOP");
                goto main_loop;

                /* PUSH p[lit]   */
                /* push a litteral to stack */
            case OP_PUSH:
                (context->interpre_RxStckTop)++;
                STACKTOP = (PLstr) (*(dword *) (context->interpreRxcip));
                INCDWORD((context->interpreRxcip));
                CHKERROR;
                DEBUGDISPLAY("PUSH");
                goto chk4trace;

                /* PUSHTMP */
            case OP_PUSHTMP:
                (context->interpre_RxStckTop)++;
                STACKTOP = &(context->interpre__tmpstr)[(context->interpre_RxStckTop)];
                CHKERROR;
                DEBUGDISPLAY0("PUSHTMP");
                goto main_loop;

                /* POP b[num]  */
                /* pop NUM stack items */
            case OP_POP:
                DEBUGDISPLAY0("POP");
                (context->interpre_RxStckTop) -= *((context->interpreRxcip)++);
                goto main_loop;

                /* DUP b[rel]   */
                /* duplicate RELative stck item */
            case OP_DUP:
                (context->interpre_RxStckTop)++;
                STACKTOP = (context->interpre_RxStck)[
                        (context->interpre_RxStckTop) -
                        *((context->interpreRxcip)++) - 1];
                CHKERROR;
                DEBUGDISPLAY("DUP");
                goto main_loop;

                /* COPY    */
                /* copy (Lstrcpy) top item */
                /* to previous one  */
            case OP_COPY:
                DEBUGDISPLAY("COPY");
                Lstrcpy(STACKP(1), STACKTOP);
                (context->interpre_RxStckTop) -= 2;
                goto main_loop;

                /* COPY2TMP   */
                /* if top item is not a pointer */
                /* to a tmp var then copy the */
                /* value to a tmp var  */
            case OP_COPY2TMP:
                /* copy to temporary only if different */
                if (STACKTOP !=
                    &((context->interpre__tmpstr)[(context->interpre_RxStckTop)])) {
                    Lstrcpy(&((context->interpre__tmpstr)[(context->interpre_RxStckTop)]),
                            STACKTOP);
                    STACKTOP = &((context->interpre__tmpstr)[(context->interpre_RxStckTop)]);
                }
                DEBUGDISPLAY("COPY2TMP");
                goto main_loop;

                /* PATCH w[rel] b[code] */
                /* patch CODE string to RELative pos with CODE */
            case OP_PATCH:
                w = *(CWORD *) (context->interpreRxcip);
                INCWORD((context->interpreRxcip));
                *(CIPTYPE *) ((byte huge *) (context->interpreRxcodestart) +
                              w) = *((context->interpreRxcip)++);
                DEBUGDISPLAY0("PATCH");
                goto main_loop;

                /* RAISE b[cond] b[errno] b[subno] */
                /* raise an error condition  */
            case OP_RAISE:
                errno = *((context->interpreRxcip)++);
                subno = *((context->interpreRxcip)++);
                DEBUGDISPLAY("RAISE");
                (context->lstring_Lerror)(errno, subno, STACKTOP);
                goto main_loop;

                /* LOADARG b[arg]  */
                /* push an ARGument to stck */
            case OP_LOADARG: INCSTACK;
                na = (unsigned) *((context->interpreRxcip)++); /* argument to push */
                if ((context->rexx_proc)[(context->rexx_rx_proc)].arg.a[na])
                    STACKTOP = (context->rexx_proc)[(context->rexx_rx_proc)].arg.a[na];
                else {
                    LZEROSTR(
                            (context->interpre__tmpstr)[(context->interpre_RxStckTop)]);
                    STACKTOP = &((context->interpre__tmpstr)[(context->interpre_RxStckTop)]);
                }
                DEBUGDISPLAY("LOADARG");
                goto main_loop;


                /* LOADOPT [data] */
                /* load an option */
            case OP_LOADOPT: INCSTACK;
                nf = (unsigned) *((context->interpreRxcip)++); /* option to load */
/**
/// Maybe only pointer to Option!!!
**/
                STACKTOP = &((context->interpre__tmpstr)[(context->interpre_RxStckTop)]);
                I_LoadOption(STACKTOP, nf);
                DEBUGDISPLAY("LOADOPT");
                goto main_loop;

                /* STOREOPT [data] */
                /* store an option */
            case OP_STOREOPT:
                DEBUGDISPLAY("STOREOPT");
                nf = (unsigned) *((context->interpreRxcip)++); /* option to store */
                I_StoreOption(STACKTOP, nf);
                (context->interpre_RxStckTop)--;
                goto main_loop;

                /* LOAD p[leaf]   */
                /* push a VARiable to stack */
            case OP_LOAD: INCSTACK;  /* make space */
                PLEAF(litleaf);  /* get variable ptr */
                DEBUGDISPLAYi("LOAD", &(litleaf->key));

                inf = (IdentInfo *) (litleaf->value);

                /* check to see if we have allready its position */
                if (inf->id == (context->interpreRx_id)) {
                    leaf = inf->leaf[0];
                    STACKTOP = LEAFVAL(leaf);
                } else {
                    leaf = RxVarFind((context->interpre_VarScope), litleaf,
                                     &found);
                    if (found)
                        STACKTOP = LEAFVAL(leaf);
                    else {
                        if (inf->stem) {
                            /* Lstrcpy to a temp variable */
                            Lstrcpy(&((context->interpre__tmpstr)[(context->interpre_RxStckTop)]),
                                    &(context->variable_stemvaluenotfound));
                            STACKTOP = &((context->interpre__tmpstr)[(context->interpre_RxStckTop)]);
                            if (leaf == NULL &&
                                (context->rexx_proc)[(context->rexx_rx_proc)].condition &
                                SC_NOVALUE)
                                RxSignalCondition(SC_NOVALUE);
                        } else {
                            if ((context->rexx_proc)[(context->rexx_rx_proc)].condition &
                                SC_NOVALUE)
                                RxSignalCondition(SC_NOVALUE);
                            STACKTOP = &(litleaf->key);
                        }
                    }
                }
                goto chk4trace;

                /* STORE p[leaf]   */
                /* store top stack item to VARiable */
            case OP_CREATE:  /* assigmnent */
            INCSTACK;
                PLEAF(litleaf); /* Get pointer to variable */
                DEBUGDISPLAYi("CREATE", &(litleaf->key));

                inf = (IdentInfo *) (litleaf->value);
                if (inf->id == (context->interpreRx_id)) {
                    leaf = inf->leaf[0];
                    STACKTOP = LEAFVAL(leaf);
                } else {
                    leaf = RxVarFind((context->interpre_VarScope), litleaf,
                                     &found);

                    if (found)
                        STACKTOP = LEAFVAL(leaf);
                    else {
                        leaf = RxVarAdd((context->interpre_VarScope),
                                        &(litleaf->key),
                                        inf->stem,
                                        leaf);
                        STACKTOP = LEAFVAL(leaf);
                        if (inf->stem == 0) {
                            inf->id = (context->interpreRx_id);
                            inf->leaf[0] = leaf;
                        }
                    }

                }
                goto main_loop;

                /* DROP p[leaf]  */
                /* drop VARiable */
            case OP_DROP:
                PLEAF(litleaf); /* Get pointer to variable */
                DEBUGDISPLAYi("DROP", &(litleaf->key));

                inf = (IdentInfo *) (litleaf->value);
                if (inf->id == (context->interpreRx_id)) {
                    leaf = inf->leaf[0];
                    RxVarDel((context->interpre_VarScope), litleaf, leaf);
                } else {
                    leaf = RxVarFind((context->interpre_VarScope), litleaf,
                                     &found);
                    if (found)
                        RxVarDel((context->interpre_VarScope), litleaf, leaf);
                }
                inf->id = NO_CACHE;
                goto chk4trace;

                /* indirect drop, from stack */
                /* asssume that is UPPER case tmp */
            case OP_DROPIND:
                DEBUGDISPLAY("DROP_IND");
                RxVarDelInd((context->interpre_VarScope), STACKTOP);
                (context->interpre_RxStckTop)--;
                goto chk4trace;

            case OP_ASSIGNSTEM:
                PLEAF(litleaf); /* Get pointer to stem */
                DEBUGDISPLAYi("ASSIGNSTEM", &(litleaf->key));
                inf = (IdentInfo *) (litleaf->value);
                if (inf->id == (context->interpreRx_id)) {
                    leaf = inf->leaf[0];
                    RxScopeAssign(leaf);
                } else {
                    leaf = RxVarFind((context->interpre_VarScope), litleaf,
                                     &found);
                    if (found)
                        RxScopeAssign(leaf);
                }
                goto main_loop;

                /* BYINIT [patchpos] */
            case OP_BYINIT:
                w = *(CWORD *) (context->interpreRxcip);
                INCWORD((context->interpreRxcip));
                DEBUGDISPLAY("BYINIT");
                /* copy to temporary only if different */
                if (STACKTOP !=
                    &((context->interpre__tmpstr)[(context->interpre_RxStckTop)])) {
                    Lstrcpy(&((context->interpre__tmpstr)[(context->interpre_RxStckTop)]),
                            STACKTOP);
                    STACKTOP = &((context->interpre__tmpstr)[(context->interpre_RxStckTop)]);
                }
                /* patch comparision code */
                if (Llt(STACKTOP, &((context->rexxzeroStr)->key)))
                    *(CIPTYPE *) ((byte huge *) (context->interpreRxcodestart) +
                                  w) = OP_TLE;
                else
                    *(CIPTYPE *) ((byte huge *) (context->interpreRxcodestart) +
                                  w) = OP_TGE;
                goto main_loop;

                /* FORINIT  */
                /* Initialise a FOR loop*/
            case OP_FORINIT:
                DEBUGDISPLAY("FORINIT");
                /* copy to temporary only if different */
                if (STACKTOP !=
                    &((context->interpre__tmpstr)[(context->interpre_RxStckTop)])) {
                    Lstrcpy(&((context->interpre__tmpstr)[(context->interpre_RxStckTop)]),
                            STACKTOP);
                    STACKTOP = &((context->interpre__tmpstr)[(context->interpre_RxStckTop)]);
                }
                L2INT(STACKTOP); /* it is in temporary */
                if (Llt(STACKTOP, &((context->rexxzeroStr)->key)))
                    (context->lstring_Lerror)(ERR_INVALID_INTEGER, 3, STACKTOP);
                goto main_loop;

                /* DECFOR  */
            case OP_DECFOR:
                DEBUGDISPLAY("DECFOR");
                a = (context->interpre_RxStck)[(context->interpre_RxStckTop) -
                                               *((context->interpreRxcip)++)];
                if (Leq(a, &((context->rexxzeroStr)->key)))
                    (context->interpreRxcip) = (CIPTYPE *) (
                            (byte huge *) (context->interpreRxcodestart) +
                            *(CWORD *) (context->interpreRxcip));
                else
                    INCWORD((context->interpreRxcip));
                Ldec(a);
                goto main_loop;

/**
//// Beware might lose something when number is like '   10   '
**/
                /* TOINT  */
                /* change to integer */
            case OP_TOINT:
                DEBUGDISPLAY("TOINT");
                L2INT(STACKTOP);
                goto main_loop;

                /* LOWER  */
                /* upper top stack */
            case OP_LOWER:
                DEBUGDISPLAY("LOWER");
                Llower(STACKTOP);
                goto main_loop;

                /* UPPER  */
                /* upper top stack */
            case OP_UPPER:
                DEBUGDISPLAY("UPPER");
                Lupper(STACKTOP);
                goto main_loop;

                /* SIGNAL p[label] */
                /* clear stack and jmp to LABEL pos */
            case OP_SIGNAL:
                /* clear stack */
                (context->interpre_RxStckTop) = (context->rexx_proc)[(context->rexx_rx_proc)].stacktop;

                /* check label */
                PLEAF(leaf);
                func = (RxFunc *) (leaf->value);
                DEBUGDISPLAYi("SIGNAL", &(leaf->key));

                if (func->label == UNKNOWN_LABEL)
                    (context->lstring_Lerror)(ERR_UNEXISTENT_LABEL, 1,
                                              &(leaf->key));
                /* jump */
                (context->interpreRxcip) = (CIPTYPE *) (
                        (byte huge *) (context->interpreRxcodestart) +
                        func->label);
                goto main_loop;

                /* SIGNALVAL [address] */
                /* get address from stack */
            case OP_SIGNALVAL:
                DEBUGDISPLAY("SIGNALVAL");

                /* search for label */
                L2STR(STACKTOP);
                leaf = BinFind(&(context->rexx_labels),
                               (context->interpre_RxStck)[(context->interpre_RxStckTop)--]);
                if (leaf == NULL ||
                    ((RxFunc *) (leaf->value))->label == UNKNOWN_LABEL)
                    (context->lstring_Lerror)(ERR_UNEXISTENT_LABEL, 1,
                                              (context->interpre_RxStck)[
                                                      (context->interpre_RxStckTop) +
                                                      1]);
                func = (RxFunc *) (leaf->value);

                /* clear stack */
                (context->interpre_RxStckTop) = (context->rexx_proc)[(context->rexx_rx_proc)].stacktop;

                /* jump */
                (context->interpreRxcip) = (CIPTYPE *) (
                        (byte huge *) (context->interpreRxcodestart) +
                        (size_t) (func->label));
                goto main_loop;

                /* JMP w[pos]    */
                /* unconditional jump to POSition */
            case OP_JMP:
                DEBUGDISPLAY0nl("JMP");
                (context->interpreRxcip) = (CIPTYPE *) (
                        (byte huge *) (context->interpreRxcodestart) +
                        *(CWORD *) (context->interpreRxcip));
#ifdef __DEBUG__
                if ((context->rexx__debug__))
                 printf("%d\n",(byte huge *)(context->interpreRxcip)-(byte huge *)(context->interpreRxcodestart));
#endif
                goto main_loop;

                /* JF w[pos]   */
                /* jump if top is 0 to POSition */
            case OP_JF:
                DEBUGDISPLAY0nl("JF");
#ifdef __DEBUG__
                if ((context->rexx__debug__)) {
                 w = *(CWORD *)(context->interpreRxcip);
                 if (!Lbool(STACKTOP))
                  printf("%ld *\n",w);
                 else
                  printf("%ld\n",w);
                }
#endif
                if (!Lbool(
                        (context->interpre_RxStck)[(context->interpre_RxStckTop)--]))
                    (context->interpreRxcip) = (CIPTYPE *) (
                            (byte huge *) (context->interpreRxcodestart) +
                            *(CWORD *) (context->interpreRxcip));
                else
                    INCWORD((context->interpreRxcip));
                goto main_loop;

                /* JT w[pos]   */
                /* jump if top is 1 to POSition */
            case OP_JT:
                DEBUGDISPLAY0nl("JT");
#ifdef __DEBUG__
                if ((context->rexx__debug__)) {
                 w = *(CWORD *)(context->interpreRxcip);
                 if (Lbool(STACKTOP))
                  printf("%ld *\n",w);
                 else
                  printf("%ld\n",w);
                }
#endif
                if (Lbool(
                        (context->interpre_RxStck)[(context->interpre_RxStckTop)--]))
                    (context->interpreRxcip) = (CIPTYPE *) (
                            (byte huge *) (context->interpreRxcodestart) +
                            *(CWORD *) (context->interpreRxcip));
                else
                    INCWORD((context->interpreRxcip));
                goto main_loop;

                /* CALL p[label] b[noargs] w[existarg] */
                /* create new stack and jmp to LABEL pos*/
            case OP_CALL:
                DEBUGDISPLAY0nl("CALL");
                if (I_CallFunction())
                    goto chk4trace;
                goto main_loop;

                /* RETURN   */
                /* clear stack and return */
                /* if first prg then exit */
            case OP_RETURN:
                DEBUGDISPLAY0("RETURN");
                if ((context->rexx_proc)[(context->rexx_rx_proc)].calltype ==
                    CT_FUNCTION)
                    (context->lstring_Lerror)(ERR_NO_DATA_RETURNED, 0);
                if ((context->rexx_rx_proc) == 0) { /* root program */
                    (context->rexxrxReturnCode) = 0;
                    if (CMScalltype() == 5)
                        Lscpy(&(context->rexxrxReturnResult), "0");
                    goto interpreter_fin;
                }
                I_ReturnProc();
                goto main_loop;

                /* RETURNF   */
                /* move top of stack to correct */
                /* position of return arg and */
                /* clear stack   */
            case OP_RETURNF:
                DEBUGDISPLAY0("RETURNF");
                if ((context->rexx_rx_proc) == 0) { /* Root program */
                    if (CMScalltype() == 5) {
                        Lstrcpy(&(context->rexxrxReturnResult),
                                (context->interpre_RxStck)[(context->interpre_RxStckTop)--]);
                        (context->rexxrxReturnCode) = 0;
                    } else
                        (context->rexxrxReturnCode) = (int) Lrdint(
                                (context->interpre_RxStck)[(context->interpre_RxStckTop)--]);
                    goto interpreter_fin;
                } else if (
                        (context->rexx_proc)[(context->rexx_rx_proc)].calltype !=
                        CT_PROCEDURE)
/**
// It is possible to do a DUP in the compile code of returnf
**/
                    Lstrcpy((context->rexx_proc)[(context->rexx_rx_proc)].arg.r,
                            STACKTOP);
                else {
                    /* is the Variable space private? */
                    /* proc: PROCEDURE */
                    if ((context->interpre_VarScope) !=
                        (context->rexx_proc)[(context->rexx_rx_proc) - 1].scope)
                        /* not a tmp var */
                        if (STACKTOP !=
                            &((context->interpre__tmpstr)[(context->interpre_RxStckTop)])) {
                            Lstrcpy(&((context->interpre__tmpstr)[(context->interpre_RxStckTop)]),
                                    STACKTOP);
                            STACKTOP =
                                    &((context->interpre__tmpstr)[(context->interpre_RxStckTop)]);
                        }
                    /* point the return data */
                    a = STACKTOP;
                }

                I_ReturnProc();

                if ((context->rexx_proc)[(context->rexx_rx_proc) +
                                         1].calltype == CT_PROCEDURE)
                    /* Assign the the RESULT variable */
                    RxVarSet((context->interpre_VarScope),
                             (context->rexxresultStr), a);
                goto main_loop;

                /* INTERPRET [string] */
            case OP_INTERPRET:
                DEBUGDISPLAY("INTERPRET");
                /* copy to a temporary var */
                if (STACKTOP !=
                    &((context->interpre__tmpstr)[(context->interpre_RxStckTop)])) {
                    Lstrcpy(&((context->interpre__tmpstr)[(context->interpre_RxStckTop)]),
                            STACKTOP);
                    STACKTOP = &((context->interpre__tmpstr)[(context->interpre_RxStckTop)]);
                }
                RxInitInterStr();
                goto main_loop;

            case OP_INTER_END:
                DEBUGDISPLAY0("INTER_END");
                RxDoneInterStr();
                goto main_loop;

                /* PROC */
            case OP_PROC:
                DEBUGDISPLAY0("ERROR-PROC");
                (context->lstring_Lerror)(ERR_UNEXPECTED_PROC, 1);
                goto chk4trace;

                /* SAY   */
                /* display TOP item */
            case OP_SAY:
                DEBUGDISPLAY("SAY");
                Lprint(STDOUT,
                       (context->interpre_RxStck)[(context->interpre_RxStckTop)--]);
                PUTCHAR('\n');
                goto main_loop;

                /* SYSTEM  */
                /* execute a system call*/
            case OP_SYSTEM:
                DEBUGDISPLAY2("SYSTEM");
                L2STR(STACKTOP);
                LASCIIZ(*(STACKTOP));
                RxExecuteCmd(STACKTOP, STACKP(1));
                (context->interpre_RxStckTop) -= 2;
                goto main_loop;

                /* EXIT   */
                /* exit prg with RC */
            case OP_EXIT:
                DEBUGDISPLAY("EXIT");
                if (CMScalltype() == 5) {
                    Lstrcpy(&(context->rexxrxReturnResult),
                            (context->interpre_RxStck)[(context->interpre_RxStckTop)--]);
                    (context->rexxrxReturnCode) = 0;
                } else
                    (context->rexxrxReturnCode) = (int) Lrdint(
                            (context->interpre_RxStck)[(context->interpre_RxStckTop)--]);
                /* free everything from stack */
#ifndef __DEBUG__
                (context->interpre_RxStckTop) = -1;
#endif
                goto interpreter_fin;

                /* IEXIT   */
                /* Implicit exit prg */
            case OP_IEXIT:
                DEBUGDISPLAY("IEXIT");
                if ((context->rexx_proc)[(context->rexx_rx_proc)].calltype ==
                    CT_FUNCTION)
                    (context->lstring_Lerror)(ERR_NO_DATA_RETURNED, 0);
                (context->rexxrxReturnCode) = 0;
                if (CMScalltype() == 5)
                    Lscpy(&(context->rexxrxReturnResult), "0");
                /* free everything from stack */
#ifndef __DEBUG__
                (context->interpre_RxStckTop) = -1;
#endif
                goto interpreter_fin;

                /* PARSE  */
                /* Initialise PARSING */
            case OP_PARSE:
                DEBUGDISPLAY("PARSE");
                /* Do not remove from stack */
                (context->interpre_ToParse) = STACKTOP;
                L2STR((context->interpre_ToParse));
                (context->interpre_DataStart) = (context->interpre_BreakStart) = (context->interpre_BreakEnd) = 1;
                (context->interpre_SourceEnd) =
                        LLEN(*(context->interpre_ToParse)) + 1;
                goto main_loop;

                /* PVAR   */
                /* Parse to stack */
            case OP_PVAR:
                DEBUGDISPLAY0("PVAR");
                if ((context->interpre_BreakEnd) <=
                    (context->interpre_DataStart))
                    (context->interpre_DataEnd) = (context->interpre_SourceEnd);
                else
                    (context->interpre_DataEnd) = (context->interpre_BreakStart);

                if ((context->interpre_DataEnd) !=
                    (context->interpre_DataStart))
                    _Lsubstr(
                            (context->interpre_RxStck)[(context->interpre_RxStckTop)--],
                            (context->interpre_ToParse),
                            (context->interpre_DataStart),
                            (context->interpre_DataEnd) -
                            (context->interpre_DataStart));
                else {
                    LZEROSTR(*(STACKTOP));
                    (context->interpre_RxStckTop)--;
                }
                if ((context->interpre__trace)) {
                    (context->interpre_RxStckTop)++;
                    TraceInstruction(*(context->interpreRxcip));
                    (context->interpre_RxStckTop)--;
                }
                (context->interpreRxcip)++;
                goto main_loop;
                /*** goto chk4trace; ***/

                /* PDOT   */
                /* Parse to hyperspace */
            case OP_PDOT:
                /* Only for debugging */
                DEBUGDISPLAY0("PDOT");
                if ((context->interpre__trace)) {
                    /* Make space */
                    (context->interpre_RxStckTop)++;
                    STACKTOP = &((context->interpre__tmpstr)[(context->interpre_RxStckTop)]);
                    if ((context->interpre_BreakEnd) <=
                        (context->interpre_DataStart))
                        (context->interpre_DataEnd) = (context->interpre_SourceEnd);
                    else
                        (context->interpre_DataEnd) = (context->interpre_BreakStart);
                    if ((context->interpre_DataEnd) !=
                        (context->interpre_DataStart))
                        _Lsubstr(STACKTOP, (context->interpre_ToParse),
                                 (context->interpre_DataStart),
                                 (context->interpre_DataEnd) -
                                 (context->interpre_DataStart));
                    else LZEROSTR(*(STACKTOP));
                    TraceInstruction(*(context->interpreRxcip));
                    (context->interpre_RxStckTop)--; /* free space */
                }
                (context->interpreRxcip)++;
                goto main_loop;

                /* TR_SPACE  */
                /* trigger a space */
            case OP_TR_SPACE:
                DEBUGDISPLAY0("TR_SPACE");
                I_trigger_space();
                goto main_loop;

                /* TR_LIT   */
                /* trigger a litteral from stck */
            case OP_TR_LIT:
                DEBUGDISPLAY("TR_LIT");
                (context->interpre_DataStart) = (context->interpre_BreakEnd);
                I_trigger_litteral(
                        (context->interpre_RxStck)[(context->interpre_RxStckTop)--]);
                goto main_loop;

                /* TR_ABS   */
                /* trigger ABSolute position */
            case OP_TR_ABS:
                DEBUGDISPLAY("TR_ABS");
/**
//   L2INT(**A);
**/
                (context->interpre_DataStart) = (context->interpre_BreakEnd);
                (context->interpre_BreakStart) = (size_t) LINT(
                        *((context->interpre_RxStck)[(context->interpre_RxStckTop)--]));

                /* check for boundaries */
                (context->interpre_BreakStart) = RANGE(1,
                                                       (context->interpre_BreakStart),
                                                       (context->interpre_SourceEnd));
                (context->interpre_BreakEnd) = (context->interpre_BreakStart);
                goto main_loop;

                /* TR_REL   */
                /* trigger RELative position */
            case OP_TR_REL:
                DEBUGDISPLAY("TR_REL");

/**
//   L2INT(**A);
**/
                (context->interpre_DataStart) = (context->interpre_BreakStart);
                (context->interpre_BreakStart) = (context->interpre_DataStart) +
                                                 (size_t) LINT(
                                                         *((context->interpre_RxStck)[(context->interpre_RxStckTop)--]));

                /* check for boundaries */
                (context->interpre_BreakStart) = RANGE(1,
                                                       (context->interpre_BreakStart),
                                                       (context->interpre_SourceEnd));
                (context->interpre_BreakEnd) = (context->interpre_BreakStart);
                goto main_loop;

                /* TR_END   */
                /* trigger to END of data */
            case OP_TR_END:
                DEBUGDISPLAY0("TR_END");
                (context->interpre_DataStart) = (context->interpre_BreakEnd);
                (context->interpre_BreakStart) = (context->interpre_SourceEnd);
                (context->interpre_BreakEnd) = (context->interpre_SourceEnd);
                goto main_loop;

                /* RX_QUEUE   */
                /* queue stck to Rexx queue */
            case OP_RX_QUEUE:
                DEBUGDISPLAY("RX_QUEUE");
                LPMALLOC(a); /* duplicate variable */
                Lfx(a, 1);
                Lstrcpy(a,
                        (context->interpre_RxStck)[(context->interpre_RxStckTop)--]);
                Queue2Stack(a);
                goto main_loop;

                /* RX_PUSH   */
                /* push stck to Rexx queue */
            case OP_RX_PUSH:
                DEBUGDISPLAY("RX_PUSH");
                LPMALLOC(a); /* duplicate variable */
                Lfx(a, 1);
                Lstrcpy(a,
                        (context->interpre_RxStck)[(context->interpre_RxStckTop)--]);
                Push2Stack(a);
                goto main_loop;

                /* RX_PULL   */
                /* pull stck from Rexx queue */
            case OP_RX_PULL:
                (context->interpre_RxStckTop)++;
                STACKTOP = &((context->interpre__tmpstr)[(context->interpre_RxStckTop)]);
                a = NULL;
                Lread(context->rawstdin, STACKTOP, LREADLINE);
                DEBUGDISPLAY("RX_PULL");
                goto main_loop;

                /* RX_EXTERNAL   */
                /* read data from external queue */
            case OP_RX_EXTERNAL:
                (context->interpre_RxStckTop)++;
                STACKTOP = &((context->interpre__tmpstr)[(context->interpre_RxStckTop)]);
                Lread(context->rawstdin, STACKTOP, LREADLINE);
                DEBUGDISPLAY("RX_EXTERNAL");
                goto main_loop;

            case OP_EQ:
                DEBUGDISPLAY2("EQ");
                a = STACKP(2);
                LICPY(*a, Leq(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_NE:
                DEBUGDISPLAY2("NE");
                a = STACKP(2);
                LICPY(*a, Lne(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_GT:
                DEBUGDISPLAY2("GT");
                a = STACKP(2);
                LICPY(*a, Lgt(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_GE:
                DEBUGDISPLAY2("GE");
                a = STACKP(2);
                LICPY(*a, Lge(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_LT:
                DEBUGDISPLAY2("LT");
                a = STACKP(2);
                LICPY(*a, Llt(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_LE:
                DEBUGDISPLAY2("LE");
                a = STACKP(2);
                LICPY(*a, Lle(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_DEQ:
                DEBUGDISPLAY2("DEQ");
                a = STACKP(2);
                LICPY(*a, Ldeq(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_DNE:
                DEBUGDISPLAY2("DNE");
                a = STACKP(2);
                LICPY(*a, Ldne(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_DGT:
                DEBUGDISPLAY2("DGT");
                a = STACKP(2);
                LICPY(*a, Ldgt(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_DGE:
                DEBUGDISPLAY2("DGE");
                a = STACKP(2);
                LICPY(*a, Ldge(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_DLT:
                DEBUGDISPLAY2("DLT");
                a = STACKP(2);
                LICPY(*a, Ldlt(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_DLE:
                DEBUGDISPLAY2("DLE");
                a = STACKP(2);
                LICPY(*a, Ldle(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_TEQ:
                DEBUGDISPLAY2("TEQ");
                a = &((context->interpre__tmpstr)[
                        (context->interpre_RxStckTop) - 1]);
                LICPY(*a, Leq(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop)--;
                STACKTOP = &(context->interpre__tmpstr)[(context->interpre_RxStckTop)];
                goto chk4trace;

            case OP_TNE:
                DEBUGDISPLAY2("TNE");
                a = &((context->interpre__tmpstr)[
                        (context->interpre_RxStckTop) - 1]);
                LICPY(*a, Lne(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop)--;
                STACKTOP = &(context->interpre__tmpstr)[(context->interpre_RxStckTop)];
                goto chk4trace;

            case OP_TDEQ:
                DEBUGDISPLAY2("TDEQ");
                a = &((context->interpre__tmpstr)[
                        (context->interpre_RxStckTop) - 1]);
                LICPY(*a, Ldeq(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop)--;
                STACKTOP = &(context->interpre__tmpstr)[(context->interpre_RxStckTop)];
                goto chk4trace;

            case OP_TDNE:
                DEBUGDISPLAY2("TNDE");
                a = &((context->interpre__tmpstr)[
                        (context->interpre_RxStckTop) - 1]);
                LICPY(*a, Ldne(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop)--;
                STACKTOP = &(context->interpre__tmpstr)[(context->interpre_RxStckTop)];
                goto chk4trace;

            case OP_TGT:
                DEBUGDISPLAY2("TGT");
                a = &((context->interpre__tmpstr)[
                        (context->interpre_RxStckTop) - 1]);
                LICPY(*a, Lgt(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop)--;
                STACKTOP = &(context->interpre__tmpstr)[(context->interpre_RxStckTop)];
                goto chk4trace;

            case OP_TGE:
                DEBUGDISPLAY2("TGE");
                a = &((context->interpre__tmpstr)[
                        (context->interpre_RxStckTop) - 1]);
                LICPY(*a, Lge(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop)--;
                STACKTOP = &(context->interpre__tmpstr)[(context->interpre_RxStckTop)];
                goto chk4trace;

            case OP_TLT:
                DEBUGDISPLAY2("TLT");
                a = &((context->interpre__tmpstr)[
                        (context->interpre_RxStckTop) - 1]);
                LICPY(*a, Llt(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop)--;
                STACKTOP = &(context->interpre__tmpstr)[(context->interpre_RxStckTop)];
                goto chk4trace;

            case OP_TLE:
                DEBUGDISPLAY2("TLE");
                a = &((context->interpre__tmpstr)[
                        (context->interpre_RxStckTop) - 1]);
                LICPY(*a, Lle(STACKP(1), STACKTOP));
                (context->interpre_RxStckTop)--;
                STACKTOP = &(context->interpre__tmpstr)[(context->interpre_RxStckTop)];
                goto chk4trace;

            case OP_NOT:
                DEBUGDISPLAY("NOT");
                a = STACKP(1);
                LICPY(*a, !Lbool(STACKTOP));
                (context->interpre_RxStckTop)--;
                goto chk4trace;

            case OP_AND:
                DEBUGDISPLAY2("AND");
                a = STACKP(2);
                LICPY(*a, Lbool(STACKP(1)) & Lbool(STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_OR:
                DEBUGDISPLAY2("OR");
                a = STACKP(2);
                LICPY(*a, Lbool(STACKP(1)) | Lbool(STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_XOR:
                DEBUGDISPLAY2("XOR");
                a = STACKP(2);
                LICPY(*a, Lbool(STACKP(1)) ^ Lbool(STACKTOP));
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_CONCAT:
                DEBUGDISPLAY2("CONCAT");
                a = STACKP(2);
                if (a != STACKTOP) {
                    Lstrcpy(a, STACKP(1));
                    Lstrcat(a, STACKTOP);
                } else {
                    Lstrcpy(&((context->interpre__tmpstr)[(context->interpre_RxStckTop)]),
                            STACKTOP);
                    Lstrcpy(a, STACKP(1));
                    Lstrcat(a,
                            &((context->interpre__tmpstr)[(context->interpre_RxStckTop)]));
                }
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_BCONCAT:
                DEBUGDISPLAY2("BCONCAT");
                a = STACKP(2);
                if (a == STACKTOP) {
                    Lstrcpy(&((context->interpre__tmpstr)[(context->interpre_RxStckTop)]),
                            STACKTOP);
                    STACKTOP = &((context->interpre__tmpstr)[(context->interpre_RxStckTop)]);
                }
                Lstrcpy(a, STACKP(1));
                L2STR(a);
                LSTR(*a)[LLEN(*a)] = ' ';
                LLEN(*a)++;
                Lstrcat(a, STACKTOP);
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_NEG:
                DEBUGDISPLAY("NEG");
                Lneg(STACKP(1), STACKTOP);
                (context->interpre_RxStckTop)--;
                goto chk4trace;

            case OP_INC:
                DEBUGDISPLAY("INC");
                Linc((context->interpre_RxStck)[(context->interpre_RxStckTop)--]);
                goto chk4trace;

            case OP_DEC:
                DEBUGDISPLAY("DEC");
                Ldec((context->interpre_RxStck)[(context->interpre_RxStckTop)--]);
                goto chk4trace;

            case OP_ADD:
                DEBUGDISPLAY2("ADD");
                Ladd(STACKP(2), STACKP(1), STACKTOP);
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_SUB:
                DEBUGDISPLAY2("SUB");
                Lsub(STACKP(2), STACKP(1), STACKTOP);
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_MUL:
                DEBUGDISPLAY2("MUL");
                Lmult(STACKP(2), STACKP(1), STACKTOP);
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_DIV:
                DEBUGDISPLAY2("DIV");
                Ldiv(STACKP(2), STACKP(1), STACKTOP);
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_IDIV:
                DEBUGDISPLAY2("IDIV");
                Lintdiv(STACKP(2), STACKP(1), STACKTOP);
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_MOD:
                DEBUGDISPLAY2("MOD");
                Lmod(STACKP(2), STACKP(1), STACKTOP);
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            case OP_POW:
                DEBUGDISPLAY2("POW");
                Lexpose(STACKP(2), STACKP(1), STACKTOP);
                (context->interpre_RxStckTop) -= 2;
                goto chk4trace;

            default:
                DEBUGDISPLAY0("error, unknown mnemonic");
                (context->interpreRxcip)--;
#ifndef WIN
                fprintf(STDERR, "Opcode found=%ld (0x%02lX)\n",
                        *(context->interpreRxcip), *(context->interpreRxcip));
#else
                PUTS("Opcode found=0x"); PUTINT(*(context->interpreRxcip),0,16);
#endif
                (context->lstring_Lerror)(ERR_INTERPRETER_FAILURE, 0);
        }
        chk4trace:
        if ((context->interpre__trace))
            TraceInstruction(*(context->interpreRxcip));
        (context->interpreRxcip)++; /* skip trace byte */
    }
    interpreter_fin:
    SIGNAL(SIGINT, SIG_IGN);

    /* Unwind and free any procedure contexts */
    while ((context->rexx_rx_proc) > 0) {
        /* free everything that it is new */
        if ((context->interpre_VarScope) !=
            (context->rexx_proc)[(context->rexx_rx_proc) - 1].scope) {
            RxScopeFree((context->interpre_VarScope));
            FREE((context->interpre_VarScope));
        }

        if ((context->rexx_proc)[(context->rexx_rx_proc)].env !=
            (context->rexx_proc)[(context->rexx_rx_proc) - 1].env) LPFREE(
                (context->rexx_proc)[(context->rexx_rx_proc)].env);

        (context->rexx_rx_proc)--;
        (context->interpreRx_id) = (context->rexx_proc)[(context->rexx_rx_proc)].id;
        (context->interpre_VarScope) = (context->rexx_proc)[(context->rexx_rx_proc)].scope;
        (context->lstring_lNumericDigits) = (context->rexx_proc)[(context->rexx_rx_proc)].digits;
        if ((context->rexx_proc)[(context->rexx_rx_proc)].trace &
            (normal_trace | off_trace | error_trace))
            (context->interpre__trace) = FALSE;
        else
            (context->interpre__trace) = TRUE;
    }

    return (context->rexxrxReturnCode);
} /* RxInterpret */
