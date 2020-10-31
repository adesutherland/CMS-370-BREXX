/*
 * $Id: nextsymb.c,v 1.8 2008/07/15 07:40:25 bnv Exp $
 * $Log: nextsymb.c,v $
 * Revision 1.8  2008/07/15 07:40:25  bnv
 * #include changed from <> to ""
 *
 * Revision 1.7  2004/04/30 15:25:41  bnv
 * Spaces...
 *
 * Revision 1.6  2004/03/27 08:34:21  bnv
 * Corrected: Line number was not restored after the comma at the end of the line
 *
 * Revision 1.5  2003/01/30 08:22:37  bnv
 * Corrected: comment after the comma_sy search
 *
 * Revision 1.4  2002/06/11 12:37:38  bnv
 * Added: CDECL
 *
 * Revision 1.3  2001/06/25 18:51:48  bnv
 * Header -> Id
 *
 * Revision 1.2  1999/11/26 13:13:47  bnv
 * Changed: Some spaces to tabs.
 *
 * Revision 1.1  1998/07/02 17:34:50  bnv
 * Initial revision
 *
 */

#define  __NEXTSYMB_C__
#include <cmssys.h>
#include "lerror.h"
#include "lstring.h"
#include "nextsymb.h"

/* ------------------- function prototypes ----------------------- */
static void literal(void);
static void identifier(int isnumber);

/* --------------------------------------------------------------- */
/*  return the next character and advance the input stream by one  */
/*  also it searches for comments                                  */
/* --------------------------------------------------------------- */
static void
nextchar(int instring)
{
 int level=0;
 Context *context = (Context*)CMSGetPG();

 (context->nextsymb_commentfound) = FALSE;

getnextchar:
 if (instring!=-1) {
  if (*(context->nextsymbsymbolptr)=='\n') (context->nextsymbsymboline)++;
  (context->nextsymbsymbolptr)++;
 } else
  instring=FALSE;

 if (!instring && *(context->nextsymbsymbolptr)=='/' && *((context->nextsymbsymbolptr)+1)=='*') {
  /* search for comment */
  (context->nextsymb_commentfound) = TRUE;
  (context->nextsymbsymbolptr) += 2;
  level++;
  for (;;) {
   while ((*(context->nextsymbsymbolptr)!='*') && (*(context->nextsymbsymbolptr)!='/')) {
    if (*(context->nextsymbsymbolptr)=='\n') (context->nextsymbsymboline)++;
    (context->nextsymbsymbolptr)++;
    if (*(context->nextsymbsymbolptr) == 0)
     (context->lstring_Lerror)(ERR_UNMATCHED_QUOTE,1);
   };
   (context->nextsymbsymbolptr)++;
   if ((*((context->nextsymbsymbolptr)-1)=='/') && (*(context->nextsymbsymbolptr)=='*')) {
    level++;
    (context->nextsymbsymbolptr)++;
   } else
   if ((*((context->nextsymbsymbolptr)-1)=='*') && (*(context->nextsymbsymbolptr)=='/')) {
    --level;
    if (!level) goto getnextchar;
    (context->nextsymbsymbolptr)++;
   }
  }
 }
} /* nextchar */

/* --------------- InitNextsymbol -------------------- */
void __CDECL
InitNextsymbol( PLstr str )
{
 Context *context = (Context*)CMSGetPG();
 Lcat(str,"\n"); /* We must have a least one new line at the end */

 (context->nextsymb_ProgStr)   = str;
 LASCIIZ(*(context->nextsymb_ProgStr));  /* Put a zero at the end */
 (context->nextsymbsymbol)    = semicolon_sy;
 (context->nextsymbsymbolptr) = LSTR(*(context->nextsymb_ProgStr));
 (context->nextsymbsymboline) = 1;

 /* Skip first line,  '#!/bin/rexx' */
 if ((context->nextsymbsymbolptr)[0]=='#' && (context->nextsymbsymbolptr)[1]=='!') {
  while (*(context->nextsymbsymbolptr)!='\n') (context->nextsymbsymbolptr)++;
  (context->nextsymbsymboline)++;
 }

 (context->nextsymb_InitNextch) = FALSE;
 (context->nextsymb_NextBlank)  = FALSE;
 (context->nextsymbsymbolstat) = normal_st;
} /* InitNextsymbol */

