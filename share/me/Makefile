#	@(#)Makefile	6.11 (Berkeley) 7/1/90

MELIB=	${BINDIR}/me
TMLIB=	${BINDIR}/tmac
MESRCS=	acm.me chars.me deltext.me eqn.me float.me footnote.me \
	index.me letterhead.me local.me null.me refer.me sh.me \
	tbl.me thesis.me
TMSRCS=	tmac.e
MAN7=	me.0

beforeinstall:
	@if [ ! -d ${DESTDIR}${MELIB} ]; then \
                /bin/rm -f ${DESTDIR}${MELIB} ; \
                mkdir -p ${DESTDIR}${MELIB} ; \
                chown root.wheel ${DESTDIR}${MELIB} ; \
                chmod 755 ${DESTDIR}${MELIB} ; \
        else \
                true ; \
        fi
	cd ${.CURDIR}; for i in ${MESRCS}; do \
		sed -f strip.sed < $$i > ${DESTDIR}${MELIB}/$$i; \
		chown ${BINOWN}.${BINGRP} ${DESTDIR}${MELIB}/$$i; \
		chmod 444 ${DESTDIR}${MELIB}/$$i; \
	done
	@if [ ! -d ${DESTDIR}${TMLIB} ]; then \
                /bin/rm -f ${DESTDIR}${TMLIB} ; \
                mkdir -p ${DESTDIR}${TMLIB} ; \
                chown root.wheel ${DESTDIR}${TMLIB} ; \
                chmod 755 ${DESTDIR}${TMLIB} ; \
        else \
                true ; \
        fi
	cd ${.CURDIR}; for i in ${TMSRCS}; do \
		sed -f strip.sed < $$i > ${DESTDIR}${TMLIB}/$$i; \
		chown ${BINOWN}.${BINGRP} ${DESTDIR}${TMLIB}/$$i; \
		chmod 444 ${DESTDIR}${TMLIB}/$$i; \
	done
	cd ${.CURDIR}; install -c -o ${BINOWN} -g ${BINGRP} -m 444 revisions \
	    ${DESTDIR}${MELIB}/revisions

.include <bsd.prog.mk>
