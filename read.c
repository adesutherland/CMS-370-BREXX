/* /* VM/370 CMS and GCCLIB read.c */
#include <stdio.h>
#include "lstring.h"

/* ---------------- Lread ------------------- */
void Lread( FILEP f, const PLstr line, long size )
{
  long l;
  char *c;
  int i;

  if (size>0) {
    /* Read this many bytes */
    Lfx(line,(size_t)size);
    LZEROSTR(*line);
    c = LSTR(*line);
    l = fread(c, 1, size, f);
  }

  else if (size==0) {
    /* Read a single line */
    l = nextrecLen(f);
    Lfx(line,(size_t)l);
    LZEROSTR(*line);
    c = LSTR(*line);
    fgets(c, l+1, f);
    if (c[l-1]=='\n') { /* Remove newline char */
      l--; c[l] = 0;
    }
  }

  else {
    /* Read entire file */
    l = fgetlen(f);
    if (!l) {
      /* Empty file */
      LZEROSTR(*line);
    }
    if (l>0) {
      /* Read the file in one go */
      rewind(f);
      Lfx(line,(size_t)l);
      LZEROSTR(*line);
      c = LSTR(*line);
      l = fread(c, 1, l, f);
    }
    else {
      /* Don't know file size - have to read the file line by line */
      rewind(f);
      Lfx(line,LREADINCSIZE);
      l = 0;
      while ((i=nextrecLen(f))>0) {
        if (l+i > LMAXLEN(*line)) {
          Lfx(line, (size_t)(l+i+LREADINCSIZE));
        }
        c = LSTR(*line) + l;
        fgets(c, i+1, f);
        l+=i;
      }
      Lfx(line,l); /* Give back unwanted memory */
    }
  }

  LLEN(*line) = l;
  LTYPE(*line) = LSTRING_TY;
} /* Lread */
