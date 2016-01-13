#include <string.h>
#include <nan.h>
#include "../data.h"
#include "../common.h"

#include <sol-platform.h>

using namespace v8;

static void defaultHostnameMonitor_node(uv_async_t *handle);

typedef struct {
	uv_async_t base;
	uv_rwlock_t lock;
	Nan::Callback *jsCallback;
	char *hostname;
} uv_async_hostname_monitor_t;

static uv_async_hostname_monitor_t *uv_async_hostname_monitor_new(Nan::Callback *jsCallback, char *hostname) {
	uv_async_hostname_monitor_t *monitor =
		(uv_async_hostname_monitor_t *)malloc(sizeof(uv_async_hostname_monitor_t));
	if (monitor) {
		uv_async_init(uv_default_loop(), (uv_async_t *)monitor, defaultHostnameMonitor_node);
		uv_rwlock_init(&(monitor->lock));
		monitor->jsCallback = jsCallback;
		monitor->hostname = hostname;
	}
	return monitor;
}

static void uv_async_hostname_monitor_free(uv_async_hostname_monitor_t *monitor) {
	if (monitor) {
		if (monitor->hostname) {
			free(monitor->hostname);
		}
		if (monitor->jsCallback) {
			delete monitor->jsCallback;
		}
		uv_rwlock_destroy(&(monitor->lock));
		uv_close((uv_handle_t *)monitor, (uv_close_cb)free);
	}
}

// This function is called from the libuv main loop via uv_async_send(). Call the JS callback with
// the new hostname then free the hostname, because it was allocated on the soletta thread.
static void defaultHostnameMonitor_node(uv_async_t *handle) {
	uv_async_hostname_monitor_t *monitor = (uv_async_hostname_monitor_t *)handle;
	Nan::HandleScope scope;

	uv_rwlock_rdlock(&(monitor->lock));
	Local<Value> jsCallbackArguments[1] = {Nan::New(monitor->hostname).ToLocalChecked()};
	uv_rwlock_rdunlock(&(monitor->lock));

	monitor->jsCallback->Call(1, jsCallbackArguments);
}

// This function is called from the soletta thread. Copy the hostname to the async structure and
// wake the node main loop.
static void defaultHostnameMonitor_soletta(void *data, const char *hostname) {
	uv_async_hostname_monitor_t *monitor = (uv_async_hostname_monitor_t *)data;

	uv_rwlock_wrlock(&(monitor->lock));
	if (monitor->hostname) {
		free(monitor->hostname);
	}
	monitor->hostname = strdup(hostname);
	uv_rwlock_wrunlock(&(monitor->lock));

	uv_async_send((uv_async_t *)monitor);
}

NAN_METHOD(bind_sol_platform_add_hostname_monitor) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction);

	uv_async_hostname_monitor_t *monitor = uv_async_hostname_monitor_new(
		new Nan::Callback(Local<Function>::Cast(info[0])), NULL);
	if (!monitor) {
		Nan::ThrowError("Unable to add hostname monitor");
		return;
	}

	int result = sol_platform_add_hostname_monitor(defaultHostnameMonitor_soletta, monitor);

	if (result) {
		uv_async_hostname_monitor_free(monitor);
	} else {
		Nan::ForceSet(info[0]->ToObject(), Nan::New("_monitor").ToLocalChecked(),
			jsArrayFromBytes((unsigned char *)&monitor, sizeof(uv_async_hostname_monitor_t *)),
			(PropertyAttribute)(DontDelete | DontEnum | ReadOnly));
	}
	info.GetReturnValue().Set(Nan::New(result));
}

NAN_METHOD(bind_sol_platform_del_hostname_monitor) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction);

	Local<String> propertyName = Nan::New("_monitor").ToLocalChecked();
	Local<Object> jsCallbackAsObject = info[0]->ToObject();

	if (Nan::Has(jsCallbackAsObject, propertyName) == Nan::Just(false)) {
		return;
	}

	uv_async_hostname_monitor_t *monitor = 0;
	if (fillCArrayFromJSArray((unsigned char *)&monitor,
			sizeof(uv_async_hostname_monitor_t *),
			Local<Array>::Cast(Nan::Get(jsCallbackAsObject, propertyName).ToLocalChecked()))) {

		int result = sol_platform_del_hostname_monitor(defaultHostnameMonitor_soletta, monitor);
		if (result) {
			Nan::ThrowError("Failed to remove hostname monitor");
			return;
		} else {
			uv_async_hostname_monitor_free(monitor);
			Nan::Delete(jsCallbackAsObject, propertyName);
		}
		info.GetReturnValue().Set(Nan::New(result));
	}
}
