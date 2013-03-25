/*
 * Copyright (c) 2006 Christopher Maxwell.  All rights reserved.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tbevent.h"

int printline = 1;	/* generate #line directives around code block */
int printlog = 0;	/* generate log_debug() statements in functions */
int lineno;

#define DSTART(M) \
	if (debug == 1) \
		output(&lineno, conf->codefile, "/* >>>>>>>>>> %s >>>>>>>>>> */\n", (M))

#define DEND(M) \
	if (debug == 1) \
		output(&lineno, conf->codefile, "/* <<<<<<<<<< %s <<<<<<<<<< */\n", (M))

void
gen_init(struct tbevent_conf *conf)
{
	char *filename;

	if (conf->stem == NULL)
		if ((conf->stem = strdup(STEM)) == NULL)
			fatal("strdup");

	if (conf->module == NULL)
		if ((conf->module = strdup(MODULE)) == NULL)
			fatal("strdup");

	if (conf->headerbase == NULL)
		if (asprintf(&conf->headerbase, "%s_%s", conf->module, conf->stem)
				== -1)
			fatal("asprintf");

	if (asprintf(&filename, "%s.h", conf->headerbase) == -1)
		fatal("asprintf");

	if ((conf->headerfile = fopen(filename, "w")) == NULL)
		err(1, "open %s", filename);
	free(filename);

	fprintf(conf->headerfile, "/* Autogenerated - do not edit */\n\n");
	fprintf(conf->headerfile,
			"#ifndef __%s_h__\n"
			"#define __%s_h__\n\n",
			conf->headerbase, conf->headerbase);

	if ((conf->logfile = fopen(STEM "_files", "w")) == NULL)
		err(1, "open " STEM "_files");
}

/*
 * MACHINE
 *
 * Generates the state machine, source files and appends headers.
 */
void
gen_machine(struct tbevent_conf *conf, struct machine *m)
{
	char *filename;
	struct name *n;
	struct stat stx, sts;

	if (conf->codefile) {
		fclose(conf->codefile);
		conf->codefile = NULL;
	}

	lineno = 1;

	/* Log "event_COMMANDNAME.x" */
	if (asprintf(&filename, "%s_%s.x", conf->stem, m->name) == -1)
		err(1, "asprintf");
	/* attempt to detect changes */
	if (stat(filename, &stx) == 0 && stat(m->filename, &sts) == 0) {
		if (timespeccmp(&stx.st_mtimespec, &sts.st_mtimespec, >=))
			conf->codefile = fopen("/dev/null", "w");
	}
	if (conf->codefile == NULL)
		if ((conf->codefile = fopen(filename, "w")) == NULL)
			err(1, "open %s", filename);
	fprintf(conf->logfile, "%s ", filename);
	free(filename);

	CODE(	"/* Autogenerated - do not edit */\n\n");
	CODE(	"#include <stdlib.h>\n"
			"#include <string.h>\n"
			"#include <toolbox.h>\n");
	CODE(	"#include <%s.h>\n",
			conf->headerbase);
	LIST_FOREACH(n, &m->include, entry)
		CODE(	"#include \"%s.h\"\n",
				n->name);
	CODE(	"\n");

	if (m->preamble) {
		DSTART("PREAMBLE");
		CODE(	"%s\n",
				m->preamble);
		DEND("PREAMBLE");
	}

	DSTART("INTERFACE");
	gen_interface(conf, m);
	DEND("INTERFACE");

	DSTART("UTILITY");
	gen_free(conf, m);

	/* declare the state machine */
	CODE(	"static int go_%s(%s_%s_state *);\n",
			m->name, conf->module, m->name);
	DEND("UTILITY");

	/* start the state counter */
	m->state = -1;

	DSTART("ACTION");
	gen_action(conf, m, &m->action, "act");
	DEND("ACTION");

	/* set the finish state counter */
	m->error = m->state + 1;

	DSTART("FINISH");
	gen_action(conf, m, &m->finish, "fin");
	DEND("FINISH");

	DSTART("STATEMACHINE");
	gen_statemachine(conf, m);
	DEND("STATEMACHINE");

	DSTART("EXTERNAL");
	gen_extern(conf, m);
	DEND("EXTERNAL");

	fclose(conf->codefile);
	conf->codefile = NULL;
}

/*
 * INTERFACE
 *
 * Header gets a typedef TYPE_out for callback functions.
 */
void
gen_interface(struct tbevent_conf *conf, struct machine *mach)
{
	Member *m;
	struct name *n;

	/*
	 * HEADER includes, typedef
	 */
	LIST_FOREACH(n, &mach->include, entry)
		fprintf(conf->headerfile,
				"#include \"%s.h\"\n",
				n->name);
	fprintf(conf->headerfile,
			"typedef struct %s_%s_out {\n"
			"	int errorcode;\n",
			conf->module, mach->name);
	LIST_FOREACH(m, &mach->interface.out, entry)
		fprintf(conf->headerfile,
				"	%s %s%s;\n",
				m->type,
				(m->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)) ? "*" : "",
				m->name);
	fprintf(conf->headerfile,
			"} %s_%s_out;\n",
			conf->module, mach->name);

	CODE(	"typedef struct %s_%s_state {\n",
			conf->module, mach->name);

	/* IN */
	CODE(	"	struct {\n");
	LIST_FOREACH(m, &mach->interface.in, entry)
		CODE(	"		%s%s %s%s;\n",
				(m->flags & FLAG_CONSTANT) ? "const " : "",
				m->type,
				(m->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)) ? "*" : "",
				m->name);
	CODE(	"	} in;\n\n");

	/* STACK */
	CODE(	"	struct {\n");
	LIST_FOREACH(m, &mach->interface.stack, entry)
		CODE(	"		%s%s %s%s;\n",
				(m->flags & FLAG_CONSTANT) ? "const " : "",
				m->type,
				(m->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)) ? "*" : "",
				m->name);
	CODE(	"	} stack;\n\n");

	/* OUT */
	CODE(	"	/*\n"
			"	 *	struct {\n"
			"	 *		int errorcode;\n");
	LIST_FOREACH(m, &mach->interface.out, entry)
		CODE(	"	 *		%s%s %s%s;\n",
				(m->flags & FLAG_CONSTANT) ? "const " : "",
				m->type,
				(m->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)) ? "*" : "",
				m->name);
	CODE(	"	 *	} out;\n"
			"	 */\n"
			"	%s_%s_out out;\n\n",
			conf->module, mach->name);

	/* STATE */
	CODE(	"	int state;\n"
			"	int loopdetect;\n"
			"	int forkcounter;\n"
			"	void *ret;\n"
			"	void (*retfree)(void *);\n"
			"\n"
			"	EVENTCB_%s *cbfunc;\n"
			"	void *cbarg;\n"
			"} %s_%s_state;\n\n",
			mach->name, conf->module, mach->name);
}

