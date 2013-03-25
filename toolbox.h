#ifndef TOOLBOX_H
#define TOOLBOX_H
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <stdint.h>
#include "toolbox_err.h"

#define TBOX_SUCCESS		0
#define TBEVENT_SUCCESS		0
#define TBOX_MAXPUSHBACK	128

#define TBLOG_SYSLOG		0
#define TBLOG_DEBUG			1
#define TBLOG_STDERR		2
#define TBLOG_TRACE			3

/* the world is not OpenBSD */
#ifndef LIST_END
#define LIST_END(head)	NULL
#endif
#ifndef TAILQ_END
#define TAILQ_END(head)	NULL
#endif
#ifndef timespeccmp
#define timespeccmp(tsp, usp, cmp)				\
		(((tsp)->tv_sec == (usp)->tv_sec) ?		\
		 ((tsp)->tv_nsec cmp (usp)->tv_nsec) :	\
		 ((tsp)->tv_sec cmp (usp)->tv_sec))
#endif
#ifndef	__bounded__
#define __bounded__(buf, min, max)
#endif

typedef struct tbstring {
	char *s;				/* Pointer to the string */
	size_t a;				/* Amount of memory allocated */
	size_t len;				/* length for bstrings */
} tbstring;

typedef struct tbsymbol {
	TAILQ_ENTRY(tbsymbol) entry;
	int used;
	int persist;
	char *name;
	char *value;
} tbsymbol;
TAILQ_HEAD(tbsymbol_head, tbsymbol);
typedef struct tbsymbol_head tbsymbol_head;

typedef struct tbkeyval {
	const char *key;
	char *val;
	size_t len;
} tbkeyval;

/* init.c */
void tbox_init(void);

/* error.c */
const char * tb_com_right(struct et_list *, long);
void tb_initialize_error_table_r(struct et_list **, const char **, 
		int, long);
int tb_init_error_table(const char **, long, int);
void tb_free_error_table(struct et_list *);
const char * tb_error(int);
void tbfatal(const char *, int);

/* log.c */
void log_init(int);
void log_warn(const char *, ...)
	__attribute__ ((__format__(printf, 1, 2)));
void log_warnx(const char *, ...)
	__attribute__ ((__format__(printf, 1, 2)));
void log_info(const char *, ...)
	__attribute__ ((__format__(printf, 1, 2)));
void log_debug(const char *, ...)
	__attribute__ ((__format__(printf, 1, 2)));
__dead void fatal(const char *);
__dead void fatalx(const char *);
void notfatal(const char *);
const char *log_sockaddr(struct sockaddr *);
const char * log_bitstring(const char *, unsigned short, const char *);
void log_hexdump(const unsigned char *, size_t)
	__attribute__ ((__bounded__(__buffer__, 1, 2)));
void vlog(int, const char *, va_list)
	__attribute__ ((__format__(printf, 2, 0)));
void log_trace(const unsigned char *, size_t)
	__attribute__ ((__bounded__(__buffer__, 1, 2)));

/* str.c */
int tbstrcat(tbstring *, const char *);
int tbstrcpy(tbstring *, const char *);
int tbstrcats(tbstring *, tbstring *);
int tbstrcpys(tbstring *, tbstring *);
char *strnow(const char *);
int tbstrpre(tbstring *, const char *);
int tbstrinsc(tbstring *, int, size_t);
int tbstrdelc(tbstring *, size_t);
int tbstrgetc(tbstring *);

void tbstrfree(tbstring *);
int tbstrchk(tbstring *, size_t);
size_t tbstrpos(tbstring *, int, size_t);

int tbstrbcat(tbstring *, const char *, size_t);
int tbstrbcpy(tbstring *, const char *, size_t);
int tbstrbzero(tbstring *);

size_t strpos(const char *, char);
int tbstrrep(tbstring *, char *, char *, char *);
int tbstrsplit2(const char *, int, tbstring *, tbstring *);
int strcmp_host(const char *, const char *);
int tbstrclean(const char *, const char *, tbstring *);

int tbstrcatul_p(tbstring *, unsigned long, size_t);
int tbstrcatl_p(tbstring *, long, size_t);
#define tbstrcati_p(sa, i, n) tbstrcatl_p(sa, (long)i, n)
#define tbstrcati(sa, i) tbstrcatl_p(sa, (long)i, 0)
#define tbstrcatl(sa, l) tbstrcatl_p(sa, l, 0)
#define tbstrcatul(sa, ul) tbstrcatul_p(sa, ul, 0)

int tbstrprintf(tbstring *, const char *, ...)
    __attribute__((__format__(printf, 2, 3)));
int tbstrprintfcat(tbstring *, const char *, ...)
    __attribute__((__format__(printf, 2, 3)));

/* strtonum.c */
long long strtonum(const char *numstr, long long minval, long long maxval,
		const char **errstrp);

/* symbol.c */
tbsymbol_head * tbsymbol_init(void);
const char * tbsymbol_get(tbsymbol_head *, const char *);
int tbsymbol_set(tbsymbol_head *, const char *, const char *,
		int);
int tbsymbol_set_cmdline(tbsymbol_head *, const char *);

#endif
