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

var fs = require( "fs" ),
	path = require( "path" ),
	configuration = require( "./configure-bindings.json" );

// List containing the names of source files for the bindings we wish to include.
// Paths are relative to the location of nodejs-bindings-sources.gyp (generated below).
var sources = configuration[ "" ].sources.slice( 0 );

// List containing the names of the header files in which to search for constants and enums
var headers = configuration[ "" ].headers.slice( 0 );

var oneVariable, match;
for ( oneVariable in process.env ) {

	// If it's an environment variable starting with SOL_CONFIG_ then examine its value
	match = oneVariable.match( /^SOL_CONFIG_(.*)$/ ) ? process.env[ oneVariable ] : null;

	// If the value is "y" then add files based on the name of the variable, removing the prefix
	match = match && ( match === "y" ) ? oneVariable.replace( /^SOL_CONFIG_/, "" ) : null;

	if ( match in configuration ) {
		sources = sources.concat( configuration[ match ].sources );
		headers = headers.concat( configuration[ match ].headers );
	}
}

fs.writeFileSync( path.join( __dirname, "generated", "header-files-list" ), headers.join( "\n" ) );

fs.writeFileSync( path.join( __dirname, "generated", "nodejs-bindings-sources.gyp" ),
	JSON.stringify( { sources: sources } ) );
