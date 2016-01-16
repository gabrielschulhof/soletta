#include "hijack-handles.h"
#ifdef SOL_MAINLOOP_FORK_WATCH_ENABLED

#include <stdio.h>

void *node_sol_loop_child_watch_add(uint64_t pid, void (*cb)(void *data, uint64_t pid, int status), const void *data) {
	void *returnValue = NULL;
	HANDLE_ADD(child_watch, returnValue, pid, cb, data);
	return returnValue;
}

bool node_sol_loop_child_watch_del(void *handle) {
	bool returnValue = false;
	HANDLE_DEL(child_watch, returnValue, handle);
	return returnValue;
}
#endif /* def SOL_MAINLOOP_FORK_WATCH_ENABLED */
