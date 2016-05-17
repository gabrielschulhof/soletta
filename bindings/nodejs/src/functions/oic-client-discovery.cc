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

#include <string>
#include <errno.h>
#include <string.h>
#include <nan.h>
#include <sol-oic-client.h>

#include "../common.h"
#include "../structures/network.h"
#include "../structures/oic-handles.h"
#include "../structures/oic-client-callback-data.h"

using namespace v8;

static bool resourceFound(void *data, struct sol_oic_client *client,
    struct sol_oic_resource *resource) {
    Nan::HandleScope scope;

    // If Soletta tells us there are no more resources, we detach this callback
    // no matter what the JS callback returns
    bool keepDiscovering = !!resource;
    OicCallbackData *callbackData = (OicCallbackData *)data;

    // Call the JS callback
    Local<Value> arguments[2] = {
        Nan::New(callbackData->jsClient),
        Nan::Null()
    };
    if (resource) {
        arguments[1] = SolOicClientResource::New(resource);
    }

	// We store the JS-wrapped pending call handle in a local object because
	// from it, we can determine whether callbackData has been destroyed during
	// the call to the JS callback. For example, if the JS callback calls
	// sol_oic_pending_cancel() then we don't need to delete callbackData at
	// the end of this function. In fact, if we delete it, it's a double free
	// and will cause a segfault.
	Local<Object> jsPending = Nan::New<Object>(callbackData->jsPending);

    Local<Value> jsResult = callbackData->callback.Call(2, arguments);

    // Determine whether we should keep discovering
    if (!jsResult->IsBoolean()) {
        Nan::ThrowTypeError(
            "Resource discovery callback return value is not boolean");
    } else {
        keepDiscovering = keepDiscovering &&
            Nan::To<bool>(jsResult).FromJust();
    }

    // Tear down this callback if discovery is done and the callbackData has
    // not yet been freed.
    if (!keepDiscovering && SolOicPending::Resolve(jsPending, false)) {
        delete callbackData;
    }

    return keepDiscovering;
}

NAN_METHOD(bind_sol_oic_client_find_resources) {
    VALIDATE_ARGUMENT_COUNT(info, 4);
    VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
    VALIDATE_ARGUMENT_TYPE(info, 1, IsObject);
    VALIDATE_ARGUMENT_TYPE(info, 2, IsString);
    VALIDATE_ARGUMENT_TYPE(info, 3, IsString);
    VALIDATE_ARGUMENT_TYPE(info, 4, IsFunction);

    struct sol_network_link_addr theAddress;
    if (!c_sol_network_link_addr(info[1], &theAddress)) {
        return;
    }

    OicCallbackData *callbackData = OicCallbackData::New(info[0], info[4]);
    if (!callbackData) {
        return;
    }

    struct sol_oic_pending *pending =
        sol_oic_client_find_resources(
			(struct sol_oic_client *)SolOicClient::Resolve(
				Nan::New<Object>(callbackData->jsClient)),
	        &theAddress, (const char *)*String::Utf8Value(info[2]),
    	    (const char *)*String::Utf8Value(info[3]), resourceFound,
        	callbackData);

	SOL_OIC_PENDING_HANDLE_FAILURE(pending, info, callbackData,
		"sol_oic_client_find_resources: ");

    info.GetReturnValue().Set(callbackData->assignNativePending(pending));
}

NAN_METHOD(bind_sol_oic_pending_cancel) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);

	OicCallbackData *callbackData = (OicCallbackData *)
		SolOicPending::Resolve(Nan::To<Object>(info[0]).ToLocalChecked());
	if (!callbackData) {
		return;
	}

	sol_oic_pending_cancel(callbackData->pending);

	delete callbackData;
}
