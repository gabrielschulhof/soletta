#ifndef __SOLETTA_NODE_JS_ASYNC_PATTERN_INT_STRING__
#define __SOLETTA_NODE_JS_ASYNC_PATTERN_INT_STRING__

#include <queue>
#include <uv.h>
#include <nan.h>
#include "generic.h"

typedef uv_async_monitor_t uv_async_int_string_monitor_t;

uv_async_int_string_monitor_t *uv_async_int_string_monitor_new(Nan::Callback *jsCallback);

void uv_async_int_string_monitor_free(uv_async_int_string_monitor_t *monitor);

#endif /* ndef __SOLETTA_NODE_JS_ASYNC_PATTERN_INT_STRING__ */
