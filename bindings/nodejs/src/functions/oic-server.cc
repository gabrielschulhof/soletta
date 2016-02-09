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
#include <sol-oic-server.h>

#include "../common.h"
#include "../structures/oic-resource.h"
#include "../structures/network-link-addr.h"
#include "../structures/oic-map.h"

#define HANDLER_SIGNATURE \
	const struct sol_network_link_addr *cliaddr, \
	const void *data, \
	const struct sol_oic_map_reader *input, \
	struct sol_oic_map_writer *output

using namespace v8;

static sol_coap_responsecode_t entityHandler(HANDLER_SIGNATURE,
		const char *methodName) {
	Nan::HandleScope scope;
	Local<Object> jsHandlers =
		Nan::New<Object>(*((Nan::Persistent<Object> *)data));

	Local<Value> jsHandlerValue = Nan::Get(jsHandlers,
		Nan::New(methodName).ToLocalChecked()).ToLocalChecked();

	if (!jsHandlerValue->IsFunction()) {
		return SOL_COAP_RSPCODE_NOT_IMPLEMENTED;
	}

	Local<Object> jsOutput = Nan::New<Object>();
	Local<Value> arguments[3] = {
		js_sol_network_link_addr(cliaddr),
		js_sol_oic_map_reader(input),
		jsOutput
	};
	Local<Value> jsReturnValue =
		Local<Function>::Cast(jsHandlerValue)->Call(jsHandlers, 3, arguments);
	VALIDATE_CALLBACK_RETURN_VALUE_TYPE(jsReturnValue, IsUint32,
		"entity handler");

	return c_sol_oic_map_writer(jsOutput, output) ?
		(sol_coap_responsecode_t)(jsReturnValue->Uint32Value()) :
		SOL_COAP_RSPCODE_INTERNAL_ERROR;
}

static sol_coap_responsecode_t defaultDel(HANDLER_SIGNATURE) {
	return entityHandler(cliaddr, data, input, output, "del");
}

static sol_coap_responsecode_t defaultGet(HANDLER_SIGNATURE) {
	return entityHandler(cliaddr, data, input, output, "get");
}

static sol_coap_responsecode_t defaultPost(HANDLER_SIGNATURE) {
	return entityHandler(cliaddr, data, input, output, "post");
}

static sol_coap_responsecode_t defaultPut(HANDLER_SIGNATURE) {
	return entityHandler(cliaddr, data, input, output, "put");
}

#define JS_STRING_TO_STR_SLICE_MEMBER(source, destination, member) \
	do { \
		Local<Value>js_##member = Nan::Get((source), \
			Nan::New(#member).ToLocalChecked()).ToLocalChecked(); \
		VALIDATE_VALUE_TYPE(js_##member, \
			IsString, "resource definition " #member, false); \
		(destination).member.data = \
			strdup((const char *)*String::Utf8Value(js_##member)); \
		(destination).member.len = strlen((destination).member.data); \
	} while(0)

static bool c_sol_oic_resource_type(Local<Object> jsDefinition,
		sol_oic_resource_type *definition) {
	struct sol_oic_resource_type local = {
#ifdef SOL_OIC_RESOURCE_TYPE_API_VERSION
		SOL_OIC_RESOURCE_TYPE_API_VERSION,
#endif /* def SOL_OIC_RESOURCE_TYPE_API_VERSION */
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ defaultGet },
		{ defaultPut },
		{ defaultPost },
		{ defaultDel }
	};

	JS_STRING_TO_STR_SLICE_MEMBER(jsDefinition, local, interface);
	JS_STRING_TO_STR_SLICE_MEMBER(jsDefinition, local, resource_type);
	JS_STRING_TO_STR_SLICE_MEMBER(jsDefinition, local, path);

	*definition = local;
	return true;
}

NAN_METHOD(bind_sol_oic_server_add_resource) {
	VALIDATE_ARGUMENT_COUNT(info, 2);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsUint32);

	Local<Object> jsDefinition = Nan::To<Object>(info[0]).ToLocalChecked();
	struct sol_oic_resource_type resourceDefinition;
	if (!c_sol_oic_resource_type(jsDefinition,
			&resourceDefinition)) {
		return;
	}
	enum sol_oic_resource_flag flags =
		(enum sol_oic_resource_flag)(info[1]->Uint32Value());

	Nan::Persistent<Object> *jsHandlers =
		new Nan::Persistent<Object>(jsDefinition);
	struct sol_oic_server_resource *resource = sol_oic_server_add_resource(
		&resourceDefinition, jsHandlers, flags);

	free((void *)resourceDefinition.interface.data);
	free((void *)resourceDefinition.resource_type.data);
	if (resource) {
		Local<Object> jsResource =
			SolOicResource::New(resource, flags, jsHandlers);
		info.GetReturnValue().Set(jsResource);
	} else {
		delete jsHandlers;
	}
}

NAN_METHOD(bind_sol_oic_server_del_resource) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);

	SolOicResource::Empty(Nan::To<Object>(info[0]).ToLocalChecked());
}

NAN_METHOD(bind_sol_oic_notify_observers) {
	VALIDATE_ARGUMENT_COUNT(info, 2);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
	VALIDATE_ARGUMENT_TYPE_OR_NULL(info, 1, IsObject);

	struct sol_oic_server_resource *resource =
		(struct sol_oic_server_resource *)SolOicResource::CResource(
			Nan::To<Object>(info[0]).ToLocalChecked(), true);
	if (!resource) {
		return;
	}

	Nan::Persistent<Object> *jsPayload = 0;
	if (!info[1]->IsNull()) {
		jsPayload = new Nan::Persistent<Object>(
			Nan::To<Object>(info[0]).ToLocalChecked());
	}

	info.GetReturnValue().Set(Nan::New(
		sol_oic_notify_observers(resource,
			jsPayload ? oic_map_writer_callback : 0, jsPayload)));
}

NAN_METHOD(bind_sol_oic_server_init) {
	info.GetReturnValue().Set(Nan::New(sol_oic_server_init()));
}

NAN_METHOD(bind_sol_oic_server_shutdown) {
	sol_oic_server_shutdown();
}
