var x = function( newHostName ) { console.log( newHostName ); }
require( "./lowlevel" ).sol_platform_add_hostname_monitor( x );
x = null;
if ( typeof gc === "undefined" ) {
	console.log( "gc() not defined" );
} else {
	console.log( "Calling gc()" );
	gc();
}
