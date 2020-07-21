/* VM/370 CMS and GCCLIB linein.c */

#include <stdio.h>
#include <errno.h>
#include "lstring.h"


/* ---------------- Llinein ------------------- */
void __CDECL
Llinein( FILEP f, const PLstr line, long start, long length )
{
  int l, i;
  char *c;

  /* initialise line */
  LZEROSTR(*line);

  /* find start line */
  if (start>=0) {
    if (!fsetrec(f,start)) {
      if (errno==ENOTBLK) Lerror(ERR_NOT_RECORD_ACCESS,0);
      else Lerror(ERR_INCORRECT_CALL,0);
    }
  }

  if (length<=0) return;

  else if (length==1) Lread(f,line,LREADLINE);

  else {
    Lfx(line,LREADINCSIZE);
    l = 0;
    while (length && (i=nextrecLen(f))>0) {
      if (l+i > LMAXLEN(*line)) {
        Lfx(line, (size_t)(l+i+LREADINCSIZE));
      }
      c = LSTR(*line) + l;
      fgets(c, i+1, f);
      l+=i;
      length--;
    }
    Lfx(line,l); /* Give back unwanted memory */
    LLEN(*line) = l;
    LTYPE(*line) = LSTRING_TY;
  }
} /* Llinein */
