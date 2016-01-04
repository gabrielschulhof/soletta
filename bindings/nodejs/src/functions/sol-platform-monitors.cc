#include <nan.h>
#include "../data.h"
#include "../common.h"
#include "../async-bridge.h"

#include <sol-platform.h>

using namespace v8;

#define MONITOR_BODY(eventNameVariable, wantedCount, opName, callStatement) \
	do { \
		int returnValue = 0; \
		Local<String> eventName = Nan::New((eventNameVariable)) \
			.ToLocalChecked(); \
		if (async_bridge_get_listener_count(eventName) == (wantedCount)) { \
			returnValue = (callStatement); \
		} \
\
		if (returnValue == 0) { \
			async_bridge_ ## opName(eventName, Local<Function>::Cast(info[0])); \
		} \
\
		info.GetReturnValue().Set(Nan::New(returnValue)); \
	} while(0)

#define MONITOR_BODY_FULL(eventNameVariable, wantedCount, opName, callStatement) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 1); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction); \
		MONITOR_BODY(eventNameVariable, wantedCount, opName, callStatement); \
	} while(0)

static const char *hostnameEvent = "platform.hostname";
static const char *timezoneEvent = "platform.timezone";
static const char *stateEvent = "platform.state";
static const char *localeEvent = "platform.locale";
static const char *systemClockEvent = "platform.systemclock";

static void defaultStringMonitor(void *data, const char *theString) {
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[2] = {
		Nan::New((const char *)data).ToLocalChecked(),
		Nan::New(theString).ToLocalChecked()
	};
	async_bridge_call(2, jsCallbackArguments);
}

NAN_METHOD(bind_sol_platform_add_hostname_monitor) {
	MONITOR_BODY_FULL(hostnameEvent, 0, add,
		sol_platform_add_hostname_monitor(defaultStringMonitor,
			hostnameEvent));
}

NAN_METHOD(bind_sol_platform_del_hostname_monitor) {
	MONITOR_BODY_FULL(hostnameEvent, 1, del,
		sol_platform_del_hostname_monitor(defaultStringMonitor,
			hostnameEvent));
}

NAN_METHOD(bind_sol_platform_add_timezone_monitor) {
	MONITOR_BODY_FULL(timezoneEvent, 0, add,
		sol_platform_add_timezone_monitor(defaultStringMonitor,
			timezoneEvent));
}

NAN_METHOD(bind_sol_platform_del_timezone_monitor) {
	MONITOR_BODY_FULL(timezoneEvent, 1, del,
		sol_platform_del_timezone_monitor(defaultStringMonitor,
			timezoneEvent));
}

static void defaultLocaleMonitor(void *data, enum sol_platform_locale_category localeCategory, const char *localeValue) {
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[3] = {
		Nan::New((const char *)data).ToLocalChecked(),
		Nan::New((int)localeCategory),
		Nan::New(localeValue).ToLocalChecked()
	};
	async_bridge_call(3, jsCallbackArguments);
}

NAN_METHOD(bind_sol_platform_add_locale_monitor) {
	MONITOR_BODY_FULL(localeEvent, 0, add,
		sol_platform_add_locale_monitor(defaultLocaleMonitor, localeEvent));
}

NAN_METHOD(bind_sol_platform_del_locale_monitor) {
	MONITOR_BODY_FULL(localeEvent, 1, del,
		sol_platform_del_locale_monitor(defaultLocaleMonitor, localeEvent));
}

void defaultServiceMonitor(void *data, const char *service, enum sol_platform_service_state state) {
	Nan::HandleScope scope;
	char serviceEvent[1024] = "";

	snprintf(serviceEvent, 1023, "platform.service.%s", service);

	Local<Value> jsCallbackArguments[3] = {
		Nan::New(serviceEvent).ToLocalChecked(),
		Nan::New(service).ToLocalChecked(),
		Nan::New((int)state)
	};
	async_bridge_call(3, jsCallbackArguments);
}

#define MONITOR_BODY_SERVICE(wantedCount, opName) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 2); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction); \
		VALIDATE_ARGUMENT_TYPE(info, 1, IsString); \
\
		char serviceEvent[1024] = ""; \
		String::Utf8Value jsService(info[1]); \
		snprintf(serviceEvent, 1023, "platform.service.%s", \
			(const char *)*jsService); \
\
		MONITOR_BODY(serviceEvent, 0, add, \
			sol_platform_ ## opName ## _service_monitor( \
				defaultServiceMonitor, (const char *)*jsService, NULL)); \
	} while(0)

NAN_METHOD(bind_sol_platform_add_service_monitor) {
	MONITOR_BODY_SERVICE(0, add);
}

NAN_METHOD(bind_sol_platform_del_service_monitor) {
	MONITOR_BODY_SERVICE(1, add);
}

static void defaultStateMonitor(void *data, enum sol_platform_state theState) {
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[2] = {
		Nan::New((const char *)data).ToLocalChecked(),
		Nan::New(theState)
	};
	async_bridge_call(2, jsCallbackArguments);
}

NAN_METHOD(bind_sol_platform_add_state_monitor) {
	MONITOR_BODY_FULL(stateEvent, 0, add,
		sol_platform_add_state_monitor(defaultStateMonitor, stateEvent));
}

NAN_METHOD(bind_sol_platform_del_state_monitor) {
	MONITOR_BODY_FULL(stateEvent, 1, del,
		sol_platform_del_state_monitor(defaultStateMonitor, stateEvent));
}

static void defaultSystemClockMonitor(void *data, int64_t theTimestamp) {
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[2] = {
		Nan::New((const char *)data).ToLocalChecked(),
		Nan::New<Date>((double)theTimestamp).ToLocalChecked()
	};
	async_bridge_call(2, jsCallbackArguments);
}

NAN_METHOD(bind_sol_platform_add_system_clock_monitor) {
	MONITOR_BODY_FULL(systemClockEvent, 0, add,
		sol_platform_add_system_clock_monitor(defaultSystemClockMonitor,
			systemClockEvent));
}

NAN_METHOD(bind_sol_platform_del_system_clock_monitor) {
	MONITOR_BODY_FULL(systemClockEvent, 1, del,
		sol_platform_del_system_clock_monitor(defaultSystemClockMonitor,
			systemClockEvent));
}
