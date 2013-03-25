/*
 * Copyright (c) 2005 Christopher Maxwell.  All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include "toolbox.h"

static struct et_list *_tb_et_list = NULL;

const char *
tb_com_right(struct et_list *list, long code)
{
    struct et_list *p;

	if (list == NULL)
		list = _tb_et_list;

    for (p = list; p; p = p->next)
		if (code >= p->table->base && code < p->table->base + p->table->count)
			return p->table->messages[code - p->table->base];

    return NULL;
}

struct foobar {
	struct et_list etl;
	struct error_table et;
};

void
tb_initialize_error_table_r(struct et_list **list, const char **messages, 
		int num_errors, long base)
{
    struct et_list *et, **end;
    struct foobar *f;

    for (end = list, et = *list; et; end = &et->next, et = et->next)
		if (et->table->messages == messages)
			return;
    if ((f = malloc(sizeof(*f))) == NULL)
        return;
    et = &f->etl;
    et->table = &f->et;
    /* 
	 * Orig:	et->table->messages = messages;
	 * Force a const pointer into a const member of a struct.
	 */
	*(int *)&et->table->messages = messages;
    et->table->count = num_errors;
    et->table->base = base;
    et->next = NULL;
    *end = et;
}

int
tb_init_error_table(const char **msgs, long base, int count)
{
	tb_initialize_error_table_r(&_tb_et_list, msgs, count, base);
	return 0;
}

void
tb_free_error_table(struct et_list *et)
{
    while (et){
		struct et_list *p = et;
		et = et->next;
		free(p);
    }
}

const char *
tb_error(int code)
{
	const char *p = NULL;
	
	if (code == 0)
		return "success";

	p = tb_com_right(NULL, code);

	if (p == NULL)
		p = strerror(code);

	return p;
}

void
tbfatal(const char *emsg, int code)
{
	if (emsg == NULL)
		log_warnx("fatal: %s", tb_error(code));
	else
		if (code)
			log_warnx("fatal: %s: %s",
					emsg, tb_error(code));
		else
			log_warnx("fatal: %s", emsg);

	exit(EX_OSERR);
}
