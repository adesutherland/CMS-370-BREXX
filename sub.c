/*
 * BREXX/370
 */

#include "lstring.h"

/* ---------------- Lsub ------------------- */
void __CDECL
Lsub(const PLstr to, const PLstr A, const PLstr B) {
    L2NUM(A);
    L2NUM(B);

    int int_a, int_b, overflow;

    if ((LTYPE(*A) == LINTEGER_TY) && (LTYPE(*B) == LINTEGER_TY)) {
        /* OK 2 integers - lets sub them and see if it is all good */
        int_a = LINT(*A);
        int_b = LINT(*B);

        /*
         * Detect Overflow using inline assembler (a hack but C can't do this)
         * See https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html
         * (Remember we are only in GCC version 3 and in S/370 - so trial and
         * error ... specifically the goto stuff does not seem to be supported)
        */
        overflow = 1;
        __asm__("SR %[a],%[b]\n\t"
                "BC 1,*+6\n\t"
                "SR %[overflow],%[overflow]"
        : [overflow] "=d"(overflow), [a] "+d"(int_a)  /* Output* Operands */
        : [b] "d"(int_b)                                /* Input Operands */
        );
        /*  *Note that the "+d" marks int_a as input/output
         *  (the d means type int)
         */

        if (!overflow) {
            /* No overflow - all done as fast integers */
            LINT(*to) = int_a;
            LTYPE(*to) = LINTEGER_TY;
            LLEN(*to) = sizeof(long);
            return; /* All done */
        }
    }

    /* We have to do the maths using floats */
    LREAL(*to) = TOREAL(*A) - TOREAL(*B);
    LTYPE(*to) = LREAL_TY;
    LLEN(*to) = sizeof(double);
} /* Lsub */
