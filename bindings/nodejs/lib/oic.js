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
var OicDevice = function OicDevice( role ) {
	if ( !this._isOicDevice ) {
		return new OicDevice( role );
	}
	this._construct( role );
};

_.extend( require( "./lib/utils" ).makeEventEmitter( OicDevice ).prototype, {
	_construct: function( role ) {
		Object.defineProperty( this, "device", {
			configurable: true,
			enumerable: true,
			get: _.bind( this._getDeviceInfo, this ),
			set: _.bind( this._setDeviceInfo, this )
		} );

		Object.defineProperty( this, "platform", {
			configurable: true,
			enumerable: true,
			get: _.bind( this._getPlatformInfo, this ),
			set: _.bind( this._setPlatformInfo, this )
		} );

		this._stopStack();
		this._startStack( role );
	},

	_isOicDevice: true,

	_getDeviceInfo: function() {
		return this._info.device;
	},

	_setDeviceInfo: function( info ) {

		// FIXME: Underlying API does not currently support this
	},

	_getPlatformInfo: function() {
		return this._info.platform;
	},

	_setPlatformInfo: function( info ) {

		// FIXME: Underlying API does not currently support this
	},

	_stopStack: function() {},

	_startStack: function( role ) {
		this._info = { device: { role: role }, platform: {} };

		// TODO: If launched in server mode, at this point register persistent storage handlers
	}
} );

// Extend OicDevice with client and server interfaces
require( "./lib/oic/client" )( OicDevice.prototype );
require( "./lib/oic/server" )( OicDevice.prototype );

module.exports = OicDevice;
