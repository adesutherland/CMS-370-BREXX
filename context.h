/* Global Variable Context - for reentrancy */
#include "config.h"
#include "rexx.h"
#include "interpre.h"
#include "compile.h"
#include "nextsymb.h"
#include "dqueue.h"
#include "lstring.h"

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

typedef struct Context Context;

struct Context {
   /* interpre.h */
   int interpreRx_proc;  /* current procedure */
   int interpreRx_id;   /* current program id */
   CIPTYPE *interpreRxcodestart;  /* actual code          */
   CIPTYPE *interpreRxcip;   /* instruction pointer */

   /* compile.h */
   RxFile *compileCompileRxFile;  /* Rexx file pointer */
   byte *compileCompileCodePtr; /* code pointer  */
   PLstr compileCompileCode;  /* code space  */
#ifndef ALIGN
   word compileCompileCodeLen;  /* code length  */
#else
   dword compileCompileCodeLen;  /* code length  */
#endif
   Clause *compileCompileClause;  /* compile clauses */
   int compileCompileCurClause; /* current clause */
   int compileCompileClauseItems; /* maximum clause len */
   int compileCompileNesting;  /* current nesting */

   /* compile.c */
   int compile_str_interpreted; /* is it a string interpreted */
   int compile_checked_semicolon; /* if instruction has checked the semicolon likeIF */
   DQueue compile_Loop;

   /* nextsymb.h */
   enum symboltype nextsymbsymbol; /* contains the symbol  */
   Lstr nextsymbsymbolstr;  /* symbol identifier  */
   bool nextsymbsymbolisstr;  /* if litteral was inside quotes*/
   bool nextsymbsymbolPrevBlank; /* previous blank  */
   int nextsymbsymbolhasdot;  /* is symbol has a dot->stem */
   char *nextsymbsymbolptr;  /* current symbol pointer */
   char *nextsymbsymbolprevptr;  /* start of current symbol */
   int nextsymbsymboline;  /* current line number  */
   enum stat_type nextsymbsymbolstat; /* statement type  */

   /* nextsymb.c */
   PLstr nextsymb_ProgStr; /* pointer that holds the program string*/
   int nextsymb_InitNextch; /* NextChar initialised?  */
   bool nextsymb_NextBlank; /* Next char is blank   */
   bool nextsymb_commentfound; /* if comment found in nextchar  */
   bool nextsymb__in_nextsymbol;  /* Used only to track error inside nextsymb*/

   /* rexx.h */
#ifdef __DEBUG__
   int rexx__debug__;
#endif
   char *rexx_prgname; /* point to argv[0]  */
   jmp_buf rexx_error_trap; /* error trap for compile */
   jmp_buf rexx_exit_trap; /* exit from prg  */
   DQueue rexxrxStackList; /* dble queue of dble queues */
   RxFile *rexxrxFileList; /* rexx file list  */
   int rexxrxReturnCode; /* Global return code  */
   int rexx_procidcnt; /* procedure id counter  */
   RxProc *rexx_proc;  /* procedure & function array */
   int rexx_nesting; /* cur nesting set by TraceCurline */
   int rexx_rx_proc; /* current procedure id  */
   int rexx_proc_size; /* number of items in proc list */
   PLstr rexx_code;  /* code of program  */
   BinTree rexx_labels; /* Labels   */
   Args rexxrxArg;  /* global arguments for internal routines */
   BinTree rexxrxLitterals; /* Litterals   */
   BinLeaf *rexxnullStr; /* basic leaf Lstrings  */
   BinLeaf *rexxzeroStr;
   BinLeaf *rexxoneStr;
   BinLeaf *rexxresultStr;
   BinLeaf *rexxsiglStr;
   BinLeaf *rexxRCStr;
   BinLeaf *rexxerrorStr;
   BinLeaf *rexxhaltStr;
   BinLeaf *rexxsyntaxStr;
   BinLeaf *rexxsystemStr;
   BinLeaf *rexxnoValueStr;
   BinLeaf *rexxnotReadyStr;

   /* bmem.c */
   void *bmem_mem_head;
   long bmem_total_mem;

   /* error.c */
   Lstr error_errmsg;   /* initialise string from begining  */

   /* expr.h */
   int expr_exp_ct;
   size_t expr_exp_pos;

   /* variable.c */
   int variable_hashchar[256]; /* use the first char as hash value */
   PLstr variable_varname; /* variable name of prev find */
   Lstr variable_varidx;  /* index of previous find */
   Lstr variable_int_varname; /* used for the old RxFindVar */
   BinTree variable_PoolTree; /* external pools tree  */
   Lstr variable_stemvaluenotfound; /* this is the value of a stem if */

   /* interpre.c */
   int interpre__trace;   /* if trace is enabled */
   PLstr interpre_RxStck[STCK_SIZE]; /* Array of PLstr       */
   int interpre_RxStckTop;  /* top item of stack    */
   Lstr interpre__tmpstr[STCK_SIZE]; /* temporary strings */
   /* int interpre__interrupt; */ /* if any interrupt is pending */
   Scope interpre_VarScope; /* Variables Scope      */
   PLstr interpre_ToParse; /* Parsing variables */
   int interpre_DataStart;
   int interpre_DataEnd;
   int interpre_BreakStart;
   int interpre_BreakEnd;
   int interpre_SourceEnd; /* Length of string+1 */
   jmp_buf interpre_old_error; /* keep old value of errortrap */
#ifdef __PROFILE__
   int interpre_instr_cnt[256];  /* instruction counter */
#endif

   /* lstring.h */
   double lstring_lLastScannedNumber;
   int  lstring_lNumericDigits;
   byte lstring_l2u[256];
   byte lstring_u2l[256];
   LerrorFunc lstring_Lerror;

