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

var soletta = require( require( "path" )
	.join( require( "bindings" ).getRoot( __filename ), "lowlevel" ) );
var utils = require( "../../assert-to-console" );
var uuid = process.argv[ 2 ];

console.log( JSON.stringify( { assertionCount: 1 } ) );

var resource = soletta.sol_oic_server_register_resource( {
	get: function(request) {
		var response = soletta.sol_oic_server_response_new( request );

		utils.assert( "ok", !!response, "Server: Response successfully created" );

		_.extend( response, {
			booleanValue: true,
			floatingValue: 1.79,
			integerValue: 392,
			negativeValue: -211
			textStringValue: "Ceci n'est pas une pipe",
			byteStringValue: [ -75, 19, 125, -2, 0, 5 ]
		} );

		var result = soletta.sol_oic_server_send_response( request, response );
		utils.assert( "strictEqual", result, 0, "Server: Response successfully sent" );

		return 0;
	},
	path: "/a/" + uuid,
	interface: "oic.if.baseline",
	resource_type: "core.light"
}, soletta.sol_oic_resource_flag.SOL_OIC_FLAG_DISCOVERABLE |
	soletta.sol_oic_resource_flag.SOL_OIC_FLAG_ACTIVE );

utils.assert( "ok", !!resource, "Server: Resource created successfully" );

console.log( JSON.stringify( { ready: true } ) );

process.on( "SIGINT", function() {
	soletta.sol_oic_server_unregister_resource( resource );
} );
