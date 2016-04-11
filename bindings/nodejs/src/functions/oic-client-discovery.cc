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

#include <string.h>
#include <nan.h>
#include <sol-oic-client.h>

#include "../common.h"
#include "../structures/network.h"
#include "../structures/oic-client.h"
#include "oic-client-info-callbacks.h"

using namespace v8;

static bool resourceFound(struct sol_oic_client *client,
    struct sol_oic_resource *resource, void *data) {
    Nan::HandleScope scope;

    // If Soletta tells us there are no more resources, we detach this callback
    // no matter what the JS callback returns
    bool keepDiscovering = !!resource;
    OicCallbackData *callbackData = (OicCallbackData *)data;

    // Call the JS callback
    Local<Value> arguments[2] = {
        Nan::New(*(callbackData->jsClient)),
        Nan::Null()
    };
    if (resource) {
        arguments[1] = SolOicClientResource::New(resource);
    }
    Local<Value> jsResult = callbackData->callback->Call(2, arguments);

    // Determine whether we should keep discovering
    if (!jsResult->IsBoolean()) {
        Nan::ThrowTypeError(
            "Resource discovery callback return value is not boolean");
    } else {
        keepDiscovering = keepDiscovering &&
            Nan::To<bool>(jsResult).FromJust();
    }

    // Tear down if discovery is done
    if (!keepDiscovering) {
        delete callbackData;
    }

    return keepDiscovering;
}

bool request_setup(Local<Value> clientValue, Local<Value> addressValue,
	Local<Value> callbackValue, struct sol_oic_client **client,
	struct sol_network_link_addr *theAddress, OicCallbackData **callbackData) {

    Local<Object> jsClient = Nan::To<Object>(clientValue).ToLocalChecked();
    *client = (struct sol_oic_client *)SolOicClient::Resolve(jsClient);
    if (!(*client)) {
        return false;
    }

    if (!c_sol_network_link_addr(
		Nan::To<Object>(addressValue).ToLocalChecked(), theAddress)) {
        return false;
    }

    *callbackData =
        OicCallbackData::New(jsClient, Local<Function>::Cast(callbackValue));
    if (!(*callbackData)) {
        return false;
    }

	return true;
}

NAN_METHOD(bind_sol_oic_client_find_resource) {
    VALIDATE_ARGUMENT_COUNT(info, 4);
    VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
    VALIDATE_ARGUMENT_TYPE(info, 1, IsObject);
    VALIDATE_ARGUMENT_TYPE(info, 2, IsString);
    VALIDATE_ARGUMENT_TYPE(info, 3, IsString);
    VALIDATE_ARGUMENT_TYPE(info, 4, IsFunction);

	struct sol_network_link_addr theAddress;
	struct sol_oic_client *client = 0;
	OicCallbackData *callbackData = 0;

	if (!request_setup(info[0], info[1], info[4], &client, &theAddress,
			&callbackData)) {
		return;
	}

    bool result = sol_oic_client_find_resource((struct sol_oic_client *)client,
        &theAddress, (const char *)*String::Utf8Value(info[2]),
        (const char *)*String::Utf8Value(info[3]), resourceFound,
        callbackData);

    if (!result) {
        delete callbackData;
    }

    info.GetReturnValue().Set(Nan::New(result));
}

#define GET_INFO_BY_ADDRESS(infoType) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 3); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsObject); \
		VALIDATE_ARGUMENT_TYPE(info, 1, IsObject); \
		VALIDATE_ARGUMENT_TYPE(info, 2, IsFunction); \
\
		struct sol_oic_client *client = 0; \
		struct sol_network_link_addr theAddress; \
		OicCallbackData *callbackData = 0; \
\
		if (!request_setup(info[0], info[1], info[2], &client, &theAddress, \
				&callbackData)) { \
			return; \
		} \
\
		bool result = sol_oic_client_get_##infoType##_info_by_addr(client, \
			&theAddress, infoType##InfoReceived, callbackData); \
\
		if (!result) { \
			delete callbackData; \
		} \
\
		info.GetReturnValue().Set(Nan::New(result)); \
	} while(0)

NAN_METHOD(bind_sol_oic_client_get_platform_info_by_addr) {
	GET_INFO_BY_ADDRESS(platform);
}

NAN_METHOD(bind_sol_oic_client_get_server_info_by_addr) {
	GET_INFO_BY_ADDRESS(server);
}
