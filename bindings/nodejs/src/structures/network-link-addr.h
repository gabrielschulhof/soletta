#ifndef __SOLETTA_NODE_JS_NETWORK_LINK_ADDR_H__
#define __SOLETTA_NODE_JS_NETWORK_LINK_ADDR_H__

#include <v8.h>
#include <sol-network.h>

bool c_sol_network_link_addr(v8::Local<v8::Object> jsAddress, struct sol_network_link_addr *c_address);
v8::Local<v8::Object> js_sol_network_link_addr(const struct sol_network_link_addr *c_address);

#endif /* ndef __SOLETTA_NODE_JS_NETWORK_LINK_ADDR_H__ */
