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

var _ = require( "lodash" ),
	util = require( "util" ),
	EventEmitter = require( "events" ).EventEmitter;

module.exports = {
	setPrivate: function( object, props ) {
		var index;

		for ( index in props ) {
			Object.defineProperty( object, props[ index ], {
				writable: true,
				configurable: true
			} );
		}
	},

	// Alias "on<event>" properties to (add|remove)Listener
	addLegacyEventHandler: function( object, name ) {
		var handler;
		Object.defineProperty( object, "on" + name, {
			configurable: true,
			enumerable: true,
			get: function() {
				return handler;
			},
			set: function( newValue ) {
				if ( handler ) {
					object.removeListener( name, handler );
					handler = undefined;
				}
				if ( newValue ) {
					object.addListener( name, newValue );
					handler = newValue;
				}
			}
		} );
	},

	// Create a version of listenerCount() that works in 0.12 as well
	listenerCount: function( emitter, event ) {
		return ( ( typeof emitter.listenerCount === "undefined" ) ?
			EventEmitter.listenerCount( emitter, event ) :
			emitter.listenerCount( event ) );
	},

	makeEventEmitter: function( theObject ) {
		util.inherits( theObject, EventEmitter );
		_.extend( theObject.prototype, {
			addEventListener: theObject.prototype.addListener,
			removeEventListener: theObject.prototype.removeListener,
			dispatchEvent: theObject.prototype.emit
		} );
		return theObject;
	}
};
