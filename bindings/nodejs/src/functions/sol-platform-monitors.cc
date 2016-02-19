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
#include "../bridge.h"

#include <sol-platform.h>

using namespace v8;

#define ADD_MONITOR(prefix, marshaller) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 1); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction); \
		Nan::Callback *callback = \
			new Nan::Callback(Local<Function>::Cast(info[0])); \
		int result = sol_platform_add_##prefix##_monitor((marshaller), \
			callback); \
		if (result) { \
			delete callback; \
		} else { \
			Local<Value> key[1] = {Nan::New("platform." #prefix) \
				.ToLocalChecked()}; \
			async_bridge_add(1, key, callback); \
		} \
		info.GetReturnValue().Set(Nan::New(result)); \
	} while(0)

#define DEL_MONITOR(prefix, marshaller) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 1); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction); \
		Local<Value> key[1] = { \
			Nan::New("platform." #prefix) .ToLocalChecked() \
		}; \
		BridgeNode *theBridge = 0; \
		Nan::Callback *callback = async_bridge_get(1, key, \
			Local<Function>::Cast(info[0]), &theBridge); \
		if (!callback) { \
			return; \
		} \
		int result = sol_platform_del_##prefix##_monitor((marshaller), \
			callback); \
		if (!result) { \
			async_bridge_remove(theBridge, callback); \
			delete callback; \
		} \
		info.GetReturnValue().Set(Nan::New(result)); \
	} while(0)

static void stringMonitor(void *data, const char *theString) {
	Nan::HandleScope scope;
	Local<Value> arguments[1] = {Nan::New(theString).ToLocalChecked()};
	((Nan::Callback *)data)->Call(1, arguments);
}

NAN_METHOD(bind_sol_platform_add_hostname_monitor) {
	ADD_MONITOR(hostname, stringMonitor);
}

NAN_METHOD(bind_sol_platform_del_hostname_monitor) {
	DEL_MONITOR(hostname, stringMonitor);
}

NAN_METHOD(bind_sol_platform_add_timezone_monitor) {
	ADD_MONITOR(timezone, stringMonitor);
}

NAN_METHOD(bind_sol_platform_del_timezone_monitor) {
	DEL_MONITOR(timezone, stringMonitor);
}

static void stateMonitor(void *data, enum sol_platform_state theState) {
	Nan::HandleScope scope;
	Local<Value> arguments[1] = {Nan::New(theState)};
	((Nan::Callback *)data)->Call(1, arguments);
}

NAN_METHOD(bind_sol_platform_add_state_monitor) {
	ADD_MONITOR(state, stateMonitor);
}

NAN_METHOD(bind_sol_platform_del_state_monitor) {
	DEL_MONITOR(state, stateMonitor);
}

static void systemClockMonitor(void *data, int64_t theTimestamp) {
	Nan::HandleScope scope;
	Local<Value> arguments[1] = {
		Nan::New<Date>((double)theTimestamp).ToLocalChecked()
	};
	((Nan::Callback *)data)->Call(1, arguments);
}

NAN_METHOD(bind_sol_platform_add_system_clock_monitor) {
	ADD_MONITOR(system_clock, systemClockMonitor);
}

NAN_METHOD(bind_sol_platform_del_system_clock_monitor) {
	DEL_MONITOR(system_clock, systemClockMonitor);
}

static void localeMonitor(void *data,
	enum sol_platform_locale_category category, const char *locale) {
	Nan::HandleScope scope;
	Local<Value> arguments[2] = {
		Nan::New(category),
		Nan::New(locale).ToLocalChecked()
	};
	((Nan::Callback *)data)->Call(2, arguments);
}

NAN_METHOD(bind_sol_platform_add_locale_monitor) {
	ADD_MONITOR(locale, localeMonitor);
}

NAN_METHOD(bind_sol_platform_del_locale_monitor) {
	DEL_MONITOR(locale, localeMonitor);
}

static void serviceMonitor(void *data, const char *service,
	enum sol_platform_service_state state) {
	Nan::HandleScope scope;
	Local<Value> arguments[2] = {
		Nan::New(service).ToLocalChecked(),
		Nan::New(state)
	};
	((Nan::Callback *)data)->Call(2, arguments);
}

NAN_METHOD(bind_sol_platform_add_service_monitor) {
	VALIDATE_ARGUMENT_COUNT(info, 2);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsString);
	Nan::Callback *callback =
		new Nan::Callback(Local<Function>::Cast(info[0]));
	int result = sol_platform_add_service_monitor(serviceMonitor,
		(const char *)*(String::Utf8Value(info[1])), callback);
	if (!result) {
		Local<Value> keys[2] = {
			Nan::New("platform.service").ToLocalChecked(),
			info[1]
		};
		async_bridge_add(2, keys, callback);
	}
	info.GetReturnValue().Set(Nan::New(result));
}

NAN_METHOD(bind_sol_platform_del_service_monitor) {
	VALIDATE_ARGUMENT_COUNT(info, 2);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsFunction);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsString);
	Local<Value> keys[2] = {
		Nan::New("platform.service").ToLocalChecked(),
		info[1]
	};
	BridgeNode *theBridge = 0;
	Nan::Callback *callback = async_bridge_get(2, keys,
		Local<Function>::Cast(info[0]), &theBridge);
	if (!callback) {
		return;
	}
	int result = sol_platform_del_service_monitor(serviceMonitor,
		(const char *)*(String::Utf8Value(info[1])), callback);
	if (!result) {
		async_bridge_remove(theBridge, callback);
		delete callback;
	}
	info.GetReturnValue().Set(Nan::New(result));
}
