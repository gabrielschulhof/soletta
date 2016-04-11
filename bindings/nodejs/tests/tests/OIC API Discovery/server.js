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

var utils = require( "../../assert-to-console" ),
	oic = require( require( "path" )
		.join( require( "bindings" ).getRoot( __filename ), "oic" ) )( "server" ),
	uuid = process.argv[ 2 ];

console.log( JSON.stringify( { assertionCount: 3 } ) );

utils.assert( "ok", true, "Server: device configured successfully" );

oic.registerResource( {
	id: { path: "/a/" + uuid },
	resourceTypes: [ "core.light" ],
	interfaces: [ "oic.if.baseline" ],
	discoverable: true,
	active: true,
	properties: { someValue: 0 }
} ).then(
	function( resource ) {
		utils.assert( "ok", true, "Server: oic.registerResource() successful" );

		// Cleanup on SIGINT
		process.on( "SIGINT", function() {
			oic.unregisterResource( resource ).then(
				function() {
					utils.assert( "ok", true, "Server: oic.unregisterResource() successful" );
					process.exit( 0 );
				},
				function( error ) {
					utils.assert( "ok", false,
						"Server: oic.unregisterResource() failed with: " + error );
					process.exit( 0 );
				} );
		} );

		// Signal to the test suite that we're ready for the client
		console.log( JSON.stringify( { ready: true } ) );
	},
	function( error ) {
		utils.assert( "ok", false,
			"Server: oic.registerResource() failed with: " + error );
	} );
