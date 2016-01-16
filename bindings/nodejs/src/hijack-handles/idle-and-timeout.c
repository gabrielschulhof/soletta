#include <stdio.h>
#include <sol-mainloop.h>
#include "hijack-handles.h"

void *node_sol_loop_timeout_add(uint32_t timeout_ms, bool (*cb)(void *data), const void *data) {
	void *returnValue = NULL;
	HANDLE_ADD(timeout, returnValue, timeout_ms, cb, data);
	return returnValue;
}

bool node_sol_loop_timeout_del(void *handle) {
	bool returnValue = false;
	HANDLE_DEL(timeout, returnValue, handle);
	return returnValue;
}

void *node_sol_loop_idle_add(bool (*cb)(void *data), const void *data) {
	void *returnValue = NULL;
	HANDLE_ADD(idle, returnValue, cb, data);
	return returnValue;
}

bool node_sol_loop_idle_del(void *handle) {
	bool returnValue = false;
	HANDLE_DEL(idle, returnValue, handle);
	return returnValue;
}
