#include <stdio.h>
#include "hijack-handles.h"

void *node_sol_loop_source_add(const struct sol_mainloop_source_type *type, const void *data) {
	void *returnValue = NULL;
	DBG_UV_LOOP_HIJACK(printf("node_sol_loop_source_add: Entering\n"));

	/* Adding the main loop source should not cause reffing */
	if (type->dispatch == uv_loop_source_dispatch) {
		DBG_UV_LOOP_HIJACK(printf("node_sol_loop_source_add: Adding uv source, so skipping refcounting\n"));
		return SOL_MAINLOOP_IMPLEMENTATION_DEFAULT->source_add(type, data);
	}
	HANDLE_ADD(source, returnValue, type, data);
	return returnValue;
}

void node_sol_loop_source_del(void *source) {
	DBG_UV_LOOP_HIJACK(printf("node_sol_loop_source_del: Entering\n"));
	SOL_MAINLOOP_IMPLEMENTATION_DEFAULT->source_del(source);

	/* Removing the main loop source should not cause unreffing */
	if (source != uv_loop_source) {
		DBG_UV_LOOP_HIJACK(printf("node_sol_loop_source_add: Removing non-uv source, so doing refcounting\n"));
		hijack_unref();
	}
}


