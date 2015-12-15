#include <nan.h>

NAN_METHOD(bind__dummy) {
	int x = info[0]->Uint32Value();

	info.GetReturnValue().Set(Nan::New(x+5));
}
