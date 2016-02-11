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

#include <dlfcn.h>
#include <nan.h>
#include "common.h"

using namespace v8;

static Nan::Callback *test_hook = 0;
static void test_hook_prologue(const char *symbol) {
	if (!test_hook) {
		return;
	}
	Nan::HandleScope scope;
	Local<Value> arguments[2] = {
		Nan::New(symbol).ToLocalChecked(),
		Nan::True()
	};
	test_hook->Call(2, arguments);
}
static void test_hook_epilogue(const char *symbol) {
	if (!test_hook) {
		return;
	}
	Nan::HandleScope scope;
	Local<Value> arguments[2] = {
		Nan::New(symbol).ToLocalChecked(),
		Nan::False()
	};
	test_hook->Call(2, arguments);
}

NAN_METHOD(bind__test_hook) {
	static bool try_to_set_hook = true;
	static void (*wrapper_set_hook)(
		void (*)(const char *),
		void (*)(const char *)) = 0;

	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE_OR_NULL(info, 0, IsFunction);
	if (test_hook) {
		delete test_hook;
	}
	if (!info[0]->IsNull()) {
		test_hook = new Nan::Callback(Local<Function>::Cast(info[0]));
	}

	if (try_to_set_hook) {
		void *me = dlopen(NULL, RTLD_NOW);
		wrapper_set_hook =
			(void (*)(void (*)(const char *),void (*)(const char *)))
				dlsym(me, "wrapper_set_hook");
		if (wrapper_set_hook) {
			wrapper_set_hook(test_hook_prologue, test_hook_epilogue);
		} else {
			Nan::ThrowError( "wrapper_set_hook() not found" );
		}
		try_to_set_hook = false;
	}
}
