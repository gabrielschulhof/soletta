#include <string.h>
#include <nan.h>
#include <sol-platform.h>

#include "../data.h"
#include "../common.h"
#include "../async-patterns/string.h"
#include "../async-patterns/int-string.h"

using namespace v8;

#define MONITOR_BINDING_ADD(name, signature, callback_cast) \
	VALIDATE_ARGUMENT_COUNT(info, 1); \
	VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction); \
\
	uv_async_##signature##_monitor_t *monitor = uv_async_##signature##_monitor_new( \
		new Nan::Callback(Local<Function>::Cast(info[0]))); \
	if (!monitor) { \
		Nan::ThrowError("Unable to add " #name " monitor"); \
		return; \
	} \
\
	int result = sol_platform_add_##name##_monitor((callback_cast)(monitor->soletta_callback), monitor); \
\
	if (result) { \
		uv_close((uv_handle_t *)monitor, (void(*)(uv_handle_t *))uv_async_##signature##_monitor_free); \
	} else { \
		Nan::ForceSet(info[0]->ToObject(), Nan::New("_monitor").ToLocalChecked(), \
			jsArrayFromBytes((unsigned char *)&monitor, sizeof(uv_async_##signature##_monitor_t *)), \
			(PropertyAttribute)(DontDelete | DontEnum | ReadOnly)); \
	} \
	info.GetReturnValue().Set(Nan::New(result));

#define MONITOR_BINDING_DEL(name, signature, callback_cast) \
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
	uv_async_##signature##_monitor_t *monitor = 0; \
	if (fillCArrayFromJSArray((unsigned char *)&monitor, \
			sizeof(uv_async_##signature##_monitor_t *), \
			Local<Array>::Cast(Nan::Get(jsCallbackAsObject, propertyName).ToLocalChecked()))) { \
\
		int result = sol_platform_del_##name##_monitor((callback_cast)(monitor->soletta_callback), monitor); \
		if (result) { \
			Nan::ThrowError("Failed to remove " #name " monitor"); \
			return; \
		} else { \
			uv_close((uv_handle_t *)monitor, (void (*)(uv_handle_t *))uv_async_##signature##_monitor_free); \
			Nan::Delete(jsCallbackAsObject, propertyName); \
		} \
		info.GetReturnValue().Set(Nan::New(result)); \
	}

NAN_METHOD(bind_sol_platform_add_hostname_monitor) {
	MONITOR_BINDING_ADD(hostname, string,
		void(*)(void *data, const char *));
}

NAN_METHOD(bind_sol_platform_del_hostname_monitor) {
	MONITOR_BINDING_DEL(hostname, string,
		void(*)(void *data, const char *));
}

NAN_METHOD(bind_sol_platform_add_timezone_monitor) {
	MONITOR_BINDING_ADD(timezone, string,
		void(*)(void *data, const char *));
}

NAN_METHOD(bind_sol_platform_del_timezone_monitor) {
	MONITOR_BINDING_DEL(timezone, string,
		void(*)(void *data, const char *));
}

NAN_METHOD(bind_sol_platform_add_locale_monitor) {
	MONITOR_BINDING_ADD(locale, int_string,
		void(*)(void *, enum sol_platform_locale_category, const char *));
}

NAN_METHOD(bind_sol_platform_del_locale_monitor) {
	MONITOR_BINDING_DEL(locale, int_string,
		void(*)(void *, enum sol_platform_locale_category, const char *));
}
