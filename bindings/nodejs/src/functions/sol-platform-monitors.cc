#include <nan.h>
#include "../data.h"
#include "../common.h"
#include "../hijack.h"

#include <sol-platform.h>

using namespace v8;

#define SOL_PLATFORM_MONITOR_ADD(name, callback_type, monitor_name) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 1); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction); \
\
		Nan::Callback *jsCallback = new Nan::Callback(Local<Function>::Cast(info[0])); \
		int result = sol_platform_add_##name##_monitor((callback_type)monitor_name, jsCallback); \
\
		if (result) { \
			delete jsCallback; \
		} else { \
			Nan::ForceSet(info[0]->ToObject(), Nan::New("_callback").ToLocalChecked(), \
				jsArrayFromBytes((unsigned char *)&jsCallback, sizeof(Nan::Callback *)), \
				(PropertyAttribute)(DontDelete | DontEnum | ReadOnly)); \
		} \
		info.GetReturnValue().Set(Nan::New(result)); \
		hijack_ref(); \
	} while(0)

#define SOL_PLATFORM_MONITOR_DEL(name, callback_type, monitor_name) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 1); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction); \
\
		Nan::Callback *jsCallback = 0; \
		if (fillCArrayFromJSArray((unsigned char *)&jsCallback, \
				sizeof(Nan::Callback *), \
				Local<Array>::Cast(Nan::Get(info[0]->ToObject(), \
					Nan::New("_callback").ToLocalChecked()).ToLocalChecked()))) { \
\
			int result = sol_platform_del_##name##_monitor((callback_type)monitor_name, jsCallback); \
			if (!result) { \
				delete jsCallback; \
			} \
			info.GetReturnValue().Set(Nan::New(result)); \
			hijack_unref(); \
		} \
	} while(0)

void defaultStringMonitor(void *data, const char *the_string) {
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[1] = {Nan::New(the_string).ToLocalChecked()};
	((Nan::Callback *)data)->Call(1, jsCallbackArguments);
}

void defaultIntStringMonitor(void *data, int the_int, const char *the_string) {
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[2] = {
		Nan::New(the_int),
		Nan::New(the_string).ToLocalChecked()
	};
	((Nan::Callback *)data)->Call(2, jsCallbackArguments);
}

NAN_METHOD(bind_sol_platform_add_hostname_monitor) {
	SOL_PLATFORM_MONITOR_ADD(hostname,
		void(*)(void *, const char *),
		defaultStringMonitor);
}

NAN_METHOD(bind_sol_platform_del_hostname_monitor) {
	SOL_PLATFORM_MONITOR_DEL(hostname,
		void(*)(void *, const char *),
		defaultStringMonitor);
}

NAN_METHOD(bind_sol_platform_add_timezone_monitor) {
	SOL_PLATFORM_MONITOR_ADD(timezone,
		void(*)(void *, const char *),
		defaultStringMonitor);
}

NAN_METHOD(bind_sol_platform_del_timezone_monitor) {
	SOL_PLATFORM_MONITOR_DEL(timezone,
		void(*)(void *, const char *),
		defaultStringMonitor);
}

NAN_METHOD(bind_sol_platform_add_locale_monitor) {
	SOL_PLATFORM_MONITOR_ADD(locale,
		void(*)(void *, enum sol_platform_locale_category, const char *),
		defaultIntStringMonitor);
}

NAN_METHOD(bind_sol_platform_del_locale_monitor) {
	SOL_PLATFORM_MONITOR_DEL(locale,
		void(*)(void *, enum sol_platform_locale_category, const char *),
		defaultIntStringMonitor);
}
