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
	utils = require( "../utils" ),
	soletta = require( "bindings" )( "soletta" ),
	OicResource = function OicResource( init ) {
		if ( !this._isOicResource ) {
			return new OicResource( init );
		}

		// Will not create an object without a deviceId and path
		if ( !( init.id && init.id.deviceId && init.id.path ) ) {
			throw new Error( "Constructing OicResource: malformed id" );
		}

		utils.setPrivate( this, [
			"_isOicResource", "_events", "_eventsCount", "_address", "_handle"
		] );

		// Copy values from the initializer
		if ( init ) {
			_.extend( this,

				// The resource will have a "properties" key, even if it has no actual properties
				{ properties: {} },
				init );
		}

		this.on( "newListener", _.bind( function( event ) {
			var result;

			if ( !( event === "change" && utils.listenerCount( this, event ) === 0 ) ) {
				return;
			}

			// Start observing this resource
			result = soletta.sol_oic_client_resource_observe();
/*
			result = iotivity.OCDoResource( observationHandleReceptacle,
				iotivity.OCMethod.OC_REST_OBSERVE,
				this.id.path,
				this._address, null,
				iotivity.OCConnectivityType.CT_DEFAULT,
				iotivity.OCQualityOfService.OC_HIGH_QOS,
				_.bind( function( handle, response ) {
					if ( response.payload ) {
						_.extend( this.properties,
							utils.payloadToObject( response.payload.values ) );
					}
					this.dispatchEvent( "change", {
						type: "change",
						resource: this
					} );
					return iotivity.OCStackApplicationResult.OC_STACK_KEEP_TRANSACTION;
				}, this ),
				null, 0 );
*/

			if ( !result ) {
				throw _.extend( new Error(
					"OicResource: Failed to set up observation using OCDoResource" ), {
					result: result
				} );
			}
		}, this ) );
		this.on( "removeListener", _.bind( function( event ) {
			var result;

			if ( !( event === "change" && this._observationHandle &&
					utils.listenerCount( this, event ) === 0 ) ) {
				return;
			}

			result = soletta.sol_oic_client_resource_unobserve();
/*
			result = iotivity.OCCancel( this._observationHandle,
				iotivity.OCQualityOfService.OC_HIGH_QOS,
				null, 0 );
*/
			if ( !result ) {
				throw _.extend( new Error(
					"OicResource: Failed to cancel observation using OCDoResource" ), {
					result: result
				} );
			}
		}, this ) );

		// Define property "onresourceupdate" such that writing to it will result in handlers being
		// added to/removed from the "resourceupdate" event
		utils.addLegacyEventHandler( this, "change" );
	};

_.extend( utils.makeEventEmitter( OicResource ).prototype, {
	_isOicResource: true
} );

module.exports = OicResource;
