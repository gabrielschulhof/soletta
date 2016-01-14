#include "async-patterns.h"

using namespace v8;

static void deliver_one_item(uv_idle_t *handle) {
	char *the_entry;
	Nan::Callback *jsCallback;

	// Place an item from the queue into the_entry and retrieve the JS callback
	DELIVER_ONE_ITEM(handle, the_entry, jsCallback, std::queue<int> *);

	// Transfer the item to JS
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[1] = {Nan::New(the_entry)};
	jsCallback->Call(1, jsCallbackArguments);
}

// This function is called from the soletta thread. Push the new item into the queue and wake the
// main loop
static void defaultMonitor_soletta(void *data, int the_int) {
	SOLETTA_CALLBACK(data, the_int, std::queue<int> *);
}

static void free_the_queue(std::queue<int> *the_queue) {
	if (the_queue) {
		delete the_queue;
	}
}

DECLARE_PUBLIC_API_C(int, int)
