/* Autogenerated - do not edit */

#include <stdlib.h>
#include <string.h>
#include <toolbox.h>
#include <test_event.h>


	void set_callback(void *, void *);
	struct testent { TAILQ_ENTRY(testent) entry; };
	TAILQ_HEAD(testent_head, testent) entries;

typedef struct test_test_state {
	struct {
		int testval;
	} in;

	struct {
		int stuff;
		int copied;
	} stack;

	/*
	 *	struct {
	 *		int errorcode;
	 *		uint32_t xid;
	 *	} out;
	 */
	test_test_out out;

	int state;
	int loopdetect;
	int forkcounter;
	void *ret;
	void (*retfree)(void *);

	EVENTCB_test *cbfunc;
	void *cbarg;
} test_test_state;

static void
free_test_state(test_test_state *state)
{
	if (state->ret) {
		if (state->retfree)
			state->retfree(state->ret);
		free(state->ret);
		state->ret = NULL;
	}
	free_test_out(&state->out);
}

void
free_test_out(void *arg)
{
	/* ((test_test_out *)arg)->xid */
}

static int go_test(test_test_state *);
/*
 * Function: CALL
 * RET Type: none
 * Wait ptr: 0
 * Rtry ptr: 0
 */
static int act_test_wait_0(int, void *,
		int ,
		struct __sbuf );

static int
act_test_state_0(test_test_state *state)
{
	int ret;
	ret = function(act_test_wait_0, state,
			7,
			NULL);
	return ret;
}

static int
act_test_wait_0(int code, void *arg,
		int xid,
		struct __sbuf junk)
{
	test_test_state *state = (test_test_state  *)arg;
	int ret = TBEVENT_SUCCESS;
	if ((state->ret = calloc(1, sizeof(test_function_out))) == NULL)
		return TBOX_NOMEM;
	state->retfree = free_function_out;
	((test_function_out *)state->ret)->errorcode = code;
	((test_function_out *)state->ret)->xid = xid;
	((test_function_out *)state->ret)->junk = junk;
	if (state->loopdetect) {
		state->state = 0;
		return TBEVENT_BLOCKDONE;
	}
	state->state = 1;
	ret = go_test(state);
	return ret;
}

/*
 * Function: CODE
 * RET Type: test_function
 * Wait ptr: 0
 * Rtry ptr: 0
 */
static int
act_test_state_1(test_test_state *state)
{
	int test_test_return = TBEVENT_BLOCKDONE;
#line 71 "/Users/maxwell/code/gw/event/test/test.event"
	{
		(&(state->stack))->stuff = (&(state->in))->testval;
		log_debug("function() returned xid=%d junk=(%p, %d)",
				((test_function_out *)state->ret)->xid, ((test_function_out *)state->ret)->junk._base, ((test_function_out *)state->ret)->junk._size);
	}
#line 119 "/Users/maxwell/code/gw/event/test/generated/event_test.c"
	return test_test_return;
}

/*
 * Function: COPY
 * RET Type: test_function
 * Wait ptr: 0
 * Rtry ptr: 0
 */
static int
act_test_state_2(test_test_state *state)
{
	int ret = TBEVENT_SUCCESS;
	((&state->stack))->copied = ((&state->stack))->stuff;
	if (ret != TBEVENT_SUCCESS) {
		state->out.errorcode = ret;
		return TBEVENT_FINISH;
	}
	return TBEVENT_BLOCKDONE;
}

/*
 * Function: RETRY
 * RET Type: test_function
 * Wait ptr: 0
 * Rtry ptr: 0
 */
static int
act_test_state_3(test_test_state *state)
{
	return TBEVENT_BLOCKDONE;
}

/*
 * Function: CODE
 * RET Type: test_function
 * Wait ptr: 0
 * Rtry ptr: 3
 */
static int
act_test_state_4(test_test_state *state)
{
	int test_test_return = TBEVENT_BLOCKDONE;
#line 80 "/Users/maxwell/code/gw/event/test/test.event"
	{
			log_debug("CODE IN RETRY stuff=%u", (&(state->stack))->stuff);
		}
#line 167 "/Users/maxwell/code/gw/event/test/generated/event_test.c"
	return test_test_return;
}

/*
 * Function: SYNC
 * RET Type: test_function
 * Wait ptr: 0
 * Rtry ptr: 3
 */
static int
act_test_state_5(test_test_state *state)
{
	int test_test_return = TBEVENT_BLOCKDONE;
#line 83 "/Users/maxwell/code/gw/event/test/test.event"
	{
			log_debug("MORE IN RETRY");
		}
#line 185 "/Users/maxwell/code/gw/event/test/generated/event_test.c"
	return test_test_return;
}

