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

#include <sol-oic-common.h>
#include <sol-oic-client.h>
#include <sol-oic-server.h>

#include "device-id.h"
#include "oic-handles.h"
#include "oic-map.h"
#include "oic-request-response.h"
#include "network.h"
#include "../common.h"
#include "../data.h"

using namespace v8;

struct request_info {
	struct sol_oic_request *request;
	bool is_client_request;
};

const char *SolOicRequest::jsClassName() { return "SolOicRequest"; }

Local<Object> SolOicRequest::New(void *data, enum sol_coap_) {
	Local<Object> returnValue = JSHandle<SolOicRequest>::New(data);

	struct sol_oic_map_reader *reader =
		sol_oic_server_request_get_reader((struct sol_oic_request *)data);
	if (!js_sol_oic_request(returnValue, reader)) {
		return Nan::To<Object>(Nan::Null()).ToLocalChecked();
	}

	return returnValue;
}

Local<Object> SolOicRequest::New(Local<Value> jsMethod,
	Local<Value> jsResource, bool confirm) {
	VALIDATE_VALUE_TYPE(jsMethod, IsUint32, "request method",
		Nan::To<Object>(Nan::Null()).ToLocalChecked());
	VALIDATE_VALUE_TYPE(jsResource, IsObject, "request resource",
		Nan::To<Object>(Nan::Null()).ToLocalChecked());

	struct sol_oic_resource *resource = (struct sol_oic_resource *)
		SolOicClientResource::Resolve(Nan::To<Object>(jsResource)
			.ToLocalChecked());
	if (!resource) {
		return Nan::To<Object>(Nan::Null()).ToLocalChecked();
	}

	struct sol_oic_request *request = (confirm ?
		sol_oic_client_request_new :
		sol_oic_client_non_confirmable_request_new)(
			(enum sol_coap_method)(Nan::To<uint32_t>(jsMethod).FromJust()),
			resource);
	if (!request) {
		return Nan::To<Object>(Nan::Null()).ToLocalChecked();
	}

	return JSHandle<SolOicRequest>::New(request);
}

const char *SolOicResponse::jsClassName() { return "SolOicResponse"; }

Local<Object> SolOicResponse::New(Local<Value> jsRequest) {
	Local<Object> jsNull = Nan::To<Object>(Nan::Null()).ToLocalChecked();

	VALIDATE_VALUE_TYPE(jsRequest, IsObject,
		"request used to construct response", jsNull);

	struct sol_oic_request *request = (struct sol_oic_request *)
		SolOicRequest::Resolve(Nan::To<Object>(jsRequest).ToLocalChecked());
	if (!request) {
		return jsNull;
	}

	struct sol_oic_response *response = sol_oic_server_response_new(request);
	if (!response) {
		return jsNull;
	}

	return JSHandle<SolOicResponse>::New(response);
}
