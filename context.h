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
   int nextsymbsymbolhasdot;  /* symbol has a dot->stem */
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
   Lstr rexxrxReturnResult;  /* Global Return Result  */
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
   int interpre_no_user_fp;
   int interpre_no_loc_fp;
   int interpre_no_sys_fp;

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

   FILE* rawstdin;
};

void InitContext();

#endif
