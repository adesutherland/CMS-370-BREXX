/* Modified for VM/370 CMS and GCC by Robert O'Hara, July 2010. */
/*
 * $Id: rexx.c,v 1.12 2009/06/30 13:51:40 bnv Exp bnv $
 * $Log: rexx.c,v $
 * Revision 1.12  2009/06/30 13:51:40  bnv
 * Added -a option to break arg into words
 *
 * Revision 1.11  2008/07/15 14:57:55  bnv
 * mvs corrections
 *
 * Revision 1.10  2008/07/15 07:40:25  bnv
 * #include changed from <> to ""
 *
 * Revision 1.9  2008/07/14 13:08:42  bnv
 * MVS,CMS support
 *
 * Revision 1.8  2006/01/26 10:25:52  bnv
 * Windows CE
 *
 * Revision 1.7  2004/08/16 15:28:54  bnv
 * Changed: name of mnemonic operands from xxx_mn to O_XXX
 *
 * Revision 1.6  2003/10/30 13:16:28  bnv
 * Variable name change
 *
 * Revision 1.5  2002/06/11 12:37:38  bnv
 * Added: CDECL
 *
 * Revision 1.4  2002/06/06 08:24:36  bnv
 * Corrected: Registry support for WCE
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

#define __REXX_C__

#include <string.h>
#include <setjmp.h>

#include "lerror.h"
#include "lstring.h"

#include "rexx.h"
#include "stack.h"
#include "trace.h"
#include "bintree.h"
#include "compile.h"
#include "interpre.h"
#include "nextsymb.h"
#include "context.h"

#if defined(UNIX)
# include <dlfcn.h>
#elif defined(WCE)
# include <winfunc.h>
#endif

#ifdef __CMS__
#include <cmssys.h>               // rpo
#endif

/* ----------- Function prototypes ------------ */
void __CDECL Rerror(const int,const int,...);
void __CDECL RxRegFunctionDone(void);

/* ---------------- RxInitProc ---------------- */
static void
RxInitProc( void )
{
 _rx_proc = -1;
 _proc_size = PROC_INC;
 _proc = (RxProc*) MALLOC( _proc_size * sizeof(RxProc), "RxProc" );
 MEMSET(_proc,0,_proc_size*sizeof(RxProc));
} /* RxInitProc */

/* ----------------- RxInitialize ----------------- */
void __CDECL
RxInitialize( char *prorgram_name )
{
 Lstr str;

 _prgname = prorgram_name;

 LINITSTR(str);

#ifdef __CMS__
// We repeat the initialization of static variables in lstring.h to here because when BREXX is
// loaded as a resident program in CMS the static initializatino is not refreshed at each
// invocation.
lLastScannedNumber = 0.0;
 lNumericDigits = 9;
#endif

 /* do the basic initialisation */
 Linit(Rerror);  /* initialise with Rerror as error function */
 LINITSTR(symbolstr);
  Lfx(&symbolstr,250); /* create symbol string */
 LINITSTR(errmsg);
  Lfx(&errmsg,250); /* create error message string */

  LINITSTR(rxReturnResult);  /* Return Result */

 /* --- first locate configuration file --- */
 /* rexx.rc for DOS in the executable program directory */
 /* .rexxrc for unix in the HOME directory */

 _procidcnt = 1;  /* Program id counter */

#ifndef __CMS__
 DQINIT(rxStackList); /* initialise stacks */
 CreateStack();  /* create first stack */
#endif
 rxFileList = NULL; /* intialise rexx files */
 LPMALLOC(_code);
 CompileClause = NULL;

 RxInitProc();  /* initialize prg list */
 RxInitInterpret(); /* initialise interpreter*/
 RxInitVariables(); /* initialise hash table for variables */

 BINTREEINIT(_labels); /* initialise labels */
 BINTREEINIT(rxLitterals); /* initialise litterals */

  Lscpy(&str,"HALT"); haltStr   = _Add2Lits( &str, FALSE );

  Lscpy(&str,"1"); oneStr    = _Add2Lits( &str, FALSE );
  Lscpy(&str,"");  nullStr   = _Add2Lits( &str, FALSE );
  Lscpy(&str,"0"); zeroStr   = _Add2Lits( &str, FALSE );
  Lscpy(&str,"ERROR"); errorStr  = _Add2Lits( &str, FALSE );

  Lscpy(&str,"RESULT"); resultStr = _Add2Lits( &str, FALSE );
  Lscpy(&str,"NOVALUE"); noValueStr= _Add2Lits( &str, FALSE );
  Lscpy(&str,"NOTREADY"); notReadyStr= _Add2Lits( &str, FALSE );
  Lscpy(&str,"SIGL"); siglStr   = _Add2Lits( &str, FALSE );
  Lscpy(&str,"RC"); RCStr     = _Add2Lits( &str, FALSE );
  Lscpy(&str,"SYNTAX"); syntaxStr = _Add2Lits( &str, FALSE );
#ifdef __CMS__
  Lscpy(&str,"CMS"); systemStr = _Add2Lits( &str, FALSE );         // CMS is the default environment
#else
  Lscpy(&str,"SYSTEM"); systemStr = _Add2Lits( &str, FALSE );
#endif

 LFREESTR(str);
} /* RxInitialize */

