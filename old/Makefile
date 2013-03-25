PROG=	event
SRCS=	main.c parse.y gen.c type.c fork.c
SRCS+=	error.c log.c str.c symbol.c
OBJS=	main.o parse.o gen.o type.o fork.o error.o log.o str.o symbol.o

CFLAGS?=-O2 ${PIPE} ${DEBUG}
CFLAGS+=-I${CURDIR} -I${DESTDIR}/usr/local/include
CFLAGS+=-Wall -Werror
CFLAGS+=-Wshadow -Wpointer-arith -Wcast-align -Wcast-qual
CFLAGS+=-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
CFLAGS+=-Wsign-compare -Wbounded -Wformat

all: ${PROG}

.SUFFIXES:	.o .c .c.o
CC?=		cc
PIPE?=		-pipe
COMPILE.c?=	${CC} ${CFLAGS} -c
LINK.c?=	${CC} ${CFLAGS} ${LDFLAGS}

${PROG}: ${OBJS} ${DPADD}
	${CC} ${LDFLAGS} ${LDSTATIC} -o $@ ${OBJS} ${LDADD}

CLEANFILES=	y.tab.c y.tab.h

test:	all
	${PROG} -d ${CURDIR}/test.event

debug:	all
	/usr/bin/gdb ${PROG}

debug_core: all
	/usr/bin/gdb ${PROG} ${PROG}.core

.c:
	${LINK.c} -o $@ $<
.c.o:
	${COMPILE.c} $<

clean:
	rm -f a.out [Ee]rrs mklog core *.core ${PROG} ${OBJS} ${CLEANFILES}
