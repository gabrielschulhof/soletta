var x = function( newHostname ) {
	console.log( newHostname );
	y.sol_platform_del_hostname_monitor( x );
}
var y = require( "./lowlevel" );
y.sol_platform_add_hostname_monitor( x );