/* ----------------- RxFinalize ----------------- */
void __CDECL
RxFinalize( void )
{
 LFREESTR(symbolstr); /* delete symbol string */
 LFREESTR(errmsg); /* delete error msg str */
 LFREESTR(rxReturnResult);  /* Return Result */
 RxDoneInterpret();
 FREE(_proc);  /* free prg list */
#ifndef __CMS__
 while (rxStackList.items>0) DeleteStack();
#endif
 LPFREE(_code); _code = NULL;

  /* will free also nullStr, zeroStr and oneStr */
 BinDisposeLeaf(&rxLitterals,rxLitterals.parent,FREE);
 BinDisposeLeaf(&_labels,_labels.parent,FREE);
 RxDoneVariables();
 RxRegFunctionDone(); /* initialise register functions */
} /* RxFinalize */

/* ----------------- RxFileAlloc ------------------- */
RxFile* __CDECL
RxFileAlloc(char *fname)
{
 RxFile *rxf;

 rxf = (RxFile*)MALLOC(sizeof(RxFile),"RxFile");
 if (rxf==NULL)
  return rxf;
 MEMSET(rxf,0,sizeof(RxFile));
 Lscpy(&(rxf->name), fname);
 LASCIIZ(rxf->name);

 return rxf;
} /* RxFileAlloc */

/* ----------------- RxFileType ------------------- */
void __CDECL
RxFileType(RxFile *rxf)
{
 char *c;

 /* find file type */
 c = LSTR(rxf->name)+LLEN(rxf->name);
 for (;c>LSTR(rxf->name) && *c!='.';c--) ;;
 if (*c=='.')
  rxf->filetype = c;
 for (;c>LSTR(rxf->name) && *c!=FILESEP;c--) ;;
 if (c>LSTR(rxf->name))
  c++;
 rxf->filename = c;
} /* RxFileType */

/* ----------------- RxFileFree ------------------- */
void __CDECL
RxFileFree(RxFile *rxf)
{
 RxFile *f;

 while (rxf) {
  f = rxf;
  rxf = rxf->next;
  LFREESTR(f->name);
  LFREESTR(f->file);
  FREE(f);
 }
} /* RxFileFree */

/* ----------------- RxFileLoad ------------------- */
int __CDECL
RxFileLoad( RxFile *rxf )
{
 FILEP f;
 char filename[21];

 /* Convert to ASCIIZ */
 L2STR(&(rxf->name)); LASCIIZ(rxf->name);

 /* File Name */
 sprintf(filename, "%.8s EXEC *", LSTR(rxf->name));

 if ((f=FOPEN(filename, "r")) == NULL) return FALSE;
 Lread(f, &(rxf->file), LREADFILE);
 FCLOSE(f);

 return TRUE;
} /* RxFileLoad */

/* --- _LoadRexxLibrary --- */
#define old_trap (currentContext->rexx_old_trap)
static int
_LoadRexxLibrary(RxFile *rxf, PLstr libname)
{
 size_t ip;

 if (!RxFileLoad( rxf )) return 1;

 ip = (size_t)((byte huge *)Rxcip - (byte huge *)Rxcodestart);
 MEMCPY(old_trap,_error_trap,sizeof(_error_trap));
 RxFileType(rxf);
 RxInitCompile(rxf,NULL);
 RxCompile();

 /* restore state */
 MEMCPY(_error_trap,old_trap,sizeof(_error_trap));
 Rxcodestart = (CIPTYPE*)LSTR(*_code);
 Rxcip = (CIPTYPE*)((byte huge *)Rxcodestart + ip);

 if (rxReturnCode) RxSignalCondition(SC_SYNTAX);
 return 0;
} /* _LoadRexxLibrary */

/* ----------------- RxLoadLibrary ------------------- */
int __CDECL
RxLoadLibrary( PLstr libname )
{
  RxFile  *rxf, *last;
  Lstr name;

  LINITSTR(name);
  Lfx(&name,LLEN(*libname));
  Lstrip(&name,libname,LBOTH,' ');
  Lupper(&name);

  /* Convert to C String */
  L2STR(&name); LASCIIZ(name);

  /* check to see if it is already loaded */
  for (rxf = rxFileList; rxf != NULL; rxf = rxf->next)
    if (!strcmp(rxf->filename,LSTR(name)))
      return -1;

  /* create a RxFile structure */
  rxf = RxFileAlloc(LSTR(name));

  /* try to load the file as rexx library */
  if (_LoadRexxLibrary(rxf,&name)) {
    RxFileFree(rxf);
    LFREESTR(name);
    return 1;
  }

  /* find the last in the queue */
  for (last = rxFileList; last->next != NULL; )
    last = last->next;
  last->next = rxf;

  LFREESTR(name);

  return 0;
} /* RxLoadLibrary */

