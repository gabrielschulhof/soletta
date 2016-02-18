/*
 * This file is part of the Soletta Project
 *
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <nan.h>
#include "../data.h"
#include "../common.h"
#include "../async-bridge.h"
#include "../bridge.h"

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

static void defaultHostnameMonitor(void *data, const char *newHostName) {
	Local<Value> arguments[1] = {
		Nan::New(newHostName).ToLocalChecked()
	};
	((Nan::Callback *)data)->Call(1, arguments);
}

NAN_METHOD(bind_sol_platform_add_hostname_monitor) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction);
	Nan::Callback *callback =
		new Nan::Callback(Local<Function>::Cast(info[0]));
	int result = sol_platform_add_hostname_monitor(defaultHostnameMonitor,
		callback);
	if (!result) {
		Local<Value> key[1] = {Nan::New("platform.hostname").ToLocalChecked()};
		async_bridge_add(1, key, callback);
	} else {
		delete callback;
	}
	info.GetReturnValue().Set(Nan::New(result));
}

NAN_METHOD(bind_sol_platform_del_hostname_monitor) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction);
	Local<Value> key[1] = {Nan::New("platform.hostname").ToLocalChecked()};
	BridgeCallback *bridgeCallback = async_bridge_get(1, key,
		Local<Function>::Cast(info[0]));
	if (!bridgeCallback) {
		return;
	}
	int result = sol_platform_del_hostname_monitor(defaultHostnameMonitor,
		bridgeCallback->callback);
	if (!result) {
		async_bridge_remove(bridgeCallback);
	}
	delete bridgeCallback;
	info.GetReturnValue().Set(Nan::New(result));
}

static void defaultStringMonitor(void *data, const char *theString) {
	Nan::HandleScope scope;
	Local<Value> jsCallbackArguments[2] = {
		Nan::New((const char *)data).ToLocalChecked(),
		Nan::New(theString).ToLocalChecked()
	};
	async_bridge_call(2, jsCallbackArguments);
}

/*
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
*/
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
