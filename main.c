/*
 * Copyright (c) 2006 Christopher Maxwell.  All rights reserved.
 */
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tbevent.h"

static struct tbevent_conf conf;
int debug = 2;
int verbose = 0;
extern int printlog;
static const char *module = NULL;
const char *debugmachine = NULL;

void dump_actions(struct action_list *, int);

static void
usage(void)
{
	extern const char *__progname;
	fprintf(stderr, "usage: %s [-dv] [-D machine ] [-m module] filename\n",
			__progname);
	exit(1);
}

int
main(int argc, char **argv)
{
	int ch, u;
	struct machine *mach;
	char path[MAXPATHLEN];

	while ((ch = getopt(argc, argv, "D:dm:v")) != -1)
		switch (ch) {
			case 'D':
				debugmachine = optarg;
				break;
			case 'd':
				debug = 1;
				printlog = 1;
				break;
			case 'm':
				module = optarg;
				break;
			case 'v':
				verbose++;
				break;
			default:
				usage();
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();

	log_init(debug);

	TAILQ_INIT(&conf.machines);
	//(const char *)conf.module = module;
	conf.module = module;

	for (u = 0; u < argc; u++)
		if (parse_config(argv[u], &conf) == -1) {
			log_warnx("parsing \"%s\"", argv[u]);
			exit(1);
		}

	if (getcwd(path, sizeof(path)) == NULL)
		fatal("cannot get working directory");
	if ((conf.basepath = strdup(path)) == NULL)
		fatal("strdup");

	gen_init(&conf);
	TAILQ_FOREACH(mach, &conf.machines, entry) {
		if (debugmachine && strcmp(mach->name, debugmachine) == 0) {
			debug = 1;
			printlog = 1;
		}
		gen_machine(&conf, mach);
		if (debugmachine && strcmp(mach->name, debugmachine) == 0) {
			debug = 0;
			printlog = 0;
		}
	}
	gen_close(&conf);

	if (verbose) {
		fflush(stdout);
		printf("\n");
		TAILQ_FOREACH(mach, &conf.machines, entry) {
			Member *m;
			struct name *n;

			if (debugmachine && strcmp(mach->name, debugmachine))
				continue;

			fprintf(stderr, "\nMachine: %s\n", mach->name);
			LIST_FOREACH(n, &mach->include, entry)
				fprintf(stderr, "INCLUDE: %s\n",
						n->name);
			LIST_FOREACH(m, &mach->interface.in, entry)
				fprintf(stderr, "IN: %s type \"%s\" flags=<%02x>\n",
						m->name, m->type, m->flags);
			LIST_FOREACH(m, &mach->interface.out, entry)
				fprintf(stderr, "OUT: %s type \"%s\" flags=<%02x>\n",
						m->name, m->type, m->flags);
			LIST_FOREACH(m, &mach->interface.stack, entry)
				fprintf(stderr, "STACK: %s type \"%s\" flags=<%02x>\n",
						m->name, m->type, m->flags);

			fprintf(stderr, "ACTION:\n");
			dump_actions(&mach->action, 0);

			fprintf(stderr, "FINISH:\n");
			dump_actions(&mach->finish, 0);
		}
	}

	exit(0);
}

void
dump_actions(struct action_list *list, int shift)
{
	Action *a;
	Function *f;
	Funcarg *fa;

	LIST_FOREACH(a, list, entry) {
		fprintf(stderr, "%*s", shift + 3, "-->");
		fprintf(stderr, "%s",
				a->type == FUNC_NONE ? "NONE" :
				a->type == FUNC_CALL ? "CALL" :
				a->type == FUNC_CODE ? "CODE" :
				a->type == FUNC_CONTINUE ? "CONTINUE" :
				a->type == FUNC_COPY ? "COPY" :
				a->type == FUNC_FORK ? "FORK" :
				a->type == FUNC_FORKWAIT ? "FORKWAIT" :
				a->type == FUNC_RETRY ? "RETRY" :
				a->type == FUNC_RETURN ? "RETURN" :
				a->type == FUNC_SYNC ? "SYNC" :
				a->type == FUNC_TEST ? "TEST" :
				a->type == FUNC_WAIT ? "WAIT" :
				"????");

		if (verbose > 1 && a->data) {
			fprintf(stderr, " datalen %zu", strlen(a->data));
			fprintf(stderr, "\n%*s", shift + 3, ">>>");
			fprintf(stderr, "\n%s", a->data);
			fprintf(stderr, "\n%*s", shift + 3, "<<<");
		}
		fprintf(stderr, "\n");

		LIST_FOREACH(f, &a->functions, entry) {
			fprintf(stderr, "%*s FUNC %s\n", shift + 6, "-->", f->name);
			LIST_FOREACH(fa, &f->args, entry)
				fprintf(stderr, "%*s : %s [%s]\n", shift + 6, "",
						fa->name, fa->type);
		}

		dump_actions(&a->actions, shift + 3);
	}
}

void
dump_action(struct action *a, struct machine *mach)
{
	int shift = 0;
	Function *f;
	Funcarg *fa;

	fprintf(stderr, "%*s", shift + 3, "-->");
	if (mach)
		fprintf(stderr, " %s function '%s'", mach->filename, mach->name);
	fprintf(stderr, " %s %d",
			a->type == FUNC_NONE ? "NONE" :
			a->type == FUNC_CALL ? "CALL" :
			a->type == FUNC_CODE ? "CODE" :
			a->type == FUNC_CONTINUE ? "CONTINUE" :
			a->type == FUNC_COPY ? "COPY" :
			a->type == FUNC_FORK ? "FORK" :
			a->type == FUNC_FORKWAIT ? "FORKWAIT" :
			a->type == FUNC_RETRY ? "RETRY" :
			a->type == FUNC_RETURN ? "RETURN" :
			a->type == FUNC_SYNC ? "SYNC" :
			a->type == FUNC_TEST ? "TEST" :
			a->type == FUNC_WAIT ? "WAIT" :
			"????",
			a->lineno);

	if (a->data) {
		fprintf(stderr, " datalen %zu", strlen(a->data));
		fprintf(stderr, "\n%*s", shift + 3, ">>>");
		fprintf(stderr, "\n%s", a->data);
		fprintf(stderr, "\n%*s", shift + 3, "<<<");
	}
	fprintf(stderr, "\n");

	LIST_FOREACH(f, &a->functions, entry) {
		fprintf(stderr, "%*s FUNC %s\n", shift + 6, "-->", f->name);
		LIST_FOREACH(fa, &f->args, entry)
			fprintf(stderr, "%*s : %s [%s]\n", shift + 6, "",
					fa->name, fa->type);
	}

	dump_actions(&a->actions, shift + 3);
}

void
output(int *lineno, FILE *f, const char *fmt, ...)
{
	va_list ap;
	char *buf, *s;
	int lno = 0;

	va_start(ap, fmt);
	if (vasprintf(&buf, fmt, ap) == -1)
		fatal("vasprintf");
	va_end(ap);

	s = buf;
	while (s && *s != '\0') {
		if (*s == '\n')
			lno++;
		s++;
	}
	*lineno += lno;

	fprintf(f, "%s", buf);
	free(buf);
}

int
count_subactions(struct action *act, int start)
{
	Action *a;

	LIST_FOREACH(a, &act->actions, entry) {
		start++;
		start = count_subactions(a, start);
	}

	return start;
}
