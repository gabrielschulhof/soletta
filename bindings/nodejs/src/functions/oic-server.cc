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
#include <string.h>
#include <nan.h>
#include <sol-oic-server.h>

#include "../common.h"
#include "../hijack.h"
#include "../sys-constants.h"
#include "../structures/oic-handles.h"
#include "../structures/oic-request-response.h"
#include "../sys-constants.h"

using namespace v8;

class ResourceInfo {
	bool setCallback(Nan::Callback & destination, Local<Object> & source,
		const char *propertyName) {
		Local<Value> property = Nan::Get(source,
			Nan::New(propertyName).ToLocalChecked()).ToLocalChecked();

		if (property->IsFunction()) {
			destination.SetFunction(Local<Function>::Cast(property));
		}
		return true;
	}
public:
	ResourceInfo(Local<Object> jsResourceType): hijackRefHasSucceeded(false),
		resource(0) {
		if (!setCallback(jsGet, jsResourceType, "get")) { return; }
		if (!setCallback(jsPut, jsResourceType, "put")) { return; }
		if (!setCallback(jsDel, jsResourceType, "del")) { return; }
		if (!setCallback(jsPost, jsResourceType, "post")) { return; }
		hijackRefHasSucceeded = hijack_ref();
	}

	virtual ~ResourceInfo() {
		if (!jsResource.IsEmpty()) {
			Nan::SetInternalFieldPointer(Nan::New<Object>(jsResource), 0, 0);
			jsResource.Reset();
		}
		if (hijackRefHasSucceeded) {
			hijack_unref();
		}
	}

	Nan::Callback jsGet;
	Nan::Callback jsPut;
	Nan::Callback jsDel;
	Nan::Callback jsPost;
	Nan::Persistent<Object> jsResource;
	bool hijackRefHasSucceeded;
	struct sol_oic_server_resource *resource;
};

static int callEntityHandler(Nan::Callback & callback,
	struct sol_oic_request *request, enum sol_coap_method method) {
	Nan::HandleScope scope;
	Local<Value> arguments[1] = { SolOicRequest::New(request, method) };
	Local<Value> jsReturnValue = callback.Call(1, arguments);
	VALIDATE_CALLBACK_RETURN_VALUE_TYPE(jsReturnValue, IsInt32,
		"entity handler callback", -1);
	return Nan::To<int>(jsReturnValue).FromJust();
}

static int defaultGet(void *data, struct sol_oic_request *request) {
	return callEntityHandler(((ResourceInfo *)data)->jsGet, request,
		SOL_COAP_METHOD_GET);
}

static int defaultPut(void *data, struct sol_oic_request *request) {
	return callEntityHandler(((ResourceInfo *)data)->jsPut, request,
		SOL_COAP_METHOD_PUT);
}

static int defaultDel(void *data, struct sol_oic_request *request) {
	return callEntityHandler(((ResourceInfo *)data)->jsDel, request,
		SOL_COAP_METHOD_DELETE);
}

static int defaultPost(void *data, struct sol_oic_request *request) {
	return callEntityHandler(((ResourceInfo *)data)->jsPost, request,
		SOL_COAP_METHOD_POST);
}

static bool assign_str_slice_from_property(sol_str_slice *slice,
	Local<Object> source, const char *propertyName) {

	sol_str_slice localSlice = {0, 0};

	Local<Value> jsProperty =
		Nan::Get(source, Nan::New(propertyName).ToLocalChecked())
			.ToLocalChecked();
	VALIDATE_VALUE_TYPE(jsProperty, IsString, "server resource string", false);

	localSlice.data = strdup((const char *)*String::Utf8Value(jsProperty));
	if (!localSlice.data) {
		Nan::ThrowError((
			std::string(
				"Failed to allocate memory for server resource type member '")
					+ propertyName + std::string("'")).c_str());
		return false;
	}
	localSlice.len = strlen(localSlice.data);

	*slice = localSlice;
	return true;
}

