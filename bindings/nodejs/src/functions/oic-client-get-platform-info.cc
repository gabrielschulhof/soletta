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

#include <stdlib.h>
#include <sol-oic-client.h>
#include <nan.h>

#include "../common.h"
#include "../hijack.h"
#include "oic-client.h"
#include "../structures/oic-resource.h"
#include "../structures/network-link-addr.h"
#include "../structures/oic-platform-and-server-info.h"

using namespace v8;

void sol_oic_client_get_platform_info_callback(struct sol_oic_client *client, const struct sol_oic_platform_information *info, void *data) {
	Nan::HandleScope scope;
	Nan::Callback *callback = (Nan::Callback *)data;
	Local<Value> arguments[1];
	if (info) {
		arguments[0] = js_sol_oic_platform_information(info);
	} else {
		arguments[0] = Nan::Null();
	}
	callback->Call(1, arguments);
	delete callback;
}

void sol_oic_client_get_server_info_callback(struct sol_oic_client *client, const struct sol_oic_server_information *info, void *data) {
	Nan::HandleScope scope;
	Nan::Callback *callback = (Nan::Callback *)data;
	Local<Value> arguments[1];
	if (info) {
		arguments[0] = js_sol_oic_server_information(info);
	} else {
		arguments[0] = Nan::Null();
	};
	callback->Call(1, arguments);
	delete callback;
}

static bool c_sol_oic_resource(Local<Object> jsResource, struct sol_oic_resource **resource) {
	struct sol_oic_resource *returnValue = (struct sol_oic_resource *)
		SolOicResource::CResource(jsResource, false);
	if (returnValue) {
		*resource = returnValue;
		return true;
	}
	return false;
}

#define PLATFORM_AND_SERVER_INFO_BINDING(cFunction, theCallback, byParam, byParamPointer, toCConverter) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 2); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsObject); \
		VALIDATE_ARGUMENT_TYPE(info, 1, IsFunction); \
\
		if (!toCConverter(Local<Object>::Cast(info[0]), &(byParam))) { \
			return; \
		} \
\
		Nan::Callback *callback = \
			new Nan::Callback(Local<Function>::Cast(info[1])); \
\
		bool returnValue = cFunction(sol_oic_client_get(), byParamPointer, \
			theCallback, callback); \
\
		if (!returnValue) { \
			delete callback; \
		} \
\
		info.GetReturnValue().Set(Nan::New(returnValue)); \
	} while(0)


NAN_METHOD(bind_sol_oic_client_get_platform_info) {
	struct sol_oic_resource *resource;
	PLATFORM_AND_SERVER_INFO_BINDING(sol_oic_client_get_platform_info,
		sol_oic_client_get_platform_info_callback,
		resource, resource, c_sol_oic_resource);
}

NAN_METHOD(bind_sol_oic_client_get_platform_info_by_addr) {
	struct sol_network_link_addr theAddress;
	PLATFORM_AND_SERVER_INFO_BINDING(sol_oic_client_get_platform_info_by_addr,
		sol_oic_client_get_platform_info_callback,
		theAddress, &theAddress, c_sol_network_link_addr);
}

NAN_METHOD(bind_sol_oic_client_get_server_info) {
	struct sol_oic_resource *resource;
	PLATFORM_AND_SERVER_INFO_BINDING(sol_oic_client_get_server_info,
		sol_oic_client_get_server_info_callback,
		resource, resource, c_sol_oic_resource);
}

NAN_METHOD(bind_sol_oic_client_get_server_info_by_addr) {
	struct sol_network_link_addr theAddress;
	PLATFORM_AND_SERVER_INFO_BINDING(sol_oic_client_get_server_info_by_addr,
		sol_oic_client_get_server_info_callback,
		theAddress, &theAddress, c_sol_network_link_addr);
}