/*
 * Function: TEST
 * RET Type: test_function
 * Wait ptr: 0
 * Rtry ptr: 3
 */
static int
act_test_state_6(test_test_state *state)
{
	if ((&(state->stack))->stuff == 123456) {
		return TBEVENT_BLOCKDONE;
	} else {
		state->state = 8;
		return TBEVENT_BLOCKDONE;
	}
	return TBEVENT_BLOCKDONE;
}

/*
 * Function: COPY
 * RET Type: test_function
 * Wait ptr: 0
 * Rtry ptr: 3
 */
static int
act_test_state_7(test_test_state *state)
{
	int ret = TBEVENT_SUCCESS;
	((&state->stack))->stuff = 1234567;
	if (ret != TBEVENT_SUCCESS) {
		state->out.errorcode = ret;
		return TBEVENT_FINISH;
	}
	return TBEVENT_BLOCKDONE;
}

/*
 * Function: CONTINUE
 * RET Type: test_function
 * Wait ptr: 0
 * Rtry ptr: 3
 */
static int
act_test_state_8(test_test_state *state)
{
	state->state = 3;
	if (state->ret) {
		if (state->retfree)
			state->retfree(state->ret);
		free(state->ret);
		state->ret = NULL;
	}
	return TBEVENT_BLOCKDONE;
}

/* END TEST */
/* END RETRY */
/*
 * Function: SYNC
 * RET Type: test_function
 * Wait ptr: 0
 * Rtry ptr: 0
 */
static int
act_test_state_9(test_test_state *state)
{
	int test_test_return = TBEVENT_BLOCKDONE;
#line 92 "/Users/maxwell/code/gw/event/test/test.event"
	{
		log_debug("SYNC");
	}
#line 260 "/Users/maxwell/code/gw/event/test/generated/event_test.c"
	return test_test_return;
}

/*
 * Function: TEST
 * RET Type: test_function
 * Wait ptr: 0
 * Rtry ptr: 0
 */
static int
act_test_state_10(test_test_state *state)
{
	if (1) {
		return TBEVENT_BLOCKDONE;
	} else {
		state->state = 11;
		return TBEVENT_BLOCKDONE;
	}
	return TBEVENT_BLOCKDONE;
}

/*
 * Function: CODE
 * RET Type: test_function
 * Wait ptr: 0
 * Rtry ptr: 0
 */
static int
act_test_state_11(test_test_state *state)
{
	int test_test_return = TBEVENT_BLOCKDONE;
#line 97 "/Users/maxwell/code/gw/event/test/test.event"
	{
			log_debug("CODE IN TEST(1)");
		}
#line 296 "/Users/maxwell/code/gw/event/test/generated/event_test.c"
	return test_test_return;
}

/* END TEST */
/*
 * Function: TEST
 * RET Type: test_function
 * Wait ptr: 0
 * Rtry ptr: 0
 */
static int
act_test_state_12(test_test_state *state)
{
	if (0) {
		return TBEVENT_BLOCKDONE;
	} else {
		state->state = 13;
		return TBEVENT_BLOCKDONE;
	}
	return TBEVENT_BLOCKDONE;
}

/*
 * Function: CODE
 * RET Type: test_function
 * Wait ptr: 0
 * Rtry ptr: 0
 */
static int
act_test_state_13(test_test_state *state)
{
	int test_test_return = TBEVENT_BLOCKDONE;
#line 103 "/Users/maxwell/code/gw/event/test/test.event"
	{
			log_debug("CODE IN TEST(0)");
		}
#line 333 "/Users/maxwell/code/gw/event/test/generated/event_test.c"
	return test_test_return;
}

/* END TEST */
/*
 * Function: WAIT
 * RET Type: test_function
 * Wait ptr: 0
 * Rtry ptr: 0
 */
static int
act_test_wait_14(int var1,
		int var2,
		void *test_test_arg)
{
	test_test_state *state = (test_test_state *)test_test_arg;
	int test_test_return;
	bzero(&test_test_return, sizeof(test_test_return));
#line 108 "/Users/maxwell/code/gw/event/test/test.event"
	{
		log_debug("WAIT CALLBACK");
	}
#line 356 "/Users/maxwell/code/gw/event/test/generated/event_test.c"
	if (state->loopdetect == 0) {
		state->state++;
		go_test(state);
	}
	return test_test_return;
}

static int
act_test_state_14(test_test_state *state)
{
	return TBEVENT_BLOCKDONE;
}

/*
 * Function: CODE
 * RET Type: test_function
 * Wait ptr: 14
 * Rtry ptr: 0
 */
static int
act_test_state_15(test_test_state *state)
{
	int test_test_return = TBEVENT_BLOCKDONE;
#line 111 "/Users/maxwell/code/gw/event/test/test.event"
	{
		log_debug("CODE FOR WAIT 0x%p", &act_test_wait_14);
		set_callback(act_test_wait_14, (state));
		return TBEVENT_CALLBACK;
	}
#line 386 "/Users/maxwell/code/gw/event/test/generated/event_test.c"
	return test_test_return;
}

/*
 * Function: CODE
 * RET Type: test_function
 * Wait ptr: 14
 * Rtry ptr: 0
 */
static int
act_test_state_16(test_test_state *state)
{
	int test_test_return = TBEVENT_BLOCKDONE;
#line 117 "/Users/maxwell/code/gw/event/test/test.event"
	{
		log_debug("THIS SHOULD BE AFTER CALLBACK");
	}
#line 404 "/Users/maxwell/code/gw/event/test/generated/event_test.c"
	return test_test_return;
}

/*
 * Function: CALL
 * RET Type: test_function
 * Wait ptr: 14
 * Rtry ptr: 0
 */
static int act_test_wait_17(int, void *,
		int ,
		int );

static int
act_test_state_17(test_test_state *state)
{
	int ret;
	ret = anotherfunc(act_test_wait_17, state);
	if (ret != TBEVENT_SUCCESS && ret != TBEVENT_BLOCKDONE &&
			ret != TBEVENT_CALLBACK && ret != TBEVENT_FINISH &&
			state->ret) {
		if (state->retfree)
			state->retfree(state->ret);
		free(state->ret);
		state->ret = NULL;
	}
	return ret;
}

static int
act_test_wait_17(int code, void *arg,
		int ignore,
		int mytest)
{
	test_test_state *state = (test_test_state  *)arg;
	int ret = TBEVENT_SUCCESS;
	if (state->ret) {
		if (state->retfree)
			state->retfree(state->ret);
		free(state->ret);
		state->ret = NULL;
	}
	if ((state->ret = calloc(1, sizeof(test_anotherfunc_out))) == NULL)
		return TBOX_NOMEM;
	state->retfree = free_anotherfunc_out;
	((test_anotherfunc_out *)state->ret)->errorcode = code;
	((test_anotherfunc_out *)state->ret)->ignore = ignore;
	((test_anotherfunc_out *)state->ret)->mytest = mytest;
	if (state->loopdetect) {
		state->state = 17;
		return TBEVENT_BLOCKDONE;
	}
	state->state = 18;
	ret = go_test(state);
	return ret;
}

/*
 * Function: CODE
 * RET Type: test_anotherfunc
 * Wait ptr: 14
 * Rtry ptr: 0
 */
static int
act_test_state_18(test_test_state *state)
{
	int test_test_return = TBEVENT_BLOCKDONE;
#line 122 "/Users/maxwell/code/gw/event/test/test.event"
	{
		log_debug("anotherfunc() returned ignore=%d test=%d",
				((test_anotherfunc_out *)state->ret)->ignore, ((test_anotherfunc_out *)state->ret)->mytest);
	}
#line 477 "/Users/maxwell/code/gw/event/test/generated/event_test.c"
	return test_test_return;
}

/*
 * Function: RETURN
 * RET Type: test_anotherfunc
 * Wait ptr: 14
 * Rtry ptr: 0
 */
static int
act_test_state_19(test_test_state *state)
{
	state->out.errorcode = TBOX_API_INVALID;
	return TBEVENT_FINISH;
}

/*
 * Function: CODE
 * RET Type: test_anotherfunc
 * Wait ptr: 14
 * Rtry ptr: 0
 */
static int
act_test_state_20(test_test_state *state)
{
	int test_test_return = TBEVENT_BLOCKDONE;
#line 129 "/Users/maxwell/code/gw/event/test/test.event"
	{
		log_debug("THIS SHOULD NEVER BE RUN");
	}
#line 508 "/Users/maxwell/code/gw/event/test/generated/event_test.c"
	return test_test_return;
}

/*
 * Function: CODE
 * RET Type: test_anotherfunc
 * Wait ptr: 14
 * Rtry ptr: 0
 */
static int
fin_test_state_21(test_test_state *state)
{
	int test_test_return = TBEVENT_BLOCKDONE;
#line 135 "/Users/maxwell/code/gw/event/test/test.event"
	{
		log_debug("CODE FINISH");
	}
#line 526 "/Users/maxwell/code/gw/event/test/generated/event_test.c"
	return test_test_return;
}

