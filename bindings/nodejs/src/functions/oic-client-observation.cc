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
#include <sol-oic-client.h>

#include "../async-bridge.h"
#include "../common.h"
#include "../structures/network-link-addr.h"
#include "oic-client.h"
#include "../structures/oic-map.h"
#include "../structures/oic-resource.h"

using namespace v8;

static void resourceObserver(RECEIVER_SIGNATURE, const char *eventPrefix) {
	Nan::HandleScope scope;
	char eventName[256] = "";
	snprintf(eventName, 255, "oic.client.observe.%s.%p", eventPrefix, data);
	Local<Value> arguments[4] = {
		Nan::New(eventName).ToLocalChecked(),
		Nan::New(responseCode),
		address ? Local<Value>::Cast(js_sol_network_link_addr(address)) :
			Local<Value>::Cast(Nan::Null()),
		representation ?
			Local<Value>::Cast(js_sol_oic_map_reader(representation)) :
			Local<Value>::Cast(Nan::Null())
	};

	async_bridge_call(4, arguments);
}

static void defaultNonConfirmingResourceObserver(RECEIVER_SIGNATURE) {
	resourceObserver(responseCode, client, address, representation, data,
		"nonconfirm");
}

static void defaultConfirmingResourceObserver(RECEIVER_SIGNATURE) {
	resourceObserver(responseCode, client, address, representation, data,
		"confirm");
}

static struct {
	bool (*c_api)(struct sol_oic_client *client,
		struct sol_oic_resource *res,
		void(*callback)(RECEIVER_SIGNATURE),
		const void *data, bool observe);
	void (*c_callback)(RECEIVER_SIGNATURE);
} observationMap[2] = {
	{
		sol_oic_client_resource_set_observable,
		defaultConfirmingResourceObserver
	},
	{
		sol_oic_client_resource_set_observable_non_confirmable,
		defaultNonConfirmingResourceObserver
	}
};

static Local<Boolean> observation_implementation(struct sol_oic_resource *resource, Local<Function> jsCallback, bool observe, int index) {
	bool returnValue = true;
	char eventName[256] = "";
	snprintf(eventName, 255, "oic.client.observe.confirm.%p", (void *)resource);
	Local<String> jsEventName = Nan::New(eventName).ToLocalChecked();

	if (observe) {
		if (async_bridge_get_listener_count(jsEventName) == 0) {
			returnValue = observationMap[index].c_api(
				sol_oic_client_get(), resource,
					observationMap[index].c_callback, resource, true);
		}
		if (returnValue) {
			async_bridge_add(jsEventName, jsCallback);
		}
	} else {
		if (async_bridge_get_listener_count(jsEventName) == 1) {
			returnValue = observationMap[index].c_api(
				sol_oic_client_get(), resource,
					observationMap[index].c_callback, resource, false);
		}
		if (returnValue) {
			async_bridge_del(jsEventName, jsCallback);
		}
	}

	return Nan::New(returnValue);
}

#define OBSERVATION_IMPLEMENTATION(index) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 3); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsObject); \
		VALIDATE_ARGUMENT_TYPE(info, 1, IsFunction); \
		VALIDATE_ARGUMENT_TYPE(info, 2, IsBoolean); \
\
		struct sol_oic_resource *resource; \
		if (!c_sol_oic_resource(Local<Object>::Cast(info[0]), &resource)) { \
			return; \
		} \
\
		info.GetReturnValue().Set( \
			observation_implementation(resource, \
				Local<Function>::Cast(info[1]), info[2]->BooleanValue(), \
				(index))); \
	} while(0)

NAN_METHOD(bind_sol_oic_client_resource_set_observable) {
	OBSERVATION_IMPLEMENTATION(0);
}

NAN_METHOD(bind_sol_oic_client_resource_set_observable_non_confirmable) {
	OBSERVATION_IMPLEMENTATION(1);
}
