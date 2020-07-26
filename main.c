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

void __CRT0(void); /* Entry Point pointer */

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
 int entry_point;

 if (!ac) return -1; /* Should never happen! */

 if (ac==1) {
   printf("BREXX Version %s\n", CMS_VERSION);

   if (issamearg("DMSREX", av[0])) {
     /* Temporary aid to register entry point for HRC402DS           */
     /* Will be removed once BREXX CMS deployment approach confirmed */

     /* Entry point for REXX is stored @ 0x90 - NUCRSV6 */
     #define REXX_ENTRY_HANDLE 0x90

     /* Register Entry Point Address */
     entry_point = (int)__CRT0;
     CMSSetNUCON((void*)REXX_ENTRY_HANDLE, entry_point);

     printf("BREXX Entry Address is %x saved in NUCON at %x\n", entry_point, REXX_ENTRY_HANDLE);
   }

   return 0;
 }

 PushContext();

 for (ia=0; ia<MAXARGS; ia++) LINITSTR(args[ia]);
 LINITSTR(tracestr);

 /* Start Processing arguments */
 ia = 1;

 /* Debug flag */
 if (issamearg("-D", av[ia])) {
   ia++;
   #ifdef __DEBUG__
    __debug__ = TRUE;
   #else
     printf("WARNING: BREXX not compiled with debug, -D ignored\n");
   #endif
 }

 /* No program name */
 if (ia >= ac) {
  PopContext();
  return 0;
 }

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
 if (__debug__ && mem_allocated()!=0) {
  fprintf(STDERR,"\nMemory left allocated: %ld\n",mem_allocated());
  mem_list();
 }
#endif

 returnCode = rxReturnCode; /* The Global Context will die on the next line ... */
 PopContext();

 return returnCode;
} /* main */
