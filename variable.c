/* Modified for VM/370 CMS and GCC by Robert O'Hara, July 2010. */
/*
 * $Id: variable.c,v 1.12 2008/07/15 07:40:25 bnv Exp $
 * $Log: variable.c,v $
 * Revision 1.12  2008/07/15 07:40:25  bnv
 * #include changed from <> to ""
 *
 * Revision 1.11  2006/01/26 10:27:57  bnv
 * Added: RxVarExposeInd
 * Changed RxVar...Old() -> RxVar...Name()
 *
 * Revision 1.10  2004/04/30 15:26:09  bnv
 * Deleted: bmem.h
 *
 * Revision 1.9  2003/10/30 13:16:28  bnv
 * Variable name change
 *
 * Revision 1.8  2003/02/12 16:41:49  bnv
 * Added: Negative pool reference
 *
 * Revision 1.7  2002/08/22 12:31:28  bnv
 * Removed CR's
 *
 * Revision 1.6  2002/06/11 12:37:38  bnv
 * Added: CDECL
 *
 * Revision 1.5  2002/06/06 08:25:40  bnv
 * Corrected: A bug when PoolSet was called with variable length 0
 *
 * Revision 1.4  2001/06/25 18:51:48  bnv
 * Header -> Id
 *
 * Revision 1.3  2000/12/15 14:06:11  bnv
 * Corrected: The RxScanVarTree(), variable aux wasn't fixed before using it.
 *
 * Revision 1.2  1999/11/26 13:13:47  bnv
 * Changed: To use the new macros.
 *
 * Revision 1.1  1998/07/02 17:34:50  bnv
 * Initial revision
 *
 */

#define __VARIABLE_C__

#include "ldefs.h"
#include <string.h>
#include <stdlib.h>
#include <cmssys.h>

#include "lerror.h"
#include "lstring.h"

#include "rexx.h"
#include "trace.h"
#include "bintree.h"
#include "interpre.h"
#include "variable.h"
#include "context.h"

typedef
struct tpoolfunc {
 int (*get)(PLstr,PLstr);
 int (*set)(PLstr,PLstr);
} TPoolFunc;

/* --- local function prototypes --- */
static int SystemPoolGet(PLstr name, PLstr value);
static int SystemPoolSet(PLstr name,PLstr value);

/* -------------- RxInitVariables ---------------- */
void __CDECL
RxInitVariables(void)
{
 int i;
 Context *context = (Context*)CMSGetPG();

 /* initialise hash table */
 for (i=0; i<255; i++)
  (context->variable_hashchar)[i] = i % VARTREES;

 LINITSTR((context->variable_int_varname));
 LINITSTR((context->variable_varidx)); Lfx(&(context->variable_varidx),250);
 LINITSTR((context->variable_stemvaluenotfound));
 BINTREEINIT((context->variable_PoolTree));

 RxRegPool("SYSTEM",SystemPoolGet,SystemPoolSet);
} /* RxInitVariables */

/* -------------- RxDoneVariables ---------------- */
void __CDECL
RxDoneVariables(void)
{
 Context *context = (Context*)CMSGetPG();
 LFREESTR((context->variable_int_varname));
 LFREESTR((context->variable_varidx));
 LFREESTR((context->variable_stemvaluenotfound));
 BinDisposeLeaf(&(context->variable_PoolTree),(context->variable_PoolTree).parent,FREE);
} /* RxDoneVariables */

/* ---------------- RxVarFree -------------------- */
void __CDECL
RxVarFree(void *var)
{
 Variable *v;
 Context *context = (Context*)CMSGetPG();

 v = (Variable*)var;
 if (v->exposed==NO_PROC) {
  if (v->stem) {
   RxScopeFree(v->stem);
   FREE(v->stem);
  }
  LFREESTR(v->value);
  FREE(var);
 } else
 if (v->exposed == (context->rexx_rx_proc)-1)
  v->exposed = NO_PROC;
} /* RxVarFree */

