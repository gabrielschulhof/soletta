#include <nan.h>

#include "device-id.h"
#include "oic-platform-and-server-info.h"

using namespace v8;

#define JS_STRING_FROM_STRING_SLICE_MEMBER(destination, source, memberName) \
	do { \
		Nan::Set((destination), Nan::New(#memberName).ToLocalChecked(), \
			Nan::New<String>((source)->memberName.data, \
			(source)->memberName.len).ToLocalChecked()); \
	} while(0)

Local<Object> js_sol_oic_platform_information(const struct sol_oic_platform_information *info) {
	Local<Object> returnValue = Nan::New<Object>();

	JS_STRING_FROM_STRING_SLICE_MEMBER(returnValue, info, firmware_version);
	JS_STRING_FROM_STRING_SLICE_MEMBER(returnValue, info, hardware_version);
	JS_STRING_FROM_STRING_SLICE_MEMBER(returnValue, info, manufacture_date);
	JS_STRING_FROM_STRING_SLICE_MEMBER(returnValue, info, manufacturer_name);
	JS_STRING_FROM_STRING_SLICE_MEMBER(returnValue, info, manufacturer_url);
	JS_STRING_FROM_STRING_SLICE_MEMBER(returnValue, info, model_number);
	JS_STRING_FROM_STRING_SLICE_MEMBER(returnValue, info, os_version);
	JS_STRING_FROM_STRING_SLICE_MEMBER(returnValue, info, platform_id);
	JS_STRING_FROM_STRING_SLICE_MEMBER(returnValue, info, support_url);
	JS_STRING_FROM_STRING_SLICE_MEMBER(returnValue, info, system_time);

	return returnValue;
}

Local<Object> js_sol_oic_server_information(const struct sol_oic_server_information *info) {
	Local<Object> returnValue = Nan::New<Object>();

	JS_STRING_FROM_STRING_SLICE_MEMBER(returnValue, info, data_model_version);
	Nan::Set(returnValue, Nan::New("device_id").ToLocalChecked(),
		js_DeviceIdFromSlice(&(info->device_id)));

	JS_STRING_FROM_STRING_SLICE_MEMBER(returnValue, info, device_name);
	JS_STRING_FROM_STRING_SLICE_MEMBER(returnValue, info, spec_version);

	return returnValue;
}
