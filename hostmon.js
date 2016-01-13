var soletta = require( "./index" );

function hostnameChangeHandler( hostname ) {
	console.log( "hostname has changed. The new name is: " + hostname );

	// This should cause node to quit but currently doesn't - there must be a libuv handle hanging
	// on somewhere
	soletta.sol_platform_del_hostname_monitor( hostnameChangeHandler );
}

soletta.sol_platform_add_hostname_monitor( hostnameChangeHandler );

function timezoneChangeHandler( timezone ) {
	console.log( "timezone has changed. The new name is: " + timezone );

	// This should cause node to quit but currently doesn't - there must be a libuv handle hanging
	// on somewhere
	soletta.sol_platform_del_timezone_monitor( timezoneChangeHandler );
}

soletta.sol_platform_add_timezone_monitor( timezoneChangeHandler );