/* ---------------- RxVarAdd ------------------ */
/* adds a variable only if it is does not exist */
/* if the variable is a stem then a call to     */
/* VarFind must be done first to initialise     */
/* several variables                            */
/* -------------------------------------------- */
PBinLeaf __CDECL
RxVarAdd(Scope scope, PLstr name, int hasdot, PBinLeaf stemleaf )
{
 Variable *var,*var2;
 BinTree *tree;
 Lstr aux;
 Context *context = (Context*)CMSGetPG();

 if (hasdot==0) {
  LINITSTR(aux);  /* create memory */
  Lstrcpy(&aux,name);
  LASCIIZ(aux);
  tree = scope + (context->variable_hashchar)[ (byte)LSTR(aux)[0] ];
  var = (Variable *)MALLOC(sizeof(Variable),"Var");
  LINITSTR(var->value);
  Lfx(&(var->value),1);
  var->exposed = NO_PROC;
  var->stem    = NULL;
  return BinAdd(tree,&aux,var);
 } else {
  /* and the "(context->variable_varidx)" must have the index from */
  /* from VarFind     */
  /* also "stemleaf" must be the stem of the leaf */

  if (stemleaf==NULL) { /* we must create the stem */
   LINITSTR(aux);
   Lstrcpy(&aux,(context->variable_varname));
   LASCIIZ(aux);
   tree = scope + (context->variable_hashchar)[ (byte)LSTR(*(context->variable_varname))[0] ];
   var = (Variable *)MALLOC(sizeof(Variable),"StemVar");
   LINITSTR(var->value);
   var->exposed = NO_PROC;
   var->stem    = RxScopeMalloc();
   BinAdd(tree,&aux,var);
  } else
   var = (Variable*)(stemleaf->value);

  LINITSTR(aux); Lfx(&aux,1);
  Lstrcpy(&aux,&(context->variable_varidx)); /* make a copy of idx */
  LASCIIZ(aux);

  if (var->stem==NULL)
   var->stem = RxScopeMalloc();

  { /* do a small hashing */
   register char *s;
   char *se;
   int sum=0;

   s = LSTR((context->variable_varidx));
   /* use only first 10 chars */
   if (LLEN((context->variable_varidx))<10)
    se = s + LLEN((context->variable_varidx));
   else
    se = s + 10;
   while (s<se) sum += *(s++);
   sum %= VARTREES;
   tree = var->stem + sum;
  }
/*//  tree = var->stem + (context->variable_hashchar)[ (byte)LSTR((context->variable_varidx))[0] ];*/
  var2 = (Variable *)MALLOC(sizeof(Variable),"IdxVar");
  LINITSTR(var2->value);
  if (LLEN(var->value))
   Lstrcpy(&(var2->value),&(var->value));
  else
   Lfx(&(var2->value),1);
  var2->exposed = NO_PROC;
  var2->stem    = NULL;
  return BinAdd(tree,&aux,var2);
 }
} /* RxVarAdd */

