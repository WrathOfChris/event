PROG=	test
SRCS=	test.c ${EVENTFILES:%=generated/%}
EVSRCS=	test.event
OBJS=	${SRCS:.c=.o}
LDADD=	../log.o ../error.o

CFLAGS?=-O2 ${PIPE} ${DEBUG}
CFLAGS+=-I${CURDIR} -I${CURDIR}/.. -I${DESTDIR}/usr/local/include
CFLAGS+=-Wall #-Werror
CFLAGS+=-Wshadow -Wpointer-arith -Wcast-align -Wcast-qual
CFLAGS+=-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
CFLAGS+=-Wsign-compare -Wformat

GENERATED=  ${CURDIR}/generated
.PATH:      ${GENERATED}
EVENTFILES:= $(shell cat ${GENERATED}/event_files | sed 's,\.x,\.c,g')
CFLAGS+=    -I${GENERATED}

all: ${PROG}

regen:
	cd ${GENERATED}; \
		${CURDIR}/../event ${EVDEBUG} -m test ${EVSRCS:%=${CURDIR}/%}; \
		for f in `cat event_files`; do \
			nf=`echo $$f | sed s,.x$$,.c,g` ; \
			cmp $$f $$nf || cp $$f $$nf ; \
		done

.SUFFIXES:	.o .c .c.o
CC=			gcc
PIPE?=		-pipe
COMPILE.c?=	${CC} ${CFLAGS} -c
LINK.c?=	${CC} ${CFLAGS} ${LDFLAGS}
LOBJS?=		${OBJS:generated/%=%}

${PROG}: ${OBJS} ${DPADD}
	${CC} ${LDFLAGS} ${LDSTATIC} -o $@ ${LOBJS} ${LDADD}

.c:
	${LINK.c} -o $@ $<
.c.o:
	${COMPILE.c} $<

clean:
	rm -f a.out [Ee]rrs mklog core *.core ${PROG} ${OBJS} ${CLEANFILES}
