#ifndef __SOLETTA_NODE_OIC_PLATFORM_INFO_H__
#define __SOLETTA_NODE_OIC_PLATFORM_INFO_H__

#include <v8.h>
#include <sol-oic-common.h>

v8::Local<v8::Object> js_sol_oic_platform_information(const struct sol_oic_platform_information *info);
v8::Local<v8::Object> js_sol_oic_server_information(const struct sol_oic_server_information *info);

#endif /* ndef __SOLETTA_NODE_OIC_PLATFORM_INFO_H__ */
