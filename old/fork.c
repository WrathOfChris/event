/*
 * Copyright (c) 2006 Christopher Maxwell.  All rights reserved.
 */
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include "tbevent.h"

extern int printline;
extern int printlog;
extern int lineno;

/*
 * FORK() is a variation of CALL(), which shall never return TBEVENT_CALLBACK.
 * This allows a carefully constructed RETRY{} to manage multiple simultaneous
 * function calls.
 */
void
gen_func_fork(struct tbevent_conf *conf, struct machine *mach,
		struct action *act, const char *type)
{
	struct machine *cm;
	Member *m;
	Function *f;
	Funcarg *fa;
	int outlabel = 0;

	if ((f = LIST_FIRST(&act->functions)) == NULL)
		fatal("FORK with no function");
	/*
	 * WAIT FORWARD DECLARATION
	 */
	CODE(	"static int %s_%s_wait_%d(int, void *",
			type, mach->name, mach->state);
	TAILQ_FOREACH(cm, &conf->machines, entry)
		if (strcmp(cm->name, f->name) == 0)
			break;
	if (cm == TAILQ_END(&conf->machines))
		errx(1, "FORK(%s) not found", f->name);
	LIST_FOREACH(m, &cm->interface.out, entry)
		CODE(	",\n\t\t%s%s %s",
				(m->flags & FLAG_CONSTANT) ? "const " : "",
				m->type,
				(m->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)) ? "*" : "");
	CODE(	");\n\n");

	/*
	 * FORK FUNCTION
	 */
	gen_func_declaration(conf, mach, act, type);
	CODE(	"	int ret;\n");

	if (printlog)
		CODE(	"	log_debug(\"> %s_%s_state_%d: FORK\");\n",
				type, mach->name, mach->state);

	/* FORKCOUNTER is incremented before the callback is called */
	CODE(	"	state->forkcounter++;\n");

	CODE(	"	ret = %s(%s_%s_wait_%d, state",
			f->name, type, mach->name, mach->state);
	LIST_FOREACH(fa, &f->args, entry) {
		CODE(	",\n\t\t\t");
		gen_output_code(conf, mach, act, fa->name);
	}
	CODE(	");\n");

	/* TBEVENT errors are the only type that need to be taken care of */
	if (mach->lastrettype)
		CODE(	"	if (ret != TBEVENT_SUCCESS && ret != TBEVENT_BLOCKDONE &&\n"
				"			ret != TBEVENT_CALLBACK && ret != TBEVENT_FINISH &&\n"
				"			state->ret) {\n"
				"		if (state->retfree)\n"
				"			state->retfree(state->ret);\n"
				"		free(state->ret);\n"
				"		state->ret = NULL;\n"
				"	}\n");

	/* FORK must never return CALLBACK, instead pretend it finished */
	CODE(	"	if (ret != TBEVENT_CALLBACK)\n"
			"		return ret;\n"
			"	else\n"
			"		return TBEVENT_BLOCKDONE;\n");
	gen_func_close(conf, mach, act, type);

	/*
	 * WAIT FUNCTION
	 */
	CODE(	"static int\n"
			"%s_%s_wait_%d(int code, void *arg",
			type, mach->name, mach->state);
	TAILQ_FOREACH(cm, &conf->machines, entry)
		if (strcmp(cm->name, f->name) == 0)
			break;
	if (cm == TAILQ_END(&conf->machines))
		errx(1, "FORK(%s) not found", f->name);
	LIST_FOREACH(m, &cm->interface.out, entry)
		CODE(	",\n\t\t%s%s %s%s",
				(m->flags & FLAG_CONSTANT) ? "const " : "",
				m->type,
				(m->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)) ? "*" : "",
				m->name);
	CODE(	")\n"
			"{\n"
			"	%s_%s_state *state = (%s_%s_state  *)arg;\n",
			conf->module, mach->name, conf->module, mach->name);
	CODE(	"	int ret = TBEVENT_SUCCESS;\n");

	if (printlog)
		CODE(	"	log_debug(\"> %s_%s_wait_%d: FORK %%d RETURN: %%s\",\n"
				"			state->forkcounter, tb_error(code));\n",
				type, mach->name, mach->state);

	/* FORKCOUNTER is always decremented */
	CODE(	"	state->forkcounter--;\n");

	/*
	 * RETURN frees the old return that was needed to push arguments onto the
	 * CALL function stack.
	 */
	if (mach->lastrettype) {
		CODE(	"	if (state->ret) {\n"
				"		if (state->retfree)\n"
				"			state->retfree(state->ret);\n"
				"		free(state->ret);\n"
				"		state->ret = NULL;\n"
				"	}\n");
		free(mach->lastrettype);
		mach->lastrettype = NULL;
	}
	if (asprintf(&mach->lastrettype, "%s_%s",
				conf->module, f->name) == -1)
		fatal("asprintf");

	/* handle return arguments */
	CODE(	"	if ((state->ret = calloc(1, sizeof(%s_out))) == NULL)\n"
			"		return TBOX_NOMEM;\n"
			"	state->retfree = free_%s_out;\n",
			mach->lastrettype,
			cm->name);
	CODE(	"	((%s_out *)state->ret)->errorcode = code;\n",
			mach->lastrettype);
	LIST_FOREACH(m, &cm->interface.out, entry) {
		if ((m->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)) ||
				type_lookup(m->type) || !(m->flags & FLAG_CONSTRUCTED))
			CODE(	"	((%s_out *)state->ret)->%s = %s;\n",
					mach->lastrettype, m->name, m->name);
		else {
			CODE(	"	if ((ret = copy_%s(&%s, &((%s_out *)state->ret)->%s)))\n"
					"		goto out;\n",
					m->type, m->name, mach->lastrettype, m->name);
			outlabel++;
		}
	}

	/* DATA block, surrounded by #line defs */
	if (act->data) {
		if (printline)
			CODE(	"#line %d \"%s\"\n", act->lineno, mach->filename);
		CODE(	"	{");
		gen_output_code(conf, mach, act, act->data);
		CODE(	"}\n");
		if (printline)
			CODE(	"#line %d \"%s/%s_%s.c\"\n",
					lineno + 1, conf->basepath, conf->stem, mach->name);
		if (mach->needout)
			CODE(	"%s_%s_out:\n",
					conf->module, mach->name);
	}

	/* place the out label when called, otherwise the compiler complains */
	if (outlabel)
		CODE(	"out:\n");

	CODE(	"	if (state->loopdetect) {\n"
			"		state->state = %d;\n"
			"		return TBEVENT_BLOCKDONE;\n"
			"	}\n"
			"	state->state = %d;\n"
			"	ret = go_%s(state);\n"
			"	return ret;\n",
			mach->state, mach->state + 1, mach->name);
	CODE(	"}\n\n");
}

