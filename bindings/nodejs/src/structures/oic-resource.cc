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

#include <stdio.h>
#include <nan.h>
#include "oic-resource.h"
#include "../common.h"
#include "../data.h"
#include "network-link-addr.h"

using namespace v8;

static Local<Value> deviceIdFromSlice(struct sol_str_slice *slice) {
	char returnValue[37] = "";
	if (slice->len != 16) {
		Nan::ThrowRangeError("Data for deviceId is not 16 bytes long");
		return Local<Value>::Cast(Nan::Null());
	}

	// Canonical uuid format
	int result = snprintf(returnValue, 37,
		"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		(unsigned char)(slice->data[0]), (unsigned char)(slice->data[1]),
		(unsigned char)(slice->data[2]), (unsigned char)(slice->data[3]),
		(unsigned char)(slice->data[4]), (unsigned char)(slice->data[5]),
		(unsigned char)(slice->data[6]), (unsigned char)(slice->data[7]),
		(unsigned char)(slice->data[8]), (unsigned char)(slice->data[9]),
		(unsigned char)(slice->data[10]), (unsigned char)(slice->data[11]),
		(unsigned char)(slice->data[12]), (unsigned char)(slice->data[13]),
		(unsigned char)(slice->data[14]), (unsigned char)(slice->data[15]));

	if (result != 36) {
		Nan::ThrowError("Failed to convert deviceId to string");
		return Local<Value>::Cast(Nan::Null());
	}

	return Local<Value>::Cast(Nan::New(returnValue).ToLocalChecked());
}

class SolOicResource : public Nan::ObjectWrap {
	public:
		static inline Local<Object> New(struct sol_oic_resource *resource) {
			Local<Value> arguments[1] = {
				Local<Value>::Cast(Nan::New<External>((void *)resource))
			};
			return Nan::GetFunction(Nan::New(theTemplate())).ToLocalChecked()->NewInstance(1, arguments);
		}
		static inline bool Is(Local<Object> theObject) {
			return Nan::New(theTemplate())->HasInstance(theObject);
		}
		struct sol_oic_resource *_c_resource;
	private:
		static inline Nan::Persistent<FunctionTemplate> & theTemplate() {
			static Nan::Persistent<FunctionTemplate> my_template;

			if (my_template.IsEmpty()) {
				Local<FunctionTemplate> theTemplate = Nan::New<FunctionTemplate>(makeNewInstance);
				theTemplate->SetClassName(Nan::New("SolOicResource").ToLocalChecked());
				theTemplate->InstanceTemplate()->SetInternalFieldCount(1);
				Nan::SetNamedPropertyHandler(theTemplate->InstanceTemplate(),
					namedPropertyGetter, 0, 0, 0, namedPropertyEnumerator);
				my_template.Reset(theTemplate);
			}

			return my_template;
		}
		explicit SolOicResource(struct sol_oic_resource * c_resource = 0) : _c_resource(c_resource) {
			printf("Reffing resource\n");
			sol_oic_resource_ref(_c_resource);
		}

		~SolOicResource() {
			printf("Unreffing resource\n");
			sol_oic_resource_unref(_c_resource);
		}

		static NAN_METHOD(makeNewInstance) {
			VALIDATE_ARGUMENT_COUNT(info, 1);
			VALIDATE_ARGUMENT_TYPE(info, 0, IsExternal);
			if (info.IsConstructCall()) {
				sol_oic_resource *c_resource = (sol_oic_resource *)External::Cast(*info[0])->Value();
				SolOicResource *js_resource = new SolOicResource(c_resource);
				js_resource->Wrap(info.This());
				info.GetReturnValue().Set(info.This());
			} else {
				Nan::ThrowError("OicSolResource: call-to-construct unavailable");
			}
		}

#define CONDITIONALLY_SET_RETURN_VALUE_FROM_BOOLEAN_MEMBER(property, source, name) \
	do { \
		if (!strcmp((const char *)*String::Utf8Value(property), #name)) { \
			info.GetReturnValue().Set(Nan::New(source->name)); \
		} \
	} while(0)
#define RETURN_SOL_STRING_SLICE_ARRAY_TO_JS(source) \
	do { \
		Local<Array> jsArray = Nan::New<Array>((source).len); \
		sol_str_slice *slice; \
		int index; \
		SOL_VECTOR_FOREACH_IDX(&(source), slice, index) { \
			jsArray->Set(index, Nan::New<String>(slice->data, slice->len).ToLocalChecked()); \
		} \
		info.GetReturnValue().Set(jsArray); \
	} while(0)

		static NAN_PROPERTY_GETTER(namedPropertyGetter) {
			struct sol_oic_resource *resource = Unwrap<SolOicResource>(info.Holder())->_c_resource;

			if (!strcmp((const char *)*String::Utf8Value(property), "addr")) {
				info.GetReturnValue().Set(js_sol_network_link_addr(&(resource->addr)));
			}
			if (!strcmp((const char *)*String::Utf8Value(property), "device_id")) {
				info.GetReturnValue().Set(deviceIdFromSlice(&(resource->device_id)));
			}
			if (!strcmp((const char *)*String::Utf8Value(property), "href")) {
				info.GetReturnValue().Set(Nan::New<String>(resource->href.data, resource->href.len).ToLocalChecked());
			}
			if (!strcmp((const char *)*String::Utf8Value(property), "interfaces")) {
				RETURN_SOL_STRING_SLICE_ARRAY_TO_JS(resource->interfaces);
			}
			CONDITIONALLY_SET_RETURN_VALUE_FROM_BOOLEAN_MEMBER(property, resource, is_observing);
			CONDITIONALLY_SET_RETURN_VALUE_FROM_BOOLEAN_MEMBER(property, resource, observable);
			CONDITIONALLY_SET_RETURN_VALUE_FROM_BOOLEAN_MEMBER(property, resource, secure);
			if (!strcmp((const char *)*String::Utf8Value(property), "types")) {
				RETURN_SOL_STRING_SLICE_ARRAY_TO_JS(resource->interfaces);
			}
		}
		static NAN_PROPERTY_ENUMERATOR(namedPropertyEnumerator) {
			const char *propertyNames[10] = {
				"active", "addr", "device_id", "href", "interfaces", "is_observing", "observable",
				"secure", "slow", "types"
			};
			int index;
			Local<Array> properties = Nan::New<Array>(10);
			for (index = 0 ; index < 10 ; index++) {
				properties->Set(index, Nan::New(propertyNames[index]).ToLocalChecked());
			}
			info.GetReturnValue().Set(properties);
		}
};

Local<Object> js_sol_oic_resource(struct sol_oic_resource *resource) {
	return SolOicResource::New(resource);
}

bool c_sol_oic_resource(Local<Object> source, sol_oic_resource **resource) {
	SolOicResource *jsResource = 0;

	if (SolOicResource::Is(source)) {
		jsResource = Nan::ObjectWrap::Unwrap<SolOicResource>(Local<Object>::Cast(source));
		if (jsResource) {
			if (jsResource->_c_resource) {
				*resource = jsResource->_c_resource;
				return true;
			}
		}
	}
	Nan::ThrowError("Unable to obtain soletta resource from its Javascript wrapper");
	return false;
}
