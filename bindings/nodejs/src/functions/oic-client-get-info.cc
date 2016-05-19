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

#include "../common.h"
#include "../structures/network.h"
#include "../structures/oic-client-callback-data.h"
#include "../structures/oic-handles.h"
#include "../structures/oic-info.h"

using namespace v8;

#define INFO_CALLBACK(infoType, info) \
	do { \
		Nan::HandleScope scope; \
		OicCallbackData *callbackData = (OicCallbackData *)data; \
		Local<Value> arguments[2] = { \
			Nan::New(callbackData->jsClient), \
			Nan::Null() \
		}; \
		if (info) { \
			arguments[1] = js_sol_oic_##infoType##_info((info)); \
		} \
		Local<Object> jsPending = Nan::New<Object>(callbackData->jsPending); \
		callbackData->callback.Call(2, arguments); \
		if (SolOicPending::Resolve(jsPending, false)) { \
			delete callbackData; \
		} \
	} while(0)

static void getPlatformInfo(void *data, struct sol_oic_client *client,
	const struct sol_oic_platform_info *info) {
	INFO_CALLBACK(platform, info);
}

static void getServerInfo(void *data, struct sol_oic_client *client,
	const struct sol_oic_device_info *info) {
	INFO_CALLBACK(device, info);
}

#define GET_INFO(info, keyDeclaration, getKey, keyPointer, api, callback) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 3); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsObject); \
		VALIDATE_ARGUMENT_TYPE(info, 1, IsObject); \
		VALIDATE_ARGUMENT_TYPE(info, 2, IsFunction); \
\
		keyDeclaration; \
		if (!(getKey)) { \
			return; \
		} \
\
		OicCallbackData *callbackData = \
			OicCallbackData::New(info[0], info[2]); \
		if (!callbackData) { \
			return; \
		} \
\
		struct sol_oic_pending *pending = api( \
			(struct sol_oic_client *)SolOicClient::Resolve( \
				Nan::New<Object>(callbackData->jsClient)), keyPointer, \
				callback, callbackData); \
\
		SOL_OIC_PENDING_HANDLE_FAILURE(pending, info, callbackData, \
			#api ": "); \
\
		info.GetReturnValue() \
			.Set(callbackData->assignNativePending(pending)); \
	} while(0)

NAN_METHOD(bind_sol_oic_client_get_platform_info) {
	GET_INFO(info, struct sol_oic_resource *resource = 0,
		resource = (struct sol_oic_resource *)SolOicClientResource::Resolve(
			Nan::To<Object>(info[1]).ToLocalChecked()), resource,
		sol_oic_client_get_platform_info, getPlatformInfo);
}

NAN_METHOD(bind_sol_oic_client_get_platform_info_by_addr) {
	GET_INFO(info, struct sol_network_link_addr address,
		c_sol_network_link_addr(
			Nan::To<Object>(info[1]).ToLocalChecked(), &address), &address,
		sol_oic_client_get_platform_info_by_addr, getPlatformInfo);
}

NAN_METHOD(bind_sol_oic_client_get_server_info) {
	GET_INFO(info, struct sol_oic_resource *resource = 0,
		resource = (struct sol_oic_resource *)SolOicClientResource::Resolve(
			Nan::To<Object>(info[1]).ToLocalChecked()), resource,
		sol_oic_client_get_server_info, getServerInfo);
}

NAN_METHOD(bind_sol_oic_client_get_server_info_by_addr) {
	GET_INFO(info, struct sol_network_link_addr address,
		c_sol_network_link_addr(
			Nan::To<Object>(info[1]).ToLocalChecked(), &address), &address,
		sol_oic_client_get_server_info_by_addr, getServerInfo);
}
