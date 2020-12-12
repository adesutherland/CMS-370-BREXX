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
int issamearg(const char *arg, const char *val) {
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
main(int ac, char *av[]) {
    Lstr args[MAXARGS], tracestr;
    int ia, ir, i;
    int returnCode;
    int entry_point;
    Context *context;

    if (!ac) return -1; /* Should never happen! */

    if (ac == 1 && CMScalltype() != 5) {
        printf("BREXX Version %s\n", CMS_VERSION);

        if (issamearg("DMSREX", av[0])) {
            /* Temporary aid to register entry point for HRC402DS           */
            /* Will be removed once BREXX CMS deployment approach confirmed */

            /* Entry point for REXX is stored @ 0x90 - NUCRSV6 */
#define REXX_ENTRY_HANDLE 0x90

            /* Register Entry Point Address */
            entry_point = (int) __CRT0;
            CMSSetNUCON((void *) REXX_ENTRY_HANDLE, entry_point);

            printf("BREXX Entry Address is 0x%x saved in NUCON at 0x%x\n",
                   entry_point, REXX_ENTRY_HANDLE);
        }

        return 0;
    }

    InitContext();
    context = (Context *) CMSGetPG();

    for (ia = 0; ia < MAXARGS; ia++) LINITSTR(args[ia]);
    LINITSTR(tracestr);

    /* Start Processing arguments */
    if (CMScalltype() == 5) ia = 0;
    else {
        ia = 1;

        /* Debug flag */
#ifdef __DEBUG__
        (context->rexx__debug__) = FALSE;
#endif
        if (issamearg("-D", av[ia])) {
            ia++;
#ifdef __DEBUG__
            (context->rexx__debug__) = TRUE;
#else
            printf("WARNING: BREXX not compiled with debug, -D ignored\n");
#endif
        }
    }

    /* No program name */
    if (ia >= ac) {
        return 0;
    }

    /* --- Initialise --- */
    RxInitialize(av[ia]);

    /* Trace Flag */
    if (CMSGetFlag(TRACEFLAG)) Lscpy(&tracestr, "?A"); /* Trace All? */

    /* prepare arguments for program */
    if (CMScalltype() == 5) {
        ADLEN *param = CMSeplist()->ArgList;
        if (param) {
            for (i = 0; (int) (param[i].Data) != -1; i++) {
                Lmcpy(&args[i], param[i].Data, param[i].Len);
            }
        }
    } else {
        for (ir = ia + 1; ir < ac; ir++) {
            Lcat(&args[0], av[ir]);
            if (ir < ac - 1) Lcat(&args[0], " ");
        }
    }

    Lscpy(&(context->rexxrxReturnResult), "0");
    (context->rexxrxReturnCode) = 0;

    RxRun(av[ia], NULL, args, &tracestr, NULL);

    /* Need to get the result here before RxFinalise() */
    returnCode = (context->rexxrxReturnCode);
    L2STR(&(context->rexxrxReturnResult));
    LASCIIZ((context->rexxrxReturnResult)); /* Make sure its a C str */
    CMSreturndata(LSTR(context->rexxrxReturnResult),
                  LLEN(context->rexxrxReturnResult)); /* If calltype 5 */

    /* --- Free everything --- */
    RxFinalize();

    for (ia = 0; ia < MAXARGS; ia++) LFREESTR(args[ia]);
    LFREESTR(tracestr);

#ifdef __DEBUG__
    if ((context->rexx__debug__) && mem_allocated()!=0) {
     fprintf(STDERR,"\nMemory left allocated: %ld\n",mem_allocated());
     mem_list();
    }
#endif

    return returnCode;
} /* main */
