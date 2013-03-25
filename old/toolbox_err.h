/* Generated from /usr/src/lib/libtoolbox/toolbox_err.et */

#ifndef __toolbox_err_h__
#define __toolbox_err_h__

#include <kerberosV/com_err.h>

void initialize_tbox_error_table_r(struct et_list **);

void initialize_tbox_error_table(void);
#define init_tbox_err_tbl initialize_tbox_error_table

typedef enum tbox_error_number{
	ERROR_TABLE_BASE_tbox = -1177914880,
	tbox_err_base = -1177914880,
	TBOX_NONE = -1177914880,
	TBOX_API_INVALID = -1177914879,
	TBOX_TRUNCATED = -1177914878,
	TBOX_NOMEM = -1177914877,
	TBOX_RANGE = -1177914876,
	TBOX_NOTFOUND = -1177914875,
	TBOX_EXISTS = -1177914874,
	TBOX_BADFORMAT = -1177914873,
	TBEVENT_BLOCKDONE = -1177914816,
	TBEVENT_CALLBACK = -1177914815,
	TBEVENT_FINISH = -1177914814,
	TBOX_ERR_RCSID = -1177914752,
	tbox_num_errors = 129
} tbox_error_number;

#endif /* __toolbox_err_h__ */
