#ifndef __SOLETTA_NODE_JS_ASYNC_PATTERN_INT_STRING__
#define __SOLETTA_NODE_JS_ASYNC_PATTERN_INT_STRING__

#include <uv.h>
#include <nan.h>

typedef struct {
	uv_async_t base;
	Nan::Callback *jsCallback;
	void (*soletta_callback)(void *data, int the_int, const char *the_string);
	char *the_string;
	int the_int;
} uv_async_int_string_monitor_t;

uv_async_int_string_monitor_t *uv_async_int_string_monitor_new(Nan::Callback *jsCallback);

void uv_async_int_string_monitor_free(uv_async_int_string_monitor_t *monitor);

#endif /* ndef __SOLETTA_NODE_JS_ASYNC_PATTERN_INT_STRING__ */
