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

console.log( JSON.stringify( { assertionCount: 1 } ) );

var client = soletta.sol_oic_client_new();

var findResourcesHandle = soletta.sol_oic_client_find_resources( client,
	soletta.sol_network_link_addr_from_str( {
		bytes: _.fill( Array( 16 ), 0 ),
		family: soletta.sol_network_family.SOL_NETWORK_FAMILY_INET,
		port: 5683
	}, "224.0.1.187" ), "", "",
	function( client, resource ) {
		if ( resource && resource.path === "/a/" + uuid ) {
			soletta.sol_oic_client_get_server_info( client, resource,
				function( client, serverInfo ) {
					utils.assert( "strictEqual", serverInfo.device_id,
						soletta.sol_platform_get_machine_id()
							.match( /(.{8})(.{4})(.{4})(.{4})(.*)/ ).slice( 1 ).join( "-" ),
						"Client: Device ID is as expected" );
					console.log( JSON.stringify( { killPeer: true } ) );
				} );

			resource = null;
		}

		return !!resource;
	} );
