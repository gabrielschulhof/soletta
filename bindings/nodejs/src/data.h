#ifndef __SOLETTA_JS_DATA_H__
#define __SOLETTA_JS_DATA_H__

#include <v8.h>

v8::Local<v8::Array> jsArrayFromBytes(unsigned char *bytes, size_t length);
bool fillCArrayFromJSArray(unsigned char *bytes, size_t length,
                                  v8::Local<v8::Array> array);

#endif /* ndef __SOLETTA_JS_DATA_H__ */
