/*  Set or retrieve a variable value from the */
/*  internal variable pool.                   */
#include <cmssys.h>
#include "rexx.h"

#define __RXEXCOMM_C__

int __CDECL
RXEXCM(int set, char* varname,
       char* buffer_value, int* buffer_length)
{
/*----------------------------------------------------------*/
/* Entry conditions:                                        */
/*   set      set to 1 to assign a REXX variable value      */
/*            set to 0 to retrieve a REXX variable value    */
/*                                                          */
/*   varname  is a pointer to the name of the REXX variable */
/*            being assigned or retrieved                   */
/*                                                          */
/*   buffer_value  is a pointer to the area containing the  */
/*            data being assigned or to the area to receive */
/*            data being retrieved                          */
/*                                                          */
/*   buffer_length is a pointer to the length of the data   */
/*            for an assignment, or a pointer to the        */
/*            length of the buffer to receive the data for  */
/*            a retrieval. The length of the retrieved data */
/*            is placed into the word pointed to by this    */
/*            pointer. If the length provided on input is   */
/*            less than the length of data to be retrieved, */
/*            the data that will fit is returned to the     */
/*            caller, and the actual data length is passed  */
/*            to the caller in the length word. The caller  */
/*            then allocates a larger buffer and retries    */
/*            the variable retrieval.                       */
/*----------------------------------------------------------*/
   char response;
   Lstr name;
   Lstr pool;
   Lstr value;
   Lstr tempstr;
   int rc = 0;
   int actual_length;
   Context *context = (Context*)CMSGetPG();

   /* Build an LSTR to specify that the internal     */
   /* variable pool contains the variable.           */

   LINITSTR(pool); Lfx(&pool,sizeof(dword));
   Licpy(&pool, context->rexx_rx_proc);

   /* Build a 'name' LSTR from the varname argument. */

   LINITSTR(name); Lfx(&name, strlen(varname));
   Lscpy(&name,varname);

   /* Build a 'tempstr' LSTR structure to receive    */
   /* the value of the variable from RxPoolGet.      */

   LINITSTR(tempstr); Lfx(&tempstr, 6);
   Lscpy(&tempstr,"VALUE");

   if (set) {
     /* Assign variable a value from the EXECCOMM caller    */

     /* Build a'value' LSTR from the buffer_value           */
     /* and buffer_length input parameters.                 */

     LINITSTR(value); Lfx(&value, *buffer_length);
     /* Lscpy(&value, buffer_value);                        */
     /* We move in the value ourselves instead of calling   */
     /* Lscpy, to support data buffers which contain        */
     /* binary zeros. Lscpy has problems wirh that.         */
     MEMCPY( LSTR(value), buffer_value, *buffer_length);
     LLEN(value) = *buffer_length;
     LTYPE(value) = LSTRING_TY;

     /* Set the variable value                              */

     response = RxPoolSet(&pool, &name, &value);
     LFREESTR(value);                   /* Release the LSTR */
     }
   else {
     /* Retrieve a variable value for the EXECCOMM caller   */

     response = RxPoolGet(&pool, &name, &tempstr);

     if (response == 0) {
       /* If variable has a numeric value, convert it to    */
       /* the character form required by the EXECCOMM       */
       /* specification.                                    */

       if (LTYPE(tempstr) != LSTRING_TY) {
         L2STR(&tempstr); /* Convert numeric to string      */
         }

       /* If the data is longer than the size of the caller */
       /* provided data area, truncate the data we return to*/
       /* fit in the area, and inform the caller by setting */
       /* Return Code 4. Return the actual length in the    */
       /* caller word pointed to by buffer_length.          */

       actual_length = LLEN(tempstr); /* Actual data length */

       /* Copy all of the data if it will fit.              */
       if (*buffer_length >= actual_length) {
         MEMCPY(buffer_value, LSTR(tempstr), actual_length);
         }

       else {
         /* Copy the part of the data that will fit.        */
         MEMCPY(buffer_value, LSTR(tempstr), *buffer_length);
         rc = 4; /* Tell caller value truncated */
         }

       /* Return the actual data length to the caller.       */
       *buffer_length = actual_length;
       }
     }

   LFREESTR(tempstr);                   /* Release the LSTR */
   LFREESTR(pool);                      /* Release the LSTR */
   LFREESTR(name);                      /* Release the LSTR */

   if (response == 'P') {
      rc = 3;             /* Problem with the pool variable */
      }

   if (response == 'F') {
      rc = 2;       /* Variable name malformed or not found */
      }
   return rc;
}