void
gen_func_declaration(struct tbevent_conf *conf, struct machine *mach,
		struct action *act, const char *type)
{
	CODE(	"static int\n"
			"%s_%s_state_%d(%s_%s_state *state)\n"
			"{\n",
			type, mach->name, mach->state, conf->module, mach->name);
}

void
gen_func_close(struct tbevent_conf *conf, struct machine *mach,
		struct action *act, const char *type)
{
	CODE(	"}\n\n");
}

static void
gen_func_call(struct tbevent_conf *conf, struct machine *mach,
		struct action *act, const char *type)
{
	struct machine *cm;
	Member *m;
	Function *f;
	Funcarg *fa;
	int outlabel = 0;

	if ((f = LIST_FIRST(&act->functions)) == NULL)
		fatal("CALL with no function");
	/*
	 * WAIT FORWARD DECLARATION
	 */
	CODE(	"static int %s_%s_wait_%d(int, void *",
			type, mach->name, mach->state);
	TAILQ_FOREACH(cm, &conf->machines, entry)
		if (strcmp(cm->name, f->name) == 0)
			break;
	if (cm == TAILQ_END(&conf->machines))
		errx(1, "CALL(%s) not found", f->name);
	LIST_FOREACH(m, &cm->interface.out, entry)
		CODE(	",\n\t\t%s%s %s",
				(m->flags & FLAG_CONSTANT) ? "const " : "",
				m->type,
				(m->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)) ? "*" : "");
	CODE(	");\n\n");

	/*
	 * CALL FUNCTION
	 */
	gen_func_declaration(conf, mach, act, type);
	CODE(	"	int ret;\n");

	if (printlog)
		CODE(	"	log_debug(\"> %s_%s_state_%d: CALL\");\n",
				type, mach->name, mach->state);

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

	CODE(	"	return ret;\n");
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
		errx(1, "CALL(%s) not found", f->name);
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
		CODE(	"	log_debug(\"> %s_%s_wait_%d: RETURN: %%s\",\n"
				"			tb_error(code));\n",
				type, mach->name, mach->state);

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
		if ((m->flags & FLAG_OPTIONAL) && (m->flags & FLAG_CONSTRUCTED) &&
				type_lookup(m->type) == 0) {
			CODE(	"	if (%s) {\n"
					"		if ((((%s_out *)state->ret)->%s = calloc(1, sizeof(*%s))) == NULL)\n"
					"			goto out;\n"
					"		if ((ret = copy_%s(%s, ((%s_out *)state->ret)->%s)))\n"
					"			goto out;\n"
					"	}\n",
					m->name,
					mach->lastrettype, m->name, m->name,
					m->type, m->name, mach->lastrettype, m->name);
			outlabel++;
		} else if ((m->flags & FLAG_REFERENCE) ||
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

static void
gen_func_code(struct tbevent_conf *conf, struct machine *mach,
		struct action *act, const char *type)
{
	mach->needout = 0;
	gen_func_declaration(conf, mach, act, type);

	CODE(	"	int %s_%s_return = TBEVENT_BLOCKDONE;\n",
			conf->module, mach->name);

	if (printlog)
		CODE(	"	log_debug(\"> %s_%s_state_%d: CODE\");\n",
				type, mach->name, mach->state);

	/* DATA block, surrounded by #line defs */
	if (printline) {
		CODE(	"#line %d \"%s\"\n", act->lineno, mach->filename);
		if (act->lineno == 0)
			dump_action(act, mach);
	}
	CODE(	"	{");
	gen_output_code(conf, mach, act, act->data);
	CODE(	"}\n");
	if (printline)
		CODE(	"#line %d \"%s/%s_%s.c\"\n",
				lineno + 1, conf->basepath, conf->stem, mach->name);

	if (mach->needout)
		CODE(	"%s_%s_out:\n",
				conf->module, mach->name);
	CODE(	"	return %s_%s_return;\n",
			conf->module, mach->name);

	gen_func_close(conf, mach, act, type);
}

static void
gen_func_continue(struct tbevent_conf *conf, struct machine *mach,
		struct action *act, const char *type)
{
	gen_func_declaration(conf, mach, act, type);

	if (printlog)
		CODE(	"	log_debug(\"> %s_%s_state_%d: CONTINUE\");\n",
				type, mach->name, mach->state);

	CODE(	"	state->state = %d;\n",
			mach->retrystate);
	CODE(	"	if (state->ret) {\n"
			"		if (state->retfree)\n"
			"			state->retfree(state->ret);\n"
			"		free(state->ret);\n"
			"		state->ret = NULL;\n"
			"	}\n");
	CODE(	"	return TBEVENT_BLOCKDONE;\n");

	gen_func_close(conf, mach, act, type);
}

static void
gen_func_copy(struct tbevent_conf *conf, struct machine *mach,
		struct action *act, const char *type)
{
	Function *f;
	Funcarg *fa_src, *fa_dst;
	struct member_list *int_src = NULL, *int_dst = NULL;
	char *c, *p;
	struct machine *cm;
	Member *m_src = NULL, *m_dst = NULL;
	int outlabel = 0;
	char srcbuf[4096], dstbuf[4096];

	if ((f = LIST_FIRST(&act->functions)) == LIST_END(&act->functions))
		fatal("COPY with no arguments");
	if ((fa_src = LIST_FIRST(&f->args)) == LIST_END(&f->args))
		fatal("COPY missing source argument");
	if ((fa_dst = LIST_NEXT(fa_src, entry)) == LIST_END(&f->args))
		fatal("COPY missing destination argument");

#define allowed_in_replace(x) \
	(isalnum(x) || x == '_')

#define compare(c, t, p) \
		((uint32_t)((p) - (t)) >= strlen(c) && strncmp(c, t, p - t) == 0)

	/*
	 * SRC
	 */
	c = fa_src->name;
	p = srcbuf;
	while (*c != '\0' && !allowed_in_replace(*c))
		*p++ = *c++;
	*p = '\0';
	p = c;
	while (*c != '\0' && allowed_in_replace(*c))
		c++;

	/* replace KEYWORD with state pointer */
	if (compare("IN", p, c)) {
		int_src = &mach->interface.in;
		strlcat(srcbuf, "((&state->in))", sizeof(srcbuf));
	} else if (compare("OUT", p, c)) {
		int_src = &mach->interface.out;
		strlcat(srcbuf, "((&state->out))", sizeof(srcbuf));
	} else if (compare("STACK", p, c)) {
		int_src = &mach->interface.stack;
		strlcat(srcbuf, "((&state->stack))", sizeof(srcbuf));
	} else if (compare("RET", p, c)) {
		/* have to search for the correct machine */
		if (mach->lastrettype == NULL)
			fatalx("COPY attempting to use UNDEFINED return type for source");
		/* forward past 'module_' */
		p = mach->lastrettype + MIN(strlen(mach->lastrettype),
				strlen(conf->module) + 1);
		TAILQ_FOREACH(cm, &conf->machines, entry)
			if (strcmp(cm->name, p) == 0)
				break;
		if (cm == TAILQ_END(&conf->machines))
			fatalx("COPY cannot find machine for source type");
		int_src = &cm->interface.out;
		strlcat(srcbuf, "((", sizeof(srcbuf));
		strlcat(srcbuf, mach->lastrettype ? mach->lastrettype : "UNDEFINED",
				sizeof(srcbuf));
		strlcat(srcbuf, "_out *)state->ret)", sizeof(srcbuf));
	} else if (compare("ERRORCODE", p, c)) {
		strlcat(srcbuf, "(state->out.errorcode)", sizeof(srcbuf));
	} else
		snprintf(srcbuf + strlen(srcbuf), sizeof(srcbuf) - strlen(srcbuf),
				"%.*s", (int)(c - p), p);

	/*
	 * SRC NAME
	 */

	if (int_src) {
		/* skip over reference tokens '.' '->' etc */
		p = srcbuf + strlen(srcbuf);
		while (*c != '\0' && !allowed_in_replace(*c))
			*p++ = *c++;
		*p = '\0';

		/* find the end of the name */
		p = c;
		while (*p != '\0' && allowed_in_replace(*p))
			p++;

		LIST_FOREACH(m_src, int_src, entry)
			if (compare(m_src->name, c, p))
				break;
		if (m_src == LIST_END(int_src))
			errx(1, "COPY (%s:%s) cannot find named source variable \"%.*s\"",
					conf->module, mach->name, (int)(p - c), c);

		/* the name must exist, but is only used if terminal */
		if (*p != '\0')
			m_src = NULL;
	}
	if (strlcat(srcbuf, c, sizeof(srcbuf)) >= sizeof(srcbuf))
		err(1, "source name overflow");

	/*
	 * DST
	 */
	c = fa_dst->name;
	p = dstbuf;
	while (*c != '\0' && !allowed_in_replace(*c))
		*p++ = *c++;
	*p = '\0';
	p = c;
	while (*c != '\0' && allowed_in_replace(*c))
		c++;

	/* replace KEYWORD with state pointer */
	if (compare("IN", p, c)) {
		int_dst = &mach->interface.in;
		strlcat(dstbuf, "((&state->in))", sizeof(dstbuf));
	} else if (compare("OUT", p, c)) {
		int_dst = &mach->interface.out;
		strlcat(dstbuf, "((&state->out))", sizeof(dstbuf));
	} else if (compare("STACK", p, c)) {
		int_dst = &mach->interface.stack;
		strlcat(dstbuf, "((&state->stack))", sizeof(dstbuf));
	} else if (compare("RET", p, c)) {
		/* have to search for the correct machine */
		if (mach->lastrettype == NULL)
			fatalx("COPY attempting to use UNDEFINED return type for source");
		/* forward past 'module_' */
		p = mach->lastrettype + MIN(strlen(mach->lastrettype),
				strlen(conf->module) + 1);
		TAILQ_FOREACH(cm, &conf->machines, entry)
			if (strcmp(cm->name, p) == 0)
				break;
		if (cm == TAILQ_END(&conf->machines))
			fatalx("COPY cannot find machine for destination type");
		int_dst = &cm->interface.out;
		strlcat(dstbuf, "((", sizeof(dstbuf));
		strlcat(dstbuf, mach->lastrettype ? mach->lastrettype : "UNDEFINED",
				sizeof(dstbuf));
		strlcat(dstbuf, "_out *)state->ret)", sizeof(dstbuf));
	} else if (compare("ERRORCODE", p, c)) {
		strlcat(dstbuf, "(state->out.errorcode)", sizeof(dstbuf));
	} else
		snprintf(dstbuf + strlen(dstbuf), sizeof(dstbuf) - strlen(dstbuf),
				"%.*s", (int)(c - p), p);

	/*
	 * DST NAME
	 */

	if (int_dst) {
		/* skip over reference tokens '.' '->' etc */
		p = dstbuf + strlen(dstbuf);
		while (*c != '\0' && !allowed_in_replace(*c))
			*p++ = *c++;
		*p = '\0';

		/* find the end of the name */
		p = c;
		while (*p != '\0' && allowed_in_replace(*p))
			p++;

		LIST_FOREACH(m_dst, int_dst, entry)
			if (compare(m_dst->name, c, p))
				break;
		if (m_dst == LIST_END(int_dst))
			errx(1, "COPY cannot find named destination variable \"%.*s\"",
					(int)(p - c), c);

		/* the name must exist, but is only used if terminal */
		if (*p != '\0')
			m_dst = NULL;
	}
	if (strlcat(dstbuf, c, sizeof(dstbuf)) >= sizeof(dstbuf))
		err(1, "destination name overflow");

#undef allowed_in_replace
#undef compare

	/*
	 * Copy uses the DESTINATION as the arbitrary decider for how to copy the
	 * variable.
	 */

	gen_func_declaration(conf, mach, act, type);
	CODE(	"	int ret = TBEVENT_SUCCESS;\n");

	if (printlog)
		CODE(	"	log_debug(\"> %s_%s_state_%d: COPY\");\n",
				type, mach->name, mach->state);

	if (m_src && m_src->flags & FLAG_OPTIONAL) {
		if (m_dst == NULL || type_lookup(m_dst->type) ||
				!(m_dst->flags & FLAG_CONSTRUCTED))
			CODE(	"	if (%s)\n"
					"		%s = %s;\n",
					srcbuf, dstbuf, srcbuf);
		else {
			CODE(	"	if (%s)\n"
					"		if ((ret = copy_%s(%s%s, %s%s)))\n"
					"			goto out;\n",
					srcbuf,
					m_dst->type,
					(m_src->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)) ? "" : "&",
					srcbuf,
					(m_dst->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)) ? "" : "&",
					dstbuf);
			outlabel++;
		}
	} else {
		if (m_dst == NULL || type_lookup(m_dst->type) ||
				!(m_dst->flags & FLAG_CONSTRUCTED))
			CODE(	"	%s = %s;\n",
					dstbuf, srcbuf);
		else {
			char *opt = "&";
			if (m_src == NULL || (m_src->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)))
				opt = "";

			CODE(	"	if ((ret = copy_%s(%s%s, %s%s)))\n"
					"		goto out;\n",
					m_dst->type,
					opt,
					srcbuf,
					(m_dst->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)) ? "" : "&",
					dstbuf);
			outlabel++;
		}
	}

	if (outlabel)
		CODE(	"out:\n");
	CODE(	"	if (ret != TBEVENT_SUCCESS) {\n"
			"		state->out.errorcode = ret;\n"
			"		return TBEVENT_FINISH;\n"
			"	}\n"
			"	return TBEVENT_BLOCKDONE;\n");

	gen_func_close(conf, mach, act, type);
}

