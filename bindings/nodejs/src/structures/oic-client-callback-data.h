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

#define OIC_CLIENT_ONE_SHOT_CALLBACK(argumentCount, clientIndex, callbackData) \
	for (bool run_once = true; run_once; run_once = false) \
		for (Nan::HandleScope scope; run_once; run_once = false) \
			for (Local<Value> arguments[(argumentCount)]; run_once; ({ \
				run_once = false; \
				arguments[(clientIndex)] = \
					Nan::New(((OicCallbackData *)(data))->jsClient); \
				Local<Object> jsPending = \
					Nan::New<Object>((callbackData)->jsPending); \
				(callbackData)->callback.Call((argumentCount), arguments); \
				if (SolOicPending::IsValid(jsPending)) { \
					delete (callbackData); \
				} \
			})) \

#define OIC_CLIENT_API_CALL(info, clientIndex, callbackIndex, api, ...) \
	do { \
		OicCallbackData *callbackData = \
			OicCallbackData::New((info)[(clientIndex)], \
				(info)[(callbackIndex)]); \
		if (!callbackData) { \
			return; \
		} \
		struct sol_oic_pending *pending = (api)( \
			(struct sol_oic_client *)SolOicClient::Resolve( \
				Nan::To<Object>((info)[(clientIndex)]) \
					.ToLocalChecked()), \
			__VA_ARGS__, callbackData); \
		if (!pending) { \
			delete callbackData; \
			Nan::ThrowError(((std::string(#api) + ": ") + \
				strerror(errno)).c_str()); \
			(info).GetReturnValue().Set(Nan::Null()); \
			return; \
		} \
		(info).GetReturnValue() \
			.Set(callbackData->assignNativePending(pending)); \
	} while(0)
