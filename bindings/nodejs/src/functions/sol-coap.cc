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

#include <nan.h>
#include <sol-coap.h>

#include "../common.h"
#include "../structures/js-handle.h"
#include "coap.h"

using namespace v8;

DECLARE_HANDLE_CLASS_IMPLEMENTATION_NEW_REFFABLE(SolCoapServer,
	struct sol_coap_server *, sol_coap_server_ref, sol_coap_server_unref);
DECLARE_HANDLE_CLASS_IMPLEMENTATION_REST(SolCoapServer);

#define NEW_SERVER(apiCall) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 1); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsUint32); \
		struct sol_coap_server *server = \
			(apiCall)(Nan::To<uint32_t>(info[0]).FromJust()); \
		if (server) { \
			Local<Object> returnValue = \
				SolCoapServer::New(sol_coap_server_ref(server)); \
			info.GetReturnValue().Set(returnValue); \
		} \
	} while(0)


NAN_METHOD(bind_sol_coap_server_new) {
	NEW_SERVER(sol_coap_server_new);
}

NAN_METHOD(bind_sol_coap_secure_server_new) {
	NEW_SERVER(sol_coap_secure_server_new);
}