static void
gen_func_retry(struct tbevent_conf *conf, struct machine *mach,
		struct action *act, const char *type)
{
	if (mach->retrystate)
		fatal("nested RETRY not yet implemented");
	mach->retrystate = mach->state;

	gen_func_declaration(conf, mach, act, type);

	if (printlog)
		CODE(	"	log_debug(\"> %s_%s_state_%d: RETRY\");\n",
				type, mach->name, mach->state);

	CODE(	"	return TBEVENT_BLOCKDONE;\n");

	gen_func_close(conf, mach, act, type);

	gen_action(conf, mach, &act->actions, type);

	mach->retrystate = 0;

	CODE(	"/* END RETRY */\n");
}

/*
 * RETURN sends a value back to the state engine.
 */
static void
gen_func_return(struct tbevent_conf *conf, struct machine *mach,
		struct action *act, const char *type)
{
	gen_func_declaration(conf, mach, act, type);

	if (printlog)
		CODE(	"	log_debug(\"> %s_%s_state_%d: RETURN\");\n",
				type, mach->name, mach->state);

	CODE(	"	state->out.errorcode = ");
	gen_output_code(conf, mach, act, act->data);
	CODE(	";\n");

	CODE(	"	return TBEVENT_FINISH;\n");

	gen_func_close(conf, mach, act, type);
}

