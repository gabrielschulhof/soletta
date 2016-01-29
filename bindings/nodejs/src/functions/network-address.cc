#include <nan.h>

#include "../structures/network-link-addr.h"
#include "../common.h"

using namespace v8;

NAN_METHOD(bind_sol_network_addr_from_str) {
	VALIDATE_ARGUMENT_COUNT(info, 2);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsObject);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsString);

	sol_network_link_addr input_address;

	if (!c_sol_network_link_addr(Local<Object>::Cast(info[0]), &input_address)) {
		return;
	}

	const sol_network_link_addr *output_address = sol_network_addr_from_str(&input_address,
		(const char *)*String::Utf8Value(info[1]));

	if (!output_address) {
		info.GetReturnValue().Set(Nan::Null());
	} else {
		info.GetReturnValue().Set(js_sol_network_link_addr(output_address));
	}
}
