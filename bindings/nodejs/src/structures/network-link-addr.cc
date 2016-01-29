#include <string.h>
#include <nan.h>
#include "network-link-addr.h"
#include "../common.h"
#include "../data.h"

using namespace v8;

bool c_sol_network_link_addr(Local<Object> jsAddress, struct sol_network_link_addr *c_address) {
	sol_network_link_addr local;

	Local<Value> jsBytesValue = Nan::Get(jsAddress, Nan::New("bytes").ToLocalChecked()).ToLocalChecked();
	VALIDATE_VALUE_TYPE(jsBytesValue, IsArray, "Network address bytes array", false);
	Local<Array> jsBytes = Local<Array>::Cast(jsBytesValue);
	if (!fillCArrayFromJSArray(local.addr.in6, 16 * sizeof(char), jsBytes)) {
		return false;
	}

	VALIDATE_AND_ASSIGN(local, family, sol_network_family, IsUint32, "Network address family", false, jsAddress, Uint32Value);
	VALIDATE_AND_ASSIGN(local, port, uint16_t, IsUint32, "Network address port", false, jsAddress, Uint32Value);

	memcpy(c_address, &local, sizeof(sol_network_link_addr));
	return true;
}

Local<Object> js_sol_network_link_addr(const struct sol_network_link_addr *c_address) {
	Local<Object> returnValue = Nan::New<Object>();

	Local<Array> bytes = jsArrayFromBytes((unsigned char *)(c_address->addr.in6), 16 * sizeof(char));
	Nan::Set(returnValue, Nan::New("bytes").ToLocalChecked(), bytes);

	SET_VALUE_ON_OBJECT(returnValue, Uint32, c_address, family);
	SET_VALUE_ON_OBJECT(returnValue, Uint32, c_address, port);

	return returnValue;
}
