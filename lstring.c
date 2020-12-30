/* BREXX lstring.c */

#define __LSTRING_C__

#include <math.h>
#include <cmssys.h>
#include "lerror.h"
#include "lstring.h"

#ifndef WIN32
# if !defined(__CMS__) && !defined(__MVS__)
# ifndef WIN
#  include <limits.h>
# endif
# endif
#endif

#ifdef WIN32

# include <limits.h>

# define MAXLONG LONG_MAX
#endif

#if defined(__CMS__) || defined(__MVS__)

/* C90 does not have round() */
#define round(N) floor(0.5+(N))

# include <limits.h>

# define MAXLONG LONG_MAX
#endif

#ifdef HAVE_READLINE_HISTORY
# if defined(HAVE_READLINE_HISTORY_H)
#  include <readline/history.h>
# elif defined(HAVE_HISTORY_H)
#  include <history.h>
# endif
/* no history */
#endif /* HAVE_READLINE_HISTORY */

/* ================= Lstring routines ================== */

/* -------------------- Linit ---------------- */
void __CDECL
Linit(LerrorFunc Lerr) {
    size_t i;
    Context *context = (Context *) CMSGetPG();

    /* setup error function */
    (context->lstring_Lerror) = Lerr;

    /* setup upper */
    for (i = 0; i < 256; i++)
        (context->lstring_u2l)[i] = (context->lstring_l2u)[i] = i;
    for (i = 0; clower[i]; i++) {
        (context->lstring_l2u)[(byte) clower[i] & 0xFF] = cUPPER[i];
        (context->lstring_u2l)[(byte) cUPPER[i] & 0xFF] = clower[i];
    }

#ifdef HAVE_READLINE_HISTORY
    using_history();
#endif

    /* setup time */
    _Ltimeinit();
} /* Linit */

/* -------------- _Lfree ------------------- */
void __CDECL
_Lfree(void *str) {
    LPFREE((PLstr) str);
} /* _Lfree */

/* ---------------- Lfx -------------------- */
void __CDECL
Lfx(const PLstr s, size_t len) {
    size_t max;

    if (LISNULL(*s)) {
        LSTR(*s) = (char *) MALLOC((max = LNORMALISE(len)) + LEXTRA, "Lstr");
        LLEN(*s) = 0;
        LMAXLEN(*s) = max;
        LTYPE(*s) = LSTRING_TY;
#ifdef USEOPTION
        LOPT(*s) = 0;
#endif
    } else
#ifdef USEOPTION
        if (!LOPTION(*s,LOPTFIX) && LMAXLEN(*s)<len) {
         LSTR(*s) = (char *) REALLOC( LSTR(*s), (max=LNORMALISE(len))+LEXTRA);
         LMAXLEN(*s) = max;
        }
#else
    if (LMAXLEN(*s) < len) {
        LSTR(*s) = (char *) REALLOC(LSTR(*s), (max = LNORMALISE(len)) + LEXTRA);
        LMAXLEN(*s) = max;
    }

#endif
} /* Lfx */

/* ---------------- Licpy ------------------ */
void __CDECL
Licpy(const PLstr to, const long from) {
    LLEN(*to) = sizeof(long);
    LTYPE(*to) = LINTEGER_TY;
    LINT(*to) = from;
} /* Licpy */

/* ---------------- Lrcpy ------------------ */
void __CDECL
Lrcpy(const PLstr to, const double from) {
    LLEN(*to) = sizeof(double);
    LTYPE(*to) = LREAL_TY;
    LREAL(*to) = from;
} /* Lrcpy */

/* ---------------- Lmcpy ------------------ */
void __CDECL
Lmcpy(const PLstr to, const char *from, size_t len) {
    if (!from)
        Lfx(to, len = 0);
    else {
        Lfx(to, len);
        MEMCPY(LSTR(*to), from, len);
    }
    LLEN(*to) = len;
    LTYPE(*to) = LSTRING_TY;
} /* Lscpy */

