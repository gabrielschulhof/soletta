#ifndef _SOLETTA_NODE_JS_HIJACK_HANDLES_H__
#define _SOLETTA_NODE_JS_HIJACK_HANDLES_H__

#ifdef __cplusplus
extern "C" {
#endif /* def __cplusplus */

#include <sol-mainloop.h>

#include "../hijack.h"

/*
 * Intercept calls which add handles to or remove them from the main loop so we can start/stop the
 * loop in response.
 */

/*
 * Common code for attempting to add a handle to the loop. Assigns @returnValue.
 */
#define HANDLE_ADD(name, returnValue, ...) \
	do { \
		if (sol_init_complete == true) { \
			DBG_UV_LOOP_HIJACK(printf(#name "_add: Reffing loop\n")); \
			hijack_ref(); \
		} \
		returnValue = SOL_MAINLOOP_IMPLEMENTATION_DEFAULT->name##_add( __VA_ARGS__ ); \
		if (!returnValue) { \
			DBG_UV_LOOP_HIJACK(printf(#name "_add: Addition failed - unreffing loop\n")); \
			hijack_unref(); \
		} \
	} while(0)

/*
 * Common code for attempting to remove a handle from the loop. Assigns @returnValue.
 */
#define HANDLE_DEL(name, returnValue, handle) \
	do { \
		DBG_UV_LOOP_HIJACK(printf(#name "_del: Removing\n")); \
		returnValue = SOL_MAINLOOP_IMPLEMENTATION_DEFAULT->name##_del(handle); \
		if (returnValue && sol_init_complete == true) { \
			DBG_UV_LOOP_HIJACK(printf(#name "_del: Removal succeeded - unreffing loop\n")); \
			hijack_unref(); \
		} \
	} while(0)

void *node_sol_loop_timeout_add(uint32_t timeout_ms, bool (*cb)(void *data), const void *data);
bool node_sol_loop_timeout_del(void *handle);

void *node_sol_loop_idle_add(bool (*cb)(void *data), const void *data);
bool node_sol_loop_idle_del(void *handle);

#ifdef SOL_MAINLOOP_FD_ENABLED
void *node_sol_loop_fd_add(int the_fd, uint32_t flags, bool (*cb)(void *data, int fd, uint32_t active_flags), const void *data);
bool node_sol_loop_fd_del(void *handle);
#endif /* def SOL_MAINLOOP_FD_ENABLED */

#ifdef SOL_MAINLOOP_FORK_WATCH_ENABLED
void *node_sol_loop_child_watch_add(uint64_t pid, void (*cb)(void *data, uint64_t pid, int status), const void *data);
bool node_sol_loop_child_watch_del(void *handle);
#endif /* def SOL_MAINLOOP_FORK_WATCH_ENABLED */

void *node_sol_loop_source_add(const struct sol_mainloop_source_type *type, const void *data);
void node_sol_loop_source_del(void *source);

#ifdef __cplusplsy
}
#endif /* def __cplusplys */

#endif /* ndef _SOLETTA_NODE_JS_HIJACK_HANDLES_H__ */
