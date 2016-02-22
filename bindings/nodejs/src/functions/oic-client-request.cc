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

#include <sol-oic-client.h>

#include "../common.h"
#include "../hijack.h"
#include "../structures/network.h"
#include "../structures/oic-client.h"
#include "../structures/oic-map.h"

using namespace v8;

static void requestAnswered(sol_coap_responsecode_t code,
	struct sol_oic_client *c_client,
	const struct sol_network_link_addr *address,
	const struct sol_oic_map_reader *response, void *data) {
	struct SolOicClient *client = (struct SolOicClient *)c_client;

	Nan::HandleScope scope;
	Local<Value> arguments[4] = {
		Nan::New(code),
		Nan::New<Object>(*(client->jsClient)),
		Nan::Null(),
		Nan::Null()
	};
	if (address) {
		arguments[2] = js_sol_network_link_addr(address);
	}
	if (response) {
		arguments[3] = js_sol_oic_map_reader(response);
	}
	client->callback->Call(4, arguments);

	delete_SolOicClient(client);
	hijack_unref();
}

static bool do_request(Local<Object> jsClient, Local<Object> jsResource,
	sol_coap_method_t method, Local<Object> jsPayload,
	Local<Function> jsCallback, bool *result,
	bool (*cAPI)(
		struct sol_oic_client *,
		struct sol_oic_resource *,
		sol_coap_method_t,
		bool (*)(void *data, struct sol_oic_map_writer *repr_map),
		void *,
		void(*)(
			sol_coap_responsecode_t,
			struct sol_oic_client *,
			const struct sol_network_link_addr *,
			const struct sol_oic_map_reader *,
			void *data),
		const void *)) {

	struct sol_oic_resource *resource = (sol_oic_resource *)
		SolOicClientResource::Resolve(jsResource);
	if (!resource) {
		return false;
	}

	struct SolOicClient *client = new_SolOicClient(jsClient, jsCallback);
	if (!client) {
		return false;
	}

	Nan::Persistent<Object> *persistentPayload =
		new Nan::Persistent<Object>(jsPayload);

	*result = cAPI((struct sol_oic_client *)client, resource, method,
		oic_map_writer_callback, persistentPayload, requestAnswered, 0);

	persistentPayload->Reset();
	delete persistentPayload;

	if (!(*result)) {
		delete_SolOicClient(client);
	}

	hijack_ref();
	return true;
}

#define DO_REQUEST(info, cAPI) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 5); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsObject); \
		VALIDATE_ARGUMENT_TYPE(info, 1, IsObject); \
		VALIDATE_ARGUMENT_TYPE(info, 2, IsUint32); \
		VALIDATE_ARGUMENT_TYPE_OR_NULL(info, 3, IsObject); \
		VALIDATE_ARGUMENT_TYPE(info, 4, IsFunction); \
		bool result; \
		if (do_request(Nan::To<Object>(info[0]).ToLocalChecked(), \
			Nan::To<Object>(info[1]).ToLocalChecked(), \
			(sol_coap_method_t)Nan::To<uint32_t>(info[2]).FromJust(), \
			Nan::To<Object>(info[3]).ToLocalChecked(), \
			Local<Function>::Cast(info[4]), &result, \
			(cAPI))) { \
			info.GetReturnValue().Set(Nan::New(result)); \
		} \
	} while(0)

NAN_METHOD(bind_sol_oic_client_resource_request) {
	DO_REQUEST(info, sol_oic_client_resource_request);
}

NAN_METHOD(bind_sol_oic_client_resource_non_confirmable_request) {
	DO_REQUEST(info, sol_oic_client_resource_non_confirmable_request);
}