/* ---------------- Lscpy ------------------ */
void __CDECL
Lscpy(const PLstr to, const char *from) {
    size_t len;

    if (!from)
        Lfx(to, len = 0);
    else {
        Lfx(to, len = STRLEN(from));
        MEMCPY(LSTR(*to), from, len);
    }
    LLEN(*to) = len;
    LTYPE(*to) = LSTRING_TY;
} /* Lscpy */

#if !defined(__CMS__) && !defined(__MVS__)

/* ---------------- Lwscpy ------------------ */
void __CDECL
Lwscpy(const PLstr to, const wchar_t *from) {
    size_t len;

    if (!from)
        Lfx(to, len = 0);
    else {
        Lfx(to, len = wcslen(from));
        wcstombs(LSTR(*to), from, len);
    }
    LLEN(*to) = len;
    LTYPE(*to) = LSTRING_TY;
} /* Lwscpy */
#endif

/* ---------------- Lcat ------------------- */
void __CDECL
Lcat(const PLstr to, const char *from) {
    size_t l;

    if (from == NULL) return;

    if (LLEN(*to) == 0)
        Lscpy(to, from);
    else {
        L2STR(to);
        l = LLEN(*to) + STRLEN(from);
        if (LMAXLEN(*to) < l) Lfx(to, l);
        STRCPY(LSTR(*to) + LLEN(*to), from);
        LLEN(*to) = l;
    }
} /* Lcat */

/* ------------------ Lcmp ------------------- */
int __CDECL
Lcmp(const PLstr a, const char *b) {
    int r, blen;

    L2STR(a);

    blen = STRLEN(b);
    if ((r = MEMCMP(LSTR(*a), b, MIN(LLEN(*a), blen))) != 0)
        return r;
    else {
        if (LLEN(*a) > blen)
            return 1;
        else if (LLEN(*a) == blen)
            return 0;
        else
            return -1;
    }
} /* Lcmp */

/* ---------------- Lstrcpy ----------------- */
void __CDECL
Lstrcpy(const PLstr to, const PLstr from) {
    if (LLEN(*from) == 0) {
        LLEN(*to) = 0;
        LTYPE(*to) = LSTRING_TY;
    } else {
        if (LMAXLEN(*to) <= LLEN(*from)) Lfx(to, LLEN(*from));
        switch (LTYPE(*from)) {
            case LSTRING_TY:
                MEMCPY(LSTR(*to), LSTR(*from), LLEN(*from));
                break;

            case LINTEGER_TY:
                LINT(*to) = LINT(*from);
                break;

            case LREAL_TY:
                LREAL(*to) = LREAL(*from);
                break;
        }
        LTYPE(*to) = LTYPE(*from);
        LLEN(*to) = LLEN(*from);
    }
} /* Lstrcpy */

/* ----------------- Lstrcat ------------------ */
void __CDECL
Lstrcat(const PLstr to, const PLstr from) {
    size_t l;
    if (LLEN(*from) == 0) return;

    if (LLEN(*to) == 0) {
        Lstrcpy(to, from);
        return;
    }

    L2STR(to);
    L2STR(from);

    l = LLEN(*to) + LLEN(*from);
    if (LMAXLEN(*to) <= l)
        Lfx(to, l);
    MEMCPY(LSTR(*to) + LLEN(*to), LSTR(*from), LLEN(*from));
    LLEN(*to) = l;
} /* Lstrcat */

/* ----------------- _Lstrcmp ----------------- */
/* -- Low level strcmp, suppose that both of -- */
/* -- are of the same type                      */
int __CDECL
_Lstrcmp(const PLstr a, const PLstr b) {
    int r;

    if ((r = MEMCMP(LSTR(*a), LSTR(*b), MIN(LLEN(*a), LLEN(*b)))) != 0)
        return r;
    else {
        if (LLEN(*a) > LLEN(*b))
            return 1;
        else if (LLEN(*a) == LLEN(*b)) {
            if (LTYPE(*a) > LTYPE(*b))
                return 1;
            else if (LTYPE(*a) < LTYPE(*b))
                return -1;
            return 0;
        } else
            return -1;
    }
} /* _Lstrcmp */

