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

#include "../common.h"
#include "../structures/device-id.h"
#include "../structures/js-handle.h"
#include "../structures/network.h"
#include "coap.h"
#include "oic-client.h"

DECLARE_HANDLE_CLASS_IMPLEMENTATION_NEW_REFFABLE(SolOicClientResource,
	struct sol_oic_resource *, sol_oic_resource_ref, sol_oic_resource_unref);
DECLARE_HANDLE_CLASS_IMPLEMENTATION_REST(SolOicClientResource);

using namespace v8;

#define RESOLVE_SERVER(local, name) \
	do { \
		Local<Value> serverValue = \
			Nan::Get(jsClient, Nan::New(#name).ToLocalChecked()) \
				.ToLocalChecked(); \
		if (serverValue->IsObject()) { \
			local.name = (struct sol_coap_server *)SolCoapServer::Resolve( \
				Nan::To<Object>(serverValue).ToLocalChecked()); \
			if (!(local.name)) { \
				return 0; \
			} \
		} \
	} while(0)

static struct sol_oic_client *new_oic_client(Local<Object> jsClient) {
	struct sol_oic_client local = {
#ifdef SOL_OIC_CLIENT_API_VERSION
		.api_version = SOL_OIC_CLIENT_API_VERSION,
#endif /* def SOL_OIC_CLIENT_API_VERSION */
		.server = 0,
		.dtls_server = 0
	};

	RESOLVE_SERVER(local, server);
	RESOLVE_SERVER(local, dtls_server);

	struct sol_oic_client *returnValue = new struct sol_oic_client;
	*returnValue = local;
	return returnValue;
}

struct ResourceFoundInfo {
	ResourceFoundInfo(Local<Function> jsCallback, Local<Object> _jsClient):
		callback(new Nan::Callback(jsCallback)),
		jsClient(new Nan::Persistent<Object>(_jsClient)) {}
	~ResourceFoundInfo() {
		delete callback;
		jsClient->Reset();
		delete jsClient;
	}
	Nan::Callback *callback;
	Nan::Persistent<Object> *jsClient;
};

Local<Array> jsStringArrayFromStrSliceVector(struct sol_vector *vector) {
	Local<Array> jsArray = Nan::New<Array>(vector->len);
	sol_str_slice *slice;
	int index;
	SOL_VECTOR_FOREACH_IDX(vector, slice, index) {
		jsArray->Set(index,
			Nan::New<String>(slice->data, slice->len).ToLocalChecked());
	}
	return jsArray;
}

Local<Object> fillJSClientResource(Local<Object> jsResource,
	struct sol_oic_resource *resource) {
	Nan::Set(jsResource, Nan::New("addr").ToLocalChecked(),
		js_sol_network_link_addr(&(resource->addr)));
	Nan::Set(jsResource, Nan::New("device_id").ToLocalChecked(),
		js_DeviceIdFromSlice(&(resource->device_id)));
	Nan::Set(jsResource, Nan::New("href").ToLocalChecked(),
		Nan::New<String>(resource->href.data,
			resource->href.len).ToLocalChecked());
	Nan::Set(jsResource, Nan::New("interfaces").ToLocalChecked(),
		jsStringArrayFromStrSliceVector(&(resource->interfaces)));
	Nan::Set(jsResource, Nan::New("is_observing").ToLocalChecked(),
		Nan::New(resource->is_observing));
	Nan::Set(jsResource, Nan::New("observable").ToLocalChecked(),
		Nan::New(resource->observable));
	Nan::Set(jsResource, Nan::New("secure").ToLocalChecked(),
		Nan::New(resource->secure));
	Nan::Set(jsResource, Nan::New("types").ToLocalChecked(),
		jsStringArrayFromStrSliceVector(&(resource->types)));
	return jsResource;
}

static bool resourceFound(struct sol_oic_client *client,
	struct sol_oic_resource *resource, void *data) {
	Nan::HandleScope scope;
	ResourceFoundInfo *info = (ResourceFoundInfo *)data;

	Local<Value> arguments[2] = {
		Nan::New(*(info->jsClient)),
		fillJSClientResource(SolOicClientResource::New(resource), resource)
	};
	Local<Value> jsResult = info->callback->Call(2, arguments);
	VALIDATE_CALLBACK_RETURN_VALUE_TYPE(jsResult, IsBoolean, "discovery",
		true);
	bool result = Nan::To<bool>(jsResult).FromJust();
	if (!result) {
		delete client;
		delete info;
	}
	return result;
}

NAN_METHOD(bind_sol_oic_client_find_resource) {
	VALIDATE_ARGUMENT_COUNT(info, 4);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsObject);
	VALIDATE_ARGUMENT_TYPE(info, 2, IsString);
	VALIDATE_ARGUMENT_TYPE(info, 3, IsFunction);

	Local<Object> jsClient = Nan::To<Object>(info[0]).ToLocalChecked();
	struct sol_oic_client *client = new_oic_client(jsClient);
	if (!client) {
		return;
	}

	struct sol_network_link_addr theAddress;
	if (!c_sol_network_link_addr(Nan::To<Object>(info[1]).ToLocalChecked(),
		&theAddress)) {
		return;
	}

	struct ResourceFoundInfo *callbackInfo = new struct ResourceFoundInfo(
		Local<Function>::Cast(info[3]), jsClient);
		
	bool result = sol_oic_client_find_resource(client, &theAddress,
		(const char *)*String::Utf8Value(info[2]), resourceFound,
		callbackInfo);
	if (!result) {
		delete callbackInfo;
	}
	info.GetReturnValue().Set(Nan::New(result));
}
