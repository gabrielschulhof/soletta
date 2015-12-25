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

#include <node.h>
#include <nan.h>
#include <dlfcn.h>

#include "../src/common.h"

using namespace v8;

static Local<Array> jsArrayFromBytes(unsigned char *bytes, size_t length) {
  size_t index;
  Local<Array> returnValue = Nan::New<Array>(length);

  for (index = 0; index < length; index++) {
    Nan::Set(returnValue, index, Nan::New(bytes[index]));
  }
  return returnValue;
}

static bool fillCArrayFromJSArray(unsigned char *bytes, size_t length,
                                  Local<Array> array) {
  size_t index, arrayLength;

  arrayLength = array->Length();
  if (arrayLength != length) {
    Nan::ThrowError("byte array has the wrong length");
    return false;
  }

  for (index = 0; index < length; index++) {
    Local<Value> byte = Nan::Get(array, index).ToLocalChecked();
    VALIDATE_VALUE_TYPE(byte, IsUint32, "byte array item", false);
    bytes[index] = (unsigned char)(byte->Uint32Value());
  }

  return true;
}

NAN_METHOD(bind_dlopen) {
	VALIDATE_ARGUMENT_COUNT(info, 2);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsString);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsUint32);

	void *handle = dlopen(
		(const char *)*String::Utf8Value(info[0]),
		info[1]->Uint32Value());

	if ( handle ) {
		Local<Array> returnValue = jsArrayFromBytes((unsigned char *)(&handle), sizeof(void *));
		info.GetReturnValue().Set(returnValue);
	} else {
		info.GetReturnValue().Set(Nan::New(Nan::Null));
	}
}

NAN_METHOD(bind_dlclose) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsArray);

	void *handle = 0;

	if (fillCArrayFromJSArray((unsigned char *)(&handle), sizeof(void *), Local<Array>::Cast(info[0]))) {
		info.GetReturnValue().Set(Nan::New(dlclose(handle)));
	}
}

NAN_METHOD(bind_dlerror) {
	VALIDATE_ARGUMENT_COUNT(info, 0);

	info.GetReturnValue().Set(Nan::New(dlerror()).ToLocalChecked());
}

void Init(Handle<Object> exports, Handle<Object> module) {
	SET_FUNCTION(exports, dlopen);
	SET_FUNCTION(exports, dlclose);
	SET_FUNCTION(exports, dlerror);

#ifdef RTLD_LAZY
	SET_CONSTANT_NUMBER(exports, RTLD_LAZY);
#endif /* def RTLD_LAZY */
#ifdef RTLD_NOW
	SET_CONSTANT_NUMBER(exports, RTLD_NOW);
#endif /* def RTLD_NOW */
#ifdef RTLD_GLOBAL
	SET_CONSTANT_NUMBER(exports, RTLD_GLOBAL);
#endif /* def RTLD_GLOBAL */
#ifdef RTLD_LOCAL
	SET_CONSTANT_NUMBER(exports, RTLD_LOCAL);
#endif /* def RTLD_LOCAL */
#ifdef RTLD_NODELETE
	SET_CONSTANT_NUMBER(exports, RTLD_NODELETE);
#endif /* def RTLD_NODELETE */
#ifdef RTLD_NOLOAD
	SET_CONSTANT_NUMBER(exports, RTLD_NOLOAD);
#endif /* def RTLD_NOLOAD */
#ifdef RTLD_DEEPBIND
	SET_CONSTANT_NUMBER(exports, RTLD_DEEPBIND);
#endif /* def RTLD_DEEPBIND */
}

NODE_MODULE(iotivity, Init)