static void
gen_func_sync(struct tbevent_conf *conf, struct machine *mach,
		struct action *act, const char *type)
{
	mach->needout = 0;
	gen_func_declaration(conf, mach, act, type);

	CODE(	"	int %s_%s_return = TBEVENT_BLOCKDONE;\n",
			conf->module, mach->name);

	if (printlog)
		CODE(	"	log_debug(\"> %s_%s_state_%d: SYNC\");\n",
				type, mach->name, mach->state);

	/* DATA block, surrounded by #line defs */
	if (printline) {
		CODE(	"#line %d \"%s\"\n", act->lineno, mach->filename);
		if (act->lineno == 0)
			dump_action(act, mach);
	}
	CODE(	"	{");
	gen_output_code(conf, mach, act, act->data);
	CODE(	"}\n");
	if (printline)
		CODE(	"#line %d \"%s/%s_%s.c\"\n",
				lineno + 1, conf->basepath, conf->stem, mach->name);

	if (mach->needout)
		CODE(	"%s_%s_out:\n",
				conf->module, mach->name);
	CODE(	"	return %s_%s_return;\n",
			conf->module, mach->name);

	gen_func_close(conf, mach, act, type);
}

static void
gen_func_test(struct tbevent_conf *conf, struct machine *mach,
		struct action *act, const char *type)
{
	int skipcount = count_subactions(act, 0);

	gen_func_declaration(conf, mach, act, type);
	if (printlog)
		CODE(	"	log_debug(\"> %s_%s_state_%d: TEST\");\n",
				type, mach->name, mach->state);

	CODE(	"	if (");
	gen_output_code(conf, mach, act, act->data);
	CODE(	") {\n"
			"		return TBEVENT_BLOCKDONE;\n"
			"	} else {\n"
			"		state->state = %d;\n"
			"		return TBEVENT_BLOCKDONE;\n"
			"	}\n",
			mach->state + skipcount);

	CODE(	"	return TBEVENT_BLOCKDONE;\n");
	gen_func_close(conf, mach, act, type);

	gen_action(conf, mach, &act->actions, type);

	CODE(	"/* END TEST */\n");
}

/*
 * WAIT functions are callbacks from external async events.  They should
 * deal with reply data and push it onto OUT or STACK.
 */
static void
gen_func_wait(struct tbevent_conf *conf, struct machine *mach,
		struct action *act, const char *type)
{
	Function *f;
	Funcarg *fa;

	mach->waitstate = mach->state;
	mach->needout = 0;

	/* generate the wait return function */
	f = LIST_FIRST(&act->functions);
	CODE(	"static %s\n"
			"%s_%s_wait_%d(",
			f->name, type, mach->name, mach->state);
	LIST_FOREACH(fa, &f->args, entry) {
		CODE(	"%s%s ",
				fa == LIST_FIRST(&f->args) ? "" : ",\n\t\t",
				fa->type);
		gen_output_name(conf, mach, fa->name);
	}
	CODE(	")\n"
			"{\n"
			"	%s_%s_state *state = (%s_%s_state *)%s_%s_arg;\n",
			conf->module, mach->name, conf->module, mach->name,
			conf->module, mach->name);
	if (strcasecmp(f->name, "void") != 0)
		CODE(	"	%s %s_%s_return;\n"
				"	bzero(&%s_%s_return, sizeof(%s_%s_return));\n",
				f->name, conf->module, mach->name,
				conf->module, mach->name, conf->module, mach->name);

	if (printlog)
		CODE(	"	log_debug(\"> %s_%s_wait_%d: WAIT RETURN\");\n",
				type, mach->name, mach->state);

	/* DATA block, surrounded by #line defs */
	if (printline) {
		CODE(	"#line %d \"%s\"\n", act->lineno, mach->filename);
		if (act->lineno == 0)
			dump_action(act, mach);
	}
	CODE(	"	{");
	gen_output_code(conf, mach, act, act->data);
	CODE(	"}\n");
	if (printline)
		CODE(	"#line %d \"%s/%s_%s.c\"\n",
				lineno + 1, conf->basepath, conf->stem, mach->name);

	if (mach->needout)
		CODE(	"%s_%s_out:\n",
				conf->module, mach->name);
	/* avoid double-counting the state when reentering */
	CODE(	"	if (state->loopdetect == 0) {\n"
			"		state->state++;\n"
			"		go_%s(state);\n"
			"	}\n",
			mach->name);
	if (strcasecmp(f->name, "void") != 0)
		CODE(	"	return %s_%s_return;\n",
				conf->module, mach->name);
	CODE(	"}\n\n");

	/*
	 * WAIT SETUP
	 */
	gen_func_declaration(conf, mach, act, type);
	if (printlog)
		CODE(	"	log_debug(\"> %s_%s_state_%d: WAIT\");\n",
				type, mach->name, mach->state);
	CODE(	"	return TBEVENT_BLOCKDONE;\n");
	gen_func_close(conf, mach, act, type);
}

void
gen_action(struct tbevent_conf *conf, struct machine *mach,
		struct action_list *list, const char *type)
{
	Action *a;

	if (LIST_EMPTY(list)) {
		mach->state++;
		CODE(	"/*\n"
				" * Function: EMPTY\n"
				" * RET Type: %s\n"
				" * Wait ptr: %d\n"
				" * Rtry ptr: %d\n"
				" */\n",
				mach->lastrettype ? mach->lastrettype : "none",
				mach->waitstate, mach->retrystate);
		gen_func_declaration(conf, mach, NULL, type);
		CODE(	"	return TBEVENT_BLOCKDONE;\n");
		gen_func_close(conf, mach, NULL, type);
	}

	LIST_FOREACH(a, list, entry) {
		mach->state++;

		CODE(	"/*\n"
				" * Function: %s\n"
				" * RET Type: %s\n"
				" * Wait ptr: %d\n"
				" * Rtry ptr: %d\n"
				" */\n",
				type_function(a->type),
				mach->lastrettype ? mach->lastrettype : "none",
				mach->waitstate, mach->retrystate);

		switch (a->type) {
			case FUNC_NONE:
				break;

			case FUNC_CALL:
				gen_func_call(conf, mach, a, type);
				break;

			case FUNC_CODE:
				gen_func_code(conf, mach, a, type);
				break;

			case FUNC_CONTINUE:
				gen_func_continue(conf, mach, a, type);
				break;

			case FUNC_COPY:
				gen_func_copy(conf, mach, a, type);
				break;

			case FUNC_RETRY:
				gen_func_retry(conf, mach, a, type);
				break;

			case FUNC_RETURN:
				gen_func_return(conf, mach, a, type);
				break;

			case FUNC_SYNC:
				gen_func_sync(conf, mach, a, type);
				break;

			case FUNC_TEST:
				gen_func_test(conf, mach, a, type);
				break;

			case FUNC_WAIT:
				gen_func_wait(conf, mach, a, type);
				break;

			case FUNC_FORK:
				gen_func_fork(conf, mach, a, type);
				break;

			case FUNC_FORKWAIT:
				gen_func_forkwait(conf, mach, a, type);
				break;
		}
	}
}

/*
 * STATE MACHINE
 */
void
gen_statemachine(struct tbevent_conf *conf, struct machine *mach)
{
	int u;
	Member *m;

	CODE(	"static int\n"
			"go_%s(%s_%s_state *state)\n"
			"{\n"
			"	int ret = TBEVENT_SUCCESS;\n"
			"\n"
			"	if (state->loopdetect)\n"
			"		return TBEVENT_SUCCESS;\n"
			"	else\n"
			"		state->loopdetect++;\n"
			"\n"
			"top:\n"
			"	switch (state->state) {\n",
			mach->name, conf->module, mach->name);

	for (u = 0; u <= mach->state; u++) {
		if (u < mach->error)
			CODE(	"		case %d:\n"
					"			ret = act_%s_state_%d(state);\n"
					"			break;\n",
					u, mach->name, u);
		else
			CODE(	"		case %d:\n"
					"			ret = fin_%s_state_%d(state);\n"
					"			break;\n",
					u, mach->name, u);
	}
	CODE(	"		case %d:\n"
			"			ret = TBEVENT_FINISH;\n"
			"			break;\n"
			"		default:\n"
			"			log_warnx(\"go_%s: invalid state %%d\", state->state);\n"
			"			ret = TBOX_API_INVALID;\n"
			"			goto out;\n"
			"	}\n",
			mach->state + 1, mach->name);

	if (printlog)
		CODE(	"	log_debug(\"< %s STATE %%d: %%s\",\n"
				"			state->state, tb_error(ret));\n",
				mach->name);

	/*
	 * BLOCKDONE:	Finished processing the state function, it did not need
	 * 				a callback to finish.  Move on to next state.
	 * CALLBACK:	State function requires an external callback, suspend
	 * 				processing and return.
	 * FINISH:		If not yet in finish, jump ahead to finish.  If in finish,
	 * 				skip to end.
	 * !SUCCESS:	Something bad happened.  If processing ACTIONS, jump to
	 * 				FINISH, otherwise just continue.
	 */
	CODE(	"	if (ret == TBEVENT_BLOCKDONE || ret == TBEVENT_SUCCESS) {\n"
			"		state->state++;\n"
			"		if (state->state >= 0 && state->state <= %d)\n"
			"			goto top;\n",
			mach->state);
	CODE(	"	} else if (ret == TBEVENT_CALLBACK) {\n"
			"		ret = TBEVENT_CALLBACK;\n"
			"		goto out;\n");
	CODE(	"	} else if (ret == TBEVENT_FINISH) {\n"
			"		if (state->state >= 0 && state->state < %d)\n"
			"			state->state = %d;\n"
			"		else\n"
			"			state->state = %d + 1;\n"
			"		if (state->state >= 0 && state->state <= %d)\n"
			"			goto top;\n",
			mach->error, mach->error, mach->state, mach->state);
	CODE(	"	} else if (ret != TBEVENT_SUCCESS) {\n"
			"		if (state->state >= 0 && state->state < %d)\n"
			"			state->state = %d;\n"
			"		else\n"
			"			state->state++;\n"
			"		if (state->state >= 0 && state->state <= %d)\n"
			"			goto top;\n"
			"	}\n",
			mach->error, mach->error, mach->state);

	/*
	 * The state machine has finished processing all states, run the user-
	 * supplied callback and free all state.  The return code should be
	 * propagated since recursion is quite likely.
	 */
	CODE(	"	if (state->state >= 0 && state->state > %d) {\n"
			"		if (state->cbfunc)\n",
			mach->state);
	CODE(	"			ret = state->cbfunc(state->out.errorcode, state->cbarg");
	LIST_FOREACH(m, &mach->interface.out, entry)
		CODE(	",\n			\t\tstate->out.%s",
				m->name);
	CODE(");\n");

	CODE(	"		free_%s_state(state);\n"
			"		free(state);\n"
			"		state = NULL;\n"
			"	}\n",
			mach->name);

	if (printlog)
		CODE(	"	log_debug(\"<<<<< %s: END\");\n",
				mach->name);

	/*
	 * FILTER the return codes, this prevents return leakage back through CALL
	 * and WAIT segments, but allows terrible errors to propagate.
	 */
	CODE(	"out:\n"
			"	if (state)\n"
			"		state->loopdetect--;\n"
			"	if (ret == TBEVENT_SUCCESS || ret == TBEVENT_BLOCKDONE ||\n"
			"			ret == TBEVENT_CALLBACK)\n"
			"		return ret;\n"
			"	else\n"
			"		log_warnx(\"%s: %%s\", tb_error(ret));\n"
			"	return TBEVENT_BLOCKDONE;\n"
			"}\n",
			mach->name);
}

void
gen_free(struct tbevent_conf *conf, struct machine *mach)
{
	Member *m;

	/*
	 * FREEDOM!
	 */
	CODE(	"static void\n"
			"free_%s_state(%s_%s_state *state)\n"
			"{\n",
			mach->name, conf->module, mach->name);
	LIST_FOREACH(m, &mach->interface.stack, entry) {
		if ((m->flags & FLAG_CONSTRUCTED)) {
			if ((m->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)))
				CODE(	"	if (state->stack.%s) {\n"
						"		free_%s(state->stack.%s);\n"
						"		free(state->stack.%s);\n"
						"	}\n",
						m->name,
						m->type, m->name,
						m->name);
			else
				CODE(	"	free_%s(&state->stack.%s);\n",
					m->type,
					m->name);
		}
	}
	CODE(	"	if (state->ret) {\n"
			"		if (state->retfree)\n"
			"			state->retfree(state->ret);\n"
			"		free(state->ret);\n"
			"		state->ret = NULL;\n"
			"	}\n"
			"	free_%s_out(&state->out);\n",
			mach->name);
	CODE(	"}\n\n");

	CODE(	"void\n"
			"free_%s_out(void *arg)\n"
			"{\n",
			mach->name);
	LIST_FOREACH(m, &mach->interface.out, entry) {
		if ((m->flags & FLAG_OPTIONAL) && (m->flags & FLAG_CONSTRUCTED) &&
				type_lookup(m->type) == 0)
			CODE(	"	if (((%s_%s_out *)arg)->%s) {\n"
					"		free_%s(((%s_%s_out *)arg)->%s);\n"
					"		free(((%s_%s_out *)arg)->%s);\n"
					"		((%s_%s_out *)arg)->%s = NULL;\n"
					"	}\n",
					conf->module, mach->name, m->name,
					m->type, conf->module, mach->name, m->name,
					conf->module, mach->name, m->name,
					conf->module, mach->name, m->name);
		else if ((m->flags & FLAG_REFERENCE) ||
				type_lookup(m->type) || !(m->flags & FLAG_CONSTRUCTED))
			CODE(	"	/* ((%s_%s_out *)arg)->%s */\n",
					conf->module, mach->name, m->name);
		else
			CODE(	"	free_%s(&((%s_%s_out *)arg)->%s);\n",
					m->type, conf->module, mach->name, m->name);
	}
	CODE(	"}\n\n");
}

