var soletta = require( "./index" );

function hostnameChangeHandler( hostname ) {
	console.log( "hostname has changed. The new name is: " + hostname );
}

soletta.sol_platform_add_hostname_monitor( hostnameChangeHandler );

process.on( "SIGINT", function processSIGINT() {
	soletta.sol_platform_del_hostname_monitor( hostnameChangeHandler );
} );
