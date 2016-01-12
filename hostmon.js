var soletta = require( "./index" );

var monitorPresent = false,
	hostnameChangeCount = 0;

function hostnameChangeHandler( hostname ) {
	console.log( "hostname has changed. The new name is: " + hostname );
	hostnameChangeCount++;
	if ( hostnameChangeCount >= 2 ) {
		setTimeout( function() {
			console.log( "enough hostname changes. Removing monitor" );
			soletta.sol_platform_del_hostname_monitor( hostnameChangeHandler );
			monitorPresent = false;
			console.log( "enough hostname changes. Monitor removed" );
			hostnameChangeCount = 0;
		}, 1000 );
	}
}

soletta.sol_platform_add_hostname_monitor( hostnameChangeHandler );
monitorPresent = true;
