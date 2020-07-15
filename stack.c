/* Modified for VM/370 CMS and GCC by Robert O'Hara, July 2010. */
/* NOTE: Not certain if these funtions are used. AS             */

#define __STACK_C__

#ifdef __CMS__
#include <cmssys.h>
#endif

#include "lerror.h"
#include "lstring.h"
#include "stack.h"

#include "rexx.h"

#ifndef __CMS__
/* ----------------- CreateStack ----------------------- */
void __CDECL
CreateStack( void )
{
 DQueue *stck;

 stck = (DQueue*) MALLOC(sizeof(DQueue),"Stack");
 DQINIT(*stck);
 DQPUSH(&rxStackList,stck);
} /* CreateStack */

/* ----------------- DeleteStack ----------------------- */
void __CDECL
DeleteStack( void )
{
 DQueue *stck;
 stck = DQPop(&rxStackList);
 DQFlush(stck,_Lfree);
 FREE(stck);
} /* DeleteStack */
#endif

/* ----------------- Queue2Stack ----------------------- */
void __CDECL
Queue2Stack( PLstr str )
{
#ifdef __CMS__
LSTR(* str)[LLEN(* str)] = '\0';                                       // rpo:add a null terminator
CMSstackLine(LSTR(*str), CMS_STACKFIFO);
#else
 DQueue *stck;
 stck = DQPEEK(&rxStackList);
 DQAdd2Head(stck,str);
#endif
} /* Queue2Stack */

/* ----------------- Push2Stack ----------------------- */
void __CDECL
Push2Stack( PLstr str )
{
#ifdef __CMS__
LSTR(* str)[LLEN(* str)] = '\0';                                       // rpo:add a null terminator
CMSstackLine(LSTR(* str), CMS_STACKLIFO);
#else
 DQueue *stck;
 stck = DQPEEK(&rxStackList);
 DQAdd2Tail(stck,str);
#endif
} /* Push2Stack */

/* ----------------- PullFromStack ----------------------- */
PLstr __CDECL
PullFromStack( )
{
#ifdef __CMS__
PLstr str;

Lfx(str, 131);
CMSconsoleRead(LSTR(*str));
return str;
#else
 DQueue *stck;
 stck = DQPEEK(&rxStackList);
 return (PLstr)DQPop(stck);
#endif
} /* PullFromStack */

/* ----------------- StackQueued ----------------------- */
long __CDECL
StackQueued( void )
{
#ifdef __CMS__
 int items;
 long retval;
 items = (long) CMSstackQuery();
 retval = items;
 return items;
#else
 DQueue *stck;
 stck = DQPEEK(&rxStackList);
 return stck->items;
#endif
} /* StackQueued*/