/* ------------------ RxVarFind ----------------------- */
/* Search for a variable in the variables scope  */
/* On input:      */
/* scope : scope to use    */
/*  (for variables indexes it uses the */
/*  current scope (context->rexx_proc)[(context->rexx_rx_proc)].scope */
/* litleaf : variables litleaf   */
/* found : if variable is found   */
/* Returns:      */
/*  : returns a BinLeaf of variable  */
/*  when a) variable is found (found=TRUE) */
/*  b) variable is an array an the  */
/*  actuall variable is not found  */
/*  but the tree head exist (found=FALSE) */
/* ---------------------------------------------------- */
PBinLeaf __CDECL
RxVarFind(const Scope scope, const PBinLeaf litleaf, bool *found)
{
 IdentInfo *inf,*infidx;
 Scope stemscope,curscope;
 BinTree *tree;
 PBinLeaf leaf,leafidx;
 PLstr name,aux;
 int i,cmp;
 size_t l;
 Context *context = (Context*)CMSGetPG();

 name = &(litleaf->key);
 inf = (IdentInfo*)(litleaf->value);

 tree = scope + (context->variable_hashchar)[ (byte)LSTR(*name)[0] ];
 if (!inf->stem) {  /* simple variable */
  /* inline version of BinFind */
  /* leaf = BinFind(tree,name); */
  leaf = tree->parent;
  while (leaf != NULL) {
   cmp = STRCMP(LSTR(*name), LSTR(leaf->key));
   if (cmp < 0)
    leaf = leaf->left;
   else
   if (cmp > 0)
    leaf = leaf->right;
   else
    break;
  }
  *found = (leaf != NULL);
  if (*found) {
   inf->id = (context->interpreRx_id);
   inf->leaf[0] = leaf;
  }
  return leaf;
 } else {

 /* ======= first find array ======= */

  leafidx = inf->leaf[0];
  (context->variable_varname) = &(leafidx->key);
  infidx = (IdentInfo*)(leafidx->value);
  if ((context->interpreRx_id)!=NO_CACHE && infidx->id==(context->interpreRx_id))
   leaf = infidx->leaf[0];
  else {
/**
////   LASCIIZ((context->variable_varname));
**/
   leaf = tree->parent;
   while (leaf != NULL) {
    cmp = STRCMP(LSTR(*(context->variable_varname)), LSTR(leaf->key));
    if (cmp < 0)
     leaf = leaf->left;
    else
    if (cmp > 0)
     leaf = leaf->right;
    else
     break;
   }
   if (leaf) {
    infidx->id = (context->interpreRx_id);
    infidx->leaf[0] = leaf;
   } else
    infidx->id = NO_CACHE;
  }

  /* construct index */
  LZEROSTR((context->variable_varidx));
  curscope = (context->rexx_proc)[(context->rexx_rx_proc)].scope;
  for (i=1; i<inf->stem; i++) {
   if (i!=1) {
    /* append a dot '.' */
    LSTR((context->variable_varidx))[LLEN((context->variable_varidx))] = '.';
    LLEN((context->variable_varidx))++;
   }
   leafidx = inf->leaf[i];
   if (leafidx==NULL) continue;

   infidx = (IdentInfo*)(leafidx->value);
   aux = &(leafidx->key);
   if (infidx==NULL) {
    L2STR(aux);
    l = LLEN((context->variable_varidx))+LLEN(*aux);
    if (LMAXLEN((context->variable_varidx)) <= l) Lfx(&(context->variable_varidx), l);
    MEMCPY(LSTR((context->variable_varidx))+LLEN((context->variable_varidx)),LSTR(*aux),LLEN(*aux));
    LLEN((context->variable_varidx)) = l;
   } else
   if ((context->interpreRx_id)!=NO_CACHE && infidx->id==(context->interpreRx_id)) {
    register PLstr lptr;
    leafidx = infidx->leaf[0];
    lptr = LEAFVAL(leafidx);
    L2STR(lptr);
    l = LLEN((context->variable_varidx))+LLEN(*lptr);
    if (LMAXLEN((context->variable_varidx)) <= l) Lfx(&(context->variable_varidx), l);
    MEMCPY(LSTR((context->variable_varidx))+LLEN((context->variable_varidx)),LSTR(*lptr),LLEN(*lptr));
    LLEN((context->variable_varidx)) = l;
   } else {
    /* search for aux-variable */
    tree = curscope+(context->variable_hashchar)[ (byte)LSTR(*aux)[0] ];

    /* inline version of BinFind */
    /* leafidx = BinFind(tree,&aux); */
    leafidx = tree->parent;
    while (leafidx != NULL) {
     cmp = STRCMP(LSTR(*aux), LSTR(leafidx->key));
     if (cmp < 0)
      leafidx = leafidx->left;
     else
     if (cmp > 0)
      leafidx = leafidx->right;
     else
      break;
    }

    if (leafidx) {
     register PLstr lptr;
     infidx->id = (context->interpreRx_id);
     infidx->leaf[0] = leafidx;
     lptr = LEAFVAL(leafidx);
     L2STR(lptr);
     l = LLEN((context->variable_varidx))+LLEN(*lptr);
     if (LMAXLEN((context->variable_varidx)) <= l) Lfx(&(context->variable_varidx), l);
     MEMCPY(LSTR((context->variable_varidx))+LLEN((context->variable_varidx)),LSTR(*lptr),LLEN(*lptr));
    LLEN((context->variable_varidx)) = l;
    } else {
     Lupper(aux);
     Lstrcat(&(context->variable_varidx),aux);
    }
   }
  }

  L2STR(&(context->variable_varidx));
  if ((context->interpre__trace))
   if ((context->rexx_proc)[(context->rexx_rx_proc)].trace == intermediates_trace) {
    int i;
    FPUTS("       >C>  ",STDERR);
    for (i=0;i<(context->rexx_nesting); i++) FPUTC(' ',STDERR);
    FPUTC('\"',STDERR);
    Lprint(STDERR,(context->variable_varname));
    Lprint(STDERR,&(context->variable_varidx));
    FPUTS("\"\n",STDERR);
   }

  if (leaf==NULL) {
   *found = FALSE;
   Lstrcpy(&(context->variable_stemvaluenotfound),(context->variable_varname));
   Lstrcat(&(context->variable_stemvaluenotfound),&(context->variable_varidx));
   return NULL;
  }
  stemscope = ((Variable*)(leaf->value))->stem;

  if (stemscope==NULL) {
   if (!LISNULL(*LEAFVAL(leaf)))
    Lstrcpy(&(context->variable_stemvaluenotfound), LEAFVAL(leaf));
   else {
    Lstrcpy(&(context->variable_stemvaluenotfound),(context->variable_varname));
    Lstrcat(&(context->variable_stemvaluenotfound),&(context->variable_varidx));
   }
   *found = FALSE;
   return leaf;
  }

  { /* do a small hashing */
   register char *s;
   char *se;
   int sum=0;

   s = LSTR((context->variable_varidx));
   /* use only first 10 chars */
   if (LLEN((context->variable_varidx))<10)
    se = s + LLEN((context->variable_varidx));
   else
    se = s + 10;
   while (s<se) sum += *(s++);
   sum %= VARTREES;
   tree = stemscope + sum;
  }
/*//  tree = stemscope + (context->variable_hashchar)[ (byte)LSTR((context->variable_varidx))[0] ];*/
  /* inline version of BinFind */
  /* leafidx = BinFind(tree,&(context->variable_varidx)); */
  leafidx = tree->parent;
  while (leafidx != NULL) {
   /* Inline version of Compare */
   {
    register char *a,*b;
    char *ae,*be;
    a = LSTR((context->variable_varidx));
    ae = a + LLEN((context->variable_varidx));
    b = LSTR(leafidx->key);
    be = b + LLEN(leafidx->key);
    for(;(a<ae) && (b<be) && (*a==*b); a++,b++) ;
    if (a==ae && b==be)
     cmp = 0;
    else
    if (a<ae && b<be)
     cmp = (*a<*b) ? -1 : 1 ;
    else
     cmp = (a<ae) ? 1 : -1 ;
   }

   if (cmp < 0)
    leafidx = leafidx->left;
   else
   if (cmp > 0)
    leafidx = leafidx->right;
   else
    break;
  }

  if (leafidx==NULL) { /* not found */
   *found = FALSE;
   if (!LISNULL(*LEAFVAL(leaf)))
    Lstrcpy(&(context->variable_stemvaluenotfound), LEAFVAL(leaf));
   else {
    Lstrcpy(&(context->variable_stemvaluenotfound),(context->variable_varname));
    Lstrcat(&(context->variable_stemvaluenotfound),&(context->variable_varidx));
   }
   return leaf; /* return stem leaf */
  } else {
   *found = TRUE;
   return leafidx;
  }
 }
} /* RxVarFind */

