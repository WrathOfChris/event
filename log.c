/* $OpenBSD: log.c,v 1.4 2004/07/08 01:19:27 henning Exp $ */

/*
 * Copyright (c) 2003, 2004 Henning Brauer <henning@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if_dl.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include "toolbox.h"

static int __log_debug;
static pid_t __log_pid;

void
log_init(int n_debug)
{
	extern char	*__progname;

	__log_debug = n_debug;
	__log_pid = getpid();

	if (__log_debug == TBLOG_SYSLOG)
		openlog(__progname, LOG_PID | LOG_NDELAY, LOG_DAEMON);

	tzset();
}

void
vlog(int pri, const char *fmt, va_list ap)
{
	char	*nfmt;
	int ret;

	if (__log_debug != TBLOG_SYSLOG) {
		if (__log_debug == TBLOG_TRACE)
			ret = asprintf(&nfmt, "%05d: %s\n", __log_pid, fmt);
		else
			ret = asprintf(&nfmt, "%s\n", fmt);
		/* best effort in out of mem situations */
		if (ret == -1) {
			vfprintf(stderr, fmt, ap);
			fprintf(stderr, "\n");
		} else {
			vfprintf(stderr, nfmt, ap);
			free(nfmt);
		}
		fflush(stderr);
	} else
		vsyslog(pri, fmt, ap);
}

static void
logit(int pri, const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	vlog(pri, fmt, ap);
	va_end(ap);
}

void
log_warn(const char *emsg, ...)
{
	char	*nfmt;
	va_list	 ap;

	/* best effort to even work in out of memory situations */
	if (emsg == NULL)
		logit(LOG_CRIT, "%s", strerror(errno));
	else {
		va_start(ap, emsg);

		if (asprintf(&nfmt, "%s: %s", emsg, strerror(errno)) == -1) {
			/* we tried it... */
			vlog(LOG_CRIT, emsg, ap);
			logit(LOG_CRIT, "%s", strerror(errno));
		} else {
			vlog(LOG_CRIT, nfmt, ap);
			free(nfmt);
		}
		va_end(ap);
	}
}

void
log_warnx(const char *emsg, ...)
{
	va_list	 ap;

	va_start(ap, emsg);
	vlog(LOG_CRIT, emsg, ap);
	va_end(ap);
}

void
log_info(const char *emsg, ...)
{
	va_list	 ap;

	va_start(ap, emsg);
	vlog(LOG_INFO, emsg, ap);
	va_end(ap);
}

void
log_debug(const char *emsg, ...)
{
	va_list	 ap;

	if (__log_debug == TBLOG_DEBUG || __log_debug == TBLOG_TRACE) {
		va_start(ap, emsg);
		vlog(LOG_DEBUG, emsg, ap);
		va_end(ap);
	}
}

void
fatal(const char *emsg)
{
	if (emsg == NULL)
		logit(LOG_CRIT, "fatal: %s", strerror(errno));
	else
		if (errno)
			logit(LOG_CRIT, "fatal: %s: %s",
			    emsg, strerror(errno));
		else
			logit(LOG_CRIT, "fatal: %s", emsg);

	exit(EX_OSERR);
}

void
fatalx(const char *emsg)
{
	errno = 0;
	fatal(emsg);
}

void
notfatal(const char *emsg)
{
	log_debug(emsg);
}

const char *
log_sockaddr(struct sockaddr *sa)
{
	static char	buf[NI_MAXHOST];

	if (sa == NULL)
		return "(none)";

	if (sa->sa_family == AF_UNIX) {
		if (strlen(((struct sockaddr_un *)sa)->sun_path))
			return ((struct sockaddr_un *)sa)->sun_path;
		else
			return "(local user)";

	} else if (sa->sa_family == AF_LINK) {
		return ether_ntoa((struct ether_addr *)LLADDR((struct sockaddr_dl *)sa));
	}

	if (getnameinfo(sa, sa->sa_len, buf, sizeof(buf), NULL, 0,
	    NI_NUMERICHOST))
		return "(unknown)";
	else
		return buf;
}

const char *
log_bitstring(const char *s, unsigned short v, const char *bits)
{
	static char buf[BUFSIZ];
	size_t blen = sizeof(buf);
	char *p, c;
	int i, any = 0;

	if (bits && *bits == 8)
		snprintf(buf, blen, "%s=%o", s, v);
	else
		snprintf(buf, blen, "%s=%x", s, v);
	p = buf + strlen(buf);
	bits++;

	if (bits) {
		*p++ = '<';

		while ((i = *bits++)) {
			if (v & (1 << (i - 1))) {
				if (any && p < (buf + sizeof(buf) - 1))
					*p++ = ',';
				any = 1;
				for (; (c = *bits) > 32 && p < (buf + sizeof(buf) - 1); bits++)
					*p++ = c;
			} else
				for (; *bits > 32; bits++)
					;
		}
		if (p < (buf + sizeof(buf) - 1))
			*p++ = '>';
		*p = '\0';
	}

	return buf;
}

void
log_hexdump(const unsigned char *data, size_t len)
{
	size_t c, d;
	char ascii[16];
	char hex[49];
	char hexc[4];

	memset(ascii, 0, sizeof(ascii));
	memset(hex, 0, sizeof(hex));

	for (c = 0, d = 0; c < len; c++, d++) {
		if (d >= 16) {
			logit(LOG_INFO, "%-48s\t%.16s", hex, ascii);
			d = 0;
			memset(hex, 0, sizeof(hex));
			memset(ascii, 0, sizeof(ascii));
		}
		snprintf(hexc, sizeof(hexc), " %.2X", (unsigned char)*(data + c));
		strlcat(hex, hexc, sizeof(hex));
		if (isprint(*(data + c)))
			ascii[d] = *(data + c);
		else
			ascii[d] = '.';
	}
	logit(LOG_INFO, "%-48s\t%.16s\n", hex, ascii);
}

void
log_trace(const unsigned char *data, size_t len)
{
	char *safe;
	size_t i;

	if ((safe = calloc(1, len + 1)) == NULL) {
		log_warn("--> trace memory allocation failure");
		return;
	}

	for (i = 0; i < len; i++) {
		if (isprint(*(data + i)))
			*(safe + i) = *(data + i);
		else
			*(safe + i) = '.';
	}

	logit(LOG_DEBUG, "--> %s", safe);

	free(safe);
}
