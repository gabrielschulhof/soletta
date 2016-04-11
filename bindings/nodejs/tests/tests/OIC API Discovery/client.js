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
		.join( require( "bindings" ).getRoot( __filename ), "oic" ) )( "client" ),
	uuid = process.argv[ 2 ];

console.log( JSON.stringify( { assertionCount: 1 } ) );

new Promise( function discoverTheResource( fulfill, reject ) {
	var teardown;
	var resourcefound = function( event ) {
		teardown( null );
	};
	teardown = function( error ) {
		oic.removeEventListener( "resourcefound", resourcefound );
		if ( error ) {
			reject( error );
		} else {
			fulfill();
		}
	}
	oic.addEventListener( "resourcefound", resourcefound );
	oic.findResources( { resourcePath: "/a/" + uuid } ).catch( teardown );
} ).then(
	function testIsDone() {
		utils.assert( "ok", true, "Client: Resource found" );
		console.log( JSON.stringify( { killPeer: true } ) );
		process.exit( 0 );
	},
	function catchError( theError ) {
		utils.die( "Client failed with error " +
			theError.message + " and result " + theError.result );
	} );
