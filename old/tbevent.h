#ifndef TBEVENT_H
#define TBEVENT_H
#include <sys/queue.h>
#include <stdio.h>
#include <toolbox.h>

#define STEM	"event"
#define MODULE	"async"

extern int debug;

enum type_flags {
	FLAG_NONE		= 0x00,
	FLAG_OPTIONAL	= 0x01,
	FLAG_REFERENCE	= 0x02,
	FLAG_CONSTRUCTED= 0x04,
	FLAG_CONSTANT	= 0x08
};

enum func_type {
	FUNC_NONE,
	FUNC_CALL,
	FUNC_CODE,
	FUNC_CONTINUE,
	FUNC_COPY,
	FUNC_FORK,
	FUNC_FORKWAIT,
	FUNC_RETRY,
	FUNC_RETURN,
	FUNC_SYNC,
	FUNC_TEST,
	FUNC_WAIT
};

struct name {
	LIST_ENTRY(name) entry;
	char *name;
};
LIST_HEAD(name_list, name);

typedef struct member {
	LIST_ENTRY(member) entry;
	char *name;
	char *type;
	int val;
	enum type_flags flags;
} Member;
LIST_HEAD(member_list, member);

typedef struct funcarg {
	LIST_ENTRY(funcarg) entry;
	char *name;
	char *type;
} Funcarg;
LIST_HEAD(funcarg_list, funcarg);

typedef struct function {
	LIST_ENTRY(function) entry;
	char *name;
	struct funcarg_list args;
} Function;
LIST_HEAD(function_list, function);

LIST_HEAD(action_list, action);
typedef struct action {
	LIST_ENTRY(action) entry;
	int lineno;
	enum func_type type;
	char *data;
	struct function_list functions;
	struct action_list actions;
} Action;

struct machine {
	TAILQ_ENTRY(machine) entry;
	char *filename;
	char *name;
	struct name_list include;
	char *preamble;
	int preambleline;
	struct {
		struct member_list in;
		struct member_list out;
		struct member_list stack;
	} interface;
	struct action_list action;
	struct action_list finish;
	int state;							/* count actions during build */
	int error;							/* state at which finish begins */
	int waitstate;						/* last defined wait state */
	char *lastrettype;					/* last registered return type */
	int retrystate;						/* last defined retry state */
	int needout;
};
TAILQ_HEAD(machine_list, machine);

struct tbevent_conf {
	struct machine_list machines;
	char *stem;							/* prefix for files */
	char *headerbase;
	const char *module;						/* prefix for types */
	char *basepath;						/* getcwd path */

	FILE *headerfile;
	FILE *logfile;
	FILE *codefile;
};

#define LIST_COPY_REVERSE(from, to, type, entry) do {	\
	type *l, *l_n;										\
	LIST_INIT(to);										\
	for (l = LIST_FIRST(from);							\
			l != LIST_END(from);						\
			l = l_n) {									\
		l_n = LIST_NEXT(l, entry);						\
		LIST_INSERT_HEAD(to, l, entry);					\
	}													\
} while (0)

#define CODE(M, args...) \
	output(&lineno, conf->codefile, M, ##args)

void dump_action(struct action *, struct machine *);
void output(int *, FILE *, const char *, ...)
	__attribute__ ((__format__(printf, 3, 4)));
int count_subactions(struct action *, int);

int parse_config(const char *, struct tbevent_conf *);
int parse_buffer(const char *, size_t, struct tbevent_conf *);

void gen_init(struct tbevent_conf *);
void gen_machine(struct tbevent_conf *, struct machine *);
void gen_interface(struct tbevent_conf *, struct machine *);
void gen_action(struct tbevent_conf *, struct machine *,
		struct action_list *, const char *);
void gen_statemachine(struct tbevent_conf *, struct machine *);
void gen_free(struct tbevent_conf *, struct machine *);
void gen_extern(struct tbevent_conf *, struct machine *);
void gen_close(struct tbevent_conf *);
void gen_output_code(struct tbevent_conf *, struct machine *,
		struct action *, char *);
void gen_output_name(struct tbevent_conf *, struct machine *,
		char *);

int type_lookup(const char *);
const char * type_function(enum func_type);

void gen_func_declaration(struct tbevent_conf *, struct machine *,
		struct action *, const char *);
void gen_func_close(struct tbevent_conf *, struct machine *,
		struct action *, const char *);

void gen_func_fork(struct tbevent_conf *, struct machine *,
		struct action *, const char *);
void gen_func_forkwait(struct tbevent_conf *, struct machine *,
		struct action *, const char *);

#endif
