var soletta = require( "../../../lowlevel" );

function timezoneMonitor( timezone ) {
	console.log( "timezone has changed: " + timezone );
	console.log( "Removing timezone monitor" );
	soletta.sol_platform_del_timezone_monitor( timezoneMonitor );
	console.log( "Removed timezone monitor" );
}

function hostnameMonitor( hostname ) {
	console.log( "hostname has changed: " + hostname );
	console.log( "Removing hostname monitor" );
	soletta.sol_platform_del_hostname_monitor( hostnameMonitor );
	console.log( "Removed hostname monitor" );
	console.log( "Adding timezone monitor" );
	soletta.sol_platform_add_timezone_monitor( timezoneMonitor );
	console.log( "Added timezone monitor" );
}

console.log( "Adding hostname monitor" );
soletta.sol_platform_add_hostname_monitor( hostnameMonitor );
console.log( "Added hostname monitor" );
