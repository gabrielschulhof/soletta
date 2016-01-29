#include <nan.h>
#include "../data.h"
#include "oic-map-reader.h"

using namespace v8;

Local<Object> js_sol_oic_map_reader(const struct sol_oic_map_reader *representation) {
	Local<Object> returnValue = Nan::New<Object>();

	struct sol_oic_repr_field field;
	enum sol_oic_map_loop_reason end_reason;
	struct sol_oic_map_reader iterator = {0, 0, 0, 0, 0, 0};
	SOL_OIC_MAP_LOOP(representation, &field, &iterator, end_reason) {
		returnValue->Set(Nan::New(field.key).ToLocalChecked(),
			field.type == SOL_OIC_REPR_TYPE_UINT ?
				Local<Value>::Cast(Nan::New<Uint32>((uint32_t)field.v_uint)) :
			field.type == SOL_OIC_REPR_TYPE_INT ?
				Local<Value>::Cast(Nan::New<Int32>((int32_t)field.v_int)) :
			field.type == SOL_OIC_REPR_TYPE_SIMPLE ?
				Local<Value>::Cast(Nan::New(field.v_simple)) :
			field.type == SOL_OIC_REPR_TYPE_TEXT_STRING ?
				Local<Value>::Cast(Nan::New<String>(field.v_slice.data,
					field.v_slice.len).ToLocalChecked()) :
			field.type == SOL_OIC_REPR_TYPE_BYTE_STRING ?
				Local<Value>::Cast(
					jsArrayFromBytes((unsigned char *)(field.v_slice.data),
						field.v_slice.len)) :
			field.type == SOL_OIC_REPR_TYPE_FLOAT ?
				Local<Value>::Cast(Nan::New(field.v_float)) :
			field.type == SOL_OIC_REPR_TYPE_DOUBLE ?
				Local<Value>::Cast(Nan::New(field.v_double)) :
			Local<Value>::Cast(Nan::Undefined()));
	}

	return returnValue;
}
