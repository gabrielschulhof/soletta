#include "async-patterns.h"

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

	// Transfer the item to JS
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[2] = {
		Nan::New(the_entry.the_int),
		Nan::New(the_entry.the_string).ToLocalChecked()
	};
	free(the_entry.the_string);
	jsCallback->Call(2, jsCallbackArguments);
}

// This function is called from the soletta thread. Push the new item into the queue and wake the
// main loop
static void defaultMonitor_soletta(void *data, int the_int, const char *the_string) {
	int_string the_entry = { the_int, strdup(the_string) };
	SOLETTA_CALLBACK(data, the_entry, std::queue<int_string> *);
}

static void free_the_queue(std::queue<int_string> *the_queue) {
	if (the_queue) {
		for(; !the_queue->empty(); the_queue->pop()) {
			free(the_queue->front().the_string);
		}
		delete the_queue;
	}
}

DECLARE_PUBLIC_API_C(int_string, int_string)