/* ----------------- Lstrcmp ------------------ */
int __CDECL
Lstrcmp(const PLstr a, const PLstr b) {
    int r;

    L2STR(a);
    L2STR(b);

    if ((r = MEMCMP(LSTR(*a), LSTR(*b), MIN(LLEN(*a), LLEN(*b)))) != 0)
        return r;
    else {
        if (LLEN(*a) > LLEN(*b))
            return 1;
        else if (LLEN(*a) == LLEN(*b))
            return 0;
        else
            return -1;
    }
}  /* Lstrcmp */

/* ----------------- Lstrset ------------------ */
void __CDECL
Lstrset(const PLstr to, const size_t length, const char value) {
    Lfx(to, length);
    LTYPE(*to) = LSTRING_TY;
    LLEN(*to) = length;
    MEMSET(LSTR(*to), value, length);
}  /* Lstrset */

/* ----------------- _Lsubstr ----------------- */
/* WARNING!!! length is size_t type DO NOT PASS A NEGATIVE value */
void __CDECL
_Lsubstr(const PLstr to, const PLstr from, size_t start, size_t length) {
    L2STR(from);

    start--;
    if ((length == 0) || (length + start > LLEN(*from)))
        length = LLEN(*from) - start;

    if (start < LLEN(*from)) {
        if (LMAXLEN(*to) < length) Lfx(to, length);
        MEMCPY(LSTR(*to), LSTR(*from) + start, length);
        LLEN(*to) = length;
    } else LZEROSTR(*to);
    LTYPE(*to) = LSTRING_TY;
}  /* Lstrsub */

/*
** Compute 10 to the E-th power.  Examples:  E==1 results in 10.
** E==2 results in 100.  E==50 results in 1.0e50.
**
** This routine only works for values of E between 1 and 75(ish) for IBM's
** Hex float (long / double)
**
** Taken from SQLite
*/
static double rxpow10(int E) {
    double x = 10.0;
    double r = 1.0;
    while (1) {
        if (E & 1) r *= x;
        E >>= 1;
        if (E == 0) break;
        x *= x;
    }
    return r;
}

/*
** The string z[] is an text representation of a real number.
** Convert this string to a double and write it into *pResult.
**
** The string z[] is length bytes in length (bytes, not characters) and
** uses the encoding enc.  The string is not necessarily zero-terminated.
**
** Return TRUE if the result is a valid real number (or integer) and FALSE
** if the string is empty or contains extraneous text.  More specifically
** return
**      1          =>  The input string is a pure integer
**      2 or more  =>  The input has a decimal point or eNNN clause
**      0 or less  =>  The input string is not a valid number
**     -1          =>  Not a valid number, but has a valid prefix which
**                     includes a decimal point and/or an eNNN clause
**
** Valid numbers are in one of these formats:
**
**    [+-]digits[E[+-]digits]
**    [+-]digits.[digits][E[+-]digits]
**    [+-].digits[E[+-]digits]
**
** Leading and trailing whitespace is ignored for the purpose of determining
** validity.
**
** If some prefix of the input string is a valid number, this routine
** returns 0 or a negative number but it still converts the prefix and writes
** the result into *pResult.
**
** Based on the atof function taken from SQLite
** Changed to add an epsilon if the least significant character is 5 so that
** any subsequent rounding rounds away from zero. Also peaks at the 17th
** digit to decide if we should round the 16th as needed (well why not!)
*/