void
gen_extern(struct tbevent_conf *conf, struct machine *mach)
{
	Member *m;

	/*
	 * CALLBACK
	 */
	fprintf(conf->headerfile,
			"typedef int (EVENTCB_%s)(int, void *",
			mach->name);
	LIST_FOREACH(m, &mach->interface.out, entry)
		fprintf(conf->headerfile,
				",\n\t\t%s%s%s",
				(m->flags & FLAG_CONSTANT) ? "const " : "",
				m->type,
				(m->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)) ? " *" : "");
	fprintf(conf->headerfile,
			");\n");

	/*
	 * DEFINITION
	 */
	fprintf(conf->headerfile,
			"int %s(EVENTCB_%s *, void *",
			mach->name, mach->name);
	LIST_FOREACH(m, &mach->interface.in, entry)
		fprintf(conf->headerfile,
				",\n\t\t%s%s%s",
				(m->flags & FLAG_CONSTANT) ? "const " : "",
				m->type,
				(m->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)) ? " *" : "");
	fprintf(conf->headerfile,
			");\n");
	fprintf(conf->headerfile,
			"void free_%s_out(void *);\n",
			mach->name);
	fprintf(conf->headerfile, "\n");

	/*
	 * CODE
	 */
	CODE(	"int\n"
			"%s(EVENTCB_%s *callback, void *arg",
			mach->name, mach->name);
	LIST_FOREACH(m, &mach->interface.in, entry)
		CODE(	",\n\t\t%s%s %s%s",
				(m->flags & FLAG_CONSTANT) ? "const " : "",
				m->type,
				(m->flags & (FLAG_REFERENCE | FLAG_OPTIONAL)) ? "*" : "",
				m->name);
	CODE(	")\n"
			"{\n");

	CODE(	"	int ret = TBEVENT_SUCCESS;\n"
			"	%s_%s_state *state;\n",
			conf->module, mach->name);

	if (printlog)
		CODE(	"	log_debug(\">>>>> %s: BEGIN\");\n",
				mach->name);

	CODE(	"	if ((state = calloc(1, sizeof(*state))) == NULL) {\n"
			"		ret = TBOX_NOMEM;\n"
			"		goto fail;\n"
			"	}\n");

	/* COPY arguments */
	CODE(	"	state->cbfunc = callback;\n"
			"	state->cbarg = arg;\n");
	LIST_FOREACH(m, &mach->interface.in, entry)
		if (m->flags & FLAG_OPTIONAL) {
			if ((m->flags & FLAG_REFERENCE) || type_lookup(m->type) ||
					!(m->flags & FLAG_CONSTRUCTED))
				CODE(	"	if (%s)\n"
						"		state->in.%s = %s;\n",
						m->name, m->name, m->name);
			else
				CODE(	"	if (%s)\n"
						"		if ((ret = copy_%s(&%s, &state->in.%s)))\n"
						"			goto fail;\n",
						m->name, m->type, m->name, m->name);
		} else {
			if ((m->flags & FLAG_REFERENCE) || type_lookup(m->type) ||
					!(m->flags & FLAG_CONSTRUCTED))
				CODE(	"	state->in.%s = %s;\n",
						m->name, m->name);
			else
				CODE(	"	if ((ret = copy_%s(&%s, &state->in.%s)))\n"
						"		goto fail;\n",
						m->type, m->name, m->name);
		}

	CODE(	"	ret = go_%s(state);\n",
			mach->name);

	/* OUT */
	CODE(	"	return ret;\n\n");

	/* FAIL */
	CODE(	"fail:\n"
			"	if (state) {\n"
			"		free_%s_state(state);\n"
			"		free(state);\n"
			"	}\n"
			"	return ret;\n",
			mach->name);

	CODE(	"}\n");
}

void
gen_close(struct tbevent_conf *conf)
{
	fprintf(conf->headerfile, "#endif /* __%s_h__ */\n",
			conf->headerbase);
	fclose(conf->headerfile);

	fprintf(conf->logfile, "\n");
	fclose(conf->logfile);
}

void
gen_output_code(struct tbevent_conf *conf, struct machine *mach,
		struct action *act, char *code)
{
	char buf[8192];
	char *c, *p, *t;
	int endc;
	char *append = NULL;
	char appendbuf[1024];

#define overflow(p, buf) \
	(p + 1 >= buf + sizeof(buf) - 1)

