#ifndef __SOLETTA_NODE_JS_DEVICE_ID_H__
#define __SOLETTA_NODE_JS_DEVICE_ID_H__

#include <v8.h>
#include <sol-str-slice.h>

v8::Local<v8::Value> js_DeviceIdFromSlice(const struct sol_str_slice *slice);

#endif /* ndef __SOLETTA_NODE_JS_DEVICE_ID_H__ */
