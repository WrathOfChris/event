event
=====

Events for C - async callback preprocessor

author
======

Chris Maxwell

what is it?
===========

It's a language overlay for C that builds automated state machines from
language definitions.

You may notice that the definition grammer is very similar to ASN.1 notation,
which was used in parallel to create an over-the-wire protocol that was then
processed by Events.

why do it?
==========

It was designed to reduce the complexity required to implement high performance
non-blocking messaging in a distributed network file system implementation.  It
allowed me to concentrate on application flow and distributing messages to
remotes without having to manually implement state machines for 150+ message
types and procedures.

when was it?
============

Event was last modified sometime in 2006.  It was feature-complete for my use
case, though some workarounds were needed for zero-copy storage buffer passing.

what else is lurking?
=====================

This was taken from one of my toolbox libraries, with enough supporting bits to
allow it to build.  log.c is originally from OpenBSD.  str.c is collected
string handling reimplemented standing on the shoulders of giants whose names
have past to the mists of time.  Apologies to those forgotten.

example
=======

	-- define an Event function
	myfunction DEFINITIONS ::= BEGIN
	INCLUDE stdio;
	INTERFACE IN {
		myarg1	int,
		myarg2	void REFERENCE
	}
	INTERFACE OUT {
		myret1	int,
		myret2	"struct mytype"
	}
	INTERFACE STACK {
		mylocal	int
	}
	ACTION {
		-- execute "anotherfunc", returning "RET"
		CALL anotherfunc();
		COPY RET->errorcode, STACK->mylocal;
		
		-- define a callback to an external callback system (ie: libevent)
		WAIT void (evutil_socket_t fd, short what, void *CBARG) {
			/* set something on the STACK */
			STACK->mylocal = what;

			/* execution now resumes */
		}
		CODE {
			/* expands to function + callback */
			printf("this happens first\n");

			/* register an external libevent callback */
			event_new(event_base, -1, 0, CBFUNC, CBARG);
			
			/* this function execution stops until CBFUNC() is called */
			CALLBACK;
		}

		-- note: event system yields here to other queued events

		SYNC {
			/* expands inline, without processing other events */
			printf("CODE and WAIT have been called back\n");
		}

		-- set a retry point, CONTINUE resets to here
		RETRY {
			CALL EVENT_FUNCTION();

			-- test a return value, retry the loop if it fails
			TEST (RET->errorcode) {
				CONTINUE;
			}
		}
	}

	FINISH {
		-- test and return an error code
		TEST (RET->errorcode)
			RETURN RET->errorcode;
		RETURN TBOX_SUCCESS;
	}
	END
