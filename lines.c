/* /* VM/370 CMS and GCCLIB lines.c */

#include <stdio.h>
#include "lstring.h"

/* ---------------- Llines ------------------- */
long Llines( FILEP f )
{
  long l;
  l = fgetrec(f);  /* read current record */
  if (l==-1) {
    /* Stream does not support record counting */
    return !fateof(f);
  }
  return fgetrecs(f) - l;
} /* Llines */
