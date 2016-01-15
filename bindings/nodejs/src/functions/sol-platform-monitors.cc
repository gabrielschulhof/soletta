#include <nan.h>
#include "../data.h"
#include "../common.h"

#include <sol-platform.h>

using namespace v8;

#define SOL_PLATFORM_MONITOR_PROCESS_ADD_RESULT(result, jsCallback, info) \
	do { \
		if (result) { \
			delete jsCallback; \
		} else { \
			Nan::ForceSet(info[0]->ToObject(), Nan::New("_callback").ToLocalChecked(), \
				jsArrayFromBytes((unsigned char *)&jsCallback, sizeof(Nan::Callback *)), \
				(PropertyAttribute)(DontDelete | DontEnum | ReadOnly)); \
		} \
		info.GetReturnValue().Set(Nan::New(result)); \
	} while(0)

#define SOL_PLATFORM_MONITOR_ADD(name, callback) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 1); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction); \
\
		Nan::Callback *jsCallback = new Nan::Callback(Local<Function>::Cast(info[0])); \
		int result = sol_platform_add_##name##_monitor((callback), jsCallback); \
\
		SOL_PLATFORM_MONITOR_PROCESS_ADD_RESULT(result, jsCallback, info); \
	} while(0)

#define SOL_PLATFORM_MONITOR_DEL(name, callback) \
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
			int result = sol_platform_del_##name##_monitor((callback), jsCallback); \
			if (!result) { \
				delete jsCallback; \
			} \
			info.GetReturnValue().Set(Nan::New(result)); \
		} \
	} while(0)

static void defaultStringMonitor(void *data, const char *the_string) {
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[1] = {Nan::New(the_string).ToLocalChecked()};
	((Nan::Callback *)data)->Call(1, jsCallbackArguments);
}

static void defaultIntStringMonitor(void *data, int the_int, const char *the_string) {
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[2] = {
		Nan::New(the_int),
		Nan::New(the_string).ToLocalChecked()
	};
	((Nan::Callback *)data)->Call(2, jsCallbackArguments);
}

static void defaultStringIntMonitor(void *data, const char *the_string, int the_int) {
	defaultIntStringMonitor(data, the_int, the_string);
}

static void defaultIntMonitor(void *data, int the_int) {
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[1] = {Nan::New(the_int)};
	((Nan::Callback *)data)->Call(1, jsCallbackArguments);
}

static void defaultTimestampMonitor(void *data, int64_t the_timestamp) {
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[1] = {Nan::New<Date>((double)the_timestamp).ToLocalChecked()};
	((Nan::Callback *)data)->Call(1, jsCallbackArguments);
}

NAN_METHOD(bind_sol_platform_add_hostname_monitor) {
	SOL_PLATFORM_MONITOR_ADD(hostname,
		(void(*)(void *, const char *))defaultStringMonitor);
}

NAN_METHOD(bind_sol_platform_del_hostname_monitor) {
	SOL_PLATFORM_MONITOR_DEL(hostname,
		(void(*)(void *, const char *))defaultStringMonitor);
}

NAN_METHOD(bind_sol_platform_add_timezone_monitor) {
	SOL_PLATFORM_MONITOR_ADD(timezone,
		(void(*)(void *, const char *))defaultStringMonitor);
}

NAN_METHOD(bind_sol_platform_del_timezone_monitor) {
	SOL_PLATFORM_MONITOR_DEL(timezone,
		(void(*)(void *, const char *))defaultStringMonitor);
}

NAN_METHOD(bind_sol_platform_add_locale_monitor) {
	SOL_PLATFORM_MONITOR_ADD(locale,
		(void(*)(void *, enum sol_platform_locale_category, const char *))defaultIntStringMonitor);
}

NAN_METHOD(bind_sol_platform_del_locale_monitor) {
	SOL_PLATFORM_MONITOR_DEL(locale,
		(void(*)(void *, enum sol_platform_locale_category, const char *))defaultIntStringMonitor);
}

NAN_METHOD(bind_sol_platform_add_service_monitor) {
	VALIDATE_ARGUMENT_COUNT(info, 2);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsString);

	Nan::Callback *jsCallback = new Nan::Callback(Local<Function>::Cast(info[0]));
	int result = sol_platform_add_service_monitor(
		(void(*)(void *, const char *, enum sol_platform_service_state))defaultStringIntMonitor,
		(const char *)*String::Utf8Value(info[1]),
		jsCallback);

	SOL_PLATFORM_MONITOR_PROCESS_ADD_RESULT(result, jsCallback, info);
}

NAN_METHOD(bind_sol_platform_del_service_monitor) {
	VALIDATE_ARGUMENT_COUNT(info, 2);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsString);

	Nan::Callback *jsCallback = 0;
	if (fillCArrayFromJSArray((unsigned char *)&jsCallback,
			sizeof(Nan::Callback *),
			Local<Array>::Cast(Nan::Get(info[0]->ToObject(),
				Nan::New("_callback").ToLocalChecked()).ToLocalChecked()))) {

		int result = sol_platform_del_service_monitor(
			(void(*)(void *, const char *, enum sol_platform_service_state))defaultStringIntMonitor,
			(const char *)*String::Utf8Value(info[1]),
			jsCallback);
		if (!result) {
			delete jsCallback;
		}
		info.GetReturnValue().Set(Nan::New(result));
	}
}

NAN_METHOD(bind_sol_platform_add_state_monitor) {
	SOL_PLATFORM_MONITOR_ADD(state,
		(void(*)(void *, enum sol_platform_state))defaultIntMonitor);
}

NAN_METHOD(bind_sol_platform_del_state_monitor) {
	SOL_PLATFORM_MONITOR_DEL(state,
		(void(*)(void *, enum sol_platform_state))defaultIntMonitor);
}

NAN_METHOD(bind_sol_platform_add_system_clock_monitor) {
	SOL_PLATFORM_MONITOR_ADD(system_clock, defaultTimestampMonitor);
}

NAN_METHOD(bind_sol_platform_del_system_clock_monitor) {
	SOL_PLATFORM_MONITOR_DEL(system_clock, defaultTimestampMonitor);
}