/*
** IBM's Hex float (long / double) uses 14 hex chars for the significant
** which can handle 16 decimal digits (plus a bit more for rounding!)
** So we will support up to 16 significant digits only - this also means
** we can use a double for the significant and avoid rolling our own 64bit
** integer maths stuff ...
*/
#define SIGDIGITS 16

static int rxatof( char *z, double *pResult, int length) {
    const char *zEnd;
    /* sign * significand * (10 ^ (esign * exponent)) */
    int sign = 1;                 /* sign of significand */
    double s = 0;    /* significand */
    int d = 0;       /* adjust exponent for shifting decimal point */
    int esign = 1;   /* sign of exponent */
    int e = 0;       /* exponent */
    int eValid = 1;  /* True exponent is either not used or is well-formed */
    double result;
    int nDigit = 0;  /* Number of digits processed */
    int nLeadingZeros = 0; /* Number of leading zeros */
    /* 1: pure integer,  2+: fractional  -1 or less: bad UTF16 */
    int eType = 1;
    double scale;
    int round_epsilon = 0; /* TRUE if we should add an epsilon */
    int round_digit = 0; /* 1 if we have handled the 17th digit for rounding */
    /* -1 if the 16th digit was 4 (so we know to add the
     * epsilon) */

    *pResult = 0.0;  /* Default return value, in case of an error */
    if (length == 0) return 0;

    zEnd = z + length;

    /* skip leading spaces  */
    while (z < zEnd && isspace(*z)) z++;
    if (z >= zEnd) return 0;

    /* get sign of significand */
    if (*z == '-') {
        sign = -1;
        z++;
    } else if (*z == '+') {
        z++;
    }

    while (z < zEnd && isspace(*z)) z++; /* skip spaces */

    /* skip leading zeros  */
    while (z < zEnd && *z == '0') {
        z++;
        nLeadingZeros++;
    }
    if (z >= zEnd) return 1; /* Zero is an integer */

    /* copy max significant digits to significand */
    while (z < zEnd && isdigit(*z)) {
        /*
         * This really should be integer maths - but we would need 64bits to
         * store the required number of digits. Oh well maybe next time!
         */
        s = s * 10 + (*z - '0');
        if (*z == '5') {
            round_epsilon = 1;
            round_digit = 0;
        }
        else if (*z == '4') {
            round_epsilon = 0;
            round_digit = -1; /* So if we round up later we know to add the epsilon */
        }
        else if (*z != '0') {
            round_epsilon = 0;
            round_digit = 0;
        }
        z++;
        nDigit++;
        if (nDigit >= SIGDIGITS) {
            /* skip non-significant significand digits
            ** (increase exponent by d to shift decimal left) */
            while (z < zEnd && isdigit(*z)) {
                if (round_digit != 1) {
                    if (*z >= '5' && *z <= '9') {
                        s = s + 1; /* Round the value */
                        /* Last digit was 4 now it is 5 so we need an epsilon */
                        if (round_digit == -1) round_epsilon = 1;
                            /* Otherwise we don't need the epsilon anymore */
                        else round_epsilon = 0;
                    }
                    round_digit = 1;
                }
                z++;
                d++;
            }
        }
    }
    if (z >= zEnd) goto do_atof_calc;

    /* if decimal point is present */
    if (*z == '.') {
        z++;
        eType++;
        /* copy digits from after decimal to significand
        ** (decrease exponent by d to shift decimal right) */
        while (z < zEnd && isdigit(*z)) {
            if (nDigit < SIGDIGITS) {
                s = s * 10 + (*z - '0');
                if (*z == '5') {
                    round_epsilon = 1;
                    round_digit = 0;
                }
                else if (*z == '4') {
                    round_epsilon = 0;
                    round_digit = -1; /* So if we round up later we know to add the epsilon */
                }
                else if (*z != '0') {
                    round_epsilon = 0;
                    round_digit = 0;
                }
                d--;
                nDigit++;
            }
            else {
                if (round_digit != 1) {
                    if (*z >= '5' && *z <= '9') {
                        s = s + 1; /* Round the value */
                        /* Last digit was 4 now it is 5 so we need an epsilon */
                        if (round_digit == -1) round_epsilon = 1;
                            /* Otherwise we don't need the epsilon anymore */
                        else round_epsilon = 0;
                    }
                    round_digit = 1;
                }
            }
            z++;
        }
    }
    if (z >= zEnd) goto do_atof_calc;

    /* if exponent is present */
    if (*z == 'e' || *z == 'E') {
        z++;
        eValid = 0;
        eType++;

        if (z >= zEnd)
            goto do_atof_calc;

        /* get sign of exponent */
        if (*z == '-') {
            esign = -1;
            z++;
        } else if (*z == '+') {
            z++;
        }
        /* copy digits to exponent */
        while (z < zEnd && isdigit(*z)) {
            e = e < 10000 ? (e * 10 + (*z - '0')) : 10000;
            z++;
            eValid = 1;
        }
    }

    /* skip trailing spaces */
    while (z < zEnd && isspace(*z)) z++;

    do_atof_calc:
    /* adjust exponent by d, and update sign */
    e = (e * esign) + d;
    if (e < 0) {
        esign = -1;
        e *= -1;
    } else {
        esign = 1;
    }

    if (s == 0) {
        /* Zero is not signed */
        result = (double)0.0;
        eType = 1;
    } else {
        /* Attempt to reduce exponent. */
        while (e >
               0) {
            if (esign > 0) {
                if (s >= rxpow10(SIGDIGITS - 1) )
                    break;
                s *= 10;
            } else {
                if ((long)fmod(s, 10) != 0)
                    break;
                s /= 10;
            }
            e--;
        }

        /* adjust the sign of significand */
        s = sign < 0 ? -s : s;
        if (e == 0) {
            result = (double) s;
        } else {
            /* attempt to handle extremely small/large numbers */
            if (e > 79) {
                if (esign < 0) result = 0.0 * s;
                else result = 7.3e75 * s;  /* Infinity */
            } else {
                scale = rxpow10(e);
                if (esign < 0) {
                    if (round_epsilon) {
                        /* Ok we scale to 17 digits and add 1
                         * (i.e. 1/(10 power of 17) */
                        double scale2 = rxpow10(SIGDIGITS - nDigit + 1);
                        result = ((s * scale2) + 1.0) / (scale * scale2);
                    }
                    else result = s / scale;
                } else {
                    /* Scaling up - epsilon is irrelevant */
                    result = s * scale;
                }
            }
        }
    }

    /* store the result */
    *pResult = result;

    /* return true if number and no extra non-whitespace characters after */
    if (z == zEnd && (nDigit+nLeadingZeros) > 0 && eValid && eType > 0) {
        return eType;
    } else if (eType >= 2 && (eType == 3 || eValid) && (nDigit+nLeadingZeros) > 0) {
        return -1;
    } else {
        return 0;
    }
}

