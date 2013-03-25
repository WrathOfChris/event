/*
 * Copyright (c) 2006 Christopher Maxwell.  All rights reserved.
 */

%{
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/limits.h>
#include <sys/mount.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <toolbox.h>

#include "tbevent.h"
#undef CODE

struct tbevent_conf *conf;
static FILE *fin = NULL;		/* for config FILE */
static tbstring fbuf = {0};		/* for config BUFFER */
#define PGETC(f) (f ? lgetc(f) : bgetc(&fbuf))

static int lineno = 1;
static int errors = 0;
static int pdebug = 1;
static enum {
	CODE_NONE,
	CODE_BRACES,
	CODE_SEMICOLON,
	CODE_BRACKET
} codechunk = CODE_NONE;
static int codeline = 0;
const char *infile;
static tbsymbol_head *symhead;

int yyerror(const char *, ...);
int yyparse(void);
int kw_cmp(const void *, const void *);
int lookup(char *);
int lgetc(FILE *);
int bgetc(tbstring *);
int lungetc(int);
int findeol(void);
int yylex(void);

typedef struct {
	union {
		int64_t number;
		char *string;
		Member *member;
		struct member_list members;
		Action *action;
		struct action_list actions;
		Function *function;
		struct function_list functions;
		struct funcarg_list args;
		struct name_list names;
	} v;
	struct {
		char *data;
		int lineno;
	} preamble;
	struct name_list include;
	struct {
		struct member_list in;
		struct member_list out;
		struct member_list stack;
	} interface;
	struct action_list action;
	struct action_list finish;
	int lineno;
} YYSTYPE;

%}

%token	ERROR
%token	DEFINITIONS EEQUAL TBEGIN INCLUDE PREAMBLE END
%token	INTERFACE IN OUT STACK
%token	ACTION
%token	FINISH
%token	REFERENCE OPTIONAL CONSTRUCTED CONSTANT
%token	CALL CODE CONTINUE COPY FORK FORKWAIT RETRY RETURN SYNC TEST WAIT
%token	<v.string>		STRING
%token	<v.string>		CODECHUNK

%type	<v.number>		number
%type	<v.string>		string
%type	<interface>		interface
%type	<v.members>		memberdecls
%type	<v.member>		memberdecl
%type	<action>		action
%type	<finish>		finish
%type	<v.actions>		statements
%type	<v.action>		statement
%type	<v.function>	function
%type	<v.args>		funcargs
%type	<v.string>		type
%type	<v.number>		typeopts
%type	<v.string>		expression
%type	<preamble>		preamble
%type	<include>		include
%type	<v.names>		namelist
%%

grammar		: /* empty */
			| grammar machine
			| grammar error 	{ errors++; }
			;

machine		: /* empty */
			| STRING DEFINITIONS EEQUAL TBEGIN include preamble interface 
					action finish END {
				struct machine *mach;
				if ((mach = calloc(1, sizeof(*mach))) == NULL)
					fatal("calloc");
				if ((mach->filename = strdup(infile)) == NULL)
					fatal("strdup");
				if ((mach->name = strdup($1)) == NULL)
					fatal("strdup");
				LIST_COPY_REVERSE(&$5, &mach->include, struct name, entry);
				mach->preamble = $6.data;
				mach->preambleline = $6.lineno;
				LIST_COPY_REVERSE(&$7.in, &mach->interface.in, Member, entry);
				LIST_COPY_REVERSE(&$7.out, &mach->interface.out, Member, entry);
				LIST_COPY_REVERSE(&$7.stack, &mach->interface.stack, Member, entry);
				LIST_COPY_REVERSE(&$8, &mach->action, Action, entry);
				LIST_COPY_REVERSE(&$9, &mach->finish, Action, entry);
				TAILQ_INSERT_TAIL(&conf->machines, mach, entry);
				free($1);
			}
			;

include		: /* empty */ { LIST_INIT(&$$); }
			| INCLUDE namelist ';' {
				$$ = $2;
			}
			;