/* --------------------------------------------------------------- */
/*            P A R S E   next  B A S I C   S Y M B O L            */
/*  Return the next basic (context->nextsymbsymbol) and advance the input stream      */
/* --------------------------------------------------------------- */
void __CDECL
nextsymbol(void)
{
 Context *context = (Context*)CMSGetPG();
#define NEXTCHAR {*(ns++)=*(context->nextsymbsymbolptr); LLEN((context->nextsymbsymbolstr))++; nextchar(FALSE);}

 char *Psymbolptr, *ns;
 int _lineno;

 if (!(context->nextsymb_InitNextch)) {
  /* call nextchar to search for comments */
  nextchar(-1); /* initialise */
  (context->nextsymb_commentfound) = FALSE;
  (context->nextsymb_InitNextch) = TRUE;
 }
 /* make the type always to be LSTRING */
 LTYPE((context->nextsymbsymbolstr)) = LSTRING_TY;
 LLEN((context->nextsymbsymbolstr)) = 0;

 (context->nextsymb__in_nextsymbol) = TRUE;
 (context->nextsymbsymbolPrevBlank) = (context->nextsymb_NextBlank);
 (context->nextsymb_NextBlank) = FALSE;
_NEXTSYMBOL:

 while (*(context->nextsymbsymbolptr)==' ' || *(context->nextsymbsymbolptr)=='\t')
  nextchar(FALSE);

 ns = LSTR((context->nextsymbsymbolstr));

 (context->nextsymbsymbolprevptr) = (context->nextsymbsymbolptr);

 switch ((context->lstring_l2u)[(byte)*(context->nextsymbsymbolptr)]) {
  case '0':    case '1':    case '2':
  case '3':    case '4':    case '5':
  case '6':    case '7':    case '8':
  case '9':
   (context->nextsymbsymbol) = literal_sy;
   identifier(TRUE);
   break;

  case 'A':    case 'B':    case 'C':
  case 'D':    case 'E':    case 'F':
  case 'G':    case 'H':    case 'I':
  case 'J':    case 'K':    case 'L':
  case 'M':    case 'N':    case 'O':
  case 'P':    case 'Q':    case 'R':
  case 'S':    case 'T':    case 'U':
  case 'V':    case 'W':    case 'X':
  case 'Y':    case 'Z':

  case '@':
  case '#':
  case '$':
  case '_':
  case '?':
  case '!':
   (context->nextsymbsymbol) = ident_sy;
   identifier(FALSE);
   break;

  case '/':
   NEXTCHAR;
   if (*(context->nextsymbsymbolptr)=='/')  {
    (context->nextsymbsymbol) = mod_sy;
    NEXTCHAR;
   } else
   if (*(context->nextsymbsymbolptr)=='=')  {
    NEXTCHAR;
    if (*(context->nextsymbsymbolptr)=='=')  {
     (context->nextsymbsymbol) = dne_sy;
     NEXTCHAR;
    }  else
     (context->nextsymbsymbol) = ne_sy;
   } else
    (context->nextsymbsymbol) = div_sy;
   break;

  case '-':
   (context->nextsymbsymbol) = minus_sy;
   NEXTCHAR;
   break;

  case '*':
   NEXTCHAR;
   if (*(context->nextsymbsymbolptr)=='/')
    (context->lstring_Lerror)(ERR_INVALID_EXPRESSION,0);
   if (*(context->nextsymbsymbolptr)=='*')  {
    (context->nextsymbsymbol) = power_sy;
    NEXTCHAR;
   } else
    (context->nextsymbsymbol) = times_sy;
   break;

  case '~':
  case '^':
  case '\\':
   NEXTCHAR;
   switch (*(context->nextsymbsymbolptr))  {
    case '=':
     NEXTCHAR;
     if (*(context->nextsymbsymbolptr)=='=') {
      (context->nextsymbsymbol) = dne_sy;
      NEXTCHAR;
     } else
      (context->nextsymbsymbol) = ne_sy;
     break;

    case '>':
     NEXTCHAR;
     if (*(context->nextsymbsymbolptr)=='>') {
      (context->nextsymbsymbol) = dle_sy;
      NEXTCHAR;
     } else
      (context->nextsymbsymbol) = le_sy;
     break;

    case '<':
     NEXTCHAR;
     if (*(context->nextsymbsymbolptr)=='<') {
      (context->nextsymbsymbol) = dge_sy;
      NEXTCHAR;
     } else
      (context->nextsymbsymbol) = ge_sy;
     break;

    default:
     (context->nextsymbsymbol) = not_sy;
   }
   break;

  case '%':
   (context->nextsymbsymbol) = intdiv_sy;
   NEXTCHAR;
   break;

  case '(':
   (context->nextsymbsymbol) = le_parent;
   NEXTCHAR;
   break;

  case ')':
   (context->nextsymbsymbol) = ri_parent;
   NEXTCHAR;
   break;

#ifndef MSDOS
 /* Only for UNIX where \r is not recognised as \n */
  case '\r':
   NEXTCHAR;
#endif
  case '\n':
  case ';':
   (context->nextsymbsymbol) = semicolon_sy;
   NEXTCHAR;
   break;

  case ',':
   NEXTCHAR;
   Psymbolptr = (context->nextsymbsymbolptr);
   _lineno = (context->nextsymbsymboline);
   nextsymbol();
   (context->nextsymb_NextBlank) = FALSE;
   (context->nextsymbsymbolPrevBlank) = TRUE;
   if ((context->nextsymbsymbol)==semicolon_sy &&
    _lineno!=(context->nextsymbsymboline)) goto _NEXTSYMBOL;
   (context->nextsymbsymbolptr) = Psymbolptr;
   (context->nextsymbsymboline) = _lineno;
   (context->nextsymbsymbol) = comma_sy;
   (context->nextsymb_NextBlank) = FALSE;
   (context->nextsymbsymbolPrevBlank) = TRUE;
   (context->nextsymb_commentfound) = FALSE;
   break;

  case '.':
   Psymbolptr = (context->nextsymbsymbolptr)+1;
   if ( *Psymbolptr !=' '   &&
    *Psymbolptr !=','   &&
    *Psymbolptr !='\n'  &&
    *Psymbolptr !='\r'  &&
    *Psymbolptr !='\t'  &&
    *Psymbolptr !=';' )  {
     (context->nextsymbsymbol) = literal_sy;
     identifier(TRUE);
   } else {
    (context->nextsymbsymbol) = dot_sy;
    NEXTCHAR;
   }
   break;

  case '|':
   NEXTCHAR;
   if (*(context->nextsymbsymbolptr)=='|')  {
    (context->nextsymbsymbol) = concat_sy;
    NEXTCHAR;
   } else
    (context->nextsymbsymbol) = or_sy;
   break;

  case '&':
   NEXTCHAR;
   if (*(context->nextsymbsymbolptr)=='&')  {
    (context->nextsymbsymbol) = xor_sy;
    NEXTCHAR;
   } else
    (context->nextsymbsymbol) = and_sy;
   break;

  case '+':
   (context->nextsymbsymbol) = plus_sy;
   NEXTCHAR;
   break;

  case '=':
   NEXTCHAR;
   switch (*(context->nextsymbsymbolptr))  {
    case '=':
     NEXTCHAR;
     (context->nextsymbsymbol) = deq_sy;
     break;

    case '>':
     NEXTCHAR;
     if (*(context->nextsymbsymbolptr)=='>') {
      NEXTCHAR;
      (context->nextsymbsymbol) = dge_sy;
     } else
      (context->nextsymbsymbol) = ge_sy;
     break;

    case '<':
     NEXTCHAR;
     if (*(context->nextsymbsymbolptr)=='<') {
      NEXTCHAR;
      (context->nextsymbsymbol) = dle_sy;
     } else
      (context->nextsymbsymbol) = le_sy;
     break;

    default:
     (context->nextsymbsymbol) = eq_sy;
   }
   break;

  case '<':
   NEXTCHAR;
   switch (*(context->nextsymbsymbolptr))  {
    case '<':
     NEXTCHAR;
     if (*(context->nextsymbsymbolptr)=='=') {
      (context->nextsymbsymbol) = dle_sy;
      NEXTCHAR;
     } else
      (context->nextsymbsymbol) = dlt_sy;
     break;

    case '=':
     (context->nextsymbsymbol) = le_sy;
     NEXTCHAR;
     break;

    case '>':
     (context->nextsymbsymbol) = ne_sy;
     NEXTCHAR;
     break;

    default:
     (context->nextsymbsymbol) = lt_sy;
   }
   break;

  case '>':
   NEXTCHAR;
   switch (*(context->nextsymbsymbolptr))  {
    case '>':
     NEXTCHAR;
     if (*(context->nextsymbsymbolptr)=='=') {
      (context->nextsymbsymbol) = dge_sy;
      NEXTCHAR;
     } else
      (context->nextsymbsymbol) = dgt_sy;
     break;

    case '=':
     (context->nextsymbsymbol) = ge_sy;
     NEXTCHAR;
     break;

    case '<':
     (context->nextsymbsymbol) = ne_sy;
     NEXTCHAR;
     break;

    default:
     (context->nextsymbsymbol) = gt_sy;
   }
   break;

  case '\'':
  case '\"':
   literal();
   break;

  case ':':
   (context->lstring_Lerror)(ERR_SYMBOL_EXPECTED,0);
   break;

  case 0:
   (context->nextsymb__in_nextsymbol) = FALSE;
   if (((context->nextsymbsymbolstat) == in_do_st) ||
       ((context->nextsymbsymbolstat) == in_if_st))
    (context->lstring_Lerror)(ERR_INCOMPLETE_STRUCT,0);
   else
   if ((context->nextsymbsymbolstat) == in_if_init_st)
    (context->lstring_Lerror)(ERR_THEN_EXPECTED,0);
   else
   if ((context->nextsymbsymbolstat) != normal_st)
    (context->lstring_Lerror)(ERR_SYMBOL_EXPECTED,0);
   (context->nextsymbsymbol) = exit_sy;
   (context->nextsymbsymboline)--; /* Seems to add a line number ... */
   break;

  default:
   (context->lstring_Lerror)(ERR_INVALID_CHAR,0);
 }
 if ( *(context->nextsymbsymbolptr)== ' ' ||
  *(context->nextsymbsymbolptr)=='\t' ||
  *(context->nextsymbsymbolptr)== ',' )
   (context->nextsymb_NextBlank) = TRUE;
 (context->nextsymb__in_nextsymbol) = FALSE;
} /* nextsymbol */

