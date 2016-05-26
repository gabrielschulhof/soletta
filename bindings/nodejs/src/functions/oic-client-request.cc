/*
 * This file is part of the Soletta Project
 *
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include <errno.h>
#include <string.h>
#include <nan.h>
#include <sol-oic-client.h>

#include "../common.h"
#include "../structures/network.h"
#include "../structures/oic-handles.h"
#include "../structures/oic-map.h"
#include "../structures/oic-request-response.h"
#include "../structures/oic-client-callback-data.h"

using namespace v8;

NAN_METHOD(bind_sol_oic_client_request_new) {
	VALIDATE_ARGUMENT_COUNT(info, 2);
	info.GetReturnValue().Set(SolOicRequest::New(info[0], info[1], true));
}

NAN_METHOD(bind_sol_oic_client_non_confirmable_request_new) {
	VALIDATE_ARGUMENT_COUNT(info, 2);
	info.GetReturnValue().Set(SolOicRequest::New(info[0], info[1], false));
}

NAN_METHOD(bind_sol_oic_client_request_free) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
	Local<Object> jsRequest = Nan::To<Object>(info[0]).ToLocalChecked();
	struct sol_oic_request *request = (struct sol_oic_request *)
		SolOicRequest::Resolve(jsRequest);
	if (!request) {
		return;
	}

	sol_oic_client_request_free(request);
	Nan::SetInternalFieldPointer(jsRequest, 0, 0);
}

static void defaultRequest(void *data, enum sol_coap_response_code code,
	struct sol_oic_client *client, const struct sol_network_link_addr *address,
	const struct sol_oic_map_reader *payload) {
	OIC_CLIENT_ONE_SHOT_CALLBACK(4, 1, (OicCallbackData *)data) {
		arguments[0] = Nan::New(code);
		arguments[2] = js_sol_network_link_addr(address);
		Local<Object> jsPayload = Nan::New<Object>();
		if (js_sol_oic_request(jsPayload, payload)) {
			arguments[3] = jsPayload;
		} else {
			arguments[3] = Nan::Null();
		}
	}
}

NAN_METHOD(bind_sol_oic_client_request) {
	VALIDATE_ARGUMENT_COUNT(info, 3);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsObject);
	VALIDATE_ARGUMENT_TYPE(info, 2, IsFunction);

	Local<Object> jsRequest = Nan::To<Object>(info[1]).ToLocalChecked();

	struct sol_oic_request *request = (struct sol_oic_request *)
		SolOicRequest::Resolve(jsRequest);
	if (!request) {
		return;
	}

	struct sol_oic_map_writer *out =
		sol_oic_client_request_get_writer(request);
	if (!out) {
		Nan::ThrowError("Failed to acquire OIC map writer");
		return;
	}

	if (!c_sol_oic_request(Nan::To<Object>(info[1]).ToLocalChecked(), out)) {
		return;
	}

	OIC_CLIENT_API_CALL(info, 0, 2, sol_oic_client_request, request,
		defaultRequest);

	Nan::SetInternalFieldPointer(jsRequest, 0, 0);
}