preamble	: /* empty */ { $$.data = NULL; $$.lineno = 0; }
			| PREAMBLE '{' { codechunk=CODE_BRACES; codeline=lineno; }
					CODECHUNK '}' {
				$$.data = $4;
				$$.lineno = codeline;
			}
			;

interface	: /* empty */ {
				LIST_INIT(&$$.in);
				LIST_INIT(&$$.out);
				LIST_INIT(&$$.stack);
			}
			| interface INTERFACE IN '{' memberdecls '}' {
				$$.in = $5;
			}
			| interface INTERFACE OUT '{' memberdecls '}' {
				$$.out = $5;
			}
			| interface INTERFACE STACK '{' memberdecls '}' {
				$$.stack = $5;
			}
			;

action		: /* empty */ { LIST_INIT(&$$); }
			| ACTION '{' statements '}' {
				$$ = $3;
			}
			| ACTION '{' '}' {
				LIST_INIT(&$$);
			}
			;

finish		: /* empty */ { LIST_INIT(&$$); }
			| FINISH '{' statements '}' {
				$$ = $3;
			}
			| FINISH '{' '}' {
				LIST_INIT(&$$);
			}
			;

statements	: statement {
				LIST_INIT(&$$);
				if ($1->type == FUNC_WAIT || $1->type == FUNC_CODE ||
						$1->type == FUNC_SYNC)
					$1->lineno = codeline;
				else
					$1->lineno = lineno;
				LIST_INSERT_HEAD(&$$, $1, entry);
			}
			| statements statement {
				$$ = $1;
				if ($2->type == FUNC_WAIT || $2->type == FUNC_CODE ||
						$2->type == FUNC_SYNC)
					$2->lineno = codeline;
				else
					$2->lineno = lineno;
				LIST_INSERT_HEAD(&$$, $2, entry);
			}
			;

statement	: CALL function ';' {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				$$->type = FUNC_CALL;
				LIST_INIT(&$$->functions);
				LIST_INSERT_HEAD(&$$->functions, $2, entry);
			}
			| COPY STRING ',' STRING ';' {
				Function *f;
				Funcarg *fa;
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				$$->type = FUNC_COPY;
				if ((f = calloc(1, sizeof(*f))) == NULL)
					fatal("calloc");
				LIST_INIT(&$$->functions);
				LIST_INSERT_HEAD(&$$->functions, f, entry);
				LIST_INIT(&f->args);
				/* do #4, then #2 so they're in the correct order */
				if ((fa = calloc(1, sizeof(*fa))) == NULL)
					fatal("calloc");
				if ((fa->name = strdup($4)) == NULL)
					fatal("strdup");
				LIST_INSERT_HEAD(&f->args, fa, entry);
				if ((fa = calloc(1, sizeof(*fa))) == NULL)
					fatal("calloc");
				if ((fa->name = strdup($2)) == NULL)
					fatal("strdup");
				LIST_INSERT_HEAD(&f->args, fa, entry);
			}
			| RETRY '{' statements '}' {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				$$->type = FUNC_RETRY;
				LIST_COPY_REVERSE(&$3, &$$->actions, Action, entry);
			}
			| RETURN expression ';' {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				$$->type = FUNC_RETURN;
				$$->data = $2;
			}
			| CONTINUE ';' {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				$$->type = FUNC_CONTINUE;
			}
			| TEST expression '{' statements '}' {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				$$->type = FUNC_TEST;
				$$->data = $2;
				LIST_COPY_REVERSE(&$4, &$$->actions, Action, entry);
			}
			| TEST expression statement {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				$$->type = FUNC_TEST;
				$$->data = $2;
				LIST_INIT(&$$->actions);
				LIST_INSERT_HEAD(&$$->actions, $3, entry);
			}
			| WAIT STRING '(' funcargs ')'
					'{' { codechunk=CODE_BRACES; codeline=lineno; }
					CODECHUNK '}' {
				Function *f;
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				$$->type = FUNC_WAIT;
				LIST_INIT(&$$->functions);
				if ((f = calloc(1, sizeof(*f))) == NULL)
					fatal("calloc");
				if ((f->name = strdup($2)) == NULL)
					fatal("strdup");
				LIST_COPY_REVERSE(&$4, &f->args, Funcarg, entry);
				LIST_INSERT_HEAD(&$$->functions, f, entry);
				$$->data = $8;
				free($2);
			}
			| CODE '{' { codechunk=CODE_BRACES; codeline=lineno; }
					CODECHUNK '}' {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				$$->type = FUNC_CODE;
				$$->data = $4;
				$$->lineno = codeline;
			}
			| SYNC '{' { codechunk=CODE_BRACES; codeline=lineno; }
					CODECHUNK '}' {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				$$->type = FUNC_SYNC;
				$$->data = $4;
				$$->lineno = codeline;
			}
			| FORK function ';' {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				$$->type = FUNC_FORK;
				LIST_INIT(&$$->functions);
				LIST_INSERT_HEAD(&$$->functions, $2, entry);
			}
			| FORK function '{' { codechunk=CODE_BRACES; codeline=lineno; }
					CODECHUNK '}' {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				$$->type = FUNC_FORK;
				LIST_INIT(&$$->functions);
				LIST_INSERT_HEAD(&$$->functions, $2, entry);
				$$->data = $5;
			}
			| FORKWAIT ';' {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				$$->type = FUNC_FORKWAIT;
				LIST_INIT(&$$->functions);
			}
			| FORKWAIT '{' { codechunk=CODE_BRACES; codeline=lineno; }
					CODECHUNK '}' {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				$$->type = FUNC_FORKWAIT;
				LIST_INIT(&$$->functions);
				$$->data = $4;
			}
			;

