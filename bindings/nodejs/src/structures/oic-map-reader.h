#ifndef __SOLETTA_NODE_JS_OIC_MAP_READER_H__
#define __SOLETTA_NODE_JS_OIC_MAP_READER_H__

#include <v8.h>
#include <sol-oic-client.h>

v8::Local<v8::Object> js_sol_oic_map_reader(const struct sol_oic_map_reader *representation);

#endif /* ndef __SOLETTA_NODE_JS_OIC_MAP_READER_H__ */
