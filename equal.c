/*
 * $Id: equal.c,v 1.7 2008/07/15 07:40:54 bnv Exp $
 * $Log: equal.c,v $
 * Revision 1.7  2008/07/15 07:40:54  bnv
 * #include changed from <> to ""
 *
 * Revision 1.6  2004/03/26 22:51:11  bnv
 * Increased the accuracy
 *
 * Revision 1.5  2002/06/11 12:37:15  bnv
 * Added: CDECL
 *
 * Revision 1.4  2001/06/25 18:49:48  bnv
 * Header changed to Id
 *
 * Revision 1.3  1999/11/26 09:56:55  bnv
 * Changed: To use the new macros.
 * Changed: From char* to byte* comparison, to avoid signed char problems.
 * Changed: To use the lLastScannedNumber
 *
 * Revision 1.2  1998/11/10 13:36:14  bnv
 * Comparison for reals is done with fabs(a-b)<smallnumber
 *
 * Revision 1.1  1998/07/02 17:18:00  bnv
 * Initial Version
 *
 */

#include <math.h>
#include <ctype.h>
#include <string.h>
#include <cmssys.h>
#include "lstring.h"

/* -------------------- Lequal ----------------- */
int __CDECL
Lequal(const PLstr A, const PLstr B) {
    int ta, tb;
    byte *a, *b;  /* start position in string */
    byte *ae, *be; /* ending position in string */
    double ra, rb;

    /* Are A & B both integers? - Optimise for this */
    if (LTYPE(*A) == LINTEGER_TY && LTYPE(*B) == LINTEGER_TY) {
        /* TODO - DIGITS and FUZZ */
        if ( LINT(*A) == LINT(*B) )
            return 0;
        else if (LINT(*A) > LINT(*B))
            return 1;
        else
            return -1;
    }

    /* OK Now go through the logic of comparing integers/floats/strings */
    Context *context = (Context *) CMSGetPG();

    if (LTYPE(*A) == LSTRING_TY) {
        ta = _Lisnum(A);

        /* check to see if the first argument is string? */
        if (ta == LSTRING_TY) {
            L2STR(B); /* make string and the second */
            goto eq_str; /* go and check strings  */
        }

        ra = (context->lstring_lLastScannedNumber);
    } else {
        ta = LTYPE(*A);
        ra = TOREAL(*A);
    }

    if (LTYPE(*B) == LSTRING_TY) {
        tb = _Lisnum(B);
        rb = (context->lstring_lLastScannedNumber);
    } else {
        tb = LTYPE(*B);
        rb = TOREAL(*B);
    }

    /* is B also a number */
    if (tb != LSTRING_TY) {
        /*
         * The numbers are floats and to work out if two floats are
         * equal we have to mess around with an epsilon
         */
        int magnitude_a, magnitude_b;
        double epsilon;
        if (ra == 0) magnitude_a = 0;
        else {
            if (ra < 0) magnitude_a = (int)log10(-ra);
            else magnitude_a = (int)log10(ra);
            if (magnitude_a >= 0) magnitude_a++;
        }
        if (rb == 0) magnitude_b = 0;
        else {
            if (rb < 0) magnitude_b = (int)log10(-rb);
            else magnitude_b = (int)log10(rb);
            if (magnitude_b >= 0) magnitude_b++;
        }
        if (magnitude_a != magnitude_b) {
            /* They can't be equal! */
            if (ra > rb) return 1;
            else return -1;
        }

        epsilon = pow(10.0, -(double)(context->lstring_lNumericDigits
            - (context->rexx_proc)[(context->rexx_rx_proc)].fuzz
            - magnitude_a)) / 0.9;

        /* Is the difference between the them less that the epsilon */
        if (fabs(ra - rb) < epsilon)
            return 0;
        else if (ra > rb)
            return 1;
        else
            return -1;
    }

    /* nope it was a string */
    L2STR(A);  /* convert A string */
  eq_str:
    a = (byte *) LSTR(*A);
    ae = a + LLEN(*A);
    for (; (a < ae) && ISSPACE(*a); a++);

    b = (byte *) LSTR(*B);
    be = b + LLEN(*B);
    for (; (b < be) && ISSPACE(*b); b++);

    for (; (a < ae) && (b < be) && (*a == *b); a++, b++);

    for (; (a < ae) && ISSPACE(*a); a++);
    for (; (b < be) && ISSPACE(*b); b++);

    if (a == ae && b == be)
        return 0;
    else if (a < ae && b < be)
        return (*a < *b) ? -1 : 1;
    else
        return (a < ae) ? 1 : -1;
} /* Lequal */