function	: STRING '(' funcargs ')' {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				if (($$->name = strdup($1)) == NULL)
					fatal("strdup");
				LIST_COPY_REVERSE(&$3, &$$->args, Funcarg, entry);
				free($1);
			}
			;

funcargs	: /* empty */ { LIST_INIT(&$$); }
			| funcargs ',' STRING {
				Funcarg *a;
				if ((a = calloc(1, sizeof(*a))) == NULL)
					fatal("calloc");
				if ((a->name = strdup($3)) == NULL)
					fatal("strdup");
				$$ = $1;
				LIST_INSERT_HEAD(&$$, a, entry);
				free($3);
			}
			| STRING {
				Funcarg *a;
				if ((a = calloc(1, sizeof(*a))) == NULL)
					fatal("calloc");
				if ((a->name = strdup($1)) == NULL)
					fatal("strdup");
				LIST_INIT(&$$);
				LIST_INSERT_HEAD(&$$, a, entry);
				free($1);
			}
			| funcargs ',' STRING STRING {
				Funcarg *a;
				if ((a = calloc(1, sizeof(*a))) == NULL)
					fatal("calloc");
				if ((a->type = strdup($3)) == NULL)
					fatal("strdup");
				if ((a->name = strdup($4)) == NULL)
					fatal("strdup");
				$$ = $1;
				LIST_INSERT_HEAD(&$$, a, entry);
				free($3);
			}
			| STRING STRING {
				Funcarg *a;
				if ((a = calloc(1, sizeof(*a))) == NULL)
					fatal("calloc");
				if ((a->type = strdup($1)) == NULL)
					fatal("strdup");
				if ((a->name = strdup($2)) == NULL)
					fatal("strdup");
				LIST_INIT(&$$);
				LIST_INSERT_HEAD(&$$, a, entry);
				free($1);
			}
			;

expression	: '(' { codechunk=CODE_BRACKET; codeline=lineno;} CODECHUNK ')' {
				$$ = $3;
			}
			| STRING
			;

