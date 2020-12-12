/*
 * $Id: trace.c,v 1.8 2008/07/15 07:40:25 bnv Exp $
 * $Log: trace.c,v $
 * Revision 1.8  2008/07/15 07:40:25  bnv
 * #include changed from <> to ""
 *
 * Revision 1.7  2004/04/30 15:27:34  bnv
 * Type changes
 *
 * Revision 1.6  2003/10/30 13:16:28  bnv
 * Variable name change
 *
 * Revision 1.5  2002/06/11 12:37:38  bnv
 * Added: CDECL
 *
 * Revision 1.4  2001/06/25 18:51:23  bnv
 * Added: Memory check in debug version when the trace is enabled.
 *
 * Revision 1.3  1999/11/26 13:13:47  bnv
 * Changed: To use the new macros.
 * Changed: To support 64-bit cpus.
 *
 * Revision 1.2  1999/03/10 16:55:35  bnv
 * A bracket addition to keep compiler happy.
 *
 * Revision 1.1  1998/07/02 17:34:50  bnv
 * Initial revision
 *
 */

#define __TRACE_C__

#include <stdlib.h>
#include <cmssys.h>
#include "lstring.h"

#include "rexx.h"
#include "trace.h"
#include "compile.h"
#include "interpre.h"
#include "variable.h"
#include "nextsymb.h"

/* ---------- function prototypes ------------- */
void    __CDECL RxInitInterStr();

/* ---------- Extern variables ---------------- */
#include "context.h"

extern const ErrorMsg errortext[];  /* CONST - from lstring/errortxt.c */

static const char TraceChar[] = {' ', '>', 'L', 'V', 'C', 'O', 'F',
                                 '.'}; /* CONST */

/* ----------------- TraceCurline ----------------- */
int __CDECL
TraceCurline(RxFile **rxf, int print) {
    size_t line;
    size_t cl, codepos;
    char *ch, *chend;
    Context *context = (Context *) CMSGetPG();

    if ((context->nextsymbsymbolptr) == NULL) { /* we are in intepret */
        if ((context->compileCompileClause) == NULL) {
            if (rxf) *rxf = (context->rexxrxFileList);
            return -1;
        }

        codepos = (size_t) ((byte huge *) (context->interpreRxcip) -
                            (byte huge *) (context->interpreRxcodestart));
        /* search for clause */
        cl = 0;
        while ((context->compileCompileClause)[cl].ptr) {
            if ((context->compileCompileClause)[cl].code >= codepos)
                break;
            cl++;
        }
        cl--;
        line = (context->compileCompileClause)[cl].line;
        ch = (context->compileCompileClause)[cl].ptr;
        chend = (context->compileCompileClause)[cl + 1].ptr;
        if (chend == NULL)
            for (chend = ch; *chend != '\n'; chend++) /*do nothing*/;;
        (context->rexx_nesting) = (context->rexx_rx_proc) +
                                  (context->compileCompileClause)[cl].nesting;
        if (rxf)
            *rxf = (context->compileCompileClause)[cl].fptr;
    } else {  /* we are in compile  */
        if ((context->compileCompileCurClause) == 0)
            cl = 0;
        else
            cl = (context->compileCompileCurClause) - 1;

        (context->rexx_nesting) = (context->compileCompileClause)[cl].nesting;
        if (rxf) {
            if ((context->compileCompileCurClause) == 0)
                *rxf = (context->compileCompileRxFile);
            else
                *rxf = (context->compileCompileClause)[cl].fptr;
        }
        if ((context->nextsymb__in_nextsymbol)) {
            line = (context->nextsymbsymboline);
            ch = (context->nextsymbsymbolptr);
            while (ch > (context->nextsymbsymbolprevptr))
                if (*ch-- == '\n') line--;
            ch = (context->nextsymbsymbolprevptr);
        } else if (cl == 0) {
            line = 1;
            ch = LSTR((*rxf)->file);
        } else {
            cl = (context->compileCompileCurClause) - 1;
            line = (context->compileCompileClause)[cl].line;
            ch = (context->compileCompileClause)[cl].ptr;
        }
        for (chend = ch;
             *chend != ';' && *chend != '\n'; chend++) /*do nothing*/;;
    }

#ifndef WIN
    if (print) {
        int i;

        fprintf(STDERR, "%6d *-* ", line);
        for (i = 1; i < (context->rexx_nesting); i++) fputc(' ', STDERR);

        while (*ch && ch < chend) {
            if (*ch != '\n')
                fputc(*ch, STDERR);
            ch++;
        }
        fputc('\n', STDERR);
    }
#else
    if (print) {
     int i;

     PUTINT(line,6,10);
     PUTS(" *-* ");
     for (i=1; i<(context->rexx_nesting); i++) PUTCHAR(' ');

     while (*ch && ch<chend) {
      if (*ch!='\n')
       PUTCHAR(*ch);
      ch++;
     }
     PUTCHAR('\n');
    }
#endif
    return line;
} /* TraceCurline */

