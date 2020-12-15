/* CMS GCCLIB write.c */

#include "lstring.h"

/* ---------------- Lwrite ------------------- */
void __CDECL
Lwrite(FILEP f, const PLstr line, const bool newline) {
    long l;
    char *c;

    L2STR(line);
    c = LSTR(*line);
    l = LLEN(*line);
    fwrite(c, 1, l, f);
    if (newline) fputc('\n', f);
} /* Lwrite */
