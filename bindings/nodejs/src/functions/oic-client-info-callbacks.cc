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
#include "../structures/oic-client.h"
#include "../structures/oic-info.h"
#include "oic-client-info-callbacks.h"

using namespace v8;

#define INFO_CALLBACK(infoType, info) \
	do { \
		Nan::HandleScope scope; \
		OicCallbackData *callbackData = (OicCallbackData *)data; \
		Local<Value> arguments[2] = { \
			Nan::New(*(callbackData->jsClient)), \
			js_sol_oic_##infoType##_information((info)) \
		}; \
		callbackData->callback->Call(2, arguments); \
		delete callbackData; \
	} while(0)

void platformInfoReceived(struct sol_oic_client *client,
		const struct sol_oic_platform_information *info, void *data) {
	INFO_CALLBACK(platform, info);
}

void serverInfoReceived(struct sol_oic_client *client,
		const struct sol_oic_server_information *info, void *data) {
	INFO_CALLBACK(server, info);
}
