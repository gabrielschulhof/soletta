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
#include "../hijack.h"
#include "../structures/network-link-addr.h"
#include "../structures/oic-map.h"
#include "../structures/oic-resource.h"
#include "oic-client.h"

using namespace v8;

static bool resourceSender(void *data, struct sol_oic_map_writer *map) {
	bool returnValue = true;

	if (data) {
		Nan::HandleScope scope;
		Nan::Persistent<Object> *jsPayload = (Nan::Persistent<Object> *)data;
		returnValue = c_sol_oic_map_writer(Nan::New<Object>(*jsPayload), map);
		delete jsPayload;
	}
	return returnValue;
}

static void resourceReceiver(RECEIVER_SIGNATURE) {
	Nan::HandleScope scope;
	Nan::Callback *jsCallback = (Nan::Callback *)data;
	Local<Value> arguments[3] = {
		Nan::New(responseCode),
		address ? Local<Value>::Cast(js_sol_network_link_addr(address)) :
			Local<Value>::Cast(Nan::Null()),
		representation ?
			Local<Value>::Cast(js_sol_oic_map_reader(representation)) :
			Local<Value>::Cast(Nan::Null())
	};

	jsCallback->Call(3, arguments);
	hijack_unref();
	delete jsCallback; 
}


NAN_METHOD(bind_sol_oic_client_resource_request) {
	// resource, method, payload, callback
	VALIDATE_ARGUMENT_COUNT(info, 4);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsUint32);
	VALIDATE_ARGUMENT_TYPE_OR_NULL(info, 2, IsObject);
	VALIDATE_ARGUMENT_TYPE(info, 3, IsFunction);

	struct sol_oic_resource *resource;
	if (!c_sol_oic_resource(Local<Object>::Cast(info[0]), &resource)) {
		return;
	}

	Nan::Callback *jsCallback =
		new Nan::Callback(Local<Function>::Cast(info[3]));
	Nan::Persistent<Object> *jsPayload = (info[2]->IsNull() ? 0 :
		new Nan::Persistent<Object>(Local<Object>::Cast(info[2])));
	bool returnValue = sol_oic_client_resource_request(sol_oic_client_get(),
		resource, (sol_coap_method_t)info[1]->Uint32Value(),
		jsPayload ? resourceSender : 0, jsPayload,
		resourceReceiver, jsCallback);
	if (!returnValue) {
		delete jsCallback;
		if (jsPayload) {
			delete jsPayload;
		}
	} else {
		hijack_ref();
	}
	info.GetReturnValue().Set(Nan::New(returnValue));
}
