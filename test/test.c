#include <stdint.h>
#include <toolbox.h>
#include "test_event.h"

void set_callback(void *, void *);
int cb(int, void *, uint32_t);
void *myvoid = (void *)0xDEADBEEF;
int myint = 123456;
typedef void (cbfunc)(void *);
void *myfunc = NULL;
void *myarg = NULL;

int
main(int argc, char **argv)
{
	int ret;
	log_init(1);
	ret = test(&cb, myvoid, myint);
	log_debug("test done, calling callback");
	if (myfunc)
		((cbfunc *)myfunc)(myarg);
	log_debug("now done");
	return 0;
}

int
cb(int code, void *arg, uint32_t mine)
{
	printf("in callback -> code=%d arg=%08x mine=%u", code, (unsigned int)(uintptr_t)arg, mine);
	return 0;
}

void
set_callback(void *func, void *arg)
{
	myfunc = func;
	myarg = arg;
}
