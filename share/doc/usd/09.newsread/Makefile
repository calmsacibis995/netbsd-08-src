#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	usd/09.newsread
SRCS=	../../smm/10.newsop/tmac.n howto.mn

paper.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}

.include <bsd.doc.mk>