/*
 * Function: RETURN
 * RET Type: test_anotherfunc
 * Wait ptr: 14
 * Rtry ptr: 0
 */
static int
fin_test_state_22(test_test_state *state)
{
	state->out.errorcode = TBOX_SUCCESS;
	return TBEVENT_FINISH;
}

/*
 * Function: CODE
 * RET Type: test_anotherfunc
 * Wait ptr: 14
 * Rtry ptr: 0
 */
static int
fin_test_state_23(test_test_state *state)
{
	int test_test_return = TBEVENT_BLOCKDONE;
#line 139 "/Users/maxwell/code/gw/event/test/test.event"
	{
		log_debug("THIS SHOULD NOT BE RUN");
	}
#line 557 "/Users/maxwell/code/gw/event/test/generated/event_test.c"
	return test_test_return;
}

static int
go_test(test_test_state *state)
{
	int ret = TBEVENT_SUCCESS;

	if (state->loopdetect)
		return TBEVENT_SUCCESS;
	else
		state->loopdetect++;

top:
	switch (state->state) {
		case 0:
			ret = act_test_state_0(state);
			break;
		case 1:
			ret = act_test_state_1(state);
			break;
		case 2:
			ret = act_test_state_2(state);
			break;
		case 3:
			ret = act_test_state_3(state);
			break;
		case 4:
			ret = act_test_state_4(state);
			break;
		case 5:
			ret = act_test_state_5(state);
			break;
		case 6:
			ret = act_test_state_6(state);
			break;
		case 7:
			ret = act_test_state_7(state);
			break;
		case 8:
			ret = act_test_state_8(state);
			break;
		case 9:
			ret = act_test_state_9(state);
			break;
		case 10:
			ret = act_test_state_10(state);
			break;
		case 11:
			ret = act_test_state_11(state);
			break;
		case 12:
			ret = act_test_state_12(state);
			break;
		case 13:
			ret = act_test_state_13(state);
			break;
		case 14:
			ret = act_test_state_14(state);
			break;
		case 15:
			ret = act_test_state_15(state);
			break;
		case 16:
			ret = act_test_state_16(state);
			break;
		case 17:
			ret = act_test_state_17(state);
			break;
		case 18:
			ret = act_test_state_18(state);
			break;
		case 19:
			ret = act_test_state_19(state);
			break;
		case 20:
			ret = act_test_state_20(state);
			break;
		case 21:
			ret = fin_test_state_21(state);
			break;
		case 22:
			ret = fin_test_state_22(state);
			break;
		case 23:
			ret = fin_test_state_23(state);
			break;
		case 24:
			ret = TBEVENT_FINISH;
			break;
		default:
			log_warnx("go_test: invalid state %d", state->state);
			ret = TBOX_API_INVALID;
			goto out;
	}
	if (ret == TBEVENT_BLOCKDONE || ret == TBEVENT_SUCCESS) {
		state->state++;
		if (state->state >= 0 && state->state <= 23)
			goto top;
	} else if (ret == TBEVENT_CALLBACK) {
		ret = TBEVENT_CALLBACK;
		goto out;
	} else if (ret == TBEVENT_FINISH) {
		if (state->state >= 0 && state->state < 21)
			state->state = 21;
		else
			state->state = 23 + 1;
		if (state->state >= 0 && state->state <= 23)
			goto top;
	} else if (ret != TBEVENT_SUCCESS) {
		if (state->state >= 0 && state->state < 21)
			state->state = 21;
		else
			state->state++;
		if (state->state >= 0 && state->state <= 23)
			goto top;
	}
	if (state->state >= 0 && state->state > 23) {
		if (state->cbfunc)
			ret = state->cbfunc(state->out.errorcode, state->cbarg,
					state->out.xid);
		free_test_state(state);
		free(state);
		state = NULL;
	}
out:
	if (state)
		state->loopdetect--;
	if (ret == TBEVENT_SUCCESS || ret == TBEVENT_BLOCKDONE ||
			ret == TBEVENT_CALLBACK)
		return ret;
	else
		log_warnx("test: %s", tb_error(ret));
	return TBEVENT_BLOCKDONE;
}
int
test(EVENTCB_test *callback, void *arg,
		int testval)
{
	int ret = TBEVENT_SUCCESS;
	test_test_state *state;
	if ((state = calloc(1, sizeof(*state))) == NULL) {
		ret = TBOX_NOMEM;
		goto fail;
	}
	state->cbfunc = callback;
	state->cbarg = arg;
	state->in.testval = testval;
	ret = go_test(state);
	return ret;

fail:
	if (state) {
		free_test_state(state);
		free(state);
	}
	return ret;
}
