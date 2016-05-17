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

#pragma once

#include <v8.h>
#include <sol-oic-client.h>
#include "oic-handles.h"

class OicCallbackData {
public:
    OicCallbackData(v8::Local<v8::Value> jsClientValue, v8::Local<v8::Value> jsCallbackValue);
    virtual ~OicCallbackData();
	v8::Local<v8::Object> assignNativePending(struct sol_oic_pending *pending);
    static OicCallbackData *New(v8::Local<v8::Value> jsClient, v8::Local<v8::Value> jsCallback);
    Nan::Persistent<v8::Object> jsClient;
    Nan::Callback callback;
    Nan::Persistent<v8::Object> jsPending;
    struct sol_oic_pending *pending;
    bool hijackRefWasSuccessful;
};

class SolOicPending : public JSHandle<SolOicPending> {
public:
    static const char *jsClassName();
};

#define SOL_OIC_PENDING_HANDLE_FAILURE(pending, info, callbackData, message) \
	do { \
		if (!(pending)) { \
			delete (callbackData); \
			Nan::ThrowError((std::string((message)) + strerror(errno)).c_str()); \
			(info).GetReturnValue().Set(Nan::Null()); \
			return; \
		} \
	} while(0)
