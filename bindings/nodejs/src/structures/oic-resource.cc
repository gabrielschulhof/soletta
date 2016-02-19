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

#include <string>
#include <map>
#include <nan.h>
#include "oic-resource.h"
#include "../common.h"
#include "../data.h"
#include "../hijack.h"
#include "network-link-addr.h"
#include "device-id.h"

using namespace v8;

static const char *clientPropertyNames[10] = {
	"active", "addr", "device_id", "href", "interfaces",
	"is_observing", "observable", "secure", "slow", "types"
};
static const int clientPropertyCount = 10;
static const char *serverPropertyNames[7] = {
	"active", "discoverable", "interfaces",
	"observable", "secure", "slow", "types"
};
static const int serverPropertyCount = 7;

static std::map<std::string, enum sol_oic_resource_flag> & serverFlagLookup() {
	static std::map<std::string, enum sol_oic_resource_flag> theLookup;

	if (SOL_UNLIKELY(theLookup.empty())) {
		theLookup["discoverable"] = SOL_OIC_FLAG_DISCOVERABLE;
		theLookup["observable"] = SOL_OIC_FLAG_OBSERVABLE;
		theLookup["active"] = SOL_OIC_FLAG_ACTIVE;
		theLookup["slow"] = SOL_OIC_FLAG_SLOW;
		theLookup["secure"] = SOL_OIC_FLAG_SECURE;
	}

	return theLookup;
}

Local<Object> SolOicResource::New(struct sol_oic_resource *c_resource) {
	Local<Value> arguments[2] = {
		Nan::False(),
		Nan::New<External>((void *)c_resource)
	};
	return Nan::GetFunction(Nan::New(theTemplate())).ToLocalChecked()
		->NewInstance(2, arguments);
}

Local<Object> SolOicResource::New(struct sol_oic_server_resource *c_resource,
		enum sol_oic_resource_flag flags,
		Nan::Persistent<Object> *jsHandlers) {
	Local<Value> arguments[4] = {
		Nan::True(),
		Nan::New<External>((void *)c_resource),
		Nan::New(flags),
		Nan::New<External>((void *)jsHandlers)
	};
	return Nan::GetFunction(Nan::New(theTemplate())).ToLocalChecked()
		->NewInstance(4, arguments);
}

void *SolOicResource::CResource(Local<Object> source, bool serverResource) {
	SolOicResource *wrapper = SolOicResource::tryUnwrap(source);
	void *returnValue = 0;
	if (wrapper) {
		if (serverResource) {
			returnValue = (void *)(wrapper->_c_server_resource);
		} else {
			returnValue = (void *)(wrapper->_c_resource);
		}
	}
	if (!returnValue) {
		Nan::ThrowTypeError(serverResource ?
			"server resource not found" : "client resource not found");
	}
	return returnValue;
}

void SolOicResource::Empty(Local<Object> source) {
	SolOicResource *wrapper = SolOicResource::tryUnwrap(source);
	if (wrapper) {
		wrapper->Empty();
	}
}

SolOicResource *SolOicResource::tryUnwrap(Local<Object> source) {
	if (!Nan::New(theTemplate())->HasInstance(source)) {
		Nan::ThrowTypeError("object is not of type SolOicResource");
		return 0;
	}
	SolOicResource *returnValue =
		Nan::ObjectWrap::Unwrap<SolOicResource>(
			Nan::To<Object>(source).ToLocalChecked());
	if (!returnValue) {
		Nan::ThrowTypeError("object did not contain a SolOicResource");
	}
	return returnValue;
}

inline Nan::Persistent<FunctionTemplate> & SolOicResource::theTemplate() {
	static Nan::Persistent<FunctionTemplate> my_template;

	if (my_template.IsEmpty()) {
		Local<FunctionTemplate> theTemplate =
			Nan::New<FunctionTemplate>(makeNewInstance);
		theTemplate->SetClassName(Nan::New("SolOicResource").ToLocalChecked());
		theTemplate->InstanceTemplate()->SetInternalFieldCount(1);
		Nan::SetNamedPropertyHandler(theTemplate->InstanceTemplate(),
			namedPropertyGetter, 0, 0, 0, namedPropertyEnumerator);
		my_template.Reset(theTemplate);
	}

	return my_template;
}

NAN_METHOD(SolOicResource::makeNewInstance) {
	if (!info.IsConstructCall()) {
		Nan::ThrowError("OicSolResource: call-to-construct unavailable");
		return;
	}

	VALIDATE_ARGUMENT_COUNT(info, 2);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsBoolean);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsExternal);

	SolOicResource *jsResource = 0;
	if (info[0]->BooleanValue()) {

		//Server-side resource
		VALIDATE_ARGUMENT_COUNT(info, 4);
		VALIDATE_ARGUMENT_TYPE(info, 2, IsUint32);
		VALIDATE_ARGUMENT_TYPE(info, 3, IsExternal);
		jsResource = new SolOicResource(
			(struct sol_oic_server_resource *)
				External::Cast(*info[1])->Value(),
			(enum sol_oic_resource_flag)info[2]->Uint32Value(),
			(Nan::Persistent<Object> *)External::Cast(*info[3])->Value());
	} else {

		// Client-side resource
		jsResource = new SolOicResource(
			(struct sol_oic_resource *)External::Cast(*info[1])->Value());
	}

	jsResource->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}