/* ------------------ RxVarFindName ------------------ */
PBinLeaf __CDECL
RxVarFindName(Scope scope, PLstr name, bool *found)
{
 Scope stemscope;
 BinTree *tree;
 PBinLeaf leaf,leafidx;
 Lstr aux;
 int hasdot,start,stop;
 register char *ch;
 Context *context = (Context*)CMSGetPG();

 /* search for a '.' except in the last character */
 ch = MEMCHR(LSTR(*name),'.',LLEN(*name)-1);
 if (ch)
  hasdot = (size_t)((char huge *)ch - (char huge *)LSTR(*name) + 1);
 else
  hasdot = 0;

 tree = scope + (context->variable_hashchar)[ (byte)LSTR(*name)[0] ];
 if (!hasdot) {  /* simple variable */
  leaf = BinFind(tree,name);
  *found = (leaf != NULL);
  return leaf;
 } else {
  LINITSTR(aux);
  (context->variable_varname) = &(context->variable_int_varname);
  _Lsubstr((context->variable_varname),name,1,hasdot);
  leaf = BinFind(tree,(context->variable_varname));

  LZEROSTR((context->variable_varidx));
  LASCIIZ(*name);
  stop = hasdot+1; /* after dot */
  ch++;
  while (1) {
   while (*ch=='.') {
    /* concatanate a dot '.' */
    L2STR(&(context->variable_varidx));
    LSTR((context->variable_varidx))[LLEN((context->variable_varidx))] = '.';
    LLEN((context->variable_varidx))++;
    ch++; stop++;
   }
   if (!*ch) break;

   start = stop; /* find next dot */
   while (*ch && *ch!='.') {
    ch++; stop++;
   }
   _Lsubstr(&aux,name,start,stop-start);
   /* search for variable */
   tree = scope + (context->variable_hashchar)[ (byte)LSTR(aux)[0] ];
   leafidx = BinFind(tree,&aux);

   if (leafidx)
    Lstrcat(&(context->variable_varidx),LEAFVAL(leafidx));
   else {
    Lupper(&aux);
    Lstrcat(&(context->variable_varidx),&aux);
   }
  }

  /* free strings */
  LFREESTR(aux);

  if (leaf==NULL) {
   *found = FALSE;
   Lstrcpy(&(context->variable_stemvaluenotfound),(context->variable_varname));
   Lstrcat(&(context->variable_stemvaluenotfound),&(context->variable_varidx));
   return NULL;
  }
  stemscope = ((Variable*)(leaf->value))->stem;

  if (stemscope==NULL) {
   if (!LISNULL(*LEAFVAL(leaf)))
    Lstrcpy(&(context->variable_stemvaluenotfound),LEAFVAL(leaf));
   else {
    Lstrcpy(&(context->variable_stemvaluenotfound),(context->variable_varname));
    Lstrcat(&(context->variable_stemvaluenotfound),&(context->variable_varidx));
   }
   *found = FALSE;
   return leaf;
  }

  { /* do a small hashing */
   register char *s;
   char *se;
   int sum=0;

   s = LSTR((context->variable_varidx));
   /* use only first 10 chars */
   if (LLEN((context->variable_varidx))<10)
    se = s + LLEN((context->variable_varidx));
   else
    se = s + 10;
   while (s<se) sum += *(s++);
   sum %= VARTREES;
   tree = stemscope + sum;
  }
/*//  tree = stemscope + (context->variable_hashchar)[ (byte)LSTR((context->variable_varidx))[0] ]; */
  leafidx = BinFind(tree,&(context->variable_varidx));

  if (leafidx==NULL) { /* not found */
   *found = FALSE;
   if (!LISNULL(*LEAFVAL(leaf)))
    Lstrcpy(&(context->variable_stemvaluenotfound),LEAFVAL(leaf));
   else {
    Lstrcpy(&(context->variable_stemvaluenotfound),(context->variable_varname));
    Lstrcat(&(context->variable_stemvaluenotfound),&(context->variable_varidx));
   }
   return leaf; /* return stem leaf */
  } else {
   *found = TRUE;
   return leafidx;
  }
 }
} /* RxVarFindName */

