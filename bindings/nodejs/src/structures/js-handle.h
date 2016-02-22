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

#ifndef __SOLETTA_NODEJS_HANDLE_H__
#define __SOLETTA_NODEJS_HANDLE_H__

#include <nan.h>
#include <sol-macros.h>

#define DECLARE_HANDLE_CLASS_PROTOTYPE(theName) \
    class theName { \
public: \
        static v8::Local<v8::Object> New(void *data); \
        static void *Resolve(v8::Local<v8::Object> jsObject); \
private: \
        static Nan::Persistent<v8::FunctionTemplate> & theTemplate(); \
    }

#define DECLARE_HANDLE_CLASS_IMPLEMENTATION_NEW_REFFABLE(theName, nativeType, ref, unref) \
    static void theName ## _isGone( \
    const Nan::WeakCallbackInfo<Nan::Persistent<v8::Object> > &data) { \
        Nan::Persistent<v8::Object> *persistent = data.GetParameter(); \
        nativeType nativeStruct = (nativeType)Nan::GetInternalFieldPointer( \
            Nan::New(*persistent), 0); \
        (unref)(nativeStruct); \
        persistent->Reset(); \
        delete persistent; \
    } \
    v8::Local<v8::Object> theName::New(void *data) { \
        v8::Local<v8::Object> returnValue = \
            Nan::GetFunction(Nan::New(theTemplate())).ToLocalChecked() \
            ->NewInstance(); \
        Nan::SetInternalFieldPointer(returnValue, 0, data); \
        Nan::Persistent<v8::Object> *persistent = \
            new Nan::Persistent<v8::Object>(returnValue); \
        persistent->SetWeak(persistent, theName ## _isGone, \
            Nan::WeakCallbackType::kParameter); \
        return returnValue; \
    }

#define DECLARE_HANDLE_CLASS_IMPLEMENTATION_NEW(theName) \
    v8::Local<v8::Object> theName::New(void *data) { \
        v8::Local<v8::Object> returnValue = \
            Nan::GetFunction(Nan::New(theTemplate())).ToLocalChecked() \
            ->NewInstance(); \
        Nan::SetInternalFieldPointer(returnValue, 0, data); \
        return returnValue; \
    } \

#define DECLARE_HANDLE_CLASS_IMPLEMENTATION_REST(theName) \
    void *theName::Resolve(v8::Local<v8::Object> jsObject) { \
        if (!Nan::New(theTemplate())->HasInstance(jsObject)) { \
            Nan::ThrowTypeError("Object is not of type " #theName); \
            return 0; \
        } \
        return Nan::GetInternalFieldPointer(jsObject, 0); \
    } \
    Nan::Persistent<v8::FunctionTemplate> & theName::theTemplate() { \
        static Nan::Persistent<v8::FunctionTemplate> returnValue; \
        if (SOL_UNLIKELY(returnValue.IsEmpty())) { \
            v8::Local<v8::FunctionTemplate> theTemplate = \
                Nan::New<v8::FunctionTemplate>(); \
            theTemplate \
            ->SetClassName(Nan::New(#theName).ToLocalChecked()); \
            theTemplate->InstanceTemplate()->SetInternalFieldCount(1); \
            Nan::Set(Nan::GetFunction(theTemplate).ToLocalChecked(), \
                Nan::New("displayName").ToLocalChecked(), \
                Nan::New(#theName).ToLocalChecked()); \
            returnValue.Reset(theTemplate); \
        } \
        return returnValue; \
    }

#endif /* ndef __SOLETTA_NODEJS_HANDLE_H__ */