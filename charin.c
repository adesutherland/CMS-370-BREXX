/* VM/370 CMS and GCCLIB charin.c */
#include <stdio.h>
#include "lstring.h"

/* ---------------- Lcharin ------------------- */
void __CDECL
Lcharin( FILEP f, const PLstr line, const long start, const long length )
{
  int l;

  if (start>=1) {
    l = fseek(f,start-1,SEEK_SET);
    if (l)
     Lerror(ERR_NOT_RANDOM_ACCESS,0);
  }
  if (length<=0) {
    LZEROSTR(*line);
    return;
  }
  Lread(f,line,length);
} /* Lcharin */
