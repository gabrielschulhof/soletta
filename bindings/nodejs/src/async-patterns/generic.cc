#include <stdlib.h>
#include <nan.h>
#include "generic.h"

using namespace v8;

extern uv_mutex_t big_giant_mutex;

// This function is called from the libuv main loop via uv_async_send(). Call the JS callback with
// the new string then free the string.
static void defaultMonitor_node(uv_async_t *handle) {
	uv_idle_t *the_idle = (uv_idle_t *)&(((uv_async_monitor_t *)handle)->idle);

	uv_idle_stop(the_idle);
	uv_idle_init(uv_default_loop(), the_idle);
	uv_idle_start(the_idle, ((uv_async_idle_t *)the_idle)->deliver_one_item);
}

uv_async_monitor_t *uv_async_monitor_new(
		Nan::Callback *jsCallback,
		generic_queue *the_queue,
		void (*free_the_queue)(generic_queue *the_queue),
		void (*deliver_one_item)(uv_idle_t *idle),
		void (*soletta_callback)(void *data)) {

	uv_loop_t *the_loop = 0;
	uv_async_monitor_t *monitor = (uv_async_monitor_t *)malloc(sizeof(uv_async_monitor_t));

	if (monitor) {
		memset(monitor, 0, sizeof(uv_async_monitor_t));
		the_loop = uv_default_loop();
		uv_async_init(the_loop, (uv_async_t *)monitor, defaultMonitor_node);
		uv_idle_init(the_loop, (uv_idle_t *)(&(monitor->idle)));
		monitor->idle.the_queue = the_queue;
		monitor->idle.jsCallback = jsCallback;
		monitor->idle.free_the_queue = free_the_queue;
		monitor->idle.deliver_one_item = deliver_one_item;
		monitor->soletta_callback = soletta_callback;
		monitor->idle.monitor = monitor;
	}

	return monitor;
}

static void monitor_free(uv_handle_t *handle) {
	uv_async_monitor_t *monitor = ((uv_async_idle_t *)handle)->monitor;

	if (monitor) {
		monitor->idle.free_the_queue(monitor->idle.the_queue);
		if (monitor->idle.jsCallback) {
			delete monitor->idle.jsCallback;
		}
		free(monitor);
	}
}

void uv_async_monitor_free(uv_async_monitor_t *monitor) {
	uv_close((uv_handle_t *)(&(monitor->idle)), monitor_free);
}
