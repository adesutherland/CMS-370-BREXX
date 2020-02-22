/* Modified for VM/370 CMS and GCC by Robert O'Hara, July 2010. */
/*
 * $Id: trunc.c,v 1.6 2008/07/15 07:40:54 bnv Exp $
 * $Log: trunc.c,v $
 * Revision 1.6  2008/07/15 07:40:54  bnv
 * #include changed from <> to ""
 *
 * Revision 1.5  2008/07/14 13:08:16  bnv
 * MVS,CMS support
 *
 * Revision 1.4  2002/06/11 12:37:15  bnv
 * Added: CDECL
 *
 * Revision 1.3  2001/06/25 18:49:48  bnv
 * Header changed to Id
 *
 * Revision 1.2  1999/11/26 12:52:25  bnv
 * Changed: To use the fcvt()
 *
 * Revision 1.1  1998/07/02 17:18:00  bnv
 * Initial Version
 *
 */

#include "lstring.h"

/* ---------------- Ltrunc ----------------- */
void __CDECL
Ltrunc( const PLstr to, const PLstr from, long n)
{
#ifdef __CMS__
 Lformat(to, from, -1, n, 0, 0);
#else
#ifdef WCE
 char *snum, *s;
 int decp, sign;
#endif
 if (n<0) n = 0;

 if (!n) {
  Lstrcpy(to,from);
  L2REAL(to);
  LINT(*to)  = (long)LREAL(*to);
  LTYPE(*to) = LINTEGER_TY;
  LLEN(*to)  = sizeof(long);
 } else {
  L2REAL(from);
  Lfx(to,n+15);
#ifndef WCE
  /*sprintf(LSTR(*to),"%.*f", (int)n, LREAL(*from));*/
  GCVT(LREAL(*from) , (n), LSTR(*to));
#else
//////// WARNING NO ROUNDING ON THE NUMBER IS DONE!
#define NDIG 20
  s = LSTR(*to);
  snum = FCVT(LREAL(*from), NDIG, &decp, &sign);
  if (sign) *s++ = '-';
  if (decp<0)  // Move it in the front
   decp += NDIG;
  if (decp>=0) {
   while (decp--)
    *s++ = *snum++;
   *s++ = '.';
  } else {
   *s++ = '0';
   *s++ = '.';
   while (decp++ && n--)
    *s++ = '0';
  }
  while (n--)
   *s++ = *snum++;
  *s = 0;
#endif
  LTYPE(*to) = LSTRING_TY;
  LLEN(*to)  = STRLEN(LSTR(*to));
 }
#endif
} /* R_trunc */