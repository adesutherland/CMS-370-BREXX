/* Global Variable Context - for reentrancy */
#define __CONTEXT_C__

#include <stdio.h>
#include <stdlib.h>
#include <cmssys.h>
#include "context.h"

void InitContext() {
    /* printf("InitContext()\n"); */
    Context *context = (Context *) CMSPGAll(sizeof(Context));

    /* bmem.c */
    context->bmem_mem_head = NULL;
    context->bmem_total_mem = 0L;

    /* lstring.h */
    context->lstring_lLastScannedNumber = 0.0;
#ifdef __CMS__
    context->lstring_lNumericDigits = 9;
#else
    context->lstring_lNumericDigits = LMAXNUMERICDIGITS;
#endif

    /* ltime.c */
    context->ltime_elapsed = 0.0;

    /* rexxfunc.c */
    context->rexxfunc_ExtraFuncs = NULL;

    /* interpre.c */
    context->interpre_no_user_fp = 0;
    context->interpre_no_loc_fp = 0;
    context->interpre_no_sys_fp = 0;

    context->rawstdin = fopen("CONSOLE","rb");
}

#undef __CONTEXT_C__
