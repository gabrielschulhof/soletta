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
#include <sol-coap.h>
#include "js-handle.h"

class SolOicRequest : public JSHandle<SolOicRequest> {
public:
    static const char *jsClassName();
    static v8::Local<v8::Value> New(v8::Local<v8::Value> jsMethod, v8::Local<v8::Value> jsResource, bool confirm);
    static v8::Local<v8::Value> New(void *data, enum sol_coap_method method);
	static void *Resolve(v8::Local<v8::Value> jsRequest);
	static void Invalidate(v8::Local<v8::Value> jsRequest);
};

class SolOicResponse : public JSHandle<SolOicResponse> {
public:
    static const char *jsClassName();
    static v8::Local<v8::Value> New(v8::Local<v8::Value> jsRequestValue);
	static void *Resolve(v8::Local<v8::Object> jsResponseValue, v8::Local<v8::Object> jsRequestValue);
};