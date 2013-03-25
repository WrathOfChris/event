/*
 * Copyright (c) 2006 Christopher Maxwell.  All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include "tbevent.h"

struct types {
	const char *t_name;
};

static int
type_cmp(const void *k, const void *e)
{
	return (strcasecmp(k, ((const struct types *)e)->t_name));
}

int
type_lookup(const char *s)
{
	/* this must be sorted */
	static const struct types types[] = {
		{ "DIR" },
		{ "FILE" },
		{ "bool" },
		{ "char" },
		{ "clock_t" },
		{ "complex" },
		{ "div_t" },
		{ "double" },
		{ "float" },
		{ "fpos_t" },
		{ "int" },
		{ "int16_t" },
		{ "int32_t" },
		{ "int64_t" },
		{ "int8_t" },
		{ "int_fast16_t" },
		{ "int_fast32_t" },
		{ "int_fast64_t" },
		{ "int_fast8_t" },
		{ "int_least16_t" },
		{ "int_least32_t" },
		{ "int_least64_t" },
		{ "int_least8_t" },
		{ "intmax_t" },
		{ "intptr_t" },
		{ "jmp_buf" },
		{ "ldiv_t" },
		{ "long" },
		{ "mbstate_t" },
		{ "ptrdiff_t" },
		{ "short" },
		{ "sig_atomic_t" },
		{ "signed" },
		{ "size_t" },
		{ "ssize_t" },
		{ "time_t" },
		{ "uint16_t" },
		{ "uint32_t" },
		{ "uint64_t" },
		{ "uint8_t" },
		{ "uint_fast16_t" },
		{ "uint_fast32_t" },
		{ "uint_fast64_t" },
		{ "uint_fast8_t" },
		{ "uint_least16_t" },
		{ "uint_least32_t" },
		{ "uint_least64_t" },
		{ "uint_least8_t" },
		{ "uintmax_t" },
		{ "uintptr_t" },
		{ "unsigned" },
		{ "va_list" },
		{ "void" },
		{ "wchar_t" },
		{ "wctrans_t" },
		{ "wctype_t" },
		{ "wint_t" },
	};
	const struct types *t;

	t = bsearch(s, types, sizeof(types) / sizeof(types[0]), sizeof(types[0]),
			type_cmp);

	if (t)
		return 1;
	else
		return 0;
}

const char *
type_function(enum func_type type)
{
	switch (type) {
		case FUNC_NONE:		return "NONE";
		case FUNC_CALL:		return "CALL";
		case FUNC_CODE:		return "CODE";
		case FUNC_CONTINUE:	return "CONTINUE";
		case FUNC_COPY:		return "COPY";
		case FUNC_RETRY:	return "RETRY";
		case FUNC_RETURN:	return "RETURN";
		case FUNC_SYNC:		return "SYNC";
		case FUNC_TEST:		return "TEST";
		case FUNC_WAIT:		return "WAIT";
		case FUNC_FORK:		return "FORK";
		case FUNC_FORKWAIT:	return "FORKWAIT";
	}
	return "----";
}
