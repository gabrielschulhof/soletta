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

console.log( JSON.stringify( { assertionCount: 0 } ) );

var resource = soletta.sol_oic_server_register_resource( {
	path: "/a/" + uuid,
	interface: "oic.if.baseline",
	resource_type: "core.light"
}, soletta.sol_oic_resource_flag.SOL_OIC_FLAG_DISCOVERABLE |
	soletta.sol_oic_resource_flag.SOL_OIC_FLAG_ACTIVE );

console.log( JSON.stringify( { ready: true } ) );

process.on( "SIGINT", function() {
	soletta.sol_oic_server_unregister_resource( resource );
} );
