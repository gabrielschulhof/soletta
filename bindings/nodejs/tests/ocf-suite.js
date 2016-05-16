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
var suiteOptions = require( "yargs" ).argv;

require( "ocf-test-suite" )( _.extend( {
	location: require( "path" ).join( require( "bindings" ).getRoot( __filename ), "oic" ),
	environment: ( suiteOptions.ldPreload ? { LD_PRELOAD: suiteOptions.ldPreload } : {} )
}, suiteOptions.testList ? { tests: suiteOptions.testList.split( "," ) } : {} ) );
