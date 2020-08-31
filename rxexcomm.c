/*  Set or retrieve a variable value from the */
/*  internal variable pool.                   */
#include <cmssys.h>
#include "rexx.h"

#define __RXEXCOMM_C__

void __CDECL
RXEXCM(int set, PLstr varname, PLstr value)
{
   char response;
   Lstr str;
   Context *context = (Context*)CMSGetPG();

   /* Setup a parameter to specify use */
   /*       the internal variable pool */
   LINITSTR(str); Lfx(&str,sizeof(dword));
   Licpy(&str, context->rexx_rx_proc);

   if (set) {
     /* Convert character number to binary if needed */
     if IS_NUMBER(value) {
        L2NUM(value);
        }
     /* Set the variable value */
     response = RxPoolSet(&str, varname, value);
     }
   else {
     /* Retrieve the variable value */
     response = RxPoolGet(&str, varname, value);
     /* Convert binary number to character if needed */
     L2STR(value);
     }
   LFREESTR(str);
} /* RXE */
