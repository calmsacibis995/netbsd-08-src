/* re.c: This file contains the regular expression interface routines for
   the ed line editor. */
/*-
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Andrew Moore, Talke Studio.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)re.c	5.5 (Berkeley) 3/28/93";
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ed.h"

extern char *ibufp;
extern int patlock;

char errmsg[MAXFNAME + 40] = "";

/* optpat: return pointer to compiled pattern from command buffer */
pattern_t *
optpat()
{
	static pattern_t *exp = NULL;

	char *exps;
	char delim;
	int n;

	if ((delim = *ibufp) == '\n') {
		sprintf(errmsg, "no previous pattern");
		return exp;
	} else if (delim == ' ' || *++ibufp == '\n') {
		sprintf(errmsg, "invalid pattern delimiter");
		return NULL;
	} else if (*ibufp == delim) {
		sprintf(errmsg, "no previous pattern");
		return exp;
	} else if ((exps = getlhs(delim)) == NULL)
		return NULL;
	/* buffer alloc'd && not reserved */
	if (exp && !patlock)
		regfree(exp);
	else if ((exp = (pattern_t *) malloc(sizeof(pattern_t))) == NULL) {
		sprintf(errmsg, "out of memory");
		return NULL;
	}
	patlock = 0;
#ifdef GNU_REGEX
	/* initialize pattern buffer */
	exp->buffer = NULL;
	exp->allocated = 0L;
	exp->fastmap = (char *) malloc(FASTMAP_SIZE);
	exp->translate = 0;
#endif
	if (n = regcomp(exp, exps, 0)) {
		regerror(n, exp, errmsg, sizeof errmsg);
		return NULL;
	}
	return exp;
}


/* getlhs: copy a pattern string from the command buffer; return pointer
   to the copy */
char *
getlhs(delim)
	int delim;
{
	static char buf[MAXLINE];
	char *nd;

	for (nd = ibufp; *nd != delim && *nd != '\n'; nd++)
		switch (*nd) {
		default:
			break;
		case CCL:
			if ((nd = ccl(CCLEND, ++nd)) == NULL) {
				sprintf(errmsg, "unbalanced brackets ([])");
				return NULL;
			}
			break;
		case ESCHAR:
			if (*++nd == '\n') {
				sprintf(errmsg, "trailing backslash (\\)");
				return NULL;
			}
			break;
		}
	strncpy(buf, ibufp, nd - ibufp);
	buf[nd - ibufp] = '\0';
	ibufp = nd;
	return buf;
}


/* ccl: expand a character class */
char *
ccl(delim, src)
	int	delim;
	char	*src;
{
	if (*src == '^')
		src++;
	if (*src == delim)
		src++;
	while (*src != delim && *src != '\n')
		src++;
	return  (*src == delim) ? src : NULL;
}
