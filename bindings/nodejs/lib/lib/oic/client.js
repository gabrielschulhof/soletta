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
			utils.setPrivate( this, [ "_resolver", "_client", "_resources" ] );
			this._resources = {};
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

			// Call the callback if the resource matches the filter or if there is no filter
			if ( !resourcePath || resource.href === resourcePath ) {
				resourceCallback( resource );
			}

			return true;
		}, this );
	},

	_doDiscovery: function( options, resourceCallback ) {
		options = options || {};
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
			var resource = new OicResource( {
				id: {
					deviceId: resource.device_id,
					path: resource.href
				},
				resourceTypes: resource.types,
				interfaces: resource.interfaces,
				_handle: resource
			} );

			this._resolver.update( resource.device_id, resource.addr );
			this._resources[ resource.id.deviceId + ":" + resource.id.path ] = resource;

			this.dispatchEvent( "resourcefound", {
				type: "resourcefound",
				resource: resource
			} );
		}, this ) );
	},

	getDeviceInfo: function( id ) {
	},

	_oneShotRequest: function( options ) {
		return new Promise( _.bind( function( fulfill, reject ) {
			var resource = options.resource ||
				( options.id ?
					this._resources[ options.id.deviceId + ":" + options.id.path ] : null );
			var id = resource ? resource.id : options.id;

			if ( !resource ) {
				reject( _.extend( new Error( "Unable to find resource" ), { id : id } ) );
				return;
			}

			if ( !resource._handle ) {
				reject( _.extend( new Error( "Unable to find native resource",  { id : id } ) ) );
				return;
			}

			if ( !this._client ) {
				reject( new Error( "Native OIC client not found" ) );
				return;
			}

			if ( !soletta.sol_oic_client_resource_request( this._client, resource._handle,
					options.method, options.payload || null,
					function responseToRequest( responseCode, client, address, payload ) {
						if ( responseCode ===
							soletta.sol_coap_responsecode_t.SOL_COAP_RSPCODE_OK ) {
							if ( options.createAnswer ) {
								options.createAnswer( resource, payload );
							}
							fulfill();
							return;
						}

						reject( _.extend( new Error( "Request failed" ), {
							result: responseCode
						} ) );
					} ) ) {
				reject( new Error( "Request failed" ) );
				return;
			}
		}, this ) );
	},

	create: function( resource ) {},
	retrieve: function( id ) {
		return this._oneShotRequest( {
			id: id,
			method: soletta.sol_coap_method_t.SOL_COAP_METHOD_GET,
			createAnswer: function createGetAnswer( resource, payload ) {
				_.extend( resource.properties, payload );
			}
		} );
	},
	update: function( resource ) {
		return this._oneShotRequest( {
			resource: resource,
			method: soletta.sol_coap_method_t.SOL_COAP_METHOD_PUT,
			payload: resource.properties
		} );
	},
	delete: function( id ) {}
} );

};
