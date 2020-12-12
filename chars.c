/* VM/370 CMS and GCCLIB chars.c */

#include <stdio.h>
#include "lstring.h"

/* ---------------- Lchars ------------------- */
long Lchars(FILEP f) {
    long l;
    l = ftell(f);  /* read current position */
    if (l == -1) {
        /* File does not support character counting */
        return !fateof(f);
    }
    return fgetlen(f) - l;
} /* Lchars */
