/*
 * main.c
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <cmssys.h>

#include "lstring.h"
#include "rexx.h"
#include "rxdefs.h"
#include "context.h"

/* ------- Includes for any other external library ------- */
#ifdef RXCONIO
extern void __CDECL RxConIOInitialize();
#endif

#ifdef RXMYSQLSTATIC
# include "rxmysql.c"
#endif

/* Compare argument */
int issamearg(const char* arg, const char* val)
{
  const char *c1 = arg;
  const char *c2 = val;
  while (*c1 == toupper(*c2)) {
    if (*c1 == 0) return 1;
    c1++;
    c2++;
  }
  return 0;
}

/* --------------------- main ---------------------- */
int __CDECL
main(int ac, char *av[])
{
 Lstr args[MAXARGS], tracestr;
 int ia, ir;
 int returnCode;

 /* Reset HI Flag */
 if (CMSGetFlag(HALTFLAG)) CMSSetFlag(HALTFLAG,0);

 PushContext();

 for (ia=0; ia<MAXARGS; ia++) LINITSTR(args[ia]);
 LINITSTR(tracestr);

 /* Start Processing arguments */
 ia = 1;

 /* No program name */
 if (ac < 2) {
  PopContext();
  return 0;
 }

 /* Detct "DMSREX EXEC" and skip extra word */
 if ( issamearg("DMSREX", av[0]) &&
      issamearg("EXEC", av[1]) ) ia++;

 /* No program name after "DMSREX EXEC" */
 if (ac < 1 + ia) {
  PopContext();
  return 0;
 }

#ifdef __DEBUG__
 __debug__ = FALSE;
#endif

 /* --- Initialise --- */
 RxInitialize(av[ia]);

 /* Trace Flag */
 if (CMSGetFlag(TRACEFLAG)) Lscpy(&tracestr,"A"); /* Trace All? */

 /* prepare arguments for program */
 for (ir=ia+1; ir<ac; ir++) {
   Lcat(&args[0], av[ir]);
   if (ir<ac-1) Lcat(&args[0]," ");
 }
 RxRun(av[ia],NULL,args,&tracestr,NULL);

 /* --- Free everything --- */
 RxFinalize();
 for (ia=0; ia<MAXARGS; ia++) LFREESTR(args[ia]);
 LFREESTR(tracestr);

#ifdef __DEBUG__
 if (mem_allocated()!=0) {
  fprintf(STDERR,"\nMemory left allocated: %ld\n",mem_allocated());
  mem_list();
 }
#endif

 returnCode = rxReturnCode; /* The Global Context will die on the next line ... */
 PopContext();

 if (!currentContext) {
   /* Base call from CMS */
   CMSSetFlag(HALTFLAG,0);
   CMSSetFlag(TRACEFLAG,0);
 }
 return returnCode;
} /* main */