   /* ltime.c */
   double ltime_elapsed;

   /* rexx.c */
   jmp_buf rexx_old_trap;

   /* rexxfunc.c */
   BinTree *rexxfunc_ExtraFuncs;

   /* rxfiles.c */
   int rxfiles_file_size; /* file size in filelist structure */
   void* rxfiles_file;

   /* Previous Context */
   Context *previous;
};

#ifdef __CONTEXT_C__
# define EXTERN
#else
# define EXTERN extern
#endif
EXTERN Context *currentContext;
#undef EXTERN

void PopContext();
void PushContext();

/* Global Defines */

/* interpre.h */
#define Rx_proc (currentContext->interpreRx_proc)    /* current procedure */
#define Rx_id (currentContext->interpreRx_id)       /* current program id */
#define Rxcodestart (currentContext->interpreRxcodestart)  /* actual code */
#define Rxcip (currentContext->interpreRxcip)      /* instruction pointer */

/* compile.h */
#define CompileRxFile (currentContext->compileCompileRxFile)  /* Rexx file pointer */
#define CompileCodePtr (currentContext->compileCompileCodePtr) /* code pointer  */
#define CompileCode (currentContext->compileCompileCode) /* code space  */
#define CompileCodeLen (currentContext->compileCompileCodeLen) /* code length  */
#define CompileClause (currentContext->compileCompileClause) /* compile clauses */
#define CompileCurClause (currentContext->compileCompileCurClause) /* current clause */
#define CompileClauseItems (currentContext->compileCompileClauseItems) /* maximum clause len */
#define CompileNesting (currentContext->compileCompileNesting) /* current nesting */

/* nextsymb.h */
#define symbol (currentContext->nextsymbsymbol) /* contains the symbol  */
#define symbolstr (currentContext->nextsymbsymbolstr) /* symbol identifier  */
#define symbolisstr (currentContext->nextsymbsymbolisstr) /* if litteral was inside quotes*/
#define symbolhasdot (currentContext->nextsymbsymbolhasdot) /* is symbol has a dot->stem */
#define symbolPrevBlank (currentContext->nextsymbsymbolPrevBlank) /* previous blank  */
#define symbolptr (currentContext->nextsymbsymbolptr) /* current symbol pointer */
#define symbolprevptr (currentContext->nextsymbsymbolprevptr) /* start of current symbol */
#define symboline (currentContext->nextsymbsymboline) /* current line number  */
#define symbolstat (currentContext->nextsymbsymbolstat) /* statement type  */

/* nextsymb.c */
#define _in_nextsymbol (currentContext->nextsymb__in_nextsymbol)  /* Used only to track error inside nextsymb*/

/* rexx.h */
#ifdef __DEBUG__
#define __debug__ (currentContext->rexx__debug__)
#endif
#define _prgname (currentContext->rexx_prgname) /* point to argv[0]  */
#define _error_trap (currentContext->rexx_error_trap) /* error trap for compile */
#define _exit_trap (currentContext->rexx_exit_trap) /* exit from prg  */
#define rxStackList (currentContext->rexxrxStackList) /* dble queue of dble queues */
#define rxFileList (currentContext->rexxrxFileList) /* rexx file list  */
#define rxReturnCode (currentContext->rexxrxReturnCode) /* Global return code  */
#define _procidcnt (currentContext->rexx_procidcnt) /* procedure id counter  */
#define _proc  (currentContext->rexx_proc) /* procedure & function array */
#define _nesting (currentContext->rexx_nesting) /* cur nesting set by TraceCurline */
#define _rx_proc (currentContext->rexx_rx_proc) /* current procedure id  */
#define _proc_size (currentContext->rexx_proc_size) /* number of items in proc list */
#define _code  (currentContext->rexx_code) /* code of program  */
#define _labels (currentContext->rexx_labels) /* Labels   */
#define rxArg  (currentContext->rexxrxArg) /* global arguments for internal routines */
#define rxLitterals (currentContext->rexxrxLitterals) /* Litterals   */
#define nullStr (currentContext->rexxnullStr) /* basic leaf Lstrings  */
#define zeroStr (currentContext->rexxzeroStr)
#define oneStr (currentContext->rexxoneStr)
#define resultStr (currentContext->rexxresultStr)
#define siglStr (currentContext->rexxsiglStr)
#define RCStr (currentContext->rexxRCStr)
#define errorStr (currentContext->rexxerrorStr)
#define haltStr (currentContext->rexxhaltStr)
#define syntaxStr (currentContext->rexxsyntaxStr)
#define systemStr (currentContext->rexxsystemStr)
#define noValueStr (currentContext->rexxnoValueStr)
#define notReadyStr (currentContext->rexxnotReadyStr)

/* error.c */
#define errmsg (currentContext->error_errmsg)   /* initialise string from begining  */

/* variable.c */
#define stemvaluenotfound (currentContext->variable_stemvaluenotfound) /* this is the value of a stem if */

/* interpre.c */
#define _trace  (currentContext->interpre__trace)  /* if trace is enabled */
#define RxStck (currentContext->interpre_RxStck) /* Array of PLstr       */
#define RxStckTop (currentContext->interpre_RxStckTop)  /* top item of stack    */
#define _tmpstr (currentContext->interpre__tmpstr) /* temporary strings */
/* #define _interrupt (currentContext->interpre__interrupt) */ /* if any interrupt is pending */

/* lstring.h */
#define lLastScannedNumber (currentContext->lstring_lLastScannedNumber)
#define lNumericDigits (currentContext->lstring_lNumericDigits)
#define l2u (currentContext->lstring_l2u)
#define u2l (currentContext->lstring_u2l)
#define Lerror (currentContext->lstring_Lerror)

#endif
