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
var utils = require( "../assert-to-console" );
var theError = null;

console.log( JSON.stringify( { assertionCount: 4 } ) );

var client = soletta.sol_oic_client_new();

utils.assert( "ok", !!client, "Creating a new client was successful" );

try {
	soletta.sol_oic_client_del( client );
} catch ( anError ) {
	theError = anError;
}

utils.assert( "strictEqual", theError, null, "Deleting a client was successful" );

theError = null;
try {
	soletta.sol_oic_client_del( client );
} catch ( anError ) {
	theError = anError;
}

utils.assert( "strictEqual", !!theError, true,
	"An error has occurred when attempting to delete a client twice" );
utils.assert( "strictEqual", ( "" + theError ),
	"TypeError: Object is not of type SolOicClient",
	"The error message has the expected value when attempting to delete a client twice" );
