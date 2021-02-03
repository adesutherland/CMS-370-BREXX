/*
   Lprint function for VM/CMS
 */

#include "lstring.h"
#include <stdio.h>

/* ---------------- Lprint ------------------- */
void __CDECL
Lprint(FILEP f, const PLstr str) {
    Lstr tmp;
    int i, j;

    if (str == NULL) {
        fprintf(f, "<NULL>");
        return;
    }

    switch (LTYPE(*str)) {
        case LSTRING_TY:
            /* Logic to handled embedded nulls - converted to spaces */
            i = 0;
            while (i < LLEN(*str)) {
                for (j = i; j < LLEN(*str); j++) {
                    if (!LSTR(*str)[j]) {
                        fwrite(LSTR(*str) + i, j - i, 1, f);
                        fputc(' ', f);
                        i = j + 1;
                        break;
                    }
                }
                if (j >= LLEN(*str)) break;
            }
            fwrite(LSTR(*str) + i, LLEN(*str) - i, 1, f);
            break;

        case LINTEGER_TY:
            /*
            fprintf(f, "%ld", LINT(*str));
            break;
            */
        case LREAL_TY:
            LINITSTR(tmp);
            Lformat(&tmp, str, -1, -1, -1, -1);
            fprintf(f, "%.*s", LLEN(tmp), LSTR(tmp));
            LFREESTR(tmp);
            break;
    }
} /* Lprint */
