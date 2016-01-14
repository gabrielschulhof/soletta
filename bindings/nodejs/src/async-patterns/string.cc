#include "async-patterns.h"

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

// This function is called from the soletta thread. Push the new item into the queue and wake the
// main loop
static void defaultMonitor_soletta(void *data, const char *the_string) {
	char *the_entry = strdup(the_string);
	SOLETTA_CALLBACK(data, the_entry, std::queue<char *> *);
}

static void free_the_queue(std::queue<char *> *the_queue) {
	if (the_queue) {
		for(; !the_queue->empty(); the_queue->pop()) {
			free(the_queue->front());
		}
		delete the_queue;
	}
}

DECLARE_PUBLIC_API_C(string, char *)
