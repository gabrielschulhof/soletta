#include <nan.h>
#include "../data.h"
#include "../common.h"
#include "../hijack.h"

#include <sol-platform.h>

using namespace v8;

void defaultHostnameMonitor(void *data, const char *hostname) {
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[1] = {Nan::New(hostname).ToLocalChecked()};
	((Nan::Callback *)data)->Call(1, jsCallbackArguments);
}

NAN_METHOD(bind_sol_platform_add_hostname_monitor) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction);

	Nan::Callback *jsCallback = new Nan::Callback(Local<Function>::Cast(info[0]));

	int result = sol_platform_add_hostname_monitor(defaultHostnameMonitor, jsCallback);

	if (result) {
		delete jsCallback;
	} else {
		Nan::ForceSet(info[0]->ToObject(), Nan::New("_callback").ToLocalChecked(),
			jsArrayFromBytes((unsigned char *)&jsCallback, sizeof(Nan::Callback *)),
			(PropertyAttribute)(DontDelete | DontEnum | ReadOnly));
	}
	info.GetReturnValue().Set(Nan::New(result));
	hijack_ref();
}

NAN_METHOD(bind_sol_platform_del_hostname_monitor) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction);

	Nan::Callback *jsCallback = 0;
	if (fillCArrayFromJSArray((unsigned char *)&jsCallback,
			sizeof(Nan::Callback *),
			Local<Array>::Cast(Nan::Get(info[0]->ToObject(),
				Nan::New("_callback").ToLocalChecked()).ToLocalChecked()))) {

		int result = sol_platform_del_hostname_monitor(defaultHostnameMonitor, jsCallback);
		if (!result) {
			delete jsCallback;
		}
		info.GetReturnValue().Set(Nan::New(result));
		hijack_unref();
	}
}
