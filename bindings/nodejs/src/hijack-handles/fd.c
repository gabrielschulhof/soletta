#include "hijack-handles.h"
#ifdef SOL_MAINLOOP_FD_ENABLED

#include <stdio.h>
#include <uv.h>

struct {
	void *original_data;
	uint32_t original_flags;
} intercept_data;

static bool fd_callback(void *data, int fd, uint32_t active_flags) {
	struct intercept_data idata = *(intercept_data *)data;
	if (active_flags & SOL_FD_FLAG_NVAL) {
		hijack_unref();
	}
}

void *node_sol_loop_fd_add(int the_fd, uint32_t flags, bool (*cb)(void *data, int fd, uint32_t active_flags), const void *data) {
	void *returnValue = NULL;
	DBG_UV_LOOP_HIJACK(printf("node_sol_loop_fd_add: Entering\n"));

	// Adding the uv main loop backend fd should not cause reffing
	if (the_fd == uv_backend_fd(uv_default_loop())) {
		DBG_UV_LOOP_HIJACK(printf("node_sol_loop_fd_add: Adding uv backend fd, so skipping refcounting\n"));
		return SOL_MAINLOOP_IMPLEMENTATION_DEFAULT->fd_add(the_fd, flags, cb, data);
	}
	HANDLE_ADD(fd, returnValue, the_fd, flags, cb, data);
	return returnValue;
}

bool node_sol_loop_fd_del(void *handle) {
	bool returnValue;
	DBG_UV_LOOP_HIJACK(printf("node_sol_loop_fd_del: Entering\n"));

	// Removing the main loop backend fd should not cause unreffing
	if (handle == (void *)uv_loop_fd) {
		DBG_UV_LOOP_HIJACK(printf("node_sol_loop_fd_del: Removing uv backend fd, so skipping refcounting\n"));
		return SOL_MAINLOOP_IMPLEMENTATION_DEFAULT->fd_del(handle);
	}
	HANDLE_DEL(fd, returnValue, handle);
	return returnValue;
}
#endif /* def SOL_MAINLOOP_FD_ENABLED */
