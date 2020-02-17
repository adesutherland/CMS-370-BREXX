/* Modified for VM/370 CMS and GCC by Robert O'Hara, July 2010. */
 
#include <stdlib.h>
#include <stdio.h>
 
 
gcvt(double num, size_t ndigits, char *result)
{
double b;
int i, exp, pdigits, format;
char sign, work[45];
 
// Save original data & set sign.
// printf("gcvt: converting %f with [%d] digits.\n", num, ndigits);
 
if ( num < 0 ) {
   b = -num;
   sign = '-';
   }
else
   {
   b = num;
   sign = '+';
   }
 
// Now scale to get exponent.
 
exp = 0;
if (b > 1.0)
   while (b >= 10.0) {
      ++exp;
      b = b / 10.0;
      }
else if (b == 0.0) exp=0;
else if (b < 1.0)
   while (b <= 1.0) {
      --exp;
      b = b * 10.0;
      }
 
// printf("gcvt: after scaling %e exp is %d\n", b, exp);
 
// Now decide how to print and save in FORMAT.  If exp
//    -1 => we need leading digits
//     0 => print in exp
//    +1 => we have digits before dp.
 
if (exp >= 0) {                                                               // original number > 1
   if (ndigits > exp) {
//      printf("gcvt: can print as normal, as exp > 1.\n");
      format = 1;
      }
   else {
//      printf("gcvt: must print in exp.\n");
      format = 0;
      }
   }
else {
   if (ndigits > (- (exp + 1))) {
//      printf("gcvt: can print as normal, as exp < 1.\n");
      format = -1;
      }
   else {
//      printf("gcvt: must print in exp < 1.\n");
      format=0;
      }
   }
 
// Now extract the requesite number of digits.
switch (format) {
   case -1:                 // number less than 1.0 so we need to print the 0. and the leading zeros
      result[0] = sign;
      result[1] = '0';
      result[2] = '.';
      result[3] = 0x00;
      while (++exp) {
         --ndigits;
         strcat(result, "0");
         }
      i = b;
      ++ndigits;
      sprintf(work, "%d%", i);
      break;
   case 1:                                               // number ge 1.0 just print the first digit
      i = b;
      result[0] = sign;
      result[1] = '\0';
      sprintf(work, "%d", i);
      strcat(result, work);
      break;
   default:                                                             // printing in standard form
      i = b;
      result[0] = sign;
      result[1] = '\0';
      sprintf(work, "%d", i);
      strcat(result, work);
      strcat(result, ".");
      break;
   }
 
pdigits = ndigits;
while (--pdigits) {
   b = b - i;
   b = b * 10.0;
   i = b;
   sprintf(work, "%d", i);
   strcat(result, work);
   if ((ndigits - pdigits) == exp) strcat(result, ".");
   }
 
if (format==0) {                                                      // exp format - put exp on end
   sprintf(work, "e%3.2d", exp);
   strcat(result, work);
   }
 
// printf("gcvt: result is [%s], printf is [%15g].\n", result, num);
}