memberdecls	: /* empty */ { LIST_INIT(&$$); }
			| memberdecl {
				LIST_INIT(&$$);
				LIST_INSERT_HEAD(&$$, $1, entry);
			}
			| memberdecls ',' memberdecl {
				$$ = $1;
				LIST_INSERT_HEAD(&$$, $3, entry);
			}
			;

memberdecl	: STRING '[' number ']' type typeopts {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				if (($$->name = strdup($1)) == NULL)
					fatal("strdup");
				$$->val = $3;
				$$->type = $5;
				$$->flags = $6;
				free($1);
			}
			| STRING type typeopts {
				if (($$ = calloc(1, sizeof(*$$))) == NULL)
					fatal("calloc");
				if (($$->name = strdup($1)) == NULL)
					fatal("strdup");
				$$->val = -1;
				$$->type = $2;
				$$->flags = $3;
				free($1);
			}
			;

type		: STRING
			;

typeopts	: /* empty */ { $$ = FLAG_NONE; }
			| typeopts REFERENCE {$$ = $1 | FLAG_REFERENCE; }
			| typeopts OPTIONAL { $$ = $1 | FLAG_OPTIONAL; }
			| typeopts CONSTRUCTED { $$ = $1 | FLAG_CONSTRUCTED; }
			| typeopts CONSTANT { $$ = $1 | FLAG_CONSTANT; }
			;

namelist	: /* empty */ { LIST_INIT(&$$); }
			| namelist ',' STRING {
				struct name *n;
				if ((n = calloc(1, sizeof(*n))) == NULL)
					fatal("calloc");
				if ((n->name = strdup($3)) == NULL)
					fatal("strdup");
				$$ = $1;
				LIST_INSERT_HEAD(&$$, n, entry);
				free($3);
			}
			| STRING {
				struct name *n;
				if ((n = calloc(1, sizeof(*n))) == NULL)
					fatal("calloc");
				if ((n->name = strdup($1)) == NULL)
					fatal("strdup");
				LIST_INIT(&$$);
				LIST_INSERT_HEAD(&$$, n, entry);
				free($1);
			}
			;

number		: STRING			{
				int64_t ulval;
				int64_t mult = 1;
				int slen = strlen($1);
				const char *errstr = NULL;

				/*
				 * tera, giga, mega, kilo
				 * (tibi, gibi, mibi, kibi) really
				 */
				switch ($1[slen - 1]) {
					case 't':
					case 'T':
						mult *= 1024;
						/* FALLTHROUGH */
					case 'g':
					case 'G':
						mult *= 1024;
						/* FALLTHROUGH */
					case 'm':
					case 'M':
						mult *= 1024;
						/* FALLTHROUGH */
					case 'k':
					case 'K':
						mult *= 1024;
						/* FALLTHROUGH */
						$1[slen - 1] = '\0';
						break;
				}
				
				ulval = strtonum($1, 0, LLONG_MAX, &errstr);
				if (errstr) {
					yyerror("\"%s\" is not a number (%s)", $1, errstr);
					free($1);
					YYERROR;
				} else
					$$ = ulval * mult;
				free($1);
			}
			;

string		: string STRING				{
				if (asprintf(&$$, "%s %s", $1, $2) == -1)
					fatal("string: asprintf");
				free($1);
				free($2);
			}
			| STRING
			;

varset		: STRING '=' string		{
				int ret;
				if (pdebug > 1)
					printf("symbol: %s = \"%s\"\n", $1, $3);
				if ((ret = tbsymbol_set(symhead, $1, $3, 0)))
					tbfatal("cannot set variable: %s", ret);
				free($1);
				free($3);
			}
			;

%%

struct keywords {
	const char *k_name;
	int k_val;
};

int
yyerror(const char *fmt, ...)
{
	va_list ap;
	char *nfmt;

	/*
	 * XXX dirty hack to avoid displaying useless error when editline commands
	 * are being used
	 */
	if (infile == NULL && strcmp(fmt, "syntax error") == 0)
		return 0;

	errors = 1;
	va_start(ap, fmt);
	if (asprintf(&nfmt, "%s:%d: %s", infile ? infile : "buffer",
				yylval.lineno, fmt) == -1)
		fatalx("yyerror asprintf");
	vlog(LOG_CRIT, nfmt, ap);
	va_end(ap);
	free(nfmt);
	return (0);
}

