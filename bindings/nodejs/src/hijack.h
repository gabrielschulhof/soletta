#ifndef __UV_MAIN_LOOP_RUNS_ON_SOLETTA__
#define __UV_MAIN_LOOP_RUNS_ON_SOLETTA__

#ifdef __cplusplus
extern "C" {
#endif /* def __cplusplus */

#define DBG_UV_LOOP_HIJACK(s) s

extern struct sol_mainloop_source *uv_loop_source;
extern bool sol_init_complete;
extern struct sol_fd *uv_loop_fd;

void uv_loop_source_dispatch (void *data);

int node_sol_init();

void hijack_ref();

void hijack_unref();

#ifdef __cplusplus
}
#endif /* def __cplusplus */

#endif /* ndef __UV_MAIN_LOOP_RUNS_ON_SOLETTA__ */
