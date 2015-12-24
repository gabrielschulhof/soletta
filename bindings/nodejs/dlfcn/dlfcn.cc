#include <node.h>
#include <nan.h>
#include <dlfcn.h>

#include "../src/common.h"

using namespace v8;

static Local<Array> jsArrayFromBytes(unsigned char *bytes, size_t length) {
  size_t index;
  Local<Array> returnValue = Nan::New<Array>(length);

  for (index = 0; index < length; index++) {
    Nan::Set(returnValue, index, Nan::New(bytes[index]));
  }
  return returnValue;
}

static bool fillCArrayFromJSArray(unsigned char *bytes, size_t length,
                                  Local<Array> array) {
  size_t index, arrayLength;

  arrayLength = array->Length();
  if (arrayLength != length) {
    Nan::ThrowError("byte array has the wrong length");
    return false;
  }

  for (index = 0; index < length; index++) {
    Local<Value> byte = Nan::Get(array, index).ToLocalChecked();
    VALIDATE_VALUE_TYPE(byte, IsUint32, "byte array item", false);
    bytes[index] = (unsigned char)(byte->Uint32Value());
  }

  return true;
}

NAN_METHOD(bind_dlopen) {
	VALIDATE_ARGUMENT_COUNT(info, 2);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsString);
	VALIDATE_ARGUMENT_TYPE(info, 1, IsUint32);

	void *handle = dlopen(
		(const char *)*String::Utf8Value(info[0]),
		info[1]->Uint32Value());

	if ( handle ) {
		Local<Array> returnValue = jsArrayFromBytes((unsigned char *)(&handle), sizeof(void *));
		info.GetReturnValue().Set(returnValue);
	} else {
		info.GetReturnValue().Set(Nan::New(Nan::Null));
	}
}

NAN_METHOD(bind_dlclose) {
	VALIDATE_ARGUMENT_COUNT(info, 1);
	VALIDATE_ARGUMENT_TYPE(info, 0, IsArray);

	void *handle = 0;

	if (fillCArrayFromJSArray((unsigned char *)(&handle), sizeof(void *), Local<Array>::Cast(info[0]))) {
		info.GetReturnValue().Set(Nan::New(dlclose(handle)));
	}
}

NAN_METHOD(bind_dlerror) {
	VALIDATE_ARGUMENT_COUNT(info, 0);

	info.GetReturnValue().Set(Nan::New(dlerror()).ToLocalChecked());
}

void Init(Handle<Object> exports, Handle<Object> module) {
	SET_FUNCTION(exports, dlopen);
	SET_FUNCTION(exports, dlclose);
	SET_FUNCTION(exports, dlerror);

#ifdef RTLD_LAZY
	SET_CONSTANT_NUMBER(exports, RTLD_LAZY);
#endif /* def RTLD_LAZY */
#ifdef RTLD_NOW
	SET_CONSTANT_NUMBER(exports, RTLD_NOW);
#endif /* def RTLD_NOW */
#ifdef RTLD_GLOBAL
	SET_CONSTANT_NUMBER(exports, RTLD_GLOBAL);
#endif /* def RTLD_GLOBAL */
#ifdef RTLD_LOCAL
	SET_CONSTANT_NUMBER(exports, RTLD_LOCAL);
#endif /* def RTLD_LOCAL */
#ifdef RTLD_NODELETE
	SET_CONSTANT_NUMBER(exports, RTLD_NODELETE);
#endif /* def RTLD_NODELETE */
#ifdef RTLD_NOLOAD
	SET_CONSTANT_NUMBER(exports, RTLD_NOLOAD);
#endif /* def RTLD_NOLOAD */
#ifdef RTLD_DEEPBIND
	SET_CONSTANT_NUMBER(exports, RTLD_DEEPBIND);
#endif /* def RTLD_DEEPBIND */
}

NODE_MODULE(iotivity, Init)
