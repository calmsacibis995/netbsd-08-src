#	@(#)Makefile	5.6 (Berkeley) 6/27/91
#
#	$Header: /cvsroot/src/sbin/route/Makefile,v 1.3 1993/03/23 00:29:59 cgd Exp $

PROG=	route
MAN8=	route.0
SRCS=	route.c
#SRCS=	route.c ccitt_addr.c
CFLAGS+=-I.
#CLEANFILES+=keywords.h
BINOWN=	root
BINMODE=4555

all route depend lint tags: keywords.h

# keywords.h: keywords
# 	sed -e '/^#/d' -e '/^$$/d' ${.CURDIR}/keywords > _keywords.tmp
# 	tr a-z A-Z < _keywords.tmp | paste _keywords.tmp - | \
# 	    awk '{ \
# 		if (NF > 1) \
# 			printf "#define\tK_%s\t%d\n\t{\"%s\", K_%s},\n", \
# 			    $$2, NR, $$1, $$2 }' \
# 	    > ${.TARGET}
# 	rm -f _keywords.tmp

.include <bsd.prog.mk>