int
kw_cmp(const void *k, const void *e)
{
	return (strcmp(k, ((const struct keywords *)e)->k_name));
}

int
lookup(char *s)
{
	/* this has to be sorted always */
	static const struct keywords keywords[] = {
		{ "::=",			EEQUAL},
		{ "ACTION",			ACTION},
		{ "BEGIN",			TBEGIN},
		{ "CALL",			CALL},
		{ "CODE",			CODE},
		{ "CONSTANT",		CONSTANT},
		{ "CONSTRUCTED",	CONSTRUCTED},
		{ "CONTINUE",		CONTINUE},
		{ "COPY",			COPY},
		{ "DEFINITIONS",	DEFINITIONS},
		{ "END",			END},
		{ "FINISH",			FINISH},
		{ "FORK",			FORK},
		{ "FORKWAIT",		FORKWAIT},
		{ "IN",				IN},
		{ "INCLUDE",		INCLUDE},
		{ "INTERFACE",		INTERFACE},
		{ "OPTIONAL",		OPTIONAL},
		{ "OUT",			OUT},
		{ "PREAMBLE",		PREAMBLE},
		{ "REFERENCE",		REFERENCE},
		{ "RETRY",			RETRY},
		{ "RETURN",			RETURN},
		{ "STACK",			STACK},
		{ "SYNC",			SYNC},
		{ "TEST",			TEST},
		{ "WAIT",			WAIT},
	};
	const struct keywords	*p;

	p = bsearch(s, keywords, sizeof(keywords)/sizeof(keywords[0]),
	    sizeof(keywords[0]), kw_cmp);

	if (p) {
		if (pdebug > 1)
			fprintf(stderr, "token: %s = %d\n", s, p->k_val);
		return (p->k_val);
	} else {
		if (pdebug > 1)
			fprintf(stderr, "string: %s\n", s);
		return (STRING);
	}
}

#define MAXPUSHBACK	128

const char *parsebuf;
int parseindex;
char pushback_buffer[MAXPUSHBACK];
int pushback_index = 0;

int
lgetc(FILE *f)
{
	int	c, next;

	if (parsebuf) {
		/* Read character from the parsebuffer instead of input. */
		if (parseindex >= 0) {
			c = parsebuf[parseindex++];
			if (c != '\0')
				return (c);
			parsebuf = NULL;
		} else
			parseindex++;
	}

	if (pushback_index)
		return (pushback_buffer[--pushback_index]);

	while ((c = getc(f)) == '\\') {
		next = getc(f);
		if (next != '\n') {
			if (isspace(next))
				yyerror("whitespace after \\");
			ungetc(next, f);
			break;
		}
		yylval.lineno = lineno;
		lineno++;
	}
	if ((c == '\t' || c == ' ') && codechunk == CODE_NONE) {
		/* Compress blanks to a single space. */
		do {
			c = getc(f);
		} while (c == '\t' || c == ' ');
		ungetc(c, f);
		c = ' ';
	}

	return (c);
}

int
bgetc(tbstring *s)
{
	int c = EOF, next;

	if (parsebuf) {
		/* Read character from the parsebuffer instead of input. */
		if (parseindex >= 0) {
			c = parsebuf[parseindex++];
			if (c != '\0')
				return (c);
			parsebuf = NULL;
		} else
			parseindex++;
	}

	if (pushback_index)
		return (pushback_buffer[--pushback_index]);

	while ((c = tbstrgetc(s)) == '\\') {
		next = tbstrgetc(s);
		if (next != '\n') {
			if (isspace(next))
				yyerror("whitespace after \\");
			tbstrinsc(s, next, 0);
			break;
		}
		yylval.lineno = lineno;
		lineno++;
	}
	if ((c == '\t' || c == ' ') && codechunk == CODE_NONE) {
		/* Compress blanks to a single space. */
		do {
			c = tbstrgetc(s);
		} while (c == '\t' || c == ' ');
		tbstrinsc(s, c, 0);
		c = ' ';
	}

	return c;
}

