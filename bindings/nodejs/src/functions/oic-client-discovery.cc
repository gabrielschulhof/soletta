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

using namespace v8;

static bool sol_oic_client_find_resource_callback(struct sol_oic_client *cli, struct sol_oic_resource *resource, void *data) {
	Nan::HandleScope scope;
	Nan::Callback *callback = (Nan::Callback *)data;

	Local<Value> arguments[1];
	if (resource) {
		arguments[0] = SolOicResource::New(resource);
	} else {
		arguments[0] = Nan::Null();
	}
	Local<Value> jsReturnValue = callback->Call(1, arguments);

	VALIDATE_CALLBACK_RETURN_VALUE_TYPE(jsReturnValue, IsBoolean,
		"resource discovery callback");
	bool returnValue = jsReturnValue->BooleanValue();

	if (!returnValue || !resource) {
		delete callback;
		hijack_unref();
	}

	return returnValue;
}

NAN_METHOD(bind_sol_oic_client_find_resource) {
	VALIDATE_ARGUMENT_COUNT(info, 3);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsString);
	VALIDATE_ARGUMENT_TYPE(info, 2, IsFunction);

	Local<Object> jsDestination = Local<Object>::Cast(info[0]);

	struct sol_network_link_addr destination;

	if (!c_sol_network_link_addr(jsDestination, &destination)) {
		return;
	}

	Nan::Callback *callback =
		new Nan::Callback(Local<Function>::Cast(info[2]));

	bool returnValue = sol_oic_client_find_resource(
		sol_oic_client_get(),
		&destination,
		(const char *)*String::Utf8Value(info[1]),
		sol_oic_client_find_resource_callback,
		callback);

	if (!returnValue) {
		delete callback;
	} else {
		hijack_ref();
	}

	info.GetReturnValue().Set(Nan::New(returnValue));
}
