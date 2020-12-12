/* VM/370 CMS and GCCLIB RCFILES */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmssys.h>

#include "lerror.h"
#include "lstring.h"

#include "rexx.h"
#include "rxdefs.h"
#include "context.h"

/* -------------------------* find_file *------------------------- */
static FILE *
find_file(const PLstr fn) {
    FILE *file = NULL;
    Context *context = (Context *) CMSGetPG();

    /* search to see if it is a number - if so assume its a FILE* */
    if ((LTYPE(*fn) == LSTRING_TY) && (_Lisnum(fn) == LINTEGER_TY)) {
        file = (FILE *) Lrdint(fn);
    } else if (LTYPE(*fn) == LINTEGER_TY) {
        file = (FILE *) LINT(*fn);
    } else if (LTYPE(*fn) == LREAL_TY) {
        file = (FILE *) Lrdint(fn);
    }
    if (file) {
        if (feof(file) == EOF)
            (context->lstring_Lerror)(ERR_FILE_NOT_OPENED,
                                      0); /* feof() is a cheap check to see if it is a valid FILE* */
        return file;
    }

    L2STR(fn);
    LASCIIZ(*fn); /* Make sure its a C str */

    return fgethandle(LSTR(*fn));
} /* find_file */

/* -------------------------* open_file *------------------------- */
static FILE *
open_file(const PLstr fn, const char *mode) {
    L2STR(fn);
    LASCIIZ(*fn);
    return FOPEN(LSTR(*fn), mode);
} /* open_file */

/* -------------------------* reopen_file *------------------------- */
static FILE *
reopen_file(const PLstr fn, const char *mode, FILE *stream) {
    L2STR(fn);
    LASCIIZ(*fn);
    return freopen(LSTR(*fn), mode, stream);
} /* reopen_file */