/* ------------------------ _Lisnum ----------------------- */
/* _Lisnum      - returns if it is possible to convert      */
/*               a LSTRING to NUMBER                        */
/*               a LREAL_TY or LINTEGER_TY                  */
/* -------------------------------------------------------- */
int __CDECL
_Lisnum(const PLstr s) {
    int rc;
    char *ch;
    Context *context = (Context *) CMSGetPG();

    context->lstring_lLastScannedNumber = 0.0;
    ch = LSTR(*s);
    if (ch == NULL) return LSTRING_TY;

    rc = rxatof(ch, &(context->lstring_lLastScannedNumber), LLEN(*s));
    if (rc == 1) return LINTEGER_TY;
    else if (rc >= 2) {
        if (fabs(context->lstring_lLastScannedNumber) > LONG_MAX)
            return LREAL_TY; /* Always treat big nums as reals */
        if (Disint(context->lstring_lLastScannedNumber)) return LINTEGER_TY;
        return LREAL_TY;
    }
    else {
        context->lstring_lLastScannedNumber = 0.0;
        return LSTRING_TY;
    }
} /* _Lisnum */

/* ------------------ L2str ------------------- */
void __CDECL
L2str(const PLstr s) {
    Context *context = (Context *) CMSGetPG();
    if (LTYPE(*s) == LINTEGER_TY) {
#if defined(WCE) || defined(__BORLANDC__)
        LTOA(LINT(*s),LSTR(*s),10);
#else
        sprintf(LSTR(*s), "%ld", LINT(*s));
#endif
        LLEN(*s) = STRLEN(LSTR(*s));
    } else { /* LREAL_TY */
#ifdef __CMS__
        Lformat(s, s, -1, -1, 0, 0);
#else
        /* There is a problem with the Windows CE */
        char str[50];
        size_t len;

        GCVT(LREAL(*s), (context->lstring_lNumericDigits), str);
        /* --- remove the last dot from the number --- */
        len = STRLEN(str);
#ifdef WCE
        if (str[len-1] == '.') len--;
#endif
        if (len >= LMAXLEN(*s)) Lfx(s, len);
        MEMCPY(LSTR(*s), str, len);
        LLEN(*s) = len;
#endif
    }
    LTYPE(*s) = LSTRING_TY;
} /* L2str */

