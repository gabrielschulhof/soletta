#include <nan.h>
#include "int-string.h"

using namespace v8;

extern uv_rwlock_t big_giant_lock;

// This function is called from the libuv main loop via uv_async_send(). Call the JS callback with
// the new string then free the string.
static void defaultIntStringMonitor_node(uv_async_t *handle) {
	uv_async_int_string_monitor_t *monitor = (uv_async_int_string_monitor_t *)handle;
	Nan::HandleScope scope;

	uv_rwlock_rdlock(&big_giant_lock);
	Local<Value> jsCallbackArguments[2] = {
		Nan::New(monitor->the_int),
		Nan::New(monitor->the_string).ToLocalChecked()
	};
	uv_rwlock_rdunlock(&big_giant_lock);

	monitor->jsCallback->Call(2, jsCallbackArguments);
}

// This function is called from the soletta thread. Free an existing string if present. Copy the
// new string to the async structure and wake the node main loop.
static void defaultIntStringMonitor_soletta(void *data, int the_int, const char *the_string) {
	uv_async_int_string_monitor_t *monitor = (uv_async_int_string_monitor_t *)data;

	uv_rwlock_wrlock(&big_giant_lock);
	if (monitor->the_string) {
		free(monitor->the_string);
	}
	monitor->the_string = strdup(the_string);
	monitor->the_int = the_int;
	uv_rwlock_wrunlock(&big_giant_lock);

	uv_async_send((uv_async_t *)monitor);
}

uv_async_int_string_monitor_t *uv_async_int_string_monitor_new(Nan::Callback *jsCallback) {
	uv_async_int_string_monitor_t *monitor =
		(uv_async_int_string_monitor_t *)malloc(sizeof(uv_async_int_string_monitor_t));
	if (monitor) {
		memset((void *)monitor, 0, sizeof(uv_async_int_string_monitor_t));
		uv_async_init(uv_default_loop(), (uv_async_t *)monitor, defaultIntStringMonitor_node);
		monitor->jsCallback = jsCallback;
		monitor->soletta_callback = defaultIntStringMonitor_soletta;
	}
	return monitor;
}

void uv_async_int_string_monitor_free(uv_async_int_string_monitor_t *monitor) {
	if (monitor) {
		if (monitor->the_string) {
			free(monitor->the_string);
		}
		if (monitor->jsCallback) {
			delete monitor->jsCallback;
		}
		free(monitor);
	}
}