#define RETURN_BOOLEAN_MEMBER(property, source, name) \
	do { \
		if (!strcmp((const char *)*String::Utf8Value(property), #name)) { \
			info.GetReturnValue().Set(Nan::New((source)->name)); \
			return; \
		} \
	} while(0)
#define RETURN_SOL_STRING_SLICE_ARRAY_TO_JS(source) \
	do { \
		Local<Array> jsArray = Nan::New<Array>((source).len); \
		sol_str_slice *slice; \
		int index; \
		SOL_VECTOR_FOREACH_IDX(&(source), slice, index) { \
			jsArray->Set(index, Nan::New<String>(slice->data, slice->len) \
				.ToLocalChecked()); \
		} \
		info.GetReturnValue().Set(jsArray); \
		return; \
	} while(0)
#define RETURN_SERVER_STRING_TO_JS(jsResource, name) \
	do { \
		Local<Array> jsArray = Nan::New<Array>(1); \
		Local<Object> jsHandlers = \
			Nan::New<Object>(*((jsResource)->_jsHandlers)); \
		Nan::Set(jsArray, 0, \
			Nan::Get(jsHandlers, Nan::New<String>((name)).ToLocalChecked()) \
				.ToLocalChecked()); \
		info.GetReturnValue().Set(jsArray); \
		return; \
	} while(0)
#define RETURN_SERVER_BOOLEAN(jsResource, name) \
	do { \
		if (!strcmp((const char *)*String::Utf8Value(property), name)) { \
			if (jsResource->_flags & serverFlagLookup()[(name)]) { \
				info.GetReturnValue().Set(Nan::True()); \
			} else { \
				info.GetReturnValue().Set(Nan::False()); \
			} \
			return; \
		} \
	} while(0)
#define DECLARE_JS_RESOURCE \
	SolOicResource *jsResource = SolOicResource::tryUnwrap(info.Holder()); \
\
	if (!jsResource) { \
		return; \
	} \
\
	if (!(jsResource->_c_resource || jsResource->_c_server_resource)) { \
		Nan::ThrowTypeError("resource is orphaned"); \
		return; \
	}

NAN_PROPERTY_GETTER(SolOicResource::namedPropertyGetter) {
	DECLARE_JS_RESOURCE
	if (jsResource->_c_resource) {
		if (!strcmp((const char *)*String::Utf8Value(property), "addr")) {
			info.GetReturnValue().Set(
				js_sol_network_link_addr(&(jsResource->_c_resource->addr)));
			return;
		}
		if (!strcmp((const char *)*String::Utf8Value(property), "device_id")) {
			info.GetReturnValue().Set(
				js_DeviceIdFromSlice(&(jsResource->_c_resource->device_id)));
			return;
		}
		if (!strcmp((const char *)*String::Utf8Value(property), "href")) {
			info.GetReturnValue().Set(
				Nan::New<String>(jsResource->_c_resource->href.data,
					jsResource->_c_resource->href.len).ToLocalChecked());
			return;
		}
		if (!strcmp((const char *)*String::Utf8Value(property),
				"interfaces")) {
			RETURN_SOL_STRING_SLICE_ARRAY_TO_JS(
				jsResource->_c_resource->interfaces);
			return;
		}
		RETURN_BOOLEAN_MEMBER(property, jsResource->_c_resource, is_observing);
		RETURN_BOOLEAN_MEMBER(property, jsResource->_c_resource, observable);
		RETURN_BOOLEAN_MEMBER(property, jsResource->_c_resource, secure);
		if (!strcmp((const char *)*String::Utf8Value(property), "types")) {
			RETURN_SOL_STRING_SLICE_ARRAY_TO_JS(
				jsResource->_c_resource->types);
			return;
		}
	} else {
		if (!strcmp((const char *)*String::Utf8Value(property),
				"interfaces")) {
			RETURN_SERVER_STRING_TO_JS(jsResource, "interface");
			return;
		}
		if (!strcmp((const char *)*String::Utf8Value(property),
				"types")) {
			RETURN_SERVER_STRING_TO_JS(jsResource, "resource_type");
			return;
		}
		RETURN_SERVER_BOOLEAN(jsResource, "discoverable");
		RETURN_SERVER_BOOLEAN(jsResource, "observable");
		RETURN_SERVER_BOOLEAN(jsResource, "active");
		RETURN_SERVER_BOOLEAN(jsResource, "slow");
		RETURN_SERVER_BOOLEAN(jsResource, "secure");
	}
}
NAN_PROPERTY_ENUMERATOR(SolOicResource::namedPropertyEnumerator) {
	DECLARE_JS_RESOURCE
	const char **propertyNames = 0;
	int propertyCount = 0;
	if (jsResource->_c_resource) {
		propertyCount = clientPropertyCount;
		propertyNames = clientPropertyNames;
	} else {
		propertyCount = serverPropertyCount;
		propertyNames = serverPropertyNames;
	}
	int index;
	Local<Array> properties = Nan::New<Array>(propertyCount);
	for (index = 0 ; index < propertyCount ; index++) {
		properties->Set(index,
			Nan::New(propertyNames[index]).ToLocalChecked());
	}
	info.GetReturnValue().Set(properties);
}

void SolOicResource::Empty() {
	if (_c_resource) {
		sol_oic_resource_unref(_c_resource);
		_c_resource = 0;
	} else if (_c_server_resource) {
		sol_oic_server_del_resource(_c_server_resource);
		_c_server_resource = 0;
		if (_jsHandlers) {
			delete _jsHandlers;
			_jsHandlers = 0;
		}
		hijack_unref();
	}
}

SolOicResource::SolOicResource(struct sol_oic_resource *c_resource):
		_c_resource(c_resource), _c_server_resource(0),
		_flags(SOL_OIC_FLAG_NONE), _jsHandlers(0) {
	sol_oic_resource_ref(_c_resource);
}

SolOicResource::SolOicResource(
		struct sol_oic_server_resource *c_resource,
		enum sol_oic_resource_flag flags,
		Nan::Persistent<Object> *jsHandlers) :
		_c_resource(0), _c_server_resource(c_resource), _flags(flags),
		_jsHandlers(jsHandlers) {
	hijack_ref();
}

SolOicResource::~SolOicResource() {
	Empty();
}

static Local<Array> js_sol_string_slice_array(struct sol_vector *vector) {
	Local<Array> jsArray = Nan::New<Array>(vector->len);
	sol_str_slice *slice;
	int index;
	SOL_VECTOR_FOREACH_IDX(vector, slice, index) {
		jsArray->Set(index, Nan::New<String>(slice->data, slice->len)
			.ToLocalChecked());
	}
	return jsArray;
}

void resourceIsGone(
	const Nan::WeakCallbackInfo< Nan::Persistent<Object> > & data) {
	Nan::Persistent<Object> *persistent = data.GetParameter();

	Local<Object> jsResource = Nan::New<Object>(*persistent);
	struct sol_oic_resource *resource =
		(struct sol_oic_resource *)Nan::GetInternalFieldPointer(jsResource, 0);
	sol_oic_resource_unref(resource);

	persistent->ClearWeak();
	persistent->Reset();
	delete persistent;
}

#define SET_PROPERTY_READONLY(destination, property, value) \
	Nan::ForceSet((destination), Nan::New(#property).ToLocalChecked(), \
		(value), \
		(v8::PropertyAttribute)(v8::ReadOnly | v8::DontDelete));

Local<Object> js_sol_oic_resource(struct sol_oic_resource *resource) {

	// Establish the function template
	static Nan::Persistent<FunctionTemplate> myTemplate;
	if (SOL_UNLIKELY(myTemplate.IsEmpty())) {
		Local<FunctionTemplate> theTemplate = Nan::New<FunctionTemplate>();
		theTemplate->SetClassName(Nan::New("SolOicResource").ToLocalChecked());
		theTemplate->InstanceTemplate()->SetInternalFieldCount(1);
		myTemplate.Reset(theTemplate);
		Nan::Set(Nan::GetFunction(theTemplate).ToLocalChecked(),
			Nan::New("displayName").ToLocalChecked(),
			Nan::New("SolOicResource").ToLocalChecked());
	}

	// Create the object, including a weak reference that unrefs the resource
	Local<Object> jsResource =
		Nan::GetFunction(Nan::New(myTemplate)).ToLocalChecked()->NewInstance();
	Nan::SetInternalFieldPointer(jsResource, 0, resource);
	sol_oic_resource_ref(resource);
	Nan::Persistent<Object> *persistent =
		new Nan::Persistent<Object>(jsResource);
	persistent->SetWeak(persistent, resourceIsGone,
		Nan::WeakCallbackType::kParameter);

	SET_PROPERTY_READONLY(jsResource, addr, 
		js_sol_network_link_addr(&(resource->addr)));
	SET_PROPERTY_READONLY(jsResource, device_id, 
		Nan::New<String>(resource->device_id.data, resource->device_id.len)
			.ToLocalChecked());
	SET_PROPERTY_READONLY(jsResource, href,
		Nan::New<String>(resource->href.data, resource->href.len)
			.ToLocalChecked());
	SET_PROPERTY_READONLY(jsResource, interfaces,
		js_sol_string_slice_array(&(resource->interfaces)));
	SET_PROPERTY_READONLY(jsResource, is_observing,
		Nan::New(resource->is_observing));
	SET_PROPERTY_READONLY(jsResource, observable,
		Nan::New(resource->observable));
	SET_PROPERTY_READONLY(jsResource, secure,
		Nan::New(resource->secure));
	SET_PROPERTY_READONLY(jsResource, types,
		js_sol_string_slice_array(&(resource->types)));

	return jsResource;
}
