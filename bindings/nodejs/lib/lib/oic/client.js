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
	soletta = require( "bindings" )( "soletta" ),
	OicResolver = require( "./resolver" ),
	OicResource = require( "./resource" ),
	utils = require( "../utils" );

module.exports = function( devicePrototype ) {

_.extend( devicePrototype, {
	_construct: ( function( _super ) {
		return function() {

			// Define legacy event handlers such that setting them will attach event handlers
			utils.addLegacyEventHandler( this, "resourcefound" );
			utils.addLegacyEventHandler( this, "devicefound" );
			utils.addLegacyEventHandler( this, "discoveryerror" );
			utils.setPrivate( this, [ "_resolver", "_client" ] );
			return _super.apply( this, arguments );
		};
	} )( devicePrototype._construct ),

	_startStack: ( function( _super ) {
		return function( role ) {
			_super.apply( this, arguments );

			if ( role === "server" ) {
				return;
			}

			this._client = soletta.sol_oic_client_new();
			if ( !this._client ) {
				throw new Error( "Failed to instantiate OIC client" );
			}

			this._resolver = _.extend( new OicResolver(), {
				defaultAddress: soletta.sol_network_link_addr_from_str( {
					bytes: _.fill( Array( 16 ), 0 ),
					family: soletta.sol_network_family.SOL_NETWORK_FAMILY_INET,
					port: 5683
				}, "224.0.1.187" )
			} );
		};
	} )( devicePrototype._startStack ),

	// Cancel any and all handles upon stopping the stack
	_stopStack: ( function( _super ) {
		return function() {
			if ( this._client ) {
				soletta.sol_oic_client_del( this._client );
			}
			return _super.apply( arguments );
		};
	} )( devicePrototype._stopStack ),

	_discoveryCallback: function( resourcePath, resourceCallback ) {
		return _.bind( function( client, resource ) {

			// If discovery is over
			if ( !resource ) {
				return false;
			}

			this._resolver.update( resource.device_id, resource.addr );

			// If the resource does not match the filter
			if ( resourcePath && resource.href !== resourcePath ) {
				return true;
			}

			resourceCallback( resource );
		}, this );
	},

	_doDiscovery: function( options, resourceCallback ) {
		return new Promise( _.bind( function( fulfill, reject ) {
			var destination = options.deviceId ? this._resolver.resolve( options.deviceId ) :
				this._resolver.defaultAddress;

			if ( !destination ) {
				reject( new Error( "findResources: Failed to determine destination" ) );
				return;
			}

			if ( !soletta.sol_oic_client_find_resource( this._client, destination,
					( typeof options.resourceType === "string" ? options.resourceType : "" ), "",
					this._discoveryCallback( options.resourcePath, resourceCallback ) ) ) {
				reject( new Error( "findResources: Failed to initiate discovery request" ) );
				return;
			}

			fulfill();
		}, this ) );
	},

	findDevices: function() {
		return this._doDiscovery( { resourceType: "oic.wk.d" }, _.bind( function( resource ) {
			this.dispatchEvent( "devicefound", {
				type: "devicefound",
				device: { uuid: resource.device_id }
			} );
		}, this ) );
	},

	findResources: function( options ) {
		return this._doDiscovery( options, _.bind( function( resource ) {
			this.dispatchEvent( "resourcefound", {
				type: "resourcefound",
				resource: new OicResource( {
					id: {
						deviceId: resource.device_id,
						path: resource.href
					},
					resourceTypes: resource.types,
					interfaces: resource.interfaces
				} )
			} );
		}, this ) );
	},

	getDeviceInfo: function( id ) {
	},

	create: function( resource ) {},
	retrieve: function( id ) {},
	update: function( resource ) {},
	delete: function( id ) {}
} );

};
