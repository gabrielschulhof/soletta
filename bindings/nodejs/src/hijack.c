#include <stdio.h>
#include <uv.h>

#include "hijack-handles/hijack-handles.h"

enum MainloopState {
	MAINLOOP_HIJACKING_STARTED,
	MAINLOOP_HIJACKED,
	MAINLOOP_RELEASING_STARTED,
	MAINLOOP_RELEASED
};

static enum MainloopState mainloopState = MAINLOOP_RELEASED;
static uv_idle_t uv_idle;

struct sol_fd *uv_loop_fd = NULL;
struct sol_mainloop_source *uv_loop_source = NULL;
bool sol_init_complete = false;

static void uv_idle_callback() {
	if (mainloopState == MAINLOOP_HIJACKING_STARTED) {
		DBG_UV_LOOP_HIJACK(printf("uv_idle_callback: running sol_run()\n"));
		mainloopState = MAINLOOP_HIJACKED;
		sol_run();
		DBG_UV_LOOP_HIJACK(printf("uv_idle_callback: sol_run() has returned\n"));

		/* Somebody may have requested a hijacking of the loop in the interval between sol_quit()
		 * and the return of sol_run(), in which case another call to the idler is on its way.
		 * In that case, we should not set the state to MAINLOOP_RELEASED here. */
		if (mainloopState == MAINLOOP_RELEASING_STARTED) {
			mainloopState = MAINLOOP_RELEASED;
		}
	} else if ( mainloopState == MAINLOOP_HIJACKED) {
		DBG_UV_LOOP_HIJACK(printf("uv_idle_callback: main loop already hijacked, so stopping idler\n"));
		uv_idle_stop(&uv_idle);
	}
}

static bool uv_loop_source_check (void *data) {
	uv_loop_t *loop = (uv_loop_t *)data;
    uv_update_time (loop);

	DBG_UV_LOOP_HIJACK(printf("uv_loop_source_check: Returning %s\n", uv_loop_alive (loop) ? "true" : "false"));
    return uv_loop_alive (loop);
}

static bool uv_loop_source_get_next_timeout(void *data, struct timespec *timeout) {
    /* Otherwise, check the timeout. If the timeout is 0, that means we're
     * ready to go. Otherwise, keep sleeping until the timeout happens again. */
    int t = uv_backend_timeout ((uv_loop_t *)data);

	DBG_UV_LOOP_HIJACK(printf("uv_loop_source_get_next_timeout: t = %d\n", t));

	timeout->tv_sec = (int)(t / 1000);
	timeout->tv_nsec = (t % 1000) * 1000000;

	return ( t >= 0 ) && uv_loop_source_check(data);
}

void uv_loop_source_dispatch (void *data) {
	DBG_UV_LOOP_HIJACK(printf("uv_loop_source_dispatch: Running one uv loop iteration\n"));
    uv_run((uv_loop_t *)data, UV_RUN_NOWAIT);
}

static const struct sol_mainloop_source_type uv_loop_source_funcs = {
	SOL_MAINLOOP_SOURCE_TYPE_API_VERSION,
    NULL,
	uv_loop_source_get_next_timeout,
	uv_loop_source_check,
    uv_loop_source_dispatch,
	NULL,
};

static bool uv_loop_fd_changed(void *data, int fd, uint32_t active_flags) {
	DBG_UV_LOOP_HIJACK(printf("uv_loop_fd_changed: Running one uv loop iteration\n"));
	uv_run((uv_loop_t *)data, UV_RUN_NOWAIT);
	return true;
}

static void hijack_main_loop() {
	DBG_UV_LOOP_HIJACK(printf("hijack_main_loop: Entering\n"));
	if (mainloopState != MAINLOOP_RELEASED && mainloopState != MAINLOOP_RELEASING_STARTED) {
		return;
	}

	/* The actual hijacking starts here, inspired by node-gtk. The plan:
	 * 1. Attach a source and the uv backend file descriptor to the soletta main loop
	 * 2. Attach an idler to the uv main loop to keep it from exiting and to run sol_run()
	 * 3. When the idler runs sol_run() it interrupts an iteration of the uv main loop because the
	 *    idler never returns, but that's OK because the soletta main loop runs and eventually,
	 *    through the source we attached in step 1, causes the uv main loop to run once, now in one
	 *    level of recursion, without blocking. As part of that iteration, we remove the idler
	 *    which is stuck anyway. This makes sure its handle doesn't prevent the uv main loop from
	 *    quitting. */

	mainloopState = MAINLOOP_HIJACKING_STARTED;

	/* We attach a source to the soletta main loop only once. After that, we simply start/stop the
	 * soletta main loop. */
	if (!uv_loop_source) {
		DBG_UV_LOOP_HIJACK(printf("hijack_main_loop: Allocating loop-related variables\n"));
		uv_loop_t *uv_loop = uv_default_loop();
		uv_loop_source = sol_mainloop_source_add(&uv_loop_source_funcs, (const void *)uv_loop);
		uv_loop_fd = sol_fd_add(uv_backend_fd(uv_loop),
			SOL_FD_FLAGS_IN | SOL_FD_FLAGS_OUT | SOL_FD_FLAGS_ERR,
			uv_loop_fd_changed, (const void *)uv_loop);
		uv_idle_init(uv_loop, &uv_idle);
	}

	DBG_UV_LOOP_HIJACK(printf("hijack_main_loop: Starting idler\n"));
	uv_idle_start(&uv_idle, uv_idle_callback);
}

