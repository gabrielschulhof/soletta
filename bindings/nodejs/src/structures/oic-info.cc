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

#include <string>
#include <nan.h>

#include "../common.h"
#include "../data.h"
#include "oic-info.h"
#include "device-id.h"

using namespace v8;

#define ASSIGN_PROPERTY_FROM_STR_SLICE(destination, source, memberName) \
	(destination)->Set(Nan::New(#memberName).ToLocalChecked(), \
		Nan::New((source)->memberName.data, \
			(source)->memberName.len).ToLocalChecked())

Local<Value> js_sol_oic_platform_information(
	const struct sol_oic_platform_information *info) {
	if (!info) {
		return Nan::Null();
	}
	Local<Object> returnValue = Nan::New<Object>();

	ASSIGN_PROPERTY_FROM_STR_SLICE(returnValue, info, firmware_version);
	ASSIGN_PROPERTY_FROM_STR_SLICE(returnValue, info, hardware_version);
	ASSIGN_PROPERTY_FROM_STR_SLICE(returnValue, info, manufacture_date);
	ASSIGN_PROPERTY_FROM_STR_SLICE(returnValue, info, manufacturer_name);
	ASSIGN_PROPERTY_FROM_STR_SLICE(returnValue, info, manufacturer_url);
	ASSIGN_PROPERTY_FROM_STR_SLICE(returnValue, info, model_number);
	ASSIGN_PROPERTY_FROM_STR_SLICE(returnValue, info, os_version);
	ASSIGN_PROPERTY_FROM_STR_SLICE(returnValue, info, platform_id);
	ASSIGN_PROPERTY_FROM_STR_SLICE(returnValue, info, platform_version);
	ASSIGN_PROPERTY_FROM_STR_SLICE(returnValue, info, support_url);
	ASSIGN_PROPERTY_FROM_STR_SLICE(returnValue, info, system_time);

	return returnValue;
}
Local<Value> js_sol_oic_server_information(
	const struct sol_oic_server_information *info) {
	if (!info) {
		return Nan::Null();
	}
	Local<Object> returnValue = Nan::New<Object>();

	ASSIGN_PROPERTY_FROM_STR_SLICE(returnValue, info, data_model_version);
	Nan::Set(returnValue, Nan::New("device_id").ToLocalChecked(),
		js_DeviceIdFromSlice(&(info->device_id)));
	ASSIGN_PROPERTY_FROM_STR_SLICE(returnValue, info, device_name);
	ASSIGN_PROPERTY_FROM_STR_SLICE(returnValue, info, spec_version);

	return returnValue;
}
