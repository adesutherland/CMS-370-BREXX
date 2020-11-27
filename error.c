/*
 * $Id: error.c,v 1.10 2008/07/15 07:40:25 bnv Exp $
 * $Log: error.c,v $
 * Revision 1.10  2008/07/15 07:40:25  bnv
 * #include changed from <> to ""
 *
 * Revision 1.9  2006/01/26 10:25:11  bnv
 * Windows CE support
 *
 * Revision 1.8  2004/08/16 15:28:44  bnv
 * Spaces
 *
 * Revision 1.7  2004/04/30 15:29:44  bnv
 * const removed
 *
 * Revision 1.6  2003/10/30 13:16:28  bnv
 * Variable name change
 *
 * Revision 1.5  2002/08/22 12:31:28  bnv
 * Removed CR's
 *
 * Revision 1.4  2002/06/11 12:37:38  bnv
 * Added: CDECL
 *
 * Revision 1.3  2001/06/25 18:51:48  bnv
 * Header -> Id
 *
 * Revision 1.2  1999/11/26 13:13:47  bnv
 * Changed: To use the new macros
 *
 * Revision 1.1  1998/07/02 17:34:50  bnv
 * Initial revision
 *
 */

#include <stdarg.h>
#include <cmssys.h>
#include "lstring.h"

#include "rexx.h"
#include "trace.h"
#include "compile.h"
#include "interpre.h"
#include "variable.h"
#include "nextsymb.h"
#include "context.h"

/* ---------------- RxHaltTrap ----------------- */
void __CDECL
RxHaltTrap( int cnd )
{
 Context *context = (Context*)CMSGetPG();
 if ((context->rexx_proc)[(context->rexx_rx_proc)].condition & SC_HALT) {
  /* Reset HI */
  CMSSetFlag(HALTFLAG,0);
  RxSignalCondition(SC_HALT);
 } else (context->lstring_Lerror)(ERR_PROG_INTERRUPT,0);
} /* RxHaltTrap */

/* ---------------- RxSignalCondition -------------- */
void __CDECL
RxSignalCondition( int cnd )
{
 PBinLeaf leaf;
 RxFunc *func;
 PLstr cndstr;
 Context *context = (Context*)CMSGetPG();

/*///////// first we need to terminate all the interpret strings */
 switch (cnd) {
  case SC_ERROR:
   cndstr = (context->rexx_proc)[(context->rexx_rx_proc)].lbl_error;
   break;
  case SC_HALT:
   cndstr = (context->rexx_proc)[(context->rexx_rx_proc)].lbl_halt;
   break;
  case SC_NOVALUE:
   cndstr = (context->rexx_proc)[(context->rexx_rx_proc)].lbl_novalue;
   break;
  case SC_NOTREADY:
   cndstr = (context->rexx_proc)[(context->rexx_rx_proc)].lbl_notready;
   break;
  case SC_SYNTAX:
   cndstr = (context->rexx_proc)[(context->rexx_rx_proc)].lbl_syntax;
   break;
 }
 leaf = BinFind(&(context->rexx_labels),cndstr);
 if (leaf==NULL || ((RxFunc*)(leaf->value))->label==UNKNOWN_LABEL) {
  if (cnd==SC_SYNTAX) /* disable the error handling */
   (context->rexx_proc)[(context->rexx_rx_proc)].condition &= ~SC_SYNTAX;
  (context->lstring_Lerror)(ERR_UNEXISTENT_LABEL,1,cndstr);
 }
 func = (RxFunc*)(leaf->value);
 RxSetSpecialVar(SIGLVAR,TraceCurline(NULL,FALSE));
 (context->interpreRxcip) = (CIPTYPE*)((byte huge *)(context->interpreRxcodestart) + (size_t)(func->label));
 longjmp((context->rexx_error_trap),JMP_CONTINUE);
} /* RxSignalCondition */

/* ------------------ Rerror ------------------- */
void __CDECL
Rerror( const int errno, const int subno, ... )
{
 int line;
 RxFile *rxf;
#ifndef WIN
 va_list ap;
#endif
 Context *context = (Context*)CMSGetPG();

 if ((context->rexx_proc)[(context->rexx_rx_proc)].condition & SC_SYNTAX) {
  RxSetSpecialVar(RCVAR,errno);
  if ((context->nextsymbsymbolptr)==NULL) /* we are in intepret */
   RxSignalCondition(SC_SYNTAX);
  else {   /* we are in compile */
   (context->rexxrxReturnCode) = errno;
   longjmp((context->rexx_error_trap),JMP_ERROR);
  }
 } else {
  line = TraceCurline(&rxf,TRUE);
  if ((context->nextsymbsymbolptr)==NULL) /* we are in intepret */
   RxSetSpecialVar(SIGLVAR,line);

#ifndef WIN
  va_start(ap,subno);
  Lerrortext(&(context->error_errmsg),errno,subno,&ap);
  va_end(ap);

  if (LLEN((context->error_errmsg))==0)
   fprintf(STDERR," +++ Ooops unknown error %d.%d +++\n",errno,subno);
  else {
   LASCIIZ((context->error_errmsg));
   if (subno==0)
    fprintf(STDERR,
     "Error %d running %s, line %d: %s\n",
      errno,
      LSTR(rxf->name),
      line,
      LSTR((context->error_errmsg)));
   else
    fprintf(STDERR,
     "Error %d.%d running %s, line %d: %s\n",
      errno,
      subno,
      LSTR(rxf->name),
      line,
      LSTR((context->error_errmsg)));
  }
#else
  {
   PUTS("Error ");
   PUTINT(errno,0,10);
   PUTS(" running ");
   PUTS(LSTR(rxf->name));
   PUTS(" line ");
   PUTINT(line,0,10);
   PUTS(": ");
   Lerrortext(&(context->error_errmsg),errno,subno,NULL);
   Lprint(NULL,&(context->error_errmsg));
   PUTCHAR('\n');
  }
#endif

     /* Unwind and free any procedure contexts */
     while ((context->rexx_rx_proc) > 0) {
         /* free everything that it is new */
         if ((context->interpre_VarScope) != (context->rexx_proc)[(context->rexx_rx_proc) - 1].scope) {
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
         if ((context->rexx_proc)[(context->rexx_rx_proc)].trace & (normal_trace | off_trace | error_trace))
             (context->interpre__trace) = FALSE;
         else
             (context->interpre__trace) = TRUE;
     }

     (context->rexxrxReturnCode) = errno + 20000;
     longjmp((context->rexx_exit_trap), JMP_EXIT);
 }
} /* Rerror */
