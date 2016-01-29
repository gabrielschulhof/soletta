#include <stdlib.h>
#include <sol-coap.h>
#include "oic-client.h"

struct sol_oic_client *sol_oic_client_get() {
	static struct sol_oic_client *the_client = NULL;

	if (SOL_UNLIKELY(!the_client)) {
		the_client =
			(struct sol_oic_client *)malloc(sizeof(struct sol_oic_client));
#ifdef SOL_OIC_CLIENT_API_VERSION
		the_client->api_version = SOL_OIC_CLIENT_API_VERSION;
#endif /* def SOL_OIC_CLIENT_API_VERSION */
		the_client->dtls_server = sol_coap_secure_server_new(0);
		the_client->server = sol_coap_server_new(0);
	}

	return the_client;
}
