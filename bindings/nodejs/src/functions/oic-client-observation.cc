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

#include "../bridge.h"
#include "../common.h"
#include "../structures/network-link-addr.h"
#include "oic-client.h"
#include "../structures/oic-map.h"
#include "../structures/oic-resource.h"

using namespace v8;

typedef bool (*observeAPI)(struct sol_oic_client *, struct sol_oic_resource *,
	void(*)(RECEIVER_SIGNATURE), const void *data, bool);

static void resourceObserver(RECEIVER_SIGNATURE) {
	Nan::HandleScope scope;
	Local<Value> arguments[3] = {
		Nan::New(responseCode)
	};
	if (address) {
		arguments[1] = js_sol_network_link_addr(address);
	} else {
		arguments[1] = Nan::Null();
	}
	if (representation) {
		arguments[2] = js_sol_oic_map_reader(representation);
	} else {
		arguments[2] = Nan::Null();
	}
	((Nan::Callback *)data)->Call(3, arguments);
}

static bool do_observe(Local<Object> jsResource, Local<Function> jsCallback,
	bool startObserving, bool isConfirmable, bool *result) {

	struct sol_oic_resource *resource =
		(struct sol_oic_resource *)Nan::GetInternalFieldPointer(jsResource, 0);
	if (!resource) {
		Nan::ThrowError("Failed to retrieve native resource from JS object");
		return false;
	}

	observeAPI solAPI = isConfirmable ? sol_oic_client_resource_set_observable:
		sol_oic_client_resource_set_observable_non_confirmable;
	Nan::Callback *callback = 0;
	Local<Value> keys[3] = {
		jsResource,
		Nan::New("observe").ToLocalChecked(),
		Nan::New(isConfirmable)
	};
	if (startObserving) {
		callback = new Nan::Callback(jsCallback);
		*result = solAPI(sol_oic_client_get(), resource, resourceObserver,
			callback, startObserving);
		if (*result) {
			async_bridge_add(3, keys, callback);
		}
	} else {
		BridgeNode *theBridge = 0;
		callback = async_bridge_get(3, keys, jsCallback, &theBridge);
		if (!callback) {
			return false;
		}
		*result = solAPI(sol_oic_client_get(), resource, resourceObserver,
			callback, startObserving);
		if (*result) {
			async_bridge_remove(theBridge, callback);
			delete callback;
		}
	}
	return true;
}

#define DO_OBSERVE(isConfirmable) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 3); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsObject); \
		VALIDATE_ARGUMENT_TYPE(info, 1, IsFunction); \
		VALIDATE_ARGUMENT_TYPE(info, 2, IsBoolean); \
		bool result = false; \
		if (do_observe(Nan::To<Object>(info[0]).ToLocalChecked(), \
			Local<Function>::Cast(info[1]), \
			Nan::To<bool>(info[2]).FromJust(), \
			(isConfirmable), &result)) { \
			info.GetReturnValue().Set(Nan::New(result)); \
		} \
	} while(0)

NAN_METHOD(bind_sol_oic_client_resource_set_observable) {
	DO_OBSERVE(true);
}

NAN_METHOD(bind_sol_oic_client_resource_set_observable_non_confirmable) {
	DO_OBSERVE(false);
}