int
lungetc(int c)
{
	if (c == EOF)
		return (EOF);
	if (parsebuf) {
		parseindex--;
		if (parseindex >= 0)
			return (c);
	}
	if (pushback_index < MAXPUSHBACK-1)
		return (pushback_buffer[pushback_index++] = c);
	else
		return (EOF);
}

int
findeol(void)
{
	int	c;

	parsebuf = NULL;
	pushback_index = 0;

	/* skip to either EOF or the first real EOL */
	while (1) {
		c = PGETC(fin);
		if (c == '\n') {
			lineno++;
			break;
		}
		if (c == EOF)
			break;
	}
	return (ERROR);
}

/*
 * Careful, lions and tigers and bears (and reallocated buffers) are lurking
 * in here.  Pointers into buf must be adjusted when reallocating.
 */
int
yylex(void)
{
	static char *buf;
	static size_t buflen;

	char *p, *nbuf;
	const char *val;
	int endc, c;
	int token;

	if (buf == NULL) {
		buflen = 8192;
		if ((buf = calloc(1, buflen)) == NULL)
			fatal("calloc");
	}

top:
	p = buf;
	while ((c = PGETC(fin)) == ' ')
		; /* nothing */

	yylval.lineno = lineno;

	/* handle code chunking */
	if (codechunk != CODE_NONE) {
		int depth = 1, find = EOF;
		switch (codechunk) {
			case CODE_NONE: find = EOF; break;
			case CODE_BRACES: find = '}'; break;
			case CODE_SEMICOLON: find = ';'; break;
			case CODE_BRACKET: find = ')'; break;
		}
		
		do {
			*p++ = c;
			if ((unsigned)(p-buf) >= buflen) {
				if ((nbuf = realloc(buf, buflen * 2)) == NULL) {
					yyerror("string too long");
					return (findeol());
				}
				p = nbuf + (p - buf);
				buf = nbuf;
				buflen *= 2;
			}
			if (codechunk == CODE_BRACES && c == '{')
				++depth;
			else if (codechunk == CODE_BRACKET && c == '(')
				++depth;
			else if (c == find) {
				--depth;
				if (depth == 0)
					break;
			}
			if (c == '\n') {
				yylval.lineno = lineno;
				lineno++;
			}
		} while ((c = PGETC(fin)) != EOF && depth > 0);
		lungetc(c);
		*--p = '\0';
		if ((yylval.v.string = strdup(buf)) == NULL)
			fatal("yylex: strdup");
		codechunk = CODE_NONE;
		return CODECHUNK;
	}

	/* handle comment styles */
	if (c == '#')
		while ((c = PGETC(fin)) != '\n' && c != EOF)
			; /* nothing */
	if (c == '-') {
		if ((c = PGETC(fin)) == '-')
			while ((c = PGETC(fin)) != '\n' && c != EOF)
				;
		else {
			lungetc(c);
			c = '-';
		}
	}
	if (c == '/') {
		if ((c = PGETC(fin)) == '*') {
			while ((c = PGETC(fin)) != '*' && c != EOF &&
					(c = PGETC(fin)) != '/' && c != EOF)
				;
		} else {
			lungetc(c);
			c = '/';
		}
	}

	/* symbol resolution */
	if (c == '$' && parsebuf == NULL) {
		while (1) {
			if ((c = PGETC(fin)) == EOF)
				return (0);

			if (p + 1 >= buf + buflen - 1) {
				if ((nbuf = realloc(buf, buflen * 2)) == NULL) {
					yyerror("string too long");
					return (findeol());
				}
				p = nbuf + (p - buf);
				buf = nbuf;
				buflen *= 2;
			}
			if (isalnum(c) || c == '_') {
				*p++ = (char)c;
				continue;
			}
			*p = '\0';
			lungetc(c);
			break;
		}
		val = tbsymbol_get(symhead, buf);
		if (val == NULL) {
			yyerror("macro \"%s\" not defined", buf);
			return (findeol());
		}
		parsebuf = val;
		parseindex = 0;
		goto top;
	}

	switch (c) {
	case '\'':
	case '"':
		endc = c;
		while (1) {
			if ((c = PGETC(fin)) == EOF)
				return (0);
			if (c == endc) {
				*p = '\0';
				break;
			}
			if (c == '\n') {
				lineno++;
				continue;
			}
			if (p + 1 >= buf + buflen - 1) {
				if ((nbuf = realloc(buf, buflen * 2)) == NULL) {
					yyerror("string too long");
					return (findeol());
				}
				p = nbuf + (p - buf);
				buf = nbuf;
				buflen *= 2;
			}
			*p++ = (char)c;
		}
		yylval.v.string = strdup(buf);
		if (yylval.v.string == NULL)
			fatal("yylex: strdup");
		return (STRING);
	}

#define allowed_in_string(x) \
	(isalnum(x) || (ispunct(x) && x != '(' && x != ')' && \
	x != '{' && x != '}' /*&& x != '<' && x != '>'*/ && \
	x != '!' /*&& x != '='*/ /*&& x != '/'*/ && x != '#' && \
	x != ',' && x != '[' && x != ']' && x != ';'))

	if (isalnum(c) || c == ':' || c == '_' || c == '*' || c == '/' ||
			c == '&') {
		do {
			*p++ = c;
			if ((unsigned)(p-buf) >= buflen) {
				if ((nbuf = realloc(buf, buflen * 2)) == NULL) {
					yyerror("string too long");
					return (findeol());
				}
				p = nbuf + (p - buf);
				buf = nbuf;
				buflen *= 2;
			}
		} while ((c = PGETC(fin)) != EOF && (allowed_in_string(c)));
		lungetc(c);
		*p = '\0';
		if ((token = lookup(buf)) == STRING)
			if ((yylval.v.string = strdup(buf)) == NULL)
				fatal("yylex: strdup");
		return (token);
	}
	if (c == '\n') {
		yylval.lineno = lineno;
		lineno++;
		goto top; /* XXX */
	}
	if (c == EOF)
		return (0);
	return (c);
}

