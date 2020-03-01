/*
   Lprint function for VM/CMS
 */

#include "lstring.h"

/* ---------------- Lprint ------------------- */
void __CDECL
Lprint( FILEP f, const PLstr str )
{
 Lstr tmp;

 if (str==NULL) {
  fprintf(f,"<NULL>");
  return;
 }

 switch (LTYPE(*str)) {
  case LSTRING_TY:
    fprintf(f, "%.*s", LLEN(*str), LSTR(*str));
    break;

  case LINTEGER_TY:
    fprintf(f,"%ld", LINT(*str));
    break;

  case LREAL_TY:
    LINITSTR(tmp);
    Lformat(&tmp, str, -1, -1, 0, 0);
    fprintf(f, "%.*s", LLEN(tmp), LSTR(tmp));
    LFREESTR(tmp);
    break;
 }
} /* Lprint */
