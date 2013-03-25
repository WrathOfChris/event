/*
 * Copyright (c) 2005 Christopher Maxwell.  All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include "toolbox.h"

tbsymbol_head *
tbsymbol_init(void)
{
	tbsymbol_head *h;
	if ((h = calloc(1, sizeof(tbsymbol_head))))
		TAILQ_INIT(h);
	return h;
}

/*
 * get the value of a symbol from the symbol table
 */
const char *
tbsymbol_get(tbsymbol_head *head, const char *name)
{
	tbsymbol *sym;

	TAILQ_FOREACH(sym, head, entry)
		if (strcmp(name, sym->name) == 0) {
			sym->used = 1;
			return sym->value;
		}

	return NULL;
}

int
tbsymbol_set(tbsymbol_head *head, const char *name, const char *value,
		int persist)
{
	tbsymbol *sym;

	TAILQ_FOREACH(sym, head, entry)
		if (strcmp(name, sym->name) == 0)
			break;

	if (sym != TAILQ_END(head)) {
		if (sym->persist == 1)
			return TBOX_EXISTS;
		else {
			free(sym->name);
			free(sym->value);
			TAILQ_REMOVE(head, sym, entry);
			free(sym);
		}
	}
	if ((sym = calloc(1, sizeof(*sym))) == NULL)
		return TBOX_NOMEM;

	if ((sym->name = strdup(name)) == NULL) {
		free(sym);
		return TBOX_NOMEM;
	}
	if ((sym->value = strdup(value)) == NULL) {
		free(sym->name);
		free(sym);
		return TBOX_NOMEM;
	}
	sym->used = 0;
	sym->persist = persist;
	TAILQ_INSERT_TAIL(head, sym, entry);
	return TBOX_SUCCESS;
}

int
tbsymbol_set_cmdline(tbsymbol_head *head, const char *s)
{
	char *p, *v;
	int ret;

	if ((p = strdup(s)) == NULL)
		return TBOX_NOMEM;
	if ((v = strrchr(p, '=')) == NULL)
		return TBOX_BADFORMAT;
	*v++ = '\0';

	ret = tbsymbol_set(head, p, v, 1);
	free(p);
	return ret;
}
