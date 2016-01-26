var soletta = require( "./index" ),
	_ = require( "lodash" );

var destination = soletta.sol_network_addr_from_str( {
		bytes: _.fill( Array( 16 ), 0 ),
		port: 5683,
		family: soletta.sol_network_family.SOL_NETWORK_FAMILY_INET
	}, "224.0.1.187" );

if ( !soletta.sol_oic_client_find_resource( destination, "", function( resource ) {
	console.log( "resource: " + JSON.stringify( resource, null, 4 ) );
	if ( resource ) {
		soletta.sol_oic_client_get_platform_info(resource, function( info ) {
			console.log( "platform info: " + JSON.stringify( info, null, 4 ) );
		} );
		soletta.sol_oic_client_get_server_info(resource, function( info ) {
			console.log( "server info: " + JSON.stringify( info, null, 4 ) );
		} );
	}
	return !!resource;
} ) ) {
	console.error( "Failed to execute discovery!" );
}