/* --------------- RxVarDel ------------------- */
void __CDECL
RxVarDel(Scope scope, PBinLeaf litleaf, PBinLeaf varleaf)
{
 IdentInfo *inf;
 Variable *var;
 PLstr name;
 BinTree *tree;
 Context *context = (Context*)CMSGetPG();

 name = &(litleaf->key);
 inf = (IdentInfo*)(litleaf->value);
 var = (Variable*)(varleaf->value);

 if (!inf->stem) { /* ==== simple variable ==== */
  if (var->exposed>=0) { /* --- free only data --- */
   if (!LISNULL(var->value)) {
    LFREESTR(var->value);
    LINITSTR(var->value);
    Lstrcpy(&(var->value),name);
   }
   if (var->stem)
    RxScopeFree(var->stem);
  } else {
   tree = scope + (context->variable_hashchar)[ (byte)LSTR(*name)[0] ];
   BinDel(tree,name,RxVarFree);
   inf->id = NO_CACHE;
  }
 } else {
/**
//  if (var->exposed>=0) {
**/
   LFREESTR(var->value);
   LINITSTR(var->value);
   Lstrcpy(&(var->value),(context->variable_varname));
   Lstrcat(&(var->value),&(context->variable_varidx));
/**
//  } else {
//    * find leaf of stem *
//   tree = scope + (context->variable_hashchar)[ (byte)LSTR(*name)[0] ];
//   stemleaf = BinFind(tree,(context->variable_varname));
//   var = (Variable*)(stemleaf->value);
//    * find the actual bintree of variable *
//   tree = var->stem + (context->variable_hashchar)[ (byte)LSTR((context->variable_varidx))[0] ];
//   BinDel(tree,&(context->variable_varidx),RxVarFree);
//   inf->id = NO_CACHE;
//  }
**/
 }
} /* RxVarDel */

/* --------------- RxVarDelName ------------------- */
void __CDECL
RxVarDelName(Scope scope, PLstr name, PBinLeaf varleaf)
{
 IdentInfo *inf;
 Variable *var;
 BinTree *tree;
 PBinLeaf leaf;
 char *ch;
 Context *context = (Context*)CMSGetPG();

 var = (Variable*)(varleaf->value);

 /* search for a dot '.' except in the last character */
 ch = MEMCHR(LSTR(*name),'.',LLEN(*name)-1);

 if (var->exposed>=0) {  /* --- free only data --- */
  LFREESTR(var->value);
  LINITSTR(var->value);
  Lstrcpy(&(var->value),name);
  if (var->stem)
   RxScopeFree(var->stem);
 } else
 if (ch) {
  LFREESTR(var->value);
  LINITSTR(var->value);
  Lstrcpy(&(var->value),(context->variable_varname));
  Lstrcat(&(var->value),&(context->variable_varidx));
 } else {
  tree = scope + (context->variable_hashchar)[ (byte)LSTR(*name)[0] ];
  BinDel(tree,name,RxVarFree);

  /* Search in the litterals tree to see if it exist */
  leaf = BinFind(&(context->rexxrxLitterals),name);
  if (leaf) {
   inf = (IdentInfo*)(leaf->value);
   inf->id = NO_CACHE;
  }
 }
} /* RxVarDelName */

/* ------------- RxVarDelInd ----------------- */
void __CDECL
RxVarDelInd(Scope scope, PLstr vars)
{
 Lstr name;
 long w,i;
 int found;
 PBinLeaf leaf;

 LINITSTR(name);
 w = Lwords(vars);
 for (i=1; i<=w; i++) {
  Lword(&name,vars,i);
  Lupper(&name);
  leaf = RxVarFindName(scope,&name,&found);
  if (found)
   RxVarDelName(scope,&name,leaf);
 }
 LFREESTR(name);
} /* RxVarDelInd */