static void release_main_loop() {
	DBG_UV_LOOP_HIJACK(printf("release_main_loop: Entering\n"));
	if (mainloopState == MAINLOOP_RELEASED || mainloopState == MAINLOOP_RELEASING_STARTED) {
		return;
	}

	/* hijack_main_loop() was called, but the idler has not yet run */
	if (mainloopState == MAINLOOP_HIJACKING_STARTED) {
		DBG_UV_LOOP_HIJACK(printf("release_main_loop: idler has not yet run, so stopping it\n"));
		uv_idle_stop(&uv_idle);
		mainloopState = MAINLOOP_RELEASED;
	} else {
		DBG_UV_LOOP_HIJACK(printf("release_main_loop: quitting main loop\n"));
		mainloopState = MAINLOOP_RELEASING_STARTED;
		sol_quit();
	}
}

static int hijack_refcount = 0;

void hijack_ref() {
	DBG_UV_LOOP_HIJACK(printf("hijack_ref: Entering\n"));
	if (hijack_refcount == 0) {
		DBG_UV_LOOP_HIJACK(printf("hijack_ref: hijacking main loop\n"));
		hijack_main_loop();
	}
	hijack_refcount++;
}

void hijack_unref() {
	DBG_UV_LOOP_HIJACK(printf("hijack_unref: Entering\n"));
	hijack_refcount--;
	if (hijack_refcount == 0) {
		DBG_UV_LOOP_HIJACK(printf("hijack_unref: releasing main loop\n"));
		release_main_loop();
	}
}

/*
 * We need to intercept the addition of handles to the soletta main loop, because we need to keep
 * a close eye on the number of open handles attached to the soletta main loop. This is because the
 * node.js way of running an app is that the main loop quits when there's nothing left to do.
 * Nobody expects that they have to explicitly terminate the main loop in order to exit the
 * application. Thus, we need to intercept whenever a handle is added to the soletta main loop, and
 * also whenever a handle is removed from the soletta main loop. Note that removal can happen
 * explicitly, via sol_del_*(), or implicitly, for example by closing a file descriptor that has a
 * watch attached, or by returning false from an idle/timeout handler. We also need to intercept
 * these implicit handle removals in order to have a correct tally of open handles currently
 * attached to the main loop
 * TODO implicit handle removal:
 * - timer: callback returns false
 * - idler: callback returns false
 * - source: dispose() is called
 * ? child watch: The callback itself(?) That assumes that, when the process quits, the child watch is removed.
 * ? fd: The callback itself with some condition(SOL_FD_FLAG_NVAL?) That assumes that closing the
 *       fd causes some condition that will be reported by the watcher.
 */
struct sol_mainloop_implementation node_intercept;

static bool node_sol_already_init = false;
int node_sol_init() {
	int sol_init_result = 0;
	if ( node_sol_already_init) {
		return 0;
	}
	node_sol_already_init = true;

	/* Copy the existing soletta mainloop implementation, modify some of the functions, and set it
	 * as the new implementation */
	node_intercept = *sol_mainloop_get_implementation();

	node_intercept.timeout_add = node_sol_loop_timeout_add;
	node_intercept.timeout_del = node_sol_loop_timeout_del;
	node_intercept.idle_add = node_sol_loop_idle_add;
	node_intercept.idle_del = node_sol_loop_idle_del;
#ifdef SOL_MAINLOOP_FD_ENABLED
	node_intercept.fd_add = node_sol_loop_fd_add;
	node_intercept.fd_del = node_sol_loop_fd_del;
#endif /* def SOL_MAINLOOP_FD_ENABLED */
#ifdef SOL_MAINLOOP_FORK_WATCH_ENABLED
	node_intercept.child_watch_add = node_sol_loop_child_watch_add;
	node_intercept.child_watch_del = node_sol_loop_child_watch_del;
#endif /* def SOL_MAINLOOP_FORK_WATCH_ENABLED */
	node_intercept.source_add = node_sol_loop_source_add;
	node_intercept.source_del = node_sol_loop_source_del;

	if (!sol_mainloop_set_implementation(&node_intercept)) {
		return 1;
	}
	sol_init_result = sol_init();
	sol_init_complete = true;
	return sol_init_result;
}