/* ------------------ L2int ------------------- */
void __CDECL
L2int(const PLstr s) {
    Context *context = (Context *) CMSGetPG();
    if (LTYPE(*s) == LREAL_TY) {
        if ((double) ((long) LREAL(*s)) == LREAL(*s))
            LINT(*s) = (long) LREAL(*s);
        else
            (context->lstring_Lerror)(ERR_INVALID_INTEGER, 0);
    } else { /* LSTRING_TY */
        LASCIIZ(*s);
        switch (_Lisnum(s)) {
            case LINTEGER_TY:
                /*///LINT(*s) = atol( LSTR(*s) ); */
                LINT(*s) = (long) round(context->lstring_lLastScannedNumber);
                break;

            case LREAL_TY:
                /*///LREAL(*s) = strtod( LSTR(*s), NULL ); */
                LREAL(*s) = (context->lstring_lLastScannedNumber);
                if ((double) ((long) LREAL(*s)) == LREAL(*s))
                    LINT(*s) = (long) LREAL(*s);
                else
                    (context->lstring_Lerror)(ERR_INVALID_INTEGER, 0);
                break;

            default:
                (context->lstring_Lerror)(ERR_INVALID_INTEGER, 0);
        }
    }
    LTYPE(*s) = LINTEGER_TY;
    LLEN(*s) = sizeof(long);
} /* L2int */

/* ------------------ L2real ------------------- */
void __CDECL
L2real(const PLstr s) {
    Context *context = (Context *) CMSGetPG();
    if (LTYPE(*s) == LINTEGER_TY)
        LREAL(*s) = (double) LINT(*s);
    else { /* LSTRING_TY */
        LASCIIZ(*s);
        if (_Lisnum(s) != LSTRING_TY)
            /*/////LREAL(*s) = strtod( LSTR(*s), NULL ); */
            LREAL(*s) = (context->lstring_lLastScannedNumber);
        else
            (context->lstring_Lerror)(ERR_BAD_ARITHMETIC, 0);
    }
    LTYPE(*s) = LREAL_TY;
    LLEN(*s) = sizeof(double);
} /* L2real */

void Lround(PLstr var) {
    Context *context = (Context *) CMSGetPG();
    int n;
    double d;

    L2REAL(var);
    d = LREAL(*var);

    if (d < 0.0) d = -1.0 * d;
    n = (int) log10(d);
    if (n < 0) n = 0;
    else n++;

    n = context->lstring_lNumericDigits - n;
    if (n < 1) n = 1;

    d = pow(10, n);
    LREAL(*var) = round(LREAL(*var) * d) / d;
} /* Lround */