/* -------------- RxVarExpose ----------------- */
PBinLeaf __CDECL
RxVarExpose(Scope scope, PBinLeaf litleaf)
{
 IdentInfo *inf;
 Variable *var, *stemvar;
 Scope oldscope;
 BinTree *tree;
 PBinLeaf leaf, stemleaf;
 Lstr aux;
 int oldcurid;
 int found;
 Context *context = (Context*)CMSGetPG();

 inf = (IdentInfo*)(litleaf->value);

 /* --- first test to see if it is allready exposed --- */
 leaf = RxVarFind(scope,litleaf,&found);
 if (found)
  return leaf;
 else {
  if (leaf) { /* then it is an array and the head is only found */
   var = (Variable*)(leaf->value);
   if (var->exposed != NO_PROC)
    return leaf; /* don't worry head is allready exposed */
  }
 }

 /* --- else search in the old scope for variable --- */
 oldscope = (context->rexx_proc)[ (context->rexx_rx_proc)-1 ].scope;

 /* --- change curid since we are dealing with old scope --- */
 oldcurid = (context->interpreRx_id);
 (context->interpreRx_id) = NO_CACHE;  /* something that doesn't exist */
 leaf = RxVarFind(oldscope,litleaf,&found);
 (context->interpreRx_id) = oldcurid;

 if (!found) {
  /* added it to the tree */
  leaf = RxVarAdd(oldscope,
   &(litleaf->key),
   inf->stem,
   leaf);
  if (inf->stem) {
   Lstrcpy(LEAFVAL(leaf),(context->variable_varname));
   Lstrcat(LEAFVAL(leaf),&(context->variable_varidx));
  } else
   Lstrcpy(LEAFVAL(leaf),&(litleaf->key));
 }
 var = (Variable*)(leaf->value);
 if (var->exposed == NO_PROC)
  var->exposed = (context->rexx_rx_proc)-1; /* to who it belongs */

 /* === now create a node in the new variable scope === */

 if (inf->stem==0) {
  LINITSTR(aux);  /* create memory */
  Lstrcpy(&aux,&(litleaf->key));
  LASCIIZ(aux);
  tree = scope + (context->variable_hashchar)[ (byte)LSTR(aux)[0] ];
  return BinAdd(tree,&aux,var);
 } else {
  tree = scope + (context->variable_hashchar)[ (byte)LSTR(*(context->variable_varname))[0] ];
  stemleaf = BinFind(tree,(context->variable_varname));
  if (!stemleaf) {
   LINITSTR(aux);
   Lstrcpy(&aux,(context->variable_varname));
   LASCIIZ(aux);
   stemvar = (Variable *)MALLOC(sizeof(Variable),"ExposedStemVar");
   LINITSTR(stemvar->value);
   stemvar->exposed = NO_PROC;
   stemvar->stem    = RxScopeMalloc();
   BinAdd(tree,&aux,stemvar);
  } else
   stemvar = (Variable*)(stemleaf->value);

  LINITSTR(aux);
  Lstrcpy(&aux,&(context->variable_varidx)); /* make a copy of idx */
  LASCIIZ(aux);

  if (stemvar->stem==NULL)
   stemvar->stem = RxScopeMalloc();

  tree = stemvar->stem + (context->variable_hashchar)[ (byte)LSTR((context->variable_varidx))[0] ];
  return BinAdd(tree,&aux,var);
 }
} /* RxVarExpose */

/* ------------- RxVarExposeInd ------------- */
void __CDECL
RxVarExposeInd(Scope scope, PLstr vars)
{
 Lstr name;
 long w,i;
 PBinLeaf litleaf;
 Context *context = (Context*)CMSGetPG();

 LINITSTR(name);
 w = Lwords(vars);
 for (i=1; i<=w; i++) {
  Lword(&name,vars,i);
  Lupper(&name);
  litleaf = BinFind(&(context->rexxrxLitterals),&name);
  if (litleaf)
   RxVarExpose(scope,litleaf);
 }
 LFREESTR(name);
} /* RxVarExposeInd */

/* -------------- RxVarSet ------------------ */
void __CDECL
RxVarSet( Scope scope, PBinLeaf varleaf, PLstr value )
{
 IdentInfo *inf;
 PBinLeaf leaf;
 int found;
 Context *context = (Context*)CMSGetPG();

 inf = (IdentInfo*)(varleaf->value);
 if (inf->id == (context->interpreRx_id)) {
  leaf = inf->leaf[0];
  Lstrcpy(LEAFVAL(leaf), value);
 } else {
  leaf = RxVarFind(scope,varleaf,&found);
  if (found)
   Lstrcpy(LEAFVAL(leaf), value);
  else {
   /* added it to the tree */
   leaf = RxVarAdd(scope,
    &(varleaf->key),
    inf->stem,
    leaf);
   Lstrcpy(LEAFVAL(leaf),value);

   if (inf->stem==0) {
    inf->id = (context->interpreRx_id);
    inf->leaf[0] = leaf;
   }
  }
 }
} /* RxVarSet */

/* ----------------- RxSetSpecialVar ------------------- */
void __CDECL
RxSetSpecialVar( int rcsigl, long num )
{
 PBinLeaf varleaf;
 Lstr  value;
 Context *context = (Context*)CMSGetPG();

 switch (rcsigl) {
  case RCVAR:
   varleaf = (context->rexxRCStr);
   break;
  case SIGLVAR:
   varleaf = (context->rexxsiglStr);
   break;
 }

 LINITSTR(value);
 Lfx(&value,0);
 Licpy(&value,num);
 RxVarSet((context->rexx_proc)[(context->rexx_rx_proc)].scope,varleaf,&value);
 LFREESTR(value);
} /* RxSetSpecialVar */