/* --------------------------------------------------------------- */
/*      find the identifier                                        */
/* --------------------------------------------------------------- */
static void
identifier(int isnumber)
{
 char *s;
 int l;     /* length */     /* -+-  l > maxlen ? */
 int hasDot=FALSE, hasExp=FALSE;
 Context *context = (Context*)CMSGetPG();

 (context->nextsymbsymbolisstr)   = FALSE;
 (context->nextsymbsymbolhasdot)  = 0;
 s = LSTR((context->nextsymbsymbolstr));
 l = 0;

 for (;;) {
  if (l>LMAXLEN((context->nextsymbsymbolstr)))
   (context->lstring_Lerror)(ERR_TOO_LONG_STRING,0);

  if ((context->nextsymb_commentfound) &&
   *(context->nextsymbsymbolptr)!='(' && *(context->nextsymbsymbolptr)!=':') {
   (context->nextsymb_commentfound) = FALSE;
   *s='\0';
   LLEN((context->nextsymbsymbolstr)) = l;
   goto Nleave;
  }

  switch ((context->lstring_l2u)[(byte)*(context->nextsymbsymbolptr)]) {
   case '0':   case '1':    case '2':
   case '3':   case '4':    case '5':
   case '6':   case '7':    case '8':
   case '9':
    *s++ = *(context->nextsymbsymbolptr);
    l++;
    nextchar(FALSE);
    break;

   case 'A':   case 'B':    case 'C':
   case 'D':   case 'E':    case 'F':
   case 'G':   case 'H':    case 'I':
   case 'J':   case 'K':    case 'L':
   case 'M':   case 'N':    case 'O':
   case 'P':   case 'Q':    case 'R':
   case 'S':   case 'T':    case 'U':
   case 'V':   case 'W':    case 'X':
   case 'Y':   case 'Z':

   case '@':
   case '#':
   case '$':
   case '_':
   case '?':
   case '!':
    *s = (context->lstring_l2u)[(byte)*(context->nextsymbsymbolptr)];
    if (isnumber) {
     if (*s=='E') {
      if (hasExp)
       isnumber = FALSE;
      hasExp = TRUE;
     } else
      isnumber = FALSE;
    }
    s++; l++;
    nextchar(FALSE);
    break;

   case '+':
   case '-':
    if (isnumber) {
     if (*(s-1)=='E') {
      *s++ = *(context->nextsymbsymbolptr); l++;
      nextchar(FALSE);
     } else {
      *s='\0';
      LLEN((context->nextsymbsymbolstr)) = l;
      goto Nleave;
     }
    } else {
     *s='\0';
     LLEN((context->nextsymbsymbolstr)) = l;
     goto Nleave;
    }
    break;

   case '.':
    *s++ = *(context->nextsymbsymbolptr); l++;
    if (!(context->nextsymbsymbolhasdot)) /* position of first */
     (context->nextsymbsymbolhasdot) = l; /* dot */
    nextchar(FALSE);
    if (isnumber) {
     if (hasDot)
      isnumber = FALSE;
     hasDot = TRUE;
     (context->nextsymbsymbolhasdot) = 0;
    }
    break;

   case '(':
    nextchar(FALSE);
    *s='\0';
    LLEN((context->nextsymbsymbolstr)) = l;
    (context->nextsymbsymbol) = function_sy;
    return;

   case ':':
    nextchar(FALSE);
    *s='\0';
    LLEN((context->nextsymbsymbolstr)) = l;
    (context->nextsymbsymbol) = label_sy;
    return;

   case '\t':
   case ' ':
    (context->nextsymb_NextBlank) = TRUE;
    *s='\0';
    LLEN((context->nextsymbsymbolstr))=l;
    while (*(context->nextsymbsymbolptr)==' '||*(context->nextsymbsymbolptr)=='\t')
     nextchar(FALSE);

    /* literal finished and it is not a label? */
    if (*(context->nextsymbsymbolptr)!=':')
     goto Nleave;

    /* literal is label */
    (context->nextsymbsymbol) = label_sy;
    nextchar(FALSE);
    return;

   default:
    *s='\0';
    LLEN((context->nextsymbsymbolstr)) = l;
    goto Nleave;
  }  /* end of switch */
 } /* end of for */
Nleave:
 if ((context->nextsymbsymbol)!=ident_sy) return ;

 if ((context->nextsymbsymbolhasdot) == LLEN((context->nextsymbsymbolstr)))
  (context->nextsymbsymbolhasdot) = 0; /* treat is as a variable */

 if ((context->nextsymbsymbolstat) == in_do_init_st)  {
       if (!Lcmp(&(context->nextsymbsymbolstr),"BY"   ))  (context->nextsymbsymbol) = by_sy;
  else if (!Lcmp(&(context->nextsymbsymbolstr),"FOR"  ))  (context->nextsymbsymbol) = for_sy;
  else if (!Lcmp(&(context->nextsymbsymbolstr),"TO"   ))  (context->nextsymbsymbol) = to_sy;
  else if (!Lcmp(&(context->nextsymbsymbolstr),"UNTIL"))  (context->nextsymbsymbol) = until_sy;
  else if (!Lcmp(&(context->nextsymbsymbolstr),"WHILE"))  (context->nextsymbsymbol) = while_sy;
 } else
 if ((context->nextsymbsymbolstat) == in_parse_value_st) {
  if (!Lcmp(&(context->nextsymbsymbolstr),"WITH" ))  (context->nextsymbsymbol) = with_sy;
 } else
 if ((context->nextsymbsymbolstat) == in_if_init_st) {
  if (!Lcmp(&(context->nextsymbsymbolstr),"THEN" ))  (context->nextsymbsymbol) = then_sy;
 }
} /* identifier */

