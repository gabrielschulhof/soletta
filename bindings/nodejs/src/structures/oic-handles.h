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
#include "js-handle.h"

class SolOicClient : public JSHandle<SolOicClient> {
public: static const char *jsClassName();
};

class SolOicClientResource : public JSHandle<SolOicClientResource> {
public:
    static const char *jsClassName();
    static v8::Local<v8::Object> New(struct sol_oic_resource *resource);
};

class SolOicServerResource : public JSHandle<SolOicServerResource> {
public:
    static const char *jsClassName();
};