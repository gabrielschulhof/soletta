#include <queue>
#include <nan.h>
#include "string.h"

using namespace v8;

static void deliver_one_item(uv_idle_t *handle) {
	char *the_entry;
	Nan::Callback *jsCallback;

	// Place an item from the queue into the_entry and retrieve the JS callback
	DELIVER_ONE_ITEM(handle, the_entry, jsCallback, std::queue<char *> *);

	// Transfer the item to JS
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[1] = {Nan::New(the_entry).ToLocalChecked()};
	free(the_entry);
	jsCallback->Call(1, jsCallbackArguments);
}

// This function is called from the soletta thread. Push the new string into the queue and wake the
// main loop
static void defaultStringMonitor_soletta(void *data, const char *the_string) {
	char *the_entry = strdup(the_string);
	SOLETTA_CALLBACK(((uv_async_monitor_t *)data), the_entry, std::queue<char *> *);
}

static void free_the_queue(generic_queue *the_queue) {
	if (the_queue) {
		std::queue<char *> *real_queue = (std::queue<char *> *)the_queue;
		while (!real_queue->empty()) {
			free(real_queue->front());
			real_queue->pop();
		}
		delete real_queue;
	}
}

uv_async_string_monitor_t *uv_async_string_monitor_new(Nan::Callback *jsCallback) {
	return (uv_async_string_monitor_t *)uv_async_monitor_new(
		jsCallback,
		new std::queue<char *>,
		free_the_queue,
		deliver_one_item,
		(void(*)(void *))defaultStringMonitor_soletta);
}

void uv_async_string_monitor_free(uv_async_string_monitor_t *monitor) {
	uv_async_monitor_free((uv_async_monitor_t *)monitor);
}
