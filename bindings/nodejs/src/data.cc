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
#include <v8.h>
#include "data.h"
#include "common.h"

using namespace v8;

Local<Array> jsArrayFromBytes(const char *bytes, size_t length) {
  size_t index;
  Local<Array> returnValue = Nan::New<Array>(length);

  for (index = 0; index < length; index++) {
    Nan::Set(returnValue, index, Nan::New(bytes[index]));
  }
  return returnValue;
}

bool fillCArrayFromJSArray(char *bytes, size_t length,
                                  Local<Array> array) {
  size_t index, arrayLength;
  int32_t value;

  arrayLength = array->Length();
  if (arrayLength != length) {
    Nan::ThrowError("byte array has the wrong length");
    return false;
  }

  for (index = 0; index < length; index++) {
    Local<Value> byte = Nan::Get(array, index).ToLocalChecked();
    VALIDATE_VALUE_TYPE(byte, IsInt32, "byte array item", false);
	value = byte->Int32Value();
	if (value < SCHAR_MIN || value > SCHAR_MAX) {
		Nan::ThrowRangeError(
			"byte array item value exceeds signed character range");
		return false;
	}
    bytes[index] = (char)(value);
  }

  return true;
}
