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

#include <map>
#include <string.h>
#include <nan.h>
#include <sol-oic-server.h>

#include "../common.h"
#include "../hijack.h"
#include "../structures/js-handle.h"
#include "../structures/network.h"
#include "../structures/oic-map.h"

using namespace v8;

NAN_METHOD(bind_sol_oic_server_init) {
	info.GetReturnValue().Set(Nan::New(sol_oic_server_init()));
}

NAN_METHOD(bind_sol_oic_server_shutdown) {
	sol_oic_server_shutdown();
}

class SolOicServerResource : public JSHandle<SolOicServerResource> {
public:
	static const char *jsClassName() { return "SolOicServerResource"; }
};

static const char *keys[] = { "get", "put", "post", "del", 0 };
struct ResourceInfo {
	ResourceInfo(Local<Object> definition) : resource(0) {
		int index;
		for (index = 0; keys[index]; index++) {
			Local<Value> value =
				Nan::Get(definition, Nan::New(keys[index]).ToLocalChecked())
					.ToLocalChecked();
			handlers[keys[index]] = value->IsNull() ? 0 :
				new Nan::Callback(Local<Function>::Cast(value));
		}
	}
	~ResourceInfo() {
		for (int index = 0; keys[index]; index++) {
			delete handlers[keys[index]];
		}
	}
	std::map<const char *, Nan::Callback *>handlers;
	struct sol_oic_server_resource *resource;
};

#define ENTITY_HANDLER_SIGNATURE \
	const struct sol_network_link_addr *cliaddr, \
	const void *data, \
	const struct sol_oic_map_reader *input, \
	struct sol_oic_map_writer *output

static sol_coap_responsecode_t entityHandler(ENTITY_HANDLER_SIGNATURE,
	const char *method) {
	Nan::HandleScope scope;
	sol_coap_responsecode_t returnValue = SOL_COAP_RSPCODE_NOT_IMPLEMENTED;
	struct ResourceInfo *info = (struct ResourceInfo *)data;
	Local<Object> outputPayload = Nan::New<Object>();
	Local<Value> arguments[3] = {
		js_sol_network_link_addr(cliaddr),
		js_sol_oic_map_reader(input),
		outputPayload
	};
	Nan::Callback *callback = info->handlers[method];
	if (callback) {
		Local<Value> jsReturnValue = callback->Call(3, arguments);
		VALIDATE_CALLBACK_RETURN_VALUE_TYPE(jsReturnValue, IsUint32,
			"entity handler", returnValue);
		returnValue = (sol_coap_responsecode_t)
			Nan::To<uint32_t>(jsReturnValue).FromJust();

		// FIXME: What to do with the return value from c_sol_oic_map_writer()?
		c_sol_oic_map_writer(outputPayload, output);
	}
	return returnValue;
}

static sol_coap_responsecode_t defaultGet(ENTITY_HANDLER_SIGNATURE) {
	return entityHandler(cliaddr, data, input, output, "get");
}

static sol_coap_responsecode_t defaultPut(ENTITY_HANDLER_SIGNATURE) {
	return entityHandler(cliaddr, data, input, output, "put");
}

static sol_coap_responsecode_t defaultPost(ENTITY_HANDLER_SIGNATURE) {
	return entityHandler(cliaddr, data, input, output, "post");
}

static sol_coap_responsecode_t defaultDel(ENTITY_HANDLER_SIGNATURE) {
	return entityHandler(cliaddr, data, input, output, "del");
}

#define ASSIGN_STR_SLICE_MEMBER_FROM_PROPERTY(to, from, message, member) \
	do { \
		to.member.data = strdup((const char *)*String::Utf8Value( \
			Nan::Get(from, Nan::New(#member).ToLocalChecked()) \
				.ToLocalChecked())); \
		if (!to.member.data) { \
			message = "Failed to allocate memory for " #member; \
			goto member##_failed; \
		} \
		to.member.len = strlen(to.member.data); \
	} while(0)

static bool c_sol_oic_resource_type(Local<Object> js,
	struct sol_oic_resource_type *definition) {
	const char *error = 0;
	struct sol_oic_resource_type local = {
#ifdef SOL_OIC_RESOURCE_TYPE_API_VERSION
		.api_version = SOL_OIC_RESOURCE_TYPE_API_VERSION,
#endif /* def SOL_OIC_RESOURCE_TYPE_API_VERSION */
		.resource_type = {0, 0},
		.interface = {0, 0},
		.path = {0, 0},
		.get = { .handle = defaultGet },
		.put = { .handle = defaultPut },
		.post = { .handle = defaultPost },
		.del = { .handle = defaultDel }
	};

	ASSIGN_STR_SLICE_MEMBER_FROM_PROPERTY(local, js, error, resource_type);
	ASSIGN_STR_SLICE_MEMBER_FROM_PROPERTY(local, js, error, interface);
	ASSIGN_STR_SLICE_MEMBER_FROM_PROPERTY(local, js, error, path);

	*definition = local;
	return true;

path_failed:
	free((void *)(local.interface.data));
interface_failed:
	free((void *)(local.resource_type.data));
resource_type_failed:
	Nan::ThrowError(error);
	return false;
}

NAN_METHOD(bind_sol_oic_server_add_resource) {
	VALIDATE_ARGUMENT_COUNT(info, 2);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsUint32);

	struct sol_oic_resource_type resourceType;
	if (!c_sol_oic_resource_type(Nan::To<Object>(info[0]).ToLocalChecked(),
		&resourceType)) {
		return;
	}

	struct ResourceInfo *resourceInfo =
		new ResourceInfo(Nan::To<Object>(info[0]).ToLocalChecked());

	struct sol_oic_server_resource *resource =
		sol_oic_server_add_resource(&resourceType, resourceInfo,
			(enum sol_oic_resource_flag)
				(Nan::To<uint32_t>(info[1]).FromJust()));
	resourceInfo->resource = resource;

	free((void *)(resourceType.resource_type.data));
	free((void *)(resourceType.interface.data));
	free((void *)(resourceType.path.data));

	if (!resource) {
		delete resourceInfo;
		info.GetReturnValue().Set(Nan::Null());
		return;
	}
	info.GetReturnValue().Set(SolOicServerResource::New(resourceInfo));
	hijack_ref();
}

NAN_METHOD(bind_sol_oic_server_del_resource) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
	struct ResourceInfo *resourceInfo = (struct ResourceInfo *)
		SolOicServerResource::Resolve(
			Nan::To<Object>(info[0]).ToLocalChecked());
	if (!resourceInfo) {
		return;
	}
	sol_oic_server_del_resource(resourceInfo->resource);
	delete resourceInfo;
	hijack_unref();
}

NAN_METHOD(bind_sol_oic_notify_observers) {
	VALIDATE_ARGUMENT_COUNT(info, 2);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
	VALIDATE_ARGUMENT_TYPE_OR_NULL(info, 1, IsObject);
	struct ResourceInfo *resourceInfo = (struct ResourceInfo *)
		SolOicServerResource::Resolve(
			Nan::To<Object>(info[0]).ToLocalChecked());
	Nan::Persistent<Object> *jsPayload = 0;
	if (!info[1]->IsNull()) {
		jsPayload = new Nan::Persistent<Object>(
			Nan::To<Object>(info[1]).ToLocalChecked());
	}
	info.GetReturnValue().Set(Nan::New(
		sol_oic_notify_observers(resourceInfo->resource,
			(jsPayload ? oic_map_writer_callback : 0), jsPayload)));
}