/* ----------------- RxRun ------------------ */
int __CDECL
RxRun( char *filename, PLstr programstr,
 PLstr arguments, PLstr tracestr, char *environment )
{
 RxProc *pr;
 int i;

 /* --- set exit jmp position --- */
 if ((i=setjmp(_exit_trap))!=0)
  goto run_exit;
 /* --- set temporary error trap --- */
 if (setjmp(_error_trap)!=0)
  return rxReturnCode;

 /* ====== first load the file ====== */
 if (filename) {
  rxFileList = RxFileAlloc(filename);

  /* --- Load file --- */
  if (!RxFileLoad( rxFileList )) {
#ifndef WCE
   fprintf(STDERR,"Error %d running \"%s\": File not found\n",
     ERR_FILE_NOT_FOUND, LSTR(rxFileList->name));
#else
   PUTS("Error: File not found.");
#endif
   RxFileFree(rxFileList);
   return 1;
  }
 } else {
  rxFileList = RxFileAlloc("<STDIN>");
  Lfx(&(rxFileList->file), LLEN(*programstr));
  Lstrcpy(&(rxFileList->file), programstr);
 }
 RxFileType(rxFileList);
 LASCIIZ(rxFileList->file);

#ifdef __DEBUG__
 if (__debug__) {
  printf("File is:\n%s\n",LSTR(rxFileList->file));
  getchar();
 }
#endif

 /* ====== setup procedure ====== */
 _rx_proc++;  /* increase program items */
 pr = _proc+_rx_proc; /* pr = Proc pointer  */

 /* set program id counter */
 pr->id = _procidcnt++;

 /* --- initialise Proc structure --- */
    /* arguments...  */
 pr->arg.n = 0;
 for (i=0; i<MAXARGS; i++) {
  if (LLEN(arguments[i])) {
   pr->arg.n = i+1;
   pr->arg.a[i] = &(arguments[i]);
  } else
   pr->arg.a[i] = NULL;
 }
 pr->arg.r = NULL;

 /* call type...  */
 if (CMScalltype() == 5) {
   if (CMSisproc()) pr->calltype = CT_PROCEDURE;
   else pr->calltype = CT_FUNCTION;
 }
 else pr->calltype = CT_PROGRAM;

 pr->ip = 0;   /* procedure ip  */
 pr->stack = -1;  /* prg stck, will be set in interpret */
 pr->stacktop = -1;  /* no arguments  */

 pr->scope = RxScopeMalloc();
 LPMALLOC(pr->env);
 if (environment)
  Lscpy(pr->env,environment);
 else
  Lstrcpy(pr->env,&(systemStr->key));
#ifdef __CMS__
 pr->digits = 9;
#else
 pr->digits = LMAXNUMERICDIGITS;
#endif
 pr->fuzz = 0;
 pr->form = SCIENTIFIC;
 pr->condition = 0;
 pr->lbl_error    = &(errorStr->key);
 pr->lbl_halt     = &(haltStr->key);
 pr->lbl_novalue  = &(noValueStr->key);
 pr->lbl_notready = &(notReadyStr->key);
 pr->lbl_syntax   = &(syntaxStr->key);
 pr->codelen = 0;
 pr->trace = normal_trace;
 pr->interactive_trace = FALSE;
 if (tracestr && LLEN(*tracestr)) TraceSet(tracestr);

 /* ======= Compile file ====== */
 RxInitCompile(rxFileList,NULL);
 RxCompile();

#ifdef __DEBUG__
 if (__debug__) {
  printf("Litterals are:\n");
  BinPrint(rxLitterals.parent);
  getchar();

  printf("Labels(&functions) are:\n");
  BinPrint(_labels.parent);
  printf("Code Size: %d\n\n",LLEN(*_code));
  getchar();
 }
#endif

 /* ======= Execute code ======== */
 if (!rxReturnCode)
  RxInterpret();

run_exit:
  /* pr pointer might have changed if Proc was resized */
 pr = _proc+_rx_proc;
#ifdef __DEBUG__
 if (__debug__)
  printf("Return Code = %d\n",rxReturnCode);
#endif

 /* ======== free up memory ======== */
 RxFileFree(rxFileList);

 LPFREE(pr->env);
 if (CompileClause) {
  FREE(CompileClause);
  CompileClause = NULL;
 }

 RxScopeFree(pr->scope);
 FREE(pr->scope);
 _rx_proc--;

 return rxReturnCode;
} /* RxRun */
