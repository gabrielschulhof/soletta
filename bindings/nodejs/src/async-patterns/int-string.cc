#include <nan.h>
#include "int-string.h"

using namespace v8;

struct int_string {
	int the_int;
	char *the_string;
};

static void deliver_one_item(uv_idle_t *handle) {
	int_string the_entry;
	Nan::Callback *jsCallback;

	// Place an item from the queue into the_entry and retrieve the JS callback
	DELIVER_ONE_ITEM(handle, the_entry, jsCallback, std::queue<int_string> *);

	Local<Value> jsCallbackArguments[2] = {
		Nan::New(the_entry.the_int),
		Nan::New(the_entry.the_string).ToLocalChecked()
	};
	free(the_entry.the_string);
	jsCallback->Call(2, jsCallbackArguments);
}

// This function is called from the soletta thread. Free an existing string if present. Copy the
// new string to the async structure and wake the node main loop.
static void defaultIntStringMonitor_soletta(void *data, int the_int, const char *the_string) {
	int_string the_entry = { the_int, strdup(the_string) };
	SOLETTA_CALLBACK(((uv_async_monitor_t *)data), the_entry, std::queue<int_string> *);
}

static void free_the_queue(generic_queue *the_queue) {
	if (the_queue) {
		std::queue<int_string> *real_queue = (std::queue<int_string> *)the_queue;
		while (!real_queue->empty()) {
			free(real_queue->front().the_string);
			real_queue->pop();
		}
		delete real_queue;
	}
}

uv_async_int_string_monitor_t *uv_async_int_string_monitor_new(Nan::Callback *jsCallback) {
	return (uv_async_int_string_monitor_t *)uv_async_monitor_new(
		jsCallback,
		new std::queue<int_string>,
		free_the_queue,
		deliver_one_item,
		(void(*)(void *))defaultIntStringMonitor_soletta);
}

void uv_async_int_string_monitor_free(uv_async_int_string_monitor_t *monitor) {
	uv_async_monitor_free((uv_async_monitor_t *)monitor);
}
