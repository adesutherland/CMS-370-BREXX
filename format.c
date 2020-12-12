/* Modified for VM/370 CMS and GCC by Robert O'Hara, July 2010. */
/*
 * $Id: format.c,v 1.7 2008/07/15 07:40:54 bnv Exp $
 * $Log: format.c,v $
 * Revision 1.7  2008/07/15 07:40:54  bnv
 * #include changed from <> to ""
 *
 * Revision 1.6  2008/07/14 13:08:16  bnv
 * MVS,CMS support
 *                                                 bb
 * Revision 1.5  2002/06/11 12:37:15  bnv
 * Added: CDECL
 *
 * Revision 1.4  2001/06/25 18:49:18  bnv
 * Corrected: The calculation of the final size of the Lstring
 *
 * Revision 1.3  1999/11/26 09:56:55  bnv
 * Changed: Use of swprintf in CE version.
 *
 * Revision 1.2  1999/06/10 14:09:24  bnv
 * Added the possibility to use the E,F or G format of C printf
 *
 * Revision 1.1  1998/07/02 17:18:00  bnv
 * Initial Version
 *
 */

#include "lstring.h"
#include <cmssys.h>

/* ---------------- Lformat ------------------ */
void __CDECL
Lformat(const PLstr to, const PLstr from,
        long before, long after, long expp, long expt) {
/****************
 Lstr tmp,Integer,Befo,Afte,Mantissa,Exponent;
 long i,j,Point,Afters;
 int  Sign,ShowExp;

 LINITSTR(tmp);
 LINITSTR(Integer);
 LINITSTR(Befo);
 LINITSTR(Afte);
 LINITSTR(Mantissa);
 LINITSTR(Exponent);

 Lspace(&tmp,from,0,' '); // * trim spaces *

 for (i=0; i<LLEN(tmp); i++) // * split in Mantissa 'E' Exponent *
  if (LSTR(tmp)[i]=='e' || LSTR(tmp)[i]=='E') {
   Lsubstr(&Exponent,&tmp,i+2,0,' ');
   Lsubstr(&Mantissa,&tmp,1,i,' ');
   goto Lfo10;
  }
 Lstrcpy(&Mantissa,&tmp);
Lfo10:
 Sign = (LLSTR(Mantissa)[0] == '-');
 if (Sign) {
  Lstrcpy(&tmp,&Mantissa);
  Lsubstr(&Mantissa,&tmp,2,0,' ');
 }

 for (i=0; i<LLEN(Mantissa); i++)
  if (LSTR(Mantissa)[i]=='.') {
   Lleft(&Befo,&Mantissa,i,' ');
   Lsubstr(&Afte,&Mantissa,i+2,0,' ');
   goto Lfo20;
  }
 Lstrcpy(&Befo,&Mantissa);
Lfo20:
 Point = LLEN(Befo);

 // * Sign, Mantissa and Exponent now reflect the number. Befo, Afte
 // * and Point reflect Mantissa *

 // * The fourth and fifth argument allow for exponential notation.
 // * Decide whether exponential form to be used, setting ShowExp. *

 ShowExp = 0;
 L2INT(&Exponent);

 if (expp>=0 || expt>=0) {
  if (expt<0) expt = digits.level;
  // * decide whether exponential form to be used. *
  if ((Point + LINT(Exponent)) > expt)
   ShowExp = 1; // * Digits before rule. *
  LeftOfPoint = 0;
  if (LLEN(Befo) > 0)
   LeftOfPoint = Befo; // * Value left of the point *

  // * Digits after point rule for exponentiation *
  // * Count zeros to right of point *
  z = 0;
  while (LSTR(Afte)[z] == '0') z++;
  if ((LeftOfPoint=0) && ((z-LINT(Exponent)) > 5)) ShowExp = 1

  // * An extra rule for exponential form *
  if (expp=0) ShowExp = 0;

  // * Construct the exponential part of the result. *
  if (ShowExp) {
   LINT(Exponent) = LINT(Exponent) + (Point-1);
   Point = 1; // * As required for scientific *
   if (digits.form = ENGINEERING)
    while (LINT(Exponent)%3 != 0) {
     Point++;
     LINT(Exponent) -= 1;
    }
  } else
   Point += LINT(Exponent);
 } else {
  // * If expp and expt are not given, exponential notation
  // *  will be used if the original number+0 done by
  // *  checkArgs led to it. *
  if (LINT(Exponent))
   ShowExp = 1;
 }

 Lstrcpy(&Integer,&Befo);
 Lstrcat(&Integer,&Afte);
 if (Point<1) { // * Add extra zeros on the left *
  Lstrcpy(&tmp,&Integer);
  Lstrset(&Integer,1-Point,'0');
  Lstrcat(&Integer,&tmp);
  Point = 1;
 }

 If (Point>LLEN(Integer)) { // * and maybe on the right *
  Lleft(&tmp,&Integer,Point,'0');
  Lstrcpy(&Integer,&tmp);
 }
 // *  Deal with right of decimal point first since that can affect
 //   the left. Ensure the requested number of digits there. *
 Afters = LLEN(Integer) - Point;
 if (after==0) after = Afters;
 // * make Afters match the requested after *
 if (Afters<after) {
  Lleft(&tmp,&Integer,LLEN(Integer)+after-Afters,'0');
  Lstrcpy(&Integer,&tmp);
 }

......

 LFREESTR(tmp);
 LFREESTR(Integer);
 LFREESTR(befo);
 LFREESTR(afte);
 LFREESTR(mantissa);
 LFREESTR(exponent);
*********************/
#ifdef __CMS__
    // This code is not tied to CMS.  But I don't want to inflict it on other systems where I cannot
    // test it... RPO.
    //
    // As it stands, Lformat is called from R_format in RXCONV.C with 'before' and'after' set to 0
    // regardless of whether they were specified or omitted on the call to the FORMAT function.  We will
    // therefore adopt the convention that 'before' or 'after' with a value < 0 means they were not
    // specified on the call to FORMAT.
    char buffer[80];
    char * s;
    double t;
    int i;
    int digits;
    int exponent;
    int fractions;
    int len;
    Lstr tmp;
    LINITSTR(tmp);
    Context *context = (Context*)CMSGetPG();

    t = Lrdreal(from);
    sprintf(buffer, "%40.20f", t);
    len = strlen(buffer);
    for (i = 0; i < len; i++) if (isdigit(buffer[i])) break;             // find the start of the number
    digits = 1;
    for (i++; i < len; i++) {                                       // count digits before decimal point
       if (! isdigit(buffer[i])) break;
       digits++;
       }
    fractions = 0;
    for (i++; i < strlen(buffer); i++) {                             // count digits after decimal point
       if (! isdigit(buffer[i])) break;
       fractions++;
       }
    if ((digits > (context->lstring_lNumericDigits)) || (expp > 0)) {                     // exponential notation required?
       exponent = 1;
       if (expp > 0) fractions = expp;
       else if (fractions > ((context->lstring_lNumericDigits) - 1)) fractions = (context->lstring_lNumericDigits) - 1;
       }
    else {
       exponent = 0;  // standard display
       if ((digits + fractions) > (context->lstring_lNumericDigits)) fractions = (context->lstring_lNumericDigits) - digits;
       if ((after >= 0) && (after < fractions)) fractions = after;        // caller wants less precision
       }

    if (exponent) {                                                   // display in exponential notation
       sprintf(buffer, "%.*e", fractions, t);
       Lscpy(to, buffer);
       }
    else {  // display normally
       sprintf(buffer, "%20.*f", fractions, t);          // in the form nnnn.dddd with correct precision
       if (after < 0) {                                                     // must strip trailing zeros
          for (i = strlen(buffer) - 1; buffer[i] == '0'; i--) buffer[i] = '\0';
          if (buffer[i] == '.') buffer[i] = '\0';         // and the decimal pointif it is what is left
          }
       else if (after > fractions) {                                              // must pad with zeros
          len = strlen(buffer) + after - fractions;
          for (i = strlen(buffer); i < len ; i++) buffer[i] = '0';
          buffer[i] = '\0';
          }
       for (s = buffer; s[0] == ' '; s++);                                       // strip leading blanks
       // printf("format: [%s]\n", buffer);
       if (before > 0) {                                                   // right justify as requested
          len = before - digits + strlen(s);
          Lscpy(&tmp, s);
          Lright(to, &tmp, len, ' ');
          }
       else Lscpy(to, s);
       }
    LFREESTR(tmp);
    return;

#else
    double r;
    TCHAR str[50];

    r = Lrdreal(from);
    if (before < 0) before = 0;
    if (after < 0) after = 0;
    if (after) before += (after + 1);

# ifndef WCE
    sprintf(
# else
            swprintf(
# endif
            str,
            (expp <= 0) ? TEXT("%#*.*lf") :
            (expp == 1) ? TEXT("%#*.*lG") : TEXT("%#*.*lE"),
            (int) before, (int) after, r);

#ifndef WCE
    Lscpy(to, str);
#else
    Lwscpy(to,str);
#endif
#endif
} /* Lformat */
