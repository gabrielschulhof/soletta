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

/*
 request | client::new | client::resolve | server::new | server::resolve
 --------+-------------+-----------------+-------------+----------------
 get     | noop        | noop            | noop        | noop
 put     | noop        | writer          | reader      | noop
 post    | noop        | writer          | reader      | noop
 del     | noop        | noop            | noop        | noop
 --------+-------------+-----------------+-------------+----------------

 response | server::new | server::resolve
 ---------+-------------+----------------
 get      | noop        | writer
 put      | noop        | noop
 post     | noop        | noop
 del      | noop        | noop
 ---------+-------------+----------------
*/

class RequestInfo {
public:
	RequestInfo(void *_request, enum sol_coap_method _method, bool _isClient):
		request(_request), method(_method), isClient(_isClient) {}
	void *request;
	enum sol_coap_method method;
	bool isClient;
};

const char *SolOicRequest::jsClassName() { return "SolOicRequest"; }

Local<Value> SolOicRequest::New(void *data, enum sol_coap_method method) {
	struct RequestInfo *info = new RequestInfo(data, method, false);

	Local<Object> returnValue = JSHandle<SolOicRequest>::New(info);

	if (method == SOL_COAP_METHOD_PUT || method == SOL_COAP_METHOD_POST) {
		struct sol_oic_map_reader *reader =
			sol_oic_server_request_get_reader((struct sol_oic_request *)data);
		if (!js_sol_oic_request(returnValue, reader)) {
			delete info;
			return Nan::Null();
		}
	}

	return returnValue;
}

Local<Value> SolOicRequest::New(Local<Value> jsMethod,
	Local<Value> jsResource, bool confirm) {
	VALIDATE_VALUE_TYPE(jsMethod, IsUint32, "request method", Nan::Null());
	VALIDATE_VALUE_TYPE(jsResource, IsObject, "request resource", Nan::Null());

	struct sol_oic_resource *resource = (struct sol_oic_resource *)
		SolOicClientResource::Resolve(Nan::To<Object>(jsResource)
			.ToLocalChecked());
	if (!resource) {
		return Nan::Null();
	}

	enum sol_coap_method method =
		(enum sol_coap_method)(Nan::To<uint32_t>(jsMethod).FromJust());

	struct sol_oic_request *request = (confirm ?
		sol_oic_client_request_new :
		sol_oic_client_non_confirmable_request_new)(method, resource);
	if (!request) {
		return Nan::Null();
	}

	RequestInfo *info = new RequestInfo((void *)request, method, true);

	return JSHandle<SolOicRequest>::New(info);
}

void *SolOicRequest::Resolve(Local<Value> jsRequest) {
	RequestInfo *info =
		(RequestInfo *)JSHandle<SolOicRequest>::Resolve(
			Nan::To<Object>(jsRequest).ToLocalChecked());
	if (!info) {
		return 0;
	}

	if (info->isClient && (info->method == SOL_COAP_METHOD_PUT ||
		info->method == SOL_COAP_METHOD_POST)) {
		struct sol_oic_map_writer *writer =
			sol_oic_client_request_get_writer(
				(struct sol_oic_request *)(info->request));
		if (!writer) {
			Nan::ThrowError("Failed to acquire request writer");
			return 0;
		}
		if (!c_sol_oic_request(Nan::To<Object>(jsRequest).ToLocalChecked(),
			writer)) {
			return 0;
		}
	}

	return info->request;
}

void SolOicRequest::Invalidate(Local<Value> jsRequestValue) {
	Local<Object> jsRequest = Nan::To<Object>(jsRequestValue).ToLocalChecked();
	RequestInfo *info =
		(RequestInfo *)JSHandle<SolOicRequest>::Resolve(jsRequest);
	delete info;
	Nan::SetInternalFieldPointer(jsRequest, 0, 0);
}

const char *SolOicResponse::jsClassName() { return "SolOicResponse"; }

Local<Value> SolOicResponse::New(Local<Value> jsRequest) {
	VALIDATE_VALUE_TYPE(jsRequest, IsObject,
		"request used to construct response", Nan::Null());

	struct sol_oic_request *request = (struct sol_oic_request *)
		SolOicRequest::Resolve(Nan::To<Object>(jsRequest).ToLocalChecked());
	if (!request) {
		return Nan::Null();
	}

	struct sol_oic_response *response = sol_oic_server_response_new(request);
	if (!response) {
		return Nan::Null();
	}

	return JSHandle<SolOicResponse>::New(response);
}

void *SolOicResponse::Resolve(Local<Object> jsResponse,
	Local<Object> jsRequest) {
	RequestInfo *info =
		(RequestInfo *)JSHandle<SolOicRequest>::Resolve(jsRequest);
	if (!info) {
		return 0;
	}

	struct sol_oic_response *response = (struct sol_oic_response *)
		JSHandle<SolOicResponse>::Resolve(jsResponse);
	if (!response) {
		return 0;
	}

	if (info->method == SOL_COAP_METHOD_GET) {
		struct sol_oic_map_writer *writer =
			sol_oic_server_response_get_writer(response);
		if (!writer) {
			Nan::ThrowError("Failed to acquire response writer");
			return 0;
		}
		if (!c_sol_oic_request(jsResponse, writer)) {
			return 0;
		}
	}

	return response;
}