/* --------------------------------------------------------------- */
/*  OPEN( file, mode )                                             */
/* --------------------------------------------------------------- */
void __CDECL
R_open() {
    Context *context = (Context *) CMSGetPG();
    if (ARGN != 2) (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
    must_exist(1);
    L2STR(ARG1);
    must_exist(2);
    L2STR(ARG2);
    Llower(ARG2);
    LASCIIZ(*ARG2);
    Licpy(ARGR, (int) open_file(ARG1, LSTR(*ARG2)));
} /* R_open */

/* --------------------------------------------------------------- */
/*  CLOSE( file )                                                  */
/* --------------------------------------------------------------- */
void __CDECL
R_close() {
    FILE *i;
    Context *context = (Context *) CMSGetPG();

    if (ARGN != 1)
        (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
    i = find_file(ARG1);
    if (i == NULL) (context->lstring_Lerror)(ERR_FILE_NOT_OPENED, 0);

    Licpy(ARGR, FCLOSE(i));
} /* R_close */

/* --------------------------------------------------------------- */
/*  EOF( file )                                                    */
/* --------------------------------------------------------------- */
void __CDECL
R_eof() {
    FILE *i;
    Context *context = (Context *) CMSGetPG();
    if (ARGN != 1)
        (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
    i = find_file(ARG1);
    if (i == NULL)
        Licpy(ARGR, -1);
    else
        Licpy(ARGR, ((FEOF(i)) ? 1 : 0));
} /* R_eof */

/* --------------------------------------------------------------- */
/*  FLUSH( file )                                                  */
/* --------------------------------------------------------------- */
void __CDECL
R_flush() {
    FILE *i;
    Context *context = (Context *) CMSGetPG();
    if (ARGN != 1)
        (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
    i = find_file(ARG1);
    if (i == NULL)
        Licpy(ARGR, -1);
    else
        Licpy(ARGR, (FFLUSH(i)));
} /* R_flush */

/* --------------------------------------------------------------- */
/*  STREAM(file[,[option][,command]])                              */
/* --------------------------------------------------------------- */
void __CDECL
R_stream() {
    char option;
    Lstr cmd;
    FILE *i;
    Context *context = (Context *) CMSGetPG();

    if (!IN_RANGE(1, ARGN, 3))
        (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);

    must_exist(1);
    i = find_file(ARG1);

    if (exist(2)) {
        L2STR(ARG2);
        option = (context->lstring_l2u)[(byte) LSTR(*ARG2)[0]];
    } else
        option = 'S'; /* Status */

    /* only with option='C' we must have a third argument */
    if (option != 'C' && exist(3))
        (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);

    switch (option) {
        case 'C':  /* command */
            if (!exist(3))
                (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
            LINITSTR(cmd);
            Lfx(&cmd, LLEN(*ARG3));
            Lstrip(&cmd, ARG3, LBOTH, ' ');
            Lupper(&cmd);

            if (!Lcmp(&cmd, "READ")) {
                if (i) i = reopen_file(ARG1, "r", i);
                else i = open_file(ARG1, "r");
                if (!i) (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);
            } else if (!Lcmp(&cmd, "READBINARY")) {
                if (i) i = reopen_file(ARG1, "rb", i);
                else i = open_file(ARG1, "rb");
                if (!i) (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);
            } else if (!Lcmp(&cmd, "WRITE")) {
                if (i) i = reopen_file(ARG1, "w", i);
                else i = open_file(ARG1, "w");
                if (!i) (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);
            } else if (!Lcmp(&cmd, "WRITEBINARY")) {
                if (i) i = reopen_file(ARG1, "wb", i);
                else i = open_file(ARG1, "wb");
                if (!i) (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);
            } else if (!Lcmp(&cmd, "APPEND")) {
                if (i) i = reopen_file(ARG1, "a+", i);
                else i = open_file(ARG1, "a+");
                if (!i) (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);
            } else if (!Lcmp(&cmd, "APPENDBINARY")) {
                if (i) i = reopen_file(ARG1, "ab+", i);
                else i = open_file(ARG1, "ab+");
                if (!i) (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);
            } else if (!Lcmp(&cmd, "UPDATE")) {
                if (i) i = reopen_file(ARG1, "r+", i);
                else i = open_file(ARG1, "r+");
                if (!i) (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);
            } else if (!Lcmp(&cmd, "UPDATEBINARY")) {
                if (i) i = reopen_file(ARG1, "rb+", i);
                else i = open_file(ARG1, "rb+");
                if (!i) (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);
            } else if (!Lcmp(&cmd, "CREATE")) {
                if (i) i = reopen_file(ARG1, "w+", i);
                else i = open_file(ARG1, "w+");
                if (!i) (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);
            } else if (!Lcmp(&cmd, "CREATEBINARY")) {
                if (i) i = reopen_file(ARG1, "wb+", i);
                else i = open_file(ARG1, "wb+");
                if (!i) (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);
            } else if (!Lcmp(&cmd, "CLOSE")) {
                if (i) FCLOSE(i);
            } else if (!Lcmp(&cmd, "FLUSH")) {
                if (i) FFLUSH(i);
            } else if (!Lcmp(&cmd, "RESET")) {
                if (i) rewind(i);
            } else
                (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);

            Lscpy(ARGR, "READY");
            LFREESTR(cmd);
            break;
        case 'D':  /* get a description */
        case 'S':  /* status */
            if (!i)
                Lscpy(ARGR, "UNKNOWN");
            else {
                if (fateof(i))
                    Lscpy(ARGR, "NOTREADY");
                else
                    Lscpy(ARGR, "READY");
            }
            /* ERROR??? where */
            break;
        default:
            (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
    }
} /* R_stream */

/* --------------------------------------------------------------- */
/*  CHARS((file))                                                  */
/* --------------------------------------------------------------- */
/*  LINES((file))                                                  */
/* --------------------------------------------------------------- */
void __CDECL
R_charslines(const int func) {
    FILE *i;
    Context *context = (Context *) CMSGetPG();

    if (ARGN > 1)
        (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
    i = stdin;
    if (exist(1))
        if (LLEN(*ARG1)) i = find_file(ARG1);
    if (!i) i = open_file(ARG1, "r+");
    if (!i) i = open_file(ARG1, "r");
    if (!i)
        (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);

    if (func == f_chars)
        Licpy(ARGR, Lchars(i));
    else if (func == f_lines)
        Licpy(ARGR, Llines(i));
} /* R_charslines */

/* --------------------------------------------------------------- */
/*  CHARIN((file)(,(start)(,length)))                              */
/* --------------------------------------------------------------- */
/*  LINEIN((file)(,(line)(,count)))                                */
/* --------------------------------------------------------------- */
void __CDECL
R_charlinein(const int func) {
    FILE *i;
    long start, length;
    Context *context = (Context *) CMSGetPG();

    if (!IN_RANGE(1, ARGN, 3))
        (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
    i = stdin;
    if (exist(1))
        if (LLEN(*ARG1)) i = find_file(ARG1);
    if (!i) i = open_file(ARG1, "r+");
    if (!i) i = open_file(ARG1, "r");
    if (!i)
        (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);
    get_oiv(2, start, LSTARTPOS);
    get_oiv(3, length, 1);

    if (func == f_charin)
        Lcharin(i, ARGR, start, length);
    else if (func == f_linein)
        Llinein(i, ARGR, start, length);
} /* R_charlinein */

/* --------------------------------------------------------------- */
/*  CHAROUT((file)(,(string)(,start)))                             */
/* --------------------------------------------------------------- */
/*  LINEOUT((file)(,(string)(,start)))                             */
/* --------------------------------------------------------------- */
void __CDECL
R_charlineout(const int func) {
    FILE *i;
    long start;
    PLstr str;
    Context *context = (Context *) CMSGetPG();

    if (!IN_RANGE(1, ARGN, 3))
        (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
    i = stdout;
    if (exist(1))
        if (LLEN(*ARG1)) i = find_file(ARG1);
    if (!i) {
        i = open_file(ARG1, "r+");
        if (i) append(i);
        else i = open_file(ARG1, "w+");
    }
    if (!i)
        (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);

    if (exist(2)) {
        L2STR(ARG2);
        str = ARG2;
    } else {
        FCLOSE(i); /* LINEOUT with only the file specified closes the file */
        return;
    }

    get_oiv(3, start, LSTARTPOS);

    if (func == f_charout) {
        Lcharout(i, str, start);
        Licpy(ARGR, LLEN(*ARG2));
    } else if (func == f_lineout)
        Licpy(ARGR, Llineout(i, str, start));
} /* R_charlineout */

/* --------------------------------------------------------------- */
/*  WRITE( (file)(, string(,)))                                    */
/* --------------------------------------------------------------- */
void __CDECL
R_write() {
    FILE *i;
    Context *context = (Context *) CMSGetPG();

    if (!IN_RANGE(1, ARGN, 3))
        (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
    i = stdout;
    if (exist(1))
        if (LLEN(*ARG1)) i = find_file(ARG1);
    if (!i) i = open_file(ARG1, "w");
    if (!i)
        (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);
    if (exist(2)) {
        Lwrite(i, ARG2, FALSE);
        Licpy(ARGR, LLEN(*ARG2));
    } else {
        FPUTC('\n', i);
        Licpy(ARGR, 1);
    }
    if (ARGN == 3) {
        FPUTC('\n', i);
        LINT(*ARGR)++;
    }
}  /* R_write */

/* --------------------------------------------------------------- */
/*  READ( (file)(,length) )                                        */
/*  length can be a number declaring number of bytes to read       */
/*  or an option 'file', 'line' or 'char'                          */
/* --------------------------------------------------------------- */
void __CDECL
R_read() {
    FILE *i;
    long l;
    Context *context = (Context *) CMSGetPG();

    if (!IN_RANGE(0, ARGN, 2))
        (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
    i = stdin;
    if (exist(1))
        if (LLEN(*ARG1)) i = find_file(ARG1);
    if (!i) i = open_file(ARG1, "r");
    if (!i)
        (context->lstring_Lerror)(ERR_CANT_OPEN_FILE, 0);

    if (exist(2)) {
        /* search to see if it is a number */
        if ((LTYPE(*ARG2) == LSTRING_TY) && (_Lisnum(ARG2) == LINTEGER_TY))
            l = Lrdint(ARG2);
        else if (LTYPE(*ARG2) == LINTEGER_TY)
            l = (int) LINT(*ARG2);
        else if (LTYPE(*ARG2) == LREAL_TY)
            l = Lrdint(ARG2);
        else if (LTYPE(*ARG2) == LSTRING_TY) {
            switch ((context->lstring_l2u)[(byte) LSTR(*ARG2)[0]]) {
                case 'F':
                    l = LREADFILE;
                    break;
                case 'L':
                    l = LREADLINE;
                    break;
                case 'C':
                    l = 1;
                    break;
                default:
                    (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
            }
        } else
            (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
    } else
        l = LREADLINE;

    Lread(i, ARGR, l);
} /* R_read */

/* --------------------------------------------------------------- */
/*  SEEK( file (,offset (,"TOF","CUR","EOF")))                     */
/* --------------------------------------------------------------- */
void __CDECL
R_seek() {
    FILE *i;
    long l;
    int SEEK = SEEK_SET;
    Context *context = (Context *) CMSGetPG();

    if (!IN_RANGE(1, ARGN, 3))
        (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
    must_exist(1);
    i = find_file(ARG1);
    if (!i)
        (context->lstring_Lerror)(ERR_FILE_NOT_OPENED, 0);

    if (exist(2)) {
        l = Lrdint(ARG2);
        if (exist(3)) {
            L2STR(ARG3);
            switch ((context->lstring_l2u)[(byte) LSTR(*ARG3)[0]]) {
                case 'T': /* TOF */
                    SEEK = SEEK_SET;
                    break;
                case 'C':
                    SEEK = SEEK_CUR;
                    break;
                case 'E':
                    SEEK = SEEK_END;
                    break;
                default:
                    (context->lstring_Lerror)(ERR_INCORRECT_CALL, 0);
            }
        }
        FSEEK(i, l, SEEK);
        l = FSEEK(i, l, SEEK);
        if (l) (context->lstring_Lerror)(ERR_NOT_RANDOM_ACCESS, 0);
    }
    Licpy(ARGR, FTELL(i));
} /* R_seek */
