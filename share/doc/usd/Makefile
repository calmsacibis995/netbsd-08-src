#	@(#)Makefile	5.1.1.1 (Berkeley) 5/7/91

#Missing: 01.begin 02.learn 03.shell 05.dc 06.bc 08.mh 12.edtut 13.edadv
#	  17.jove 18.sed 19.awk 20.msmacros 24.troff 25.trofftut 26.eqn
#	  27.eqnguide 28.tbl 29.refer 30.invert 31.bib 32.diction
BINDIR=	/usr/share/doc/usd
FILES=	00.contents Makefile Title
SUBDIR=	04.csh 07.Mail \
	09.newsread 10.etiq 14.edit 15.vi \
	16.ex 21.msdiffs 22.memacros \
	23.meref \
	33.rogue 34.trek

beforeinstall:
	@if [ ! -d ${DESTDIR}${BINDIR} ]; then \
                /bin/rm -f ${DESTDIR}${BINDIR} ; \
                mkdir -p ${DESTDIR}${BINDIR} ; \
                chown root.wheel ${DESTDIR}${BINDIR} ; \
                chmod 755 ${DESTDIR}${BINDIR} ; \
        else \
                true ; \
        fi
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}

.include <bsd.subdir.mk>
