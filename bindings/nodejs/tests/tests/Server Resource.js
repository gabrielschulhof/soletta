var _ = require( "lodash" );
var testUtils = require( "../assert-to-console" );
var uuid = process.argv[ 2 ];
var cCalls = [];
var soletta = require( "../../../../lowlevel" );

function noop(){}

soletta._test_hook( function() {
	cCalls.push( Array.prototype.slice.call( arguments ) );
} );

console.log( JSON.stringify( { assertionCount: 3 } ) );

soletta.sol_oic_server_init();

var theResource = soletta.sol_oic_server_add_resource( {
	interface: "oic.if.baseline",
	resource_type: "core.ligh",
	path: "/a/" + uuid,
	get: noop,
	put: noop,
	post: noop,
	del: noop
}, soletta.sol_oic_resource_flag.SOL_OIC_FLAG_DISCOVERABLE );

testUtils.assert( "ok", !!theResource, "Resource was created" );

soletta.sol_oic_server_del_resource( theResource );

theResource = soletta.sol_oic_server_add_resource( {
	interface: "oic.if.baseline",
	resource_type: "core.ligh",
	path: "/a/" + uuid,
	get: noop,
	put: noop,
	post: noop,
	del: noop
}, soletta.sol_oic_resource_flag.SOL_OIC_FLAG_DISCOVERABLE );

testUtils.assert( "ok", !!theResource, "Resource was created again" );

cCalls = [];
theResource = null;
gc();
testUtils.assert( "deepEqual",
	_.filter( cCalls, function( value ) { return value[ 0 ] === "sol_oic_server_del_resource"; } ),
	[ [ "sol_oic_server_del_resource", true ], [ "sol_oic_server_del_resource", false ], ],
	"sol_oic_server_del_resource was called when the resource went out of scope" );
