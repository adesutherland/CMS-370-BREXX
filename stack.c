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
CreateStack(void) {
    DQueue *stck;
    Context *context = (Context *) CMSGetPG();

    stck = (DQueue *) MALLOC(sizeof(DQueue), "Stack");
    DQINIT(*stck);
    DQPUSH(&(context->rexxrxStackList), stck);
} /* CreateStack */

/* ----------------- DeleteStack ----------------------- */
void __CDECL
DeleteStack(void) {
    DQueue *stck;
    Context *context = (Context *) CMSGetPG();
    stck = DQPop(&(context->rexxrxStackList));
    DQFlush(stck, _Lfree);
    FREE(stck);
} /* DeleteStack */
#endif

/* ----------------- Queue2Stack ----------------------- */
void __CDECL
Queue2Stack(PLstr str) {
    L2STR(str);       /* Make sure its a string */
    LASCIIZ(*str); /* Make sure its a C string */
    CMSstackLine(LSTR(*str), CMS_STACKFIFO);
    LPFREE(str);
} /* Queue2Stack */

/* ----------------- Push2Stack ----------------------- */
void __CDECL
Push2Stack(PLstr str) {
    L2STR(str);       /* Make sure its a string */
    LASCIIZ(*str); /* Make sure its a C string */
    CMSstackLine(LSTR(*str), CMS_STACKLIFO);
    LPFREE(str);
} /* Push2Stack */

/* ----------------- PullFromStack ----------------------- */
PLstr __CDECL
PullFromStack() {
#ifdef __CMS__
    PLstr str;

    Lfx(str, 131);
    CMSconsoleRead(LSTR(*str));
    return str;
#else
    DQueue *stck;
    Context *context = (Context *) CMSGetPG();
    stck = DQPEEK(&(context->rexxrxStackList));
    return (PLstr) DQPop(stck);
#endif
} /* PullFromStack */

/* ----------------- StackQueued ----------------------- */
long __CDECL
StackQueued(void) {
#ifdef __CMS__
    int items;
    long retval;
    items = (long) CMSstackQuery();
    retval = items;
    return items;
#else
    DQueue *stck;
    Context *context = (Context *) CMSGetPG();
    stck = DQPEEK(&(context->rexxrxStackList));
    return stck->items;
#endif
} /* StackQueued*/
