/*
 * Lbitand for CMS
 */

#include "lstring.h"

/* ---------------- Lbitand ------------------ */
void __CDECL
Lbitand(const PLstr to, const PLstr s1, const PLstr s2,
        const bool usepad, const char pad) {
    long i;
    /* Needed to store intermediate results - perhaps GCCMVS can only do AND with ints - who knows ... */
    unsigned int v1, v2, res;

    L2STR(s1);
    L2STR(s2);

    if (LLEN(*s1) < LLEN(*s2)) {
        Lstrcpy(to, s2);
        for (i = 0; i < LLEN(*s1); i++) {
            v1 = LSTR(*s1)[i];
            v2 = LSTR(*s2)[i];
            res = v1 & v2;
            LSTR(*to)[i] = res;
        }

        if (usepad) {
            v1 = pad;
            for (; i < LLEN(*s2); i++) {
                v2 = LSTR(*s2)[i];
                res = v1 & v2;
                LSTR(*to)[i] = res;
            }
        }
    } else {
        Lstrcpy(to, s1);

        for (i = 0; i < LLEN(*s2); i++) {
            v1 = LSTR(*s1)[i];
            v2 = LSTR(*s2)[i];
            res = v1 & v2;
            LSTR(*to)[i] = res;
        }

        if (usepad) {
            v2 = pad;
            for (; i < LLEN(*s1); i++) {
                v1 = LSTR(*s1)[i];
                res = v1 & v2;
                LSTR(*to)[i] = res;

            }
        }
    }
} /* Lbitand */
