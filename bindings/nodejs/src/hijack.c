/*
 * This file is part of the Soletta Project
 *
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <uv.h>
#include <sol-mainloop.h>

//#define DEBUG_ON
#ifdef DEBUG_ON
#define DBG_UV_LOOP_HIJACK(s) s
static const char *stateDescription[] = {
    "MAINLOOP_HIJACKING_STARTED",
    "MAINLOOP_HIJACKED",
    "MAINLOOP_RELEASING_STARTED",
    "MAINLOOP_RELEASED"
};
#else /* ndef DEBUG_ON */
#define DBG_UV_LOOP_HIJACK(s)
#endif /* def DEBUG_ON */

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

static void
uv_idle_callback()
{
    DBG_UV_LOOP_HIJACK(printf("uv_idle_callback: Entering with state %s\n",
        stateDescription[mainloopState]));
    if (mainloopState == MAINLOOP_HIJACKING_STARTED) {
        DBG_UV_LOOP_HIJACK(printf("uv_idle_callback: running sol_run()\n"));
        mainloopState = MAINLOOP_HIJACKED;
        sol_run();
        DBG_UV_LOOP_HIJACK(printf("uv_idle_callback: sol_run() has returned. state is %s\n",
            stateDescription[mainloopState]));
        if (mainloopState == MAINLOOP_RELEASING_STARTED) {
            mainloopState = MAINLOOP_RELEASED;
        }
    } else if ( mainloopState == MAINLOOP_HIJACKED) {
        DBG_UV_LOOP_HIJACK(printf("uv_idle_callback: main loop already hijacked, so stopping idler\n"));
        uv_idle_stop(&uv_idle);
    }
}

static bool
uv_loop_source_check(void *data)
{
    uv_loop_t *loop = (uv_loop_t *)data;

    uv_update_time(loop);

    DBG_UV_LOOP_HIJACK(printf("uv_loop_source_check: Returning %s\n", uv_loop_alive(loop) ? "true" : "false"));
    return uv_loop_alive(loop);
}

static bool
uv_loop_source_get_next_timeout(void *data, struct timespec *timeout)
{
    /* Otherwise, check the timeout. If the timeout is 0, that means we're
     * ready to go. Otherwise, keep sleeping until the timeout happens again. */
    int t = uv_backend_timeout((uv_loop_t *)data);

    DBG_UV_LOOP_HIJACK(printf("uv_loop_source_get_next_timeout: t = %d\n", t));

    timeout->tv_sec = (int)(t / 1000);
    timeout->tv_nsec = (t % 1000) * 1000000;

    return ( t >= 0 ) && uv_loop_source_check(data);
}

static void
uv_loop_source_dispatch(void *data)
{
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

static bool
uv_loop_fd_changed(void *data, int fd, uint32_t active_flags)
{
    DBG_UV_LOOP_HIJACK(printf("uv_loop_fd_changed: Running one uv loop iteration\n"));
    uv_run((uv_loop_t *)data, UV_RUN_NOWAIT);
    return true;
}

static void
hijack_main_loop()
{
    DBG_UV_LOOP_HIJACK(printf("hijack_main_loop: Entering with state %s\n",
        stateDescription[mainloopState]));
    if (mainloopState == MAINLOOP_HIJACKED || mainloopState == MAINLOOP_HIJACKING_STARTED) {
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

static void
release_main_loop()
{
    DBG_UV_LOOP_HIJACK(printf("release_main_loop: Entering with state %s\n",
        stateDescription[mainloopState]));
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

void
hijack_ref()
{
    DBG_UV_LOOP_HIJACK(printf("hijack_ref: Entering\n"));
    if (hijack_refcount == 0) {
        DBG_UV_LOOP_HIJACK(printf("hijack_ref: hijacking main loop\n"));
        hijack_main_loop();
    }
    hijack_refcount++;
}

void
hijack_unref()
{
    DBG_UV_LOOP_HIJACK(printf("hijack_unref: Entering\n"));
    hijack_refcount--;
    if (hijack_refcount == 0) {
        DBG_UV_LOOP_HIJACK(printf("hijack_unref: releasing main loop\n"));
        release_main_loop();
    }
}
