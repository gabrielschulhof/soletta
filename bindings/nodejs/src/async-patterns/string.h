#ifndef __SOLETTA_NODE_JS_ASYNC_PATTERN_STRING__
#define __SOLETTA_NODE_JS_ASYNC_PATTERN_STRING__

#include <uv.h>
#include <nan.h>

typedef struct {
	uv_async_t base;
	Nan::Callback *jsCallback;
	char *single_string;
	void (*soletta_callback)(void *data, const char *single_string);
} uv_async_single_string_monitor_t;

uv_async_single_string_monitor_t *uv_async_single_string_monitor_new(Nan::Callback *jsCallback, char *single_string);

void uv_async_single_string_monitor_free(uv_async_single_string_monitor_t *monitor);

#endif /* ndef __SOLETTA_NODE_JS_ASYNC_PATTERN_STRING__ */