NAN_METHOD(bind_sol_oic_server_register_resource) {
	VALIDATE_ARGUMENT_COUNT(info, 2);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsUint32);

	Local<Object> jsResourceType = Nan::To<Object>(info[0]).ToLocalChecked();
	struct sol_oic_resource_type resourceType = {
		SOL_SET_API_VERSION(.api_version = SOL_OIC_RESOURCE_TYPE_API_VERSION,)
		.resource_type = {0, 0},
		.interface = {0, 0},
		.path = {0, 0},
		.get = { .handle = defaultGet },
		.put = { .handle = defaultPut },
		.post = { .handle = defaultPost },
		.del = { .handle = defaultDel }
	};
	ResourceInfo *resourceInfo = 0;
	Local<Object> returnValue;

	if (!assign_str_slice_from_property(&(resourceType.resource_type),
		jsResourceType, "resource_type")) {
		goto bail;
	}
	if (!assign_str_slice_from_property(&(resourceType.interface),
		jsResourceType, "interface")) {
		goto bail;
	}
	if (!assign_str_slice_from_property(&(resourceType.path),
		jsResourceType, "path")) {
		goto bail;
	}

	resourceInfo = new ResourceInfo(jsResourceType);
	if (!(resourceInfo && resourceInfo->hijackRefHasSucceeded)) {
		goto bail;
	}

	// Do not assign our default request handlers if we have no JS equivalents
	if (resourceInfo->jsGet.IsEmpty()) { resourceType.get.handle = 0; }
	if (resourceInfo->jsPut.IsEmpty()) { resourceType.put.handle = 0; }
	if (resourceInfo->jsDel.IsEmpty()) { resourceType.del.handle = 0; }
	if (resourceInfo->jsPost.IsEmpty()) { resourceType.post.handle = 0; }

	resourceInfo->resource = sol_oic_server_register_resource(
		&resourceType, resourceInfo,
		(enum sol_oic_resource_flag)(Nan::To<uint32_t>(info[1]).FromJust()));
	if (!resourceInfo->resource) {
		goto bail;
	}

	returnValue = SolOicServerResource::New(resourceInfo);
	resourceInfo->jsResource.Reset(returnValue);
	info.GetReturnValue().Set(returnValue);
	return;

bail:
	free((void *)(resourceType.resource_type.data));
	free((void *)(resourceType.interface.data));
	free((void *)(resourceType.path.data));
	delete resourceInfo;
	info.GetReturnValue().Set(Nan::Null());
}

NAN_METHOD(bind_sol_oic_server_unregister_resource) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);

	ResourceInfo *resourceInfo = (ResourceInfo *)SolOicServerResource::Resolve(
		Nan::To<Object>(info[0]).ToLocalChecked());
	if (!resourceInfo) {
		return;
	}

	sol_oic_server_unregister_resource(resourceInfo->resource);

	delete resourceInfo;
}

NAN_METHOD(bind_sol_oic_server_response_new) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	info.GetReturnValue().Set(SolOicResponse::New(info[0]));
}

NAN_METHOD(bind_sol_oic_server_response_free) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	Local<Object> jsResponse = Nan::To<Object>(info[0]).ToLocalChecked();
	struct sol_oic_response *response = (struct sol_oic_response *)
		JSHandle<SolOicResponse>::Resolve(jsResponse);
	if (!response) {
		return;
	}
	sol_oic_server_response_free(response);
	Nan::SetInternalFieldPointer(jsResponse, 0, 0);
}

NAN_METHOD(bind_sol_oic_server_send_response) {
	VALIDATE_ARGUMENT_COUNT(info, 3);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsObject);
	VALIDATE_ARGUMENT_TYPE(info, 2, IsUint32);

	Local<Object> jsRequest = Nan::To<Object>(info[0]).ToLocalChecked();
	struct sol_oic_request *request = (struct sol_oic_request *)
		SolOicRequest::Resolve(jsRequest);
	if (!request) {
		return;
	}

	Local<Object> jsResponse = Nan::To<Object>(info[1]).ToLocalChecked();
	struct sol_oic_response *response = (struct sol_oic_response *)
		SolOicResponse::Resolve(jsResponse, jsRequest);
	if (!response) {
		return;
	}

	int result = sol_oic_server_send_response(request, response,
		(enum sol_coap_response_code)Nan::To<int>(info[2]).FromJust());
	if (result) {
		info.GetReturnValue().Set(ReverseLookupConstant("E", result));
	} else {
		info.GetReturnValue().Set(Nan::New(result));
	}

	Nan::SetInternalFieldPointer(jsRequest, 0, 0);
	Nan::SetInternalFieldPointer(jsResponse, 0, 0);
}