/* ---------------- TraceSet -------------------- */
void __CDECL
TraceSet(PLstr trstr) {
    char *ch;
    Context *context = (Context *) CMSGetPG();

    L2STR(trstr);
    Lupper(trstr);
    LASCIIZ(*trstr);
    ch = LSTR(*trstr);
    if (*ch == '!') {
        ch++;
    } else if (*ch == '?') {
        (context->rexx_proc)[(context->rexx_rx_proc)].interactive_trace
                = 1 -
                  (context->rexx_proc)[(context->rexx_rx_proc)].interactive_trace;
        if ((context->rexx_proc)[(context->rexx_rx_proc)].interactive_trace)
#ifndef WIN
            fprintf(STDERR, "       +++ %s +++\n", errortext[2].errormsg);
#else
        PUTS("       +++ ");
        PUTS(errortext[0].errormsg);
        PUTS(" +++\n");
#endif
        ch++;
    }

    switch (*ch) {
        case 'A':
            (context->rexx_proc)[(context->rexx_rx_proc)].trace = all_trace;
            break;
        case 'C':
            (context->rexx_proc)[(context->rexx_rx_proc)].trace = commands_trace;
            break;
        case 'E':
            (context->rexx_proc)[(context->rexx_rx_proc)].trace = error_trace;
            break;
/*
///  case 'F':
///   (context->rexx_proc)[(context->rexx_rx_proc)].trace = ;
///   break;
*/
        case 'I':
            (context->rexx_proc)[(context->rexx_rx_proc)].trace = intermediates_trace;
            break;
        case 'L':
            (context->rexx_proc)[(context->rexx_rx_proc)].trace = labels_trace;
            break;
        case 'N':
            (context->rexx_proc)[(context->rexx_rx_proc)].trace = normal_trace;
            break;
        case 'O':
            (context->rexx_proc)[(context->rexx_rx_proc)].trace = off_trace;
            (context->rexx_proc)[(context->rexx_rx_proc)].interactive_trace = FALSE;
            break;
        case 'R':
            (context->rexx_proc)[(context->rexx_rx_proc)].trace = results_trace;
            break;
        case 'S':
            (context->rexx_proc)[(context->rexx_rx_proc)].trace = scan_trace;
            break;
#ifdef __DEBUG__
            case 'D':
             (context->rexx__debug__) = 1-(context->rexx__debug__);
             if ((context->rexx__debug__))
              printf("\n\nInternal DEBUG starting...\n");
             else
              printf("\n\nInternal DEBUG ended\n");
             break;
#endif
        default:
            (context->lstring_Lerror)(ERR_INVALID_TRACE, 1, trstr);
    }
} /* TraceSet */

/* --------------------- TraceByte -------------------- */
void __CDECL
TraceByte(int middlechar) {
    byte tracebyte = 0;

    tracebyte |= (middlechar & TB_MIDDLECHAR);
    tracebyte |= TB_TRACE;

    _CodeAddByte(tracebyte);
} /* TraceByte */

/* ------------------ TraceClause ----------------- */
void __CDECL
TraceClause(void) {
    Context *context = (Context *) CMSGetPG();
    if ((context->rexx_proc)[(context->rexx_rx_proc)].interactive_trace) {
        /* return if user specified a string for interactive trace */
        if (TraceInteractive(TRUE))
            return;
    }
    TraceCurline(NULL, TRUE);
#ifdef __DEBUG__
    mem_chk();
#endif
} /* TraceClause */

/* ------------------ TraceInstruction ----------------- */
void __CDECL
TraceInstruction(CIPTYPE inst) {
    Context *context = (Context *) CMSGetPG();
    if ((inst & TB_MIDDLECHAR) != nothing_middle)
        if ((context->rexx_proc)[(context->rexx_rx_proc)].trace ==
            intermediates_trace) {
            int i;
#ifndef WIN
            fprintf(STDERR, "       >%c>  ", TraceChar[inst & TB_MIDDLECHAR]);
            for (i = 0; i < (context->rexx_nesting); i++) fputc(' ', STDERR);
            fputc('\"', STDERR);
            Lprint(STDERR,
                   (context->interpre_RxStck)[(context->interpre_RxStckTop)]);
            fprintf(STDERR, "\"\n");
#else
            PUTS("       >");
            PUTCHAR(TraceChar[ inst & TB_MIDDLECHAR ]);
            PUTS(">  ");
            for (i=0; i<(context->rexx_nesting); i++) PUTCHAR(' ');
            PUTCHAR('\"');
            Lprint(NULL,(context->interpre_RxStck)[(context->interpre_RxStckTop)]);
            PUTS("\"\n");
#endif
        }
} /* TraceInstruction */

/* ---------------- TraceInteractive ------------------- */
int __CDECL
TraceInteractive(int frominterpret) {
    Context *context = (Context *) CMSGetPG();
    /* Read the interactive string into a tmp var */
    (context->interpre_RxStckTop)++;
    (context->interpre_RxStck)[(context->interpre_RxStckTop)] = &((context->interpre__tmpstr)[(context->interpre_RxStckTop)]);

    Lread(STDIN, (context->interpre_RxStck)[(context->interpre_RxStckTop)],
          LREADLINE);
    if (!LLEN(*(context->interpre_RxStck)[(context->interpre_RxStckTop)])) {
        (context->interpre_RxStckTop)--;
        return FALSE;
    }

    (context->interpre__trace) = FALSE;

    RxInitInterStr();
    (context->rexx_proc)[(context->rexx_rx_proc)].calltype = CT_INTERACTIVE;
    if (frominterpret) {
        (context->rexx_proc)[(context->rexx_rx_proc)].calltype = CT_INTERACTIVE;
        /* lets go again to NEWCLAUSE */
#ifdef ALIGN
        (context->rexx_proc)[(context->rexx_rx_proc)].ip -= sizeof(dword);
#else
        (context->rexx_proc)[(context->rexx_rx_proc)].ip--;
#endif
    }
    return TRUE;
} /* TraceInteractive */
