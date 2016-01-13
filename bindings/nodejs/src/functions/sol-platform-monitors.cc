#include <string.h>
#include <nan.h>
#include <sol-platform.h>

#include "../data.h"
#include "../common.h"
#include "../async-patterns/string.h"

using namespace v8;

extern uv_rwlock_t big_giant_lock;

#define SINGLE_STRING_BINDING_ADD(name) \
	VALIDATE_ARGUMENT_COUNT(info, 1); \
	VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction); \
\
	uv_async_single_string_monitor_t *monitor = uv_async_single_string_monitor_new( \
		new Nan::Callback(Local<Function>::Cast(info[0])), NULL); \
	if (!monitor) { \
		Nan::ThrowError("Unable to add " #name " monitor"); \
		return; \
	} \
\
	int result = sol_platform_add_##name##_monitor(monitor->soletta_callback, monitor); \
\
	if (result) { \
		uv_close((uv_handle_t *)monitor, (void(*)(uv_handle_t *))uv_async_single_string_monitor_free); \
	} else { \
		Nan::ForceSet(info[0]->ToObject(), Nan::New("_monitor").ToLocalChecked(), \
			jsArrayFromBytes((unsigned char *)&monitor, sizeof(uv_async_single_string_monitor_t *)), \
			(PropertyAttribute)(DontDelete | DontEnum | ReadOnly)); \
	} \
	info.GetReturnValue().Set(Nan::New(result));

#define SINGLE_STRING_BINDING_DEL(name) \
	VALIDATE_ARGUMENT_COUNT(info, 1); \
	VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction); \
\
	Local<String> propertyName = Nan::New("_monitor").ToLocalChecked(); \
	Local<Object> jsCallbackAsObject = info[0]->ToObject(); \
\
	if (Nan::Has(jsCallbackAsObject, propertyName) == Nan::Just(false)) { \
		return; \
	} \
\
	uv_async_single_string_monitor_t *monitor = 0; \
	if (fillCArrayFromJSArray((unsigned char *)&monitor, \
			sizeof(uv_async_single_string_monitor_t *), \
			Local<Array>::Cast(Nan::Get(jsCallbackAsObject, propertyName).ToLocalChecked()))) { \
\
		int result = sol_platform_del_##name##_monitor(monitor->soletta_callback, monitor); \
		if (result) { \
			Nan::ThrowError("Failed to remove " #name " monitor"); \
			return; \
		} else { \
			uv_close((uv_handle_t *)monitor, (void (*)(uv_handle_t *))uv_async_single_string_monitor_free); \
			Nan::Delete(jsCallbackAsObject, propertyName); \
		} \
		info.GetReturnValue().Set(Nan::New(result)); \
	}

NAN_METHOD(bind_sol_platform_add_hostname_monitor) {
	SINGLE_STRING_BINDING_ADD(hostname);
}

NAN_METHOD(bind_sol_platform_del_hostname_monitor) {
	SINGLE_STRING_BINDING_DEL(hostname);
}

NAN_METHOD(bind_sol_platform_add_timezone_monitor) {
	SINGLE_STRING_BINDING_ADD(timezone);
}

NAN_METHOD(bind_sol_platform_del_timezone_monitor) {
	SINGLE_STRING_BINDING_DEL(timezone);
}