/* --------------------------------------------------------------- */
/*  extract a literal (context->nextsymbsymbol)                                       */
/* --------------------------------------------------------------- */
static void
literal(void)
{
 char quote;
 char *s;
 int l;      /* length of (context->nextsymbsymbolstr) */
 Lstr A;
 Context *context = (Context*)CMSGetPG();

 (context->nextsymbsymbolhasdot) = 0;
 (context->nextsymbsymbol) = literal_sy;
 quote  = *(context->nextsymbsymbolptr);
 s = LSTR((context->nextsymbsymbolstr));
 l = 0;
 (context->nextsymbsymbolisstr) = TRUE;

 for (;;)  {   /* -+-  l > maxlen ? */
  nextchar(TRUE);
  if (l>=LMAXLEN((context->nextsymbsymbolstr)))
   (context->lstring_Lerror)(ERR_TOO_LONG_STRING,0);
  if (*(context->nextsymbsymbolptr)==quote) {
   nextchar(FALSE); /* quote ended?? */
   if (*(context->nextsymbsymbolptr) == '(') {
    *s = '\0';
    LLEN((context->nextsymbsymbolstr)) = l;
    (context->nextsymbsymbol) = function_sy;
    nextchar(FALSE);
    return;
   } else
   if ((context->nextsymb_commentfound)) { /* a comment was inside */
    (context->nextsymb_commentfound) = FALSE;
    *s = '\0';
    LLEN((context->nextsymbsymbolstr)) = l;
    return;
   } else
   if (STRCHR("bBxXhH",*(context->nextsymbsymbolptr))) {
    /* check next char */
    char nc=(context->lstring_l2u)[(byte)*((context->nextsymbsymbolptr)+1)];

    *s = '\0';
    LLEN((context->nextsymbsymbolstr)) = l;

    if (IN_RANGE('0',nc,'9') ||
        IN_RANGE('A',nc,'Z') ||
        nc=='@' || nc=='#' || nc=='$' ||
        nc=='_' || nc=='?' || nc=='!' ||
        nc=='.')
     return;

    LINITSTR(A);

    switch ((context->lstring_l2u)[(byte)*(context->nextsymbsymbolptr)]) {
     case 'B':
      if (!Ldatatype(&(context->nextsymbsymbolstr),'B'))
       (context->lstring_Lerror)(ERR_INVALID_HEX_CONST,0);
      Lb2x(&A,&(context->nextsymbsymbolstr));
      Lx2c(&(context->nextsymbsymbolstr),&A);
      break;
     case 'H':
      if (!Ldatatype(&(context->nextsymbsymbolstr),'X'))
       (context->lstring_Lerror)(ERR_INVALID_HEX_CONST,0);
      Lx2d(&A,&(context->nextsymbsymbolstr),0);
      Lstrcpy(&(context->nextsymbsymbolstr),&A);
      break;
     case 'X':
      if (!Ldatatype(&(context->nextsymbsymbolstr),'X'))
       (context->lstring_Lerror)(ERR_INVALID_HEX_CONST,0);
      Lx2c(&A,&(context->nextsymbsymbolstr));
      Lstrcpy(&(context->nextsymbsymbolstr),&A);
      break;
    }
    nextchar(FALSE);
    LFREESTR(A);
    return;
   } else
   if (*(context->nextsymbsymbolptr)==quote) {
    *s++ = *(context->nextsymbsymbolptr); l++;
   }  else  {
    *s = '\0';
    LLEN((context->nextsymbsymbolstr)) = l;
    return;
   }
  } else
  if (*(context->nextsymbsymbolptr) == '\n') {
   *s++ = ' '; l++;
  } else
  if (*(context->nextsymbsymbolptr) == 0)
   (context->lstring_Lerror)(ERR_UNMATCHED_QUOTE,
    (quote=='\'')?2:3);
  else {
   *s++ = *(context->nextsymbsymbolptr);
   l++;
  }
 }
} /* literal */
