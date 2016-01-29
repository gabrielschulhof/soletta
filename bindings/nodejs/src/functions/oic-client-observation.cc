#include <nan.h>
#include <sol-oic-client.h>

#include "../async-bridge.h"
#include "../common.h"
#include "../structures/network-link-addr.h"
#include "oic-client.h"
#include "../structures/oic-map-reader.h"
#include "../structures/oic-resource.h"

using namespace v8;

static void defaultConfirmingResourceObserver(
		sol_coap_responsecode_t responseCode,
		struct sol_oic_client *client,
		const struct sol_network_link_addr *address,
		const struct sol_oic_map_reader *representation,
		void *data) {
	Nan::HandleScope scope;
	char eventName[256] = "";
	snprintf(eventName, 255, "oic.client.observe.confirm.%p", data);
	Local<Value> arguments[4] = {
		Nan::New(eventName).ToLocalChecked(),
		Nan::New(responseCode),
		js_sol_network_link_addr(address),
		representation ?
			Local<Value>::Cast(js_sol_oic_map_reader(representation)) :
			Local<Value>::Cast(Nan::Null())
	};

	async_bridge_call(4, arguments);
}

NAN_METHOD(bind_sol_oic_client_resource_set_observable) {
	VALIDATE_ARGUMENT_COUNT(info, 3);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsFunction);
	VALIDATE_ARGUMENT_TYPE(info, 2, IsBoolean);

	struct sol_oic_resource *resource;
	if (!c_sol_oic_resource(Local<Object>::Cast(info[0]), &resource)) {
		return;
	}

	bool returnValue = true;
	bool observe = info[2]->BooleanValue();
	char eventName[256] = "";
	snprintf(eventName, 255, "oic.client.observe.confirm.%p", (void *)resource);
	Local<String> jsEventName = Nan::New(eventName).ToLocalChecked();

	if (observe) {
		if (async_bridge_get_listener_count(jsEventName) == 0) {
			returnValue = sol_oic_client_resource_set_observable(
				sol_oic_client_get(), resource,
					defaultConfirmingResourceObserver, resource, true);
		}
		if (returnValue) {
			async_bridge_add(jsEventName, Local<Function>::Cast(info[1]));
		}
	} else {
		if (async_bridge_get_listener_count(jsEventName) == 1) {
			returnValue = sol_oic_client_resource_set_observable(
				sol_oic_client_get(), resource,
					defaultConfirmingResourceObserver, resource, false);
		}
		if (returnValue) {
			async_bridge_del(jsEventName, Local<Function>::Cast(info[1]));
		}
	}

	info.GetReturnValue().Set(Nan::New(returnValue));
}
