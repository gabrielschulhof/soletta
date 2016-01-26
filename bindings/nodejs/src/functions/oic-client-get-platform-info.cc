#include <stdlib.h>
#include <sol-oic-client.h>
#include <nan.h>

#include "../common.h"
#include "../hijack.h"
#include "oic-client.h"
#include "../structures/oic-resource.h"
#include "../structures/network-link-addr.h"
#include "../structures/oic-platform-and-server-info.h"

using namespace v8;

void sol_oic_client_get_platform_info_callback(struct sol_oic_client *client, const struct sol_oic_platform_information *info, void *data) {
	Nan::HandleScope scope;
	Nan::Callback *callback = (Nan::Callback *)data;
	Local<Value> arguments[1] = {
		info ? Local<Value>::Cast(js_sol_oic_platform_information(info)) :
			Local<Value>::Cast(Nan::Null())
	};
	callback->Call(1, arguments);
	delete callback;
}

void sol_oic_client_get_server_info_callback(struct sol_oic_client *client, const struct sol_oic_server_information *info, void *data) {
	Nan::HandleScope scope;
	Nan::Callback *callback = (Nan::Callback *)data;
	Local<Value> arguments[1] = {
		info ? Local<Value>::Cast(js_sol_oic_server_information(info)) :
			Local<Value>::Cast(Nan::Null())
	};
	callback->Call(1, arguments);
	delete callback;
}

#define PLATFORM_AND_SERVER_INFO_BINDING(cFunction, theCallback, byParam, byParamPointer, toCConverter) \
	do { \
		VALIDATE_ARGUMENT_COUNT(info, 2); \
		VALIDATE_ARGUMENT_TYPE(info, 0, IsObject); \
		VALIDATE_ARGUMENT_TYPE(info, 1, IsFunction); \
\
		if (!toCConverter(Local<Object>::Cast(info[0]), &(byParam))) { \
			return; \
		} \
\
		Nan::Callback *callback = \
			new Nan::Callback(Local<Function>::Cast(info[1])); \
\
		bool returnValue = cFunction(sol_oic_client_get(), byParamPointer, \
			theCallback, callback); \
\
		if (!returnValue) { \
			delete callback; \
		} \
\
		info.GetReturnValue().Set(Nan::New(returnValue)); \
	} while(0)


NAN_METHOD(bind_sol_oic_client_get_platform_info) {
	struct sol_oic_resource *resource;
	PLATFORM_AND_SERVER_INFO_BINDING(sol_oic_client_get_platform_info,
		sol_oic_client_get_platform_info_callback,
		resource, resource, c_sol_oic_resource);
}

NAN_METHOD(bind_sol_oic_client_get_platform_info_by_addr) {
	struct sol_network_link_addr theAddress;
	PLATFORM_AND_SERVER_INFO_BINDING(sol_oic_client_get_platform_info_by_addr,
		sol_oic_client_get_platform_info_callback,
		theAddress, &theAddress, c_sol_network_link_addr);
}

NAN_METHOD(bind_sol_oic_client_get_server_info) {
	struct sol_oic_resource *resource;
	PLATFORM_AND_SERVER_INFO_BINDING(sol_oic_client_get_server_info,
		sol_oic_client_get_server_info_callback,
		resource, resource, c_sol_oic_resource);
}

NAN_METHOD(bind_sol_oic_client_get_server_info_by_addr) {
	struct sol_network_link_addr theAddress;
	PLATFORM_AND_SERVER_INFO_BINDING(sol_oic_client_get_server_info_by_addr,
		sol_oic_client_get_server_info_callback,
		theAddress, &theAddress, c_sol_network_link_addr);
}
