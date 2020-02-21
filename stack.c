/* Modified for VM/370 CMS and GCC by Robert O'Hara, July 2010. */
/*
 * $Id: stack.c,v 1.9 2009/02/02 09:26:23 bnv Exp $
 * $Log: stack.c,v $
 * Revision 1.9  2009/02/02 09:26:23  bnv
 * Modifications for CMS,MVS
 *
 * Revision 1.8  2008/07/15 07:40:25  bnv
 * #include changed from <> to ""
 *
 * Revision 1.7  2008/07/14 13:08:42  bnv
 * MVS,CMS support
 *
 * Revision 1.6  2004/04/30 15:27:50  bnv
 * Removed bmem.h
 *
 * Revision 1.5  2003/10/30 13:16:28  bnv
 * Variable name change
 *
 * Revision 1.4  2002/06/11 12:37:38  bnv
 * Added: CDECL
 *
 * Revision 1.3  2001/06/25 18:51:48  bnv
 * Header -> Id
 *
 * Revision 1.2  1999/11/26 13:13:47  bnv
 * Changed: To use the new macros.
 *
 * Revision 1.1  1998/07/02 17:34:50  bnv
 * Initial revision
 *
 */

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
CMSconsoleRead(str);
printf("Stack: pulling [%s].\n", str);
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