int
parse_config(const char *filename, struct tbevent_conf *xconf)
{
	tbsymbol *sym, *next;

	conf = xconf;
	lineno = 1;
	errors = 0;

	if (symhead == NULL)
		if ((symhead = tbsymbol_init()) == NULL)
			fatal("cannot initialize symbol table");
	if ((fin = fopen(filename, "r")) == NULL) {
		log_warn("%s", filename);
		return (-1);
	}
	infile = filename;

	yyparse();

	fclose(fin);

	/* unset (fin) so buffer can work later */
	fin = NULL;

	/* Free macros and check which have not been used. */
	for (sym = TAILQ_FIRST(symhead); sym != TAILQ_END(symhead);
			sym = next) {
		next = TAILQ_NEXT(sym, entry);
		if (!sym->persist) {
			free(sym->name);
			free(sym->value);
			TAILQ_REMOVE(symhead, sym, entry);
			free(sym);
		}
	}

	return (errors ? -1 : 0);
}

int
parse_buffer(const char *buf, size_t len, struct tbevent_conf *xconf)
{
	int ret;

	conf = xconf;
	errors = 0;
	fin = NULL;
	infile = NULL;

	if (symhead == NULL)
		if ((symhead = tbsymbol_init()) == NULL)
			fatal("cannot initialize symbol table");

	if ((ret = tbstrbcat(&fbuf, buf, len))) {
		log_warnx("cannot add parse buffer: %s", tb_error(ret));
		return -1;
	}

	yyparse();

	return (errors ? -1 : 0);
}
