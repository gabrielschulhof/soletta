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

var soletta = require( "bindings" )( "soletta" ),
	OicResource = require( "./resource" ),
	utils = require( "../utils" ),
	_ = require( "lodash" ),
	OicRequestEvent = function OicRequestEvent() {
		if ( !this._isOicRequestEvent ) {
			return new OicRequestEvent();
		}
		utils.setPrivate( this, [ "_resourceHandle", "_output", "_result" ] );
	},
	responseCodes = soletta.sol_coap_responsecode_t;

_.extend( OicRequestEvent.prototype, {
	_isOicRequestEvent: true,

	_constructResponse: function( entityHandlerResult, payloadObject ) {
		return new Promise( _.bind( function( fulfill, reject ) {
			this._result = entityHandlerResult;
			this._output = payloadObject;
			fulfill();
		}, this ) );
	},

	sendResponse: function( resource ) {
		return this._constructResponse( responseCodes.SOL_COAP_RSPCODE_OK,
			resource ? resource.properties : null );
	},

	sendError: function( error ) {
		return this._constructResponse( responseCodes.SOL_COAP_RSPCODE_INTERNAL_ERROR,
			_.extend( {},

				// Add a message if we can find one
				typeof error === "string" ? { message: error } :
					( util.isError( error ) && error.message ) ? { message: error.message } : {},

				// Add a representation if we can find one
				typeof error !== "string" ? { properties: _.extend( {}, error ) } : {} ) );
	}
} );

module.exports = function( devicePrototype ) {

_.extend( devicePrototype, {
	_construct: ( function( _super ) {
		return function() {

			// Define legacy "on*request" event handlers such that setting one will attach a
			// regular event handler
			utils.addLegacyEventHandler( this, "observerequest" );
			utils.addLegacyEventHandler( this, "unobserverequest" );
			utils.addLegacyEventHandler( this, "retrieverequest" );
			utils.addLegacyEventHandler( this, "deleterequest" );
			utils.addLegacyEventHandler( this, "createrequest" );
			utils.addLegacyEventHandler( this, "updaterequest" );
			return _super.apply( this, arguments );
		}
	} )( devicePrototype._construct ),

	_createHandler: function( method, resource ) {
		return _.bind( function requestHandler( clientAddress, input, output ) {
			var event = _.extend( new OicRequestEvent(), {
				type: method + "request",
				_output: {}
			} );

			this.dispatchEvent( event.type, event );

			// FIXME: This implies that the output is produced in a synchronous fashion.
			// There's currently no way around that :(
			_.extend( output, event._output );

			return event._error ?
				soletta.sol_coap_responsecode_t.SOL_COAP_INTERNAL_ERROR :
				soletta.sol_coap_responsecode_t.SOL_COAP_RSPCODE_OK;
		}, this );
	},

	register: function( init ) {
		return new Promise( _.bind( function( fulfill, reject ) {
			if ( this._info.device.role === "client" ) {
				return reject( new Error( "Not supported" ) );
			}

			var flags = soletta.sol_oic_resource_flag;

			// FIXME: Why does a resource need to be explicitly set to active in order for it to be
			// discoverable? iotivity also has this flag but it doesn't influence discoverability.
			// So, for interoperability, it is forced to be on.
			var resourceFlags = flags.SOL_OIC_FLAG_ACTIVE |
				( init.discoverable ? flags.SOL_OIC_FLAG_DISCOVERABLE : 0 ) |
				( init.observable ? flags.SOL_OIC_FLAG_OBSERVABLE : 0 ) |
				( init.secure ? flags.SOL_OIC_FLAG_SECURE : 0 ) |
				( init.slow ? flags.SOL_OIC_FLAGS_SLOW : 0 );

			if ( !init.id ) {
				return reject( new Error( "No ID found" ) );
			}

			if ( !init.id.deviceId ) {
				init.id.deviceId = soletta.sol_platform_get_machine_id()
					.match( /(.{8})(.{4})(.{4})(.{4})(.*)/ ).slice( 1 ).join( "-" );
			}

			var resource = new OicResource( init );

			var nativeResource = soletta.sol_oic_server_add_resource( {
				interface: init.interfaces[ 0 ],
				resource_type: init.resourceTypes[ 0 ],
				path: init.id.path,
				post: this._createHandler( "create", resource ),
				get: this._createHandler( "retrieve", resource ),
				put: this._createHandler( "update", resource ),
				del: this._createHandler( "delete", resource )
			}, resourceFlags );

			if ( !nativeResource ) {
				reject( new Error( "Failed to create native resource" ) );
				return;
			}

			resource._handle = nativeResource;

			fulfill( resource );
		}, this ) );
	},
	unregister: function( resource ) {
		return new Promise( _.bind( function( fulfill, reject ) {
			if ( !resource._handle ) {
				reject( new Error( "Native resource not found" ) );
			}
			soletta.sol_oic_server_del_resource( resource._handle );
			fulfill();
		}, this ) );
	},
	enablePresence: function( timeToLive ) {},
	disablePresence: function() {},
	notify: function( resource ) {
		return new Promise( _.bind( function( fulfill, reject ) {
			if ( !resource._handle ) {
				reject( new Error( "Native resource not found" ) );
			}
		}, this ) );
	},
} );

};
