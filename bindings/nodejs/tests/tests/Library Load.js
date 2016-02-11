var _ = require( "lodash" );
var testUtils = require( "../assert-to-console" );
var theError = null;
var soletta = null;
var expectedCalls = [];

console.log( JSON.stringify( { assertionCount: 2 } ) );

try {
	soletta = require( "../../../../lowlevel" );
} catch( anError ) {
	theError = anError;
}

testUtils.assert( "deepEqual", theError, null,
	"No error was thrown when loading the library and calling an API" );

if ( soletta ) {
	soletta._test_hook( function() {
		expectedCalls.push( Array.prototype.slice.call( arguments ) );
	} );
	soletta.sol_platform_get_machine_id();
	testUtils.assert( "deepEqual",
		_.filter( expectedCalls, function( value ) {
			return value[ 0 ] === "sol_platform_get_machine_id";
		} ),
		[
			[ "sol_platform_get_machine_id", true ],
			[ "sol_platform_get_machine_id", false ]
		],
		"sol_platform_get_machine_id was called exactly once" );
}
