/* Global Variable Context - for reentrancy */
#define __CONTEXT_C__

#include <stdio.h>
#include <stdlib.h>
#include "context.h"

void PopContext()
{
  Context *oldContext;
  /* printf("PopContext()\n"); */
  if (currentContext) {
    oldContext = currentContext;
    currentContext = oldContext->previous;
    free(oldContext);
  }
  else printf("currentContext is null\n");
}

void PushContext()
{
  /* printf("PushContext()\n"); */
  Context *newContext = malloc(sizeof(Context));
  newContext->previous = currentContext;
  currentContext = newContext;

  /* bmem.c */
  currentContext->bmem_mem_head = NULL;
  currentContext->bmem_total_mem = 0L;

  /* lstring.h */
  currentContext->lstring_lLastScannedNumber=0.0;
#ifdef __CMS__
  currentContext->lstring_lNumericDigits = 9;
#else
  currentContext->lstring_lNumericDigits = LMAXNUMERICDIGITS;
#endif

  /* ltime.c */
  currentContext->ltime_elapsed=0.0;

  /* rexxfunc.c */
  currentContext->rexxfunc_ExtraFuncs = NULL;

}

#undef __CONTEXT_C__