/* --------------- RxScopeMalloc ---------------- */
Scope __CDECL
RxScopeMalloc( void )
{
 int i;
 static Scope scope;

 scope = (Scope)MALLOC(VARTREES*sizeof(BinTree),"Scope");
 for (i=0; i<VARTREES; i++)
  BINTREEINIT(scope[i]);
 return scope;
} /* RxScopeMalloc */

/* ---------------- RxScopeFree ----------------- */
void __CDECL
RxScopeFree(Scope scope)
{
 int i;
 if (scope)
  for (i=0; i<VARTREES; i++)
   BinDisposeLeaf(&(scope[i]),scope[i].parent,RxVarFree);
} /* RxScopeFree */

/* ================ VarTreeAssign ================ */
static void
VarTreeAssign(PBinLeaf leaf, PLstr str, size_t mlen)
{
 Variable *v;

 if (!leaf) return;
 if (leaf->left)
  VarTreeAssign(leaf->left,str,mlen);
 if (leaf->right)
  VarTreeAssign(leaf->right,str,mlen);

 v = (Variable*)(leaf->value);
 if (LMAXLEN(v->value) > mlen) {
  LFREESTR(v->value);
  LINITSTR(v->value);
  Lfx(&(v->value),LLEN(*str));
 }
 Lstrcpy(&(v->value),str);
} /* VarTreeAssign */

/* ---------------- RxScopeAssign ---------------- */
void __CDECL
RxScopeAssign(PBinLeaf varleaf)
{
 int i;
 size_t mlen;
 PLstr str;
 Variable *v;
 Scope scope;

 v = (Variable*)(varleaf->value);
 scope = v->stem;
 if (!scope) return;
 str = &(v->value);

 /* determine minimum MAXLEN */
 mlen = LNORMALISE(LLEN(*str)) + LEXTRA;
 for (i=0; i<VARTREES; i++)
  VarTreeAssign(scope[i].parent,str,mlen);
} /* RxScopeAssign */

/* ---------------- RxVar2Str ------------------- */
void __CDECL
RxVar2Str( PLstr result, PBinLeaf leaf, int option )
{
 Lstr aux;
 Variable *var;

 Lstrcat(result,&(leaf->key));
 Lcat(result,"=\"");

 var = (Variable*)(leaf->value);
 if (option==RVT_HEX) {
  LINITSTR(aux);
  if (!LISNULL(var->value)) {
   Lc2x(&aux,&(var->value));
   Lstrcat(result,&aux);
  }
  Lcat(result,"\"x");
  LFREESTR(aux);
 } else {
  if (!LISNULL(var->value))
   Lstrcat(result,&(var->value));
  Lcat(result,"\"");
 }
} /* RxVar2Str */

/* ---------------- RxScanVarTree --------------- */
static void
RxScanVarTree( PLstr result, PBinLeaf leaf, PLstr head, int depth, int option )
{
 Variable *var;
 Lstr  aux;

 if (leaf==NULL) return;
 RxScanVarTree(result,leaf->left,head,depth+1,option);
 RxScanVarTree(result,leaf->right,head,depth+1,option);

 if (option==RVT_DEPTH) {
  LINITSTR(aux); Lfx(&aux,1);
  Licpy(&aux,depth);
  Lstrcat(result,&aux);
  Lcat(result," ");
  LFREESTR(aux);
 }
 if (head)
  Lstrcat(result,head);

 RxVar2Str(result, leaf, option);

 Lcat(result,"\n");

 var = (Variable*)(leaf->value);
 if (var->stem)
  RxReadVarTree(result,var->stem,&(leaf->key),option);
} /* RxScanVarTree */

/* ---------------- RxReadVarTree --------------- */
void __CDECL
RxReadVarTree(PLstr result, Scope scope, PLstr head, int option)
{
 int i;
 for (i=0; i<VARTREES; i++)
  RxScanVarTree(result,scope[i].parent,head,0,option);
} /* RxReadVarTree */

/* ================ POOL functions =================== */
/* ----- SystemPoolGet ----- */
static int
SystemPoolGet(PLstr name, PLstr value)
{
#ifndef WCE
 char *env;

 L2STR(name); LASCIIZ(*name);
 env = getenv(LSTR(*name));
 if (env) {
  Lscpy(value,env);
  return 0;
 } else {
  LZEROSTR(*value);
  return 1;
 }
#else
        return 0;
#endif
} /* SystemPoolGet */

/* ----- SystemPoolSet ----- */
static int
SystemPoolSet(PLstr name, PLstr value)
{
#ifndef WCE
 L2STR(name); LASCIIZ(*name);
 L2STR(value); LASCIIZ(*value);
#ifdef HAVE_SETENV
 return setenv(LSTR(*name),LSTR(*value),TRUE);
#else
 {
  Lstr str;
  int rc;

  LINITSTR(str);
  Lstrcpy(&str,name);
  Lcat(&str,"=");
  Lstrcpy(&str,value);
  LASCIIZ(str);
#ifndef __CMS__
  rc = putenv(LSTR(str));
#endif
  LFREESTR(str);
  return rc;
 }
#endif
#else
        return 0;
#endif
} /* SystemPoolSet */

