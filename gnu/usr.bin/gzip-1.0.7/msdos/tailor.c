/* tailor.c -- target dependent functions
 * Copyright (C) 1992-1993 Jean-loup Gailly
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License, see the file COPYING.
 */

/* tailor.c is a bunch of non portable routines.
 * It should be kept to a minimum.
 */

#include "tailor.h"
#include "gzip.h"

#ifndef lint
static char rcsid[] = "$Id: tailor.c,v 1.1 1993/04/10 15:59:29 cgd Exp $";
#endif

#ifdef __TURBOC__

/************************/
/*  Function fcalloc()  */
/************************/

/* Turbo C malloc() does not allow dynamic allocation of 64K bytes
 * and farmalloc(64K) returns a pointer with an offset of 8, so we
 * must fix the pointer. Warning: the pointer must be put back to its
 * original form in order to free it, use fcfree().
 * For MSC, use halloc instead of this function (see tailor.h).
 */
static ush ptr_offset;

void * fcalloc(items, size)
    unsigned items; /* number of items */
    unsigned size;  /* item size */
{
    void * buf = farmalloc((ulg)items*size + 16L);
    /* Normalize the pointer to seg:0 */
    ptr_offset = (ush)((uch*)buf-0);
    *((ush*)&buf+1) += (ptr_offset + 15) >> 4;
    *(ush*)&buf = 0;
    return buf; /* buf stays NULL if alloc failed */
}

void fcfree(ptr)
    void *ptr; /* region allocated with fcalloc() */
{
    /* Put the pointer back to its original form: */
    *((ush*)&ptr+1) -= (ptr_offset + 15) >> 4;
    *(ush*)&ptr = ptr_offset;
    farfree(ptr);
 }

#endif /* __TURBOC__ */
