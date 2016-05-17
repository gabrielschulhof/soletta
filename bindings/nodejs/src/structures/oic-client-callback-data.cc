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

#include <nan.h>
#include <sol-oic-client.h>

#include "../hijack.h"
#include "oic-handles.h"
#include "oic-client-callback-data.h"

using namespace v8;

OicCallbackData::OicCallbackData(
	Local<Value> jsClientValue, Local<Value> jsCallbackValue):
		jsClient(Nan::To<Object>(jsClientValue).ToLocalChecked()),
		callback(Local<Function>::Cast(jsCallbackValue)),
		hijackRefWasSuccessful(hijack_ref()) {}
OicCallbackData::~OicCallbackData() {
	jsClient.Reset();
	if (!jsPending.IsEmpty()) {
		Nan::SetInternalFieldPointer(Nan::New<Object>(jsPending), 0, 0);
		jsPending.Reset();
	}
    if (hijackRefWasSuccessful) {
        hijack_unref();
    }
}

OicCallbackData *OicCallbackData::New(Local<Value> jsClient,
    Local<Value> jsCallback) {
    OicCallbackData *data = new OicCallbackData(jsClient, jsCallback);
    if (!data) {
        Nan::ThrowError("Failed to allocate OicCallbackData");
    }
	if (!data->hijackRefWasSuccessful) {
		delete data;
		data = 0;
	}
    return data;
}

Local<Object> OicCallbackData::assignNativePending(
	struct sol_oic_pending *_pending) {
	pending = _pending;
	Local<Object> jsHandle = SolOicPending::New((void *)this);
	jsPending.Reset(jsHandle);
	return jsHandle;
}

const char *SolOicPending::jsClassName() { return "SolOicPending"; }