/* ------------------ Disint ------------------- */
/* Is a double an int */
int Disint(double d) {
    int len;
    double x;
    double epsilon;
    Context *context = (Context *) CMSGetPG();
    if (d == 0.0) return TRUE;

    /* Calculate precision (epsilon) - REXX DIGITS less size of int bit of the number */
    if (d < 0.0) d = (-1.0) * d;
    len = (int)log10(d);
    if (len<0) len=0;
    else len++;
    len = context->lstring_lNumericDigits - len;
    if (len < 0) len = 0;
    epsilon = pow(10.0, -(double) len) /
              2.01; /* 2.01 rather 2.0 just to tune rounding */

    /* Is the difference between the nearest integer less that the epsilon */
    x = d - floor(d);
    if (x > 0.5) x = 1.0 - x;

    if (x < epsilon) return TRUE;
    return FALSE;
} /* Disint */


/* ------------------ L2num ------------------- */
void __CDECL
L2num(const PLstr s) {
    Context *context = (Context *) CMSGetPG();
    switch (_Lisnum(s)) {
        case LINTEGER_TY:
            LINT(*s) = (long) round(context->lstring_lLastScannedNumber);
            LTYPE(*s) = LINTEGER_TY;
            LLEN(*s) = sizeof(long);
            break;

        case LREAL_TY:
            LREAL(*s) = (context->lstring_lLastScannedNumber);
            LTYPE(*s) = LREAL_TY;
            LLEN(*s) = sizeof(double);
            break;

        default:
            (context->lstring_Lerror)(ERR_BAD_ARITHMETIC, 0);
    }
} /* L2num */

/* ----------------- Lrdint ------------------ */
long __CDECL
Lrdint(const PLstr s) {
    Context *context = (Context *) CMSGetPG();
    if (LTYPE(*s) == LINTEGER_TY) return LINT(*s);

    if (LTYPE(*s) == LREAL_TY) {
        if ((double) ((long) LREAL(*s)) == LREAL(*s))
            return (long) LREAL(*s);
        else
            (context->lstring_Lerror)(ERR_INVALID_INTEGER, 0);
    } else { /* LSTRING_TY */
        LASCIIZ(*s);
        switch (_Lisnum(s)) {
            case LINTEGER_TY:
                /*///return atol( LSTR(*s) ); */
                return (long) round(context->lstring_lLastScannedNumber);

            case LREAL_TY:
                /*///d = strtod( LSTR(*s), NULL );
                //////if ((double)((long)d) == d)
                ////// return (long)d; */
                if ((double) ((long) (context->lstring_lLastScannedNumber)) ==
                    (context->lstring_lLastScannedNumber))
                    return (long) (context->lstring_lLastScannedNumber);
                else
                    (context->lstring_Lerror)(ERR_INVALID_INTEGER, 0);
                break;

            default:
                (context->lstring_Lerror)(ERR_INVALID_INTEGER, 0);
        }
    }
    return 0; /* never gets here but keeps compiler happy */
} /* Lrdint */

/* ----------------- Lrdreal ------------------ */
double __CDECL
Lrdreal(const PLstr s) {
    Context *context = (Context *) CMSGetPG();
    if (LTYPE(*s) == LREAL_TY) return LREAL(*s);

    if (LTYPE(*s) == LINTEGER_TY)
        return (double) LINT(*s);
    else { /* LSTRING_TY */
        LASCIIZ(*s);
        if (_Lisnum(s) != LSTRING_TY)
            /*///// return strtod( LSTR(*s), NULL ); */
            return (context->lstring_lLastScannedNumber);
        else
            (context->lstring_Lerror)(ERR_BAD_ARITHMETIC, 0);
    }
    return 0.0;
} /* Lrdreal */
