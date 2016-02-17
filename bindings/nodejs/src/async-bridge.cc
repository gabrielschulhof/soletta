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

#include <sol-macros.h>
#include "async-bridge.h"
#include "hijack.h"
#include "common.h"

using namespace v8;

static Nan::Persistent<Object> *bridge = 0;
static Nan::Callback *jsAdd = 0, *jsDel = 0, *jsCall = 0, *jsListenerCount = 0;

NAN_METHOD(bind__async_bridge_init) {
	if ( bridge ) {
		return Nan::ThrowError( "Async bridge already initialized" );
	}

	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);

	Local<Object> localBridge = Local<Object>::Cast(info[0]);
	bridge = new Nan::Persistent<Object>(localBridge);
	jsAdd = new Nan::Callback(Local<Function>::Cast(Nan::Get(localBridge,
		Nan::New("add").ToLocalChecked()).ToLocalChecked()));
	jsDel = new Nan::Callback(Local<Function>::Cast(Nan::Get(localBridge,
		Nan::New("del").ToLocalChecked()).ToLocalChecked()));
	jsCall = new Nan::Callback(Local<Function>::Cast(Nan::Get(localBridge,
		Nan::New("call").ToLocalChecked()).ToLocalChecked()));
	jsListenerCount = new Nan::Callback(Local<Function>::Cast(Nan::Get(localBridge,
		Nan::New("listenerCount").ToLocalChecked()).ToLocalChecked()));

	// Return "this" for chaining
	info.GetReturnValue().Set(info.This());
}

uint32_t async_bridge_get_listener_count(Local<String> eventName) {
	Local<Value> arguments[1] = { eventName };
	return jsListenerCount->Call(Nan::New<Object>(*bridge), 1, arguments)->Uint32Value();
}

void async_bridge_add(Local<String> eventName, Local<Function> jsCallback) {
	Local<Value> arguments[2] = { eventName, jsCallback };
	hijack_ref();
	jsAdd->Call(Nan::New<Object>(*bridge), 2, arguments);
}

void async_bridge_del_event(Local<String> eventName) {
	Local<Value> arguments[1] = { eventName };
	uint32_t index, eventCount = async_bridge_get_listener_count(eventName);
	jsDel->Call(Nan::New<Object>(*bridge), 1, arguments);
	for (index = 0; index < eventCount; index++) {
		hijack_unref();
	}
}

void async_bridge_del(Local<String> eventName, Local<Function> jsCallback) {
	Local<Value> arguments[2] = { eventName, jsCallback };
	jsDel->Call(Nan::New<Object>(*bridge), 2, arguments);
	hijack_unref();
}

// The event name is the first argument in @a arguments
Local<Array> async_bridge_call(uint32_t argumentCount, Local<Value> arguments[]) {
	return Local<Array>::Cast(jsCall->Call(Nan::New<Object>(*bridge), argumentCount, arguments));
}
