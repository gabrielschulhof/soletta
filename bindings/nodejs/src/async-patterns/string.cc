#include <nan.h>
#include "string.h"

using namespace v8;

extern uv_rwlock_t big_giant_lock;

// This function is called from the libuv main loop via uv_async_send(). Call the JS callback with
// the new string then free the string.
static void defaultSingleStringMonitor_node(uv_async_t *handle) {
	uv_async_single_string_monitor_t *monitor = (uv_async_single_string_monitor_t *)handle;
	Nan::HandleScope scope;

	uv_rwlock_rdlock(&big_giant_lock);
	Local<Value> jsCallbackArguments[1] = {Nan::New(monitor->single_string).ToLocalChecked()};
	uv_rwlock_rdunlock(&big_giant_lock);

	monitor->jsCallback->Call(1, jsCallbackArguments);
}

// This function is called from the soletta thread. Free an existing string if present. Copy the
// new string to the async structure and wake the node main loop.
static void defaultSingleStringMonitor_soletta(void *data, const char *single_string) {
	uv_async_single_string_monitor_t *monitor = (uv_async_single_string_monitor_t *)data;

	uv_rwlock_wrlock(&big_giant_lock);
	if (monitor->single_string) {
		free(monitor->single_string);
	}
	monitor->single_string = strdup(single_string);
	uv_rwlock_wrunlock(&big_giant_lock);

	uv_async_send((uv_async_t *)monitor);
}

uv_async_single_string_monitor_t *uv_async_single_string_monitor_new(Nan::Callback *jsCallback, char *single_string) {
	uv_async_single_string_monitor_t *monitor =
		(uv_async_single_string_monitor_t *)malloc(sizeof(uv_async_single_string_monitor_t));
	if (monitor) {
		uv_async_init(uv_default_loop(), (uv_async_t *)monitor, defaultSingleStringMonitor_node);
		monitor->jsCallback = jsCallback;
		monitor->single_string = single_string;
		monitor->soletta_callback = defaultSingleStringMonitor_soletta;
	}
	return monitor;
}

void uv_async_single_string_monitor_free(uv_async_single_string_monitor_t *monitor) {
	if (monitor) {
		if (monitor->single_string) {
			free(monitor->single_string);
		}
		if (monitor->jsCallback) {
			delete monitor->jsCallback;
		}
		free(monitor);
	}
}
