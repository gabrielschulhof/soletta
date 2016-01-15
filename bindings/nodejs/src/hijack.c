#include <stdio.h>
#include <uv.h>
#include <sol-mainloop.h>

#define DBG_UV_LOOP_HIJACK(s) s

enum MainloopState {
	MAINLOOP_HIJACKING_STARTED,
	MAINLOOP_HIJACKED,
	MAINLOOP_RELEASING_STARTED,
	MAINLOOP_RELEASED
};

static enum MainloopState mainloopState = MAINLOOP_RELEASED;
static uv_idle_t uv_idle;
static struct sol_mainloop_source *uv_loop_source = NULL;
static struct sol_fd *uv_loop_fd = NULL;
static bool sol_init_complete = false;

static void uv_idle_callback() {
	if (mainloopState == MAINLOOP_HIJACKING_STARTED) {
		DBG_UV_LOOP_HIJACK(printf("uv_idle_callback: running sol_run()\n"));
		mainloopState = MAINLOOP_HIJACKED;
		sol_run();
		DBG_UV_LOOP_HIJACK(printf("uv_idle_callback: sol_run() has returned\n"));

		// Somebody may have requested a hijacking of the loop in the interval between sol_quit()
		// and the return of sol_run(), in which case another call to the idler is on its way.
		// In that case, we should not set the state to MAINLOOP_RELEASED here.
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

static void uv_loop_source_dispatch (void *data) {
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

	// The actual hijacking starts here, inspired by node-gtk. The plan:
	// 1. Attach a source to the glib main loop
	// 2. Attach a repeating timeout to the uv main loop to keep it from exiting until the signal arrives
	// 3. Send ourselves a SIGUSR2 which we handle outside of node, and wherein we start the main loop

	mainloopState = MAINLOOP_HIJACKING_STARTED;

	// We allocate a main loop and a source only once. After that, we simple start/stop the loop.
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

	// hijack_main_loop() was called, but the idler has not yet run
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

static void hijack_ref() {
	DBG_UV_LOOP_HIJACK(printf("hijack_ref: Entering\n"));
	if (hijack_refcount == 0) {
		DBG_UV_LOOP_HIJACK(printf("hijack_ref: hijacking main loop\n"));
		hijack_main_loop();
	}
	hijack_refcount++;
}

static void hijack_unref() {
	DBG_UV_LOOP_HIJACK(printf("hijack_unref: Entering\n"));
	hijack_refcount--;
	if (hijack_refcount == 0) {
		DBG_UV_LOOP_HIJACK(printf("hijack_unref: releasing main loop\n"));
		release_main_loop();
	}
}

// Intercept calls which add handles to or remove them from the main loop so we can start/stop the
// loop in response.

#define HANDLE_ADD(name, ...) \
	do { \
		void *returnValue = 0; \
		if (sol_init_complete == true) { \
			DBG_UV_LOOP_HIJACK(printf(#name "_add: Reffing loop\n")); \
			hijack_ref(); \
		} \
		returnValue = SOL_MAINLOOP_IMPLEMENTATION_DEFAULT->name##_add( __VA_ARGS__ ); \
		if (!returnValue) { \
			DBG_UV_LOOP_HIJACK(printf(#name "_add: Addition failed - unreffing loop\n")); \
			hijack_unref(); \
		} \
		return returnValue; \
	} while(0)

#define HANDLE_DEL(name, handle) \
	do { \
		DBG_UV_LOOP_HIJACK(printf(#name "_del: Removing\n")); \
		bool returnValue = SOL_MAINLOOP_IMPLEMENTATION_DEFAULT->name##_del(handle); \
		if (returnValue && sol_init_complete == true) { \
			DBG_UV_LOOP_HIJACK(printf(#name "_del: Removal succeeded - unreffing loop\n")); \
			hijack_unref(); \
		} \
		return returnValue; \
	} while(0)

static void *node_sol_loop_timeout_add(uint32_t timeout_ms, bool (*cb)(void *data), const void *data) {
	HANDLE_ADD(timeout, timeout_ms, cb, data);
}

static bool node_sol_loop_timeout_del(void *handle) {
	HANDLE_DEL(timeout, handle);
}

static void *node_sol_loop_idle_add(bool (*cb)(void *data), const void *data) {
	HANDLE_ADD(idle, cb, data);
}

static bool node_sol_loop_idle_del(void *handle) {
	HANDLE_DEL(idle, handle);
}

#ifdef SOL_MAINLOOP_FD_ENABLED
static void *node_sol_loop_fd_add(int the_fd, uint32_t flags, bool (*cb)(void *data, int fd, uint32_t active_flags), const void *data) {
	DBG_UV_LOOP_HIJACK(printf("node_sol_loop_fd_add: Entering\n"));

	// Adding the uv main loop backend fd should not cause reffing
	if (the_fd == uv_backend_fd(uv_default_loop())) {
		DBG_UV_LOOP_HIJACK(printf("node_sol_loop_fd_add: Adding uv backend fd, so skipping refcounting\n"));
		return SOL_MAINLOOP_IMPLEMENTATION_DEFAULT->fd_add(the_fd, flags, cb, data);
	}
	HANDLE_ADD(fd, the_fd, flags, cb, data);
}

static bool node_sol_loop_fd_del(void *handle) {
	DBG_UV_LOOP_HIJACK(printf("node_sol_loop_fd_del: Entering\n"));

	// Removing the main loop backend fd should not cause unreffing
	if (handle == (void *)uv_loop_fd) {
		DBG_UV_LOOP_HIJACK(printf("node_sol_loop_fd_del: Removing uv backend fd, so skipping refcounting\n"));
		return SOL_MAINLOOP_IMPLEMENTATION_DEFAULT->fd_del(handle);
	}
	HANDLE_DEL(fd, handle);
}
#endif /* def SOL_MAINLOOP_FD_ENABLED */

#ifdef SOL_MAINLOOP_FORK_WATCH_ENABLED
static void *node_sol_loop_child_watch_add(uint64_t pid, void (*cb)(void *data, uint64_t pid, int status), const void *data) {
	HANDLE_ADD(child_watch, pid, cb, data);
}

static bool node_sol_loop_child_watch_del(void *handle) {
	HANDLE_DEL(child_watch, handle);
}
#endif /* def SOL_MAINLOOP_FORK_WATCH_ENABLED */

static void *node_sol_loop_source_add(const struct sol_mainloop_source_type *type, const void *data) {
	DBG_UV_LOOP_HIJACK(printf("node_sol_loop_source_add: Entering\n"));

	// Adding the main loop source should not cause reffing
	if (type->dispatch == uv_loop_source_dispatch) {
		DBG_UV_LOOP_HIJACK(printf("node_sol_loop_source_add: Adding uv source, so skipping refcounting\n"));
		return SOL_MAINLOOP_IMPLEMENTATION_DEFAULT->source_add(type, data);
	}
	HANDLE_ADD(source, type, data);
}

static void node_sol_loop_source_del(void *source) {
	DBG_UV_LOOP_HIJACK(printf("node_sol_loop_source_del: Entering\n"));
	SOL_MAINLOOP_IMPLEMENTATION_DEFAULT->source_del(source);

	// Removing the main loop source should not cause unreffing
	if (source != uv_loop_source) {
		DBG_UV_LOOP_HIJACK(printf("node_sol_loop_source_add: Removing non-uv source, so doing refcounting\n"));
		hijack_unref();
	}
}

struct sol_mainloop_implementation node_intercept;

static bool node_sol_already_init = false;
int node_sol_init() {
	int sol_init_result = 0;
	if ( node_sol_already_init) {
		return 0;
	}
	node_sol_already_init = true;

	// Copy the existing soletta mainloop implementation, modify some of the functions, and set it
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
