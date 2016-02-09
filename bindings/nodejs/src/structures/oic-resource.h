/*
 * This file is part of the Soletta Project
 *
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SOLETTA_NODE_JS_OIC_RESOURCE__
#define __SOLETTA_NODE_JS_OIC_RESOURCE__

#include <nan.h>
#include <sol-oic-client.h>
#include <sol-oic-server.h>

class SolOicResource : public Nan::ObjectWrap {

public:
    static v8::Local<v8::Object> New(struct sol_oic_resource *c_resource);
    static v8::Local<v8::Object> New(struct sol_oic_server_resource *c_resource, enum sol_oic_resource_flag flags, Nan::Persistent<v8::Object> *jsHandlers);
    static void *CResource(v8::Local<v8::Object> source, bool clientResource);
    static void Empty(v8::Local<v8::Object> source);
private:
    static SolOicResource *tryUnwrap(v8::Local<v8::Object> source);
    static inline Nan::Persistent<v8::FunctionTemplate> & theTemplate();
    static NAN_METHOD(makeNewInstance);
    static NAN_PROPERTY_GETTER(namedPropertyGetter);
    static NAN_PROPERTY_ENUMERATOR(namedPropertyEnumerator);

    SolOicResource(struct sol_oic_resource *c_resource);
    SolOicResource(struct sol_oic_server_resource *c_resource, enum sol_oic_resource_flag flags, Nan::Persistent<v8::Object> *jsHandlers);
    ~SolOicResource();
    void Empty();

    struct sol_oic_resource *_c_resource;
    struct sol_oic_server_resource *_c_server_resource;
    enum sol_oic_resource_flag _flags;
    Nan::Persistent<v8::Object> *_jsHandlers;

};



#endif /* ndef __SOLETTA_NODE_JS_OIC_RESOURCE__ */
