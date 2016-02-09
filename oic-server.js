var soletta = require( "./lowlevel" );

function teardown( error ) {
	console.error( error );
	process.exit( 1 );
}

if ( soletta.sol_oic_server_init() ) {
	teardown( "sol_oic_server_init() failed" );
}

function resourceRequest( methodName ) {
	return function resourceMethod( linkAddress, readPayload, writePayload ) {
		console.log( methodName + ": " + JSON.stringify( {
			linkAddress: linkAddres,
			readPayload: readPayload
		} ) );
	};
}

var theResource = soletta.sol_oic_server_add_resource( {
	interface : "oic.if.baseline",
	resource_type : "core.light",
	get: resourceRequest( "get" ),
	put: resourceRequest( "put" ),
	post: resourceRequest( "post" ),
	del: resourceRequest( "del" )
}, soletta.sol_oic_resource_flag.SOL_OIC_FLAG_DISCOVERABLE |
	soletta.sol_oic_resource_flag.SOL_OIC_FLAG_OBSERVABLE |
	soletta.sol_oic_resource_flag.SOL_OIC_FLAG_ACTIVE );

if (!theResource) {
	teardown( "sol_oic_server_add_resource() failed" );
} else {
	console.log( "Successfully created resource: " + JSON.stringify( theResource, null, 4 ) );
}
