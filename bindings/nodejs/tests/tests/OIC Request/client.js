/*
 * This file is part of the Soletta Project
 *
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

var _ = require( "lodash" );
var soletta = require( require( "path" )
	.join( require( "bindings" ).getRoot( __filename ), "lowlevel" ) );
var utils = require( "../../assert-to-console" );
var uuid = process.argv[ 2 ];

console.log( JSON.stringify( { assertionCount: 2 } ) );

var client = soletta.sol_oic_client_new();

var destination = soletta.sol_network_link_addr_from_str( {
	bytes: _.fill( Array( 16 ), 0 ),
	family: soletta.sol_network_family.SOL_NETWORK_FAMILY_INET,
	port: 5683
}, "224.0.1.187" );

function doResourceRequest( resource ) {
	var request, requestHandle;

	utils.assert( "ok", !!soletta.sol_oic_resource_ref(resource),
		"Client: Successfully referenced resource" );

	request =
		soletta.sol_oic_client_request_new(soletta.sol_coap_method.SOL_COAP_METHOD_GET, resource);
	utils.assert( "ok", !!request, "Client: Created request" );

	requestHandle = soletta.sol_oic_client_request( client, request,
		function( code, client, address, payload ) {
			console.error( "GET response: " + JSON.stringify( {
				code: code,
				client: client,
				address: address,
				payload: payload
			}, null, 4 ) );
		} );
}

var findResourcesHandle = soletta.sol_oic_client_find_resources( client, destination, "", "",
	function( client, resource ) {
		if ( !resource ) {
			utils.assert( "ok", false, "Client: Failed to locate resource" );
		}
		else if ( resource.path === "/a/" + uuid ) {
			utils.assert( "ok", true, "Client: Resource found" );
			soletta.sol_oic_pending_cancel( findResourcesHandle );

			doResourceRequest( resource );

			// It is important to cause this callback to return false after having found the
			// resource and having called the teardown (sol_oic_pending_cancel()) explicitly
			// because, on the native side, when this callback returns false the structure
			// associated with this callback is torn down, but only if it is still valid. If,
			// however, we've already torn down the structure with the above explicit call to
			// sol_oic_pending_cancel(), then this tests that the native callback performs the
			// already-torn-down check before attempting to tear down. If it doesn't, there will be
			// a double-free and this test should segfault.
			resource = null;
		}

		return !!resource;
	} );
