#ifndef __SOLETTA_NODE_JS_OIC_RESOURCE__
#define __SOLETTA_NODE_JS_OIC_RESOURCE__

#include <v8.h>
#include <sol-oic-client.h>

v8::Local<v8::Object> js_sol_oic_resource(struct sol_oic_resource *resource);
bool c_sol_oic_resource(v8::Local<v8::Object> source, sol_oic_resource **resource);

#endif /* ndef __SOLETTA_NODE_JS_OIC_RESOURCE__ */
