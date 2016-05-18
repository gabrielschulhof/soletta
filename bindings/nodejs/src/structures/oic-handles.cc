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

#include <sol-oic-client.h>

#include "device-id.h"
#include "network.h"
#include "oic-handles.h"

const char *SolOicClient::jsClassName() { return "SolOicClient"; }

const char *SolOicClientResource::jsClassName() { return "SolOicResource"; }

void SolOicClientResource::ref(void *data) {
    sol_oic_resource_ref((struct sol_oic_resource *)data);
}

void SolOicClientResource::unref(void *data) {
    sol_oic_resource_unref((struct sol_oic_resource *)data);
}

using namespace v8;

static Local<Array> jsStringArrayFromStrSliceVector(
    struct sol_vector *vector) {
    Local<Array> jsArray = Nan::New<Array>(vector->len);
    sol_str_slice *slice;
    int index;
    SOL_VECTOR_FOREACH_IDX(vector, slice, index) {
        jsArray->Set(index,
            Nan::New<String>(slice->data, slice->len).ToLocalChecked());
    }
    return jsArray;
}

Local<Object> SolOicClientResource::New(struct sol_oic_resource *resource) {
    Local<Object> jsResource =
        JSReffableHandle<SolOicClientResource>::New((void *)resource);

    Nan::Set(jsResource, Nan::New("addr").ToLocalChecked(),
        js_sol_network_link_addr(&(resource->addr)));
    Nan::Set(jsResource, Nan::New("device_id").ToLocalChecked(),
        js_DeviceIdFromSlice(&(resource->device_id)));
    Nan::Set(jsResource, Nan::New("path").ToLocalChecked(),
        Nan::New<String>(resource->path.data,
            resource->path.len).ToLocalChecked());
    Nan::Set(jsResource, Nan::New("interfaces").ToLocalChecked(),
        jsStringArrayFromStrSliceVector(&(resource->interfaces)));
    Nan::Set(jsResource, Nan::New("is_observed").ToLocalChecked(),
        Nan::New(resource->is_observed));
    Nan::Set(jsResource, Nan::New("observable").ToLocalChecked(),
        Nan::New(resource->observable));
    Nan::Set(jsResource, Nan::New("secure").ToLocalChecked(),
        Nan::New(resource->secure));
    Nan::Set(jsResource, Nan::New("types").ToLocalChecked(),
        jsStringArrayFromStrSliceVector(&(resource->types)));

    return jsResource;
}

const char *SolOicRequest::jsClassName() { return "SolOicRequest"; }

const char *SolOicServerResource::jsClassName() {
	return "SolOicServerResource";
}