	c = code;
	p = buf;
	while (*c != '\0' && !overflow(p, buf)) {
		/* skip over comments and escaped characters */
		switch (*c) {
			case '\\':
				*p++ = *c++;
				if (*c == '\0')
					goto out;
				if (overflow(p, buf))
					fatal("code too long");
				if (*c == '\'' || *c == '\"')
					*p++ = *c++;
				break;

			case '\'':
			case '"':
				endc = *p++ = *c++;
				while (1) {
					if (*c == '\0')
						goto out;
					if (overflow(p, buf))
						fatal("code too long");

					/* do not match escaped version */
					if (*c == endc && *(c - 1) != '\\') {
						*p = '\0';
						break;
					}
					*p++ = *c++;
				}
				break;
		}

#define allowed_in_replace(x) \
		(isalnum(x) || x == '_')

		t = p;
		while (*c != '\0' && !overflow(p, buf) && allowed_in_replace(*c))
			*p++ = *c++;

#define compare(c, t, p) \
		((uint32_t)((p) - (t)) >= strlen(c) && strncmp(c, t, p - t) == 0)

		*p = '\0';
		if (p - t > 0) {
			if (compare("IN", t, p)) {							/* IN */
				*t = '\0';
				CODE("%s(&(state->in))", buf);
			} else if (compare("OUT", t, p)) {					/* OUT */
				*t = '\0';
				CODE("%s(&(state->out))", buf);
			} else if (compare("STACK", t, p)) {				/* STACK */
				*t = '\0';
				CODE("%s(&(state->stack))", buf);
			} else if (compare("RET", t, p)) {					/* RET */
				*t = '\0';
				CODE("%s((%s_out *)state->ret)", buf,
						mach->lastrettype ? mach->lastrettype : "UNDEFINED");
			} else if (compare("CBFUNC", t, p)) {				/* CBFUNC */
				*t = '\0';
				CODE("%sact_%s_wait_%d", buf,
						mach->name, mach->waitstate);
			} else if (compare("CBARG", t, p)) {				/* CBARG */
				*t = '\0';
				CODE("%s(state)", buf);
			} else if (compare("ERRORCODE", t, p)) {			/* ERRORCODE */
				*t = '\0';
				CODE("%s(state->out.errorcode)", buf);
			} else if (compare("CALLBACK", t, p)) {				/* CALLBACK */
				*t = '\0';
				CODE("%sreturn TBEVENT_CALLBACK", buf);
			} else if (compare("return", t, p)) {				/* return */
				*t = '\0';
				CODE("%s{ state->out.errorcode =", buf);
				/* when in WAIT, do not set the callback return value */
				if (act && act->type == FUNC_WAIT)
					snprintf(appendbuf, sizeof(appendbuf),
							" goto %s_%s_out; }",
							conf->module, mach->name);
				else
					snprintf(appendbuf, sizeof(appendbuf),
							" %s_%s_return = TBEVENT_FINISH; goto %s_%s_out; }",
							conf->module, mach->name,
							conf->module, mach->name);
				append = appendbuf;
				mach->needout = 1;
			} else if (compare("RETURN", t, p)) {				/* RETURN */
				Function *f;
				*t = '\0';
				if ((f = LIST_FIRST(&act->functions)) &&
						strcasecmp(f->name, "void") == 0) {
					CODE("%sgoto %s_%s_out", buf,
							conf->module, mach->name);
				} else {
					CODE("%s{ %s_%s_return =", buf,
							conf->module, mach->name);
					snprintf(appendbuf, sizeof(appendbuf),
							" goto %s_%s_out; }",
							conf->module, mach->name);
					append = appendbuf;
					mach->needout = 1;
				}
				if (act && act->type != FUNC_WAIT)
					errx(1, "%s:%d RETURN in code/sync segment",
							mach->filename, act->lineno);
			} else if (compare("CONTINUE", t, p)) {				/* CONTINUE */
				*t = '\0';
				CODE("%s{ state->state = %d; ", buf, mach->retrystate);
				CODE("%s_%s_return = TBEVENT_BLOCKDONE; ",
						conf->module, mach->name);
				CODE("if (state->ret) { "
						"if (state->retfree) "
						"	state->retfree(state->ret); "
						"free(state->ret); "
						"state->ret = NULL; "
						"} ");
				CODE("goto %s_%s_out; }",
						conf->module, mach->name);
				mach->needout = 1;
			} else
				CODE("%s", buf);
		} else
			CODE("%s", buf);

		p = buf;

		/* awful hack to append a statement */
		if (append && *c == ';') {
			CODE(";%s", append);
			append = NULL;
			c++;
			continue;
		}

		if (*c == '\0') {
			*p = '\0';
			break;
		}
		*p++ = *c++;
	}
	*p = '\0';

#undef compare
#undef allowed_in_replace
#undef overflow

out:
	CODE("%s", buf);
}

/*
 * Same as gen_output_code, but translates variable names instead of entire
 * code blocks.
 */
void
gen_output_name(struct tbevent_conf *conf, struct machine *mach,
		char *name)
{
	char buf[8192];
	char *c, *p, *t;
	int endc;

#define overflow(p, buf) \
	(p + 1 >= buf + sizeof(buf) - 1)

	c = name;
	p = buf;
	while (*c != '\0' && !overflow(p, buf)) {
		/* skip over comments and escaped characters */
		switch (*c) {
			case '\'':
			case '"':
				endc = *p++ = *c++;
				while (1) {
					if (*c == '\0')
						goto out;
					if (overflow(p, buf))
						fatal("code too long");
					if (*c == endc) {
						*p = '\0';
						break;
					}
					*p++ = *c++;
				}
				break;
		}

#define allowed_in_replace(x) \
		(isalnum(x) || x == '_')

		t = p;
		while (*c != '\0' && !overflow(p, buf) && allowed_in_replace(*c))
			*p++ = *c++;

#define compare(c, t, p) \
		((uint32_t)((p) - (t)) >= strlen(c) && strncmp(c, t, p - t) == 0)

		*p = '\0';
		if (p - t > 0) {
			if (compare("CBARG", t, p)) {
				*t = '\0';
				CODE("%s%s_%s_arg", buf,
						conf->module, mach->name);
			} else
				CODE("%s", buf);
		} else
			CODE("%s", buf);
		p = buf;

		if (*c == '\0') {
			*p = '\0';
			break;
		}

		*p++ = *c++;
	}
	*p = '\0';

#undef compare
#undef allowed_in_replace
#undef overflow

out:
	CODE("%s", buf);
}
