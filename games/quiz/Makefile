#	@(#)Makefile	5.1 (Berkeley) 11/10/91

PROG=	quiz
MAN6=	quiz.0
SRCS=	quiz.c rxp.c
CATS=	africa america areas arith asia babies bard chinese collectives \
	ed elements europe greek inca index latin locomotive midearth morse \
	mult murders poetry posneg pres province seq-easy seq-hard sexes sov \
	spell state trek ucc
HIDEGAME=hidegame

beforeinstall:
	@if [ ! -d ${DESTDIR}/usr/share/games/quiz.db ]; then \
                /bin/rm -f ${DESTDIR}/usr/share/games/quiz.db ; \
                mkdir -p ${DESTDIR}/usr/share/games/quiz.db ; \
                chown root.wheel ${DESTDIR}/usr/share/games/quiz.db ; \
                chmod 755 ${DESTDIR}/usr/share/games/quiz.db ; \
        else \
                true ; \
        fi
	(cd ${.CURDIR}/datfiles; install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${CATS} ${DESTDIR}/usr/share/games/quiz.db)

.include <bsd.prog.mk>