/* -------------- PoolGet -------------- */
int __CDECL
RxPoolGet( PLstr pool, PLstr name, PLstr value )
{
 PBinLeaf leaf;
 int poolnum;
 int found;
 Context *context = (Context*)CMSGetPG();

 /* check to see if it is internal pool */
 if ( LTYPE(*pool)==LINTEGER_TY ||
  (LTYPE(*pool)==LSTRING_TY && _Lisnum(pool)==LINTEGER_TY)) {

  Lstr str;
  poolnum = (int)Lrdint(pool);
  if (poolnum<0)
   poolnum = (context->rexx_rx_proc)+poolnum;
  if (poolnum<0 || poolnum>(context->rexx_rx_proc))
   return 'P';

  /* search if it is a valid name */
  if (Ldatatype(name,'S')==0) {
   Lstrcpy(value,name);
   return 'F';
  }
  /* search in the appropriate scope */
  LINITSTR(str); /* translate to upper case */
  Lstrcpy(&str,name); Lupper(&str);
  leaf = RxVarFindName((context->rexx_proc)[poolnum].scope,&str,&found);
  if (found) {
   Lstrcpy(value,LEAFVAL(leaf));
   LFREESTR(str);
   return 0;
  } else {
   /* search for dot except last character */
   if (MEMCHR(LSTR(str),'.',LLEN(str)-1)!=NULL)
    Lstrcpy(value,&(context->variable_stemvaluenotfound));
   else
    Lstrcpy(value,&str);
   LFREESTR(str);
   return 'F';
  }
 }

 /* nope search the Pool tree */
 leaf = BinFind(&(context->variable_PoolTree),pool);
 if (!leaf) return 'P';

 if (((TPoolFunc*)(leaf->value))->get)
  found = (((TPoolFunc*)(leaf->value))->get)(name,value);
 if (found) return 'F';
 return 0;
} /* PoolGet */

/* -------------- PoolSet -------------- */
int __CDECL
RxPoolSet( PLstr pool, PLstr name, PLstr value )
{
 PBinLeaf leaf;
 int poolnum;
 int found;
 Context *context = (Context*)CMSGetPG();

 /* check to see if it is internal pool */
 if (LLEN(*name)==0)
  (context->lstring_Lerror)(ERR_INCORRECT_CALL,0);
 if ( LTYPE(*pool)==LINTEGER_TY ||
     (LTYPE(*pool)==LSTRING_TY && _Lisnum(pool)==LINTEGER_TY)) {
  Lstr str;
  poolnum = (int)Lrdint(pool);
  if (poolnum<0)
   poolnum = (context->rexx_rx_proc)+poolnum;
  if (poolnum<0 || poolnum>(context->rexx_rx_proc))
   return 'P';
  /* search in the appropriate scope */
  LINITSTR(str); /* translate to upper case */
  Lstrcpy(&str,name); Lupper(&str);
  leaf = RxVarFindName((context->rexx_proc)[poolnum].scope,&str,&found);

  /* set the new value */
  if (found) {
   /* Just copy the new value */
   Lstrcpy(LEAFVAL(leaf),value);
  } else {
   int hasdot;
   /* search for dot except last character */
   hasdot = (MEMCHR(LSTR(str),'.',LLEN(str)-1)!=NULL);

   /* added it to the tree */
   leaf = RxVarAdd((context->rexx_proc)[poolnum].scope,
    &str, hasdot, leaf);
   Lstrcpy(LEAFVAL(leaf),value);
  }
  LFREESTR(str);
  return 0;
 }

 /* nope search the Pool tree */
 leaf = BinFind(&(context->variable_PoolTree),pool);
 if (!leaf) return 'P';

 if (((TPoolFunc*)(leaf->value))->set)
  found = (((TPoolFunc*)(leaf->value))->set)(name,value);
 if (found) return 'F';
 return 0;
} /* PoolSet */

/* ------------ RxRegPool -------------- */
int __CDECL
RxRegPool(char *poolname, int (*getf)(PLstr,PLstr),
   int (*setf)(PLstr,PLstr))
{
 Lstr pn;
 TPoolFunc *pf;
 Context *context = (Context*)CMSGetPG();

 LINITSTR(pn);
 Lscpy(&pn,poolname);
 Lupper(&pn);

 pf = (TPoolFunc*)MALLOC(sizeof(TPoolFunc),"PoolFunc");
 pf->get = getf;
 pf->set = setf;
 BinAdd(&(context->variable_PoolTree),&pn,pf);

 LFREESTR(pn);
 return 0;
} /* RxRegPool */