/*
 * Generate a state function that always returns TBEVENT_CALLBACK while the
 * forkcounter > 0
 */
void
gen_func_forkwait(struct tbevent_conf *conf, struct machine *mach,
		struct action *act, const char *type)
{
	gen_func_declaration(conf, mach, act, type);

	CODE(	"	int %s_%s_return = TBEVENT_BLOCKDONE;\n",
			conf->module, mach->name);

	if (printlog)
		CODE(	"	log_debug(\"> %s_%s_state_%d: FORKWAIT %%d\", "
				"state->forkcounter);\n",
				type, mach->name, mach->state);

	/* CALLBACK is the default when the forkcounter > 0 */
	CODE(	"	if (state->forkcounter > 0)\n"
			"		%s_%s_return = TBEVENT_CALLBACK;\n",
			conf->module, mach->name);
	/* BLOCKDONE when no forks left */
	CODE(	"	else if (state->forkcounter == 0)\n"
			"		%s_%s_return = TBEVENT_BLOCKDONE;\n",
			conf->module, mach->name);
	CODE(	"	else {\n"
			"		log_warnx(\"> %s_%s_state_%d: FORKWAIT %%d is NEGATIVE!\", "
			"state->forkcounter);\n"
			"		%s_%s_return = TBEVENT_BLOCKDONE;\n"
			"	}\n",
			type, mach->name, mach->state,
			conf->module, mach->name);

	if (act->data) {
		/* DATA block, surrounded by #line defs */
		if (printline)
			CODE(	"#line %d \"%s\"\n", act->lineno, mach->filename);
		CODE(	"	{");
		gen_output_code(conf, mach, act, act->data);
		CODE(	"}\n");
		if (printline)
			CODE(	"#line %d \"%s/%s_%s.c\"\n",
					lineno + 1, conf->basepath, conf->stem, mach->name);
		if (mach->needout)
			CODE(	"%s_%s_out:\n",
					conf->module, mach->name);
	}

	/* avoid double-counting the state when reentering */
	CODE(	"	if (state->loopdetect == 0) {\n"
			"		state->state++;\n"
			"		go_%s(state);\n"
			"	}\n",
			mach->name);
	CODE(	"	return %s_%s_return;\n",
			conf->module, mach->name);

	gen_func_close(conf, mach, act, type);
}
