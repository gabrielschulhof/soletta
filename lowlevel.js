// Return the native bindings, but not before providing them with an async bridge
var soletta = require( "bindings" )( "soletta" )._async_bridge_init( {
	events: {},
	_getEvent: function( event ) {
		var theEvent = this.events[ event ];
		if ( !theEvent ) {
			theEvent = this.events[ event ] = [];
		}
		return theEvent;
	},
	listenerCount: function( event ) {
		this._getEvent( event ).count;
	},
	add: function( event, callback ) {
		this._getEvent( event ).push( callback );
	},
	del: function( event, callback ) {
		if ( arguments.length === 1 ) {
			delete this.events[ event ];
			return;
		}

		var theIndex, theLength,
			callbackFound = false,
			theEvent = this._getEvent( event );

		theLength = theEvent.length;
		for ( theIndex = 0 ; theIndex < theLength ; theIndex++ ) {
			if ( theEvent[ theIndex ] == callback ) {
				theEvent.splice( theIndex, 1 );
				callbackFound = true;
				break;
			}
		}

		if ( !callbackFound ) {
			throw new Error( event + ": del: Callback not found" );
		}

		if ( theEvent.length === 0 ) {
			delete this.events[ event ];
		}
	},
	call: function() {
		var theIndex, theLength,

			// Pop the event name off the front of the arguments and use it to retrieve the
			// array of callbacks. For that we first need to convert arguments to a real array.
			theArguments = (arguments.length === 1 ?
				[ arguments[ 0 ] ] : Array.apply( null, arguments ) );
			theEvent = this._getEvent( theArguments.shift() ),
			returnValue = [];

		theLength = theEvent.length;
		for ( theIndex = 0 ; theIndex < theLength ; theIndex++ ) {
			returnValue.push( theEvent[ theIndex ].apply( this, theArguments ) );
		}

		return returnValue;
	}
} );
module.exports = soletta;
