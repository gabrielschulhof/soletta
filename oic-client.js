if ( process.argv.length < 3 ) {
	console.error( "Usage: " + process.argv[ 0 ] + " " + process.argv[ 1 ] + " <resource path>" );
	process.exit( 1 );
}

var soletta = require( "./index" ),
	_ = require( "lodash" );

var destination = soletta.sol_network_addr_from_str( {
		bytes: _.fill( Array( 16 ), 0 ),
		port: 5683,
		family: soletta.sol_network_family.SOL_NETWORK_FAMILY_INET
	}, "224.0.1.187" );

if ( !soletta.sol_oic_client_find_resource( destination, "", function( resource ) {
	console.log( "resource: " + JSON.stringify( resource, null, 4 ) );

	if ( resource && resource.href === process.argv[ 2 ] ) {
		soletta.sol_oic_client_resource_set_observable( resource,
			( function( resource ) {
				var observations = 0;
				var returnValue = function( responseCode, address, representation ) {
					observations++;
					console.log( "resource observation: " +
						JSON.stringify( {
							responseCode: responseCode,
							address: address,
							representation: representation
						}, null, 4 ) );
					if ( observations >= 10 ) {
						console.log( "resource: Enough observations. Removing observe request" );
						soletta.sol_oic_client_resource_set_observable( resource, returnValue,
							false );
					}
				};
				return returnValue;
			} )( resource ), true );

		soletta.sol_oic_client_get_platform_info(resource, function( info ) {
			console.log( "platform info: " + JSON.stringify( info, null, 4 ) );
		} );
		soletta.sol_oic_client_get_server_info(resource, function( info ) {
			console.log( "server info: " + JSON.stringify( info, null, 4 ) );
		} );

		return false;
	}
	return true;
} ) ) {
	console.error( "Failed to execute discovery!" );
}
