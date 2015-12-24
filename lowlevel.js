var dlfcn = require( "bindings" )( "dlfcn" ),
	path = require( "path" ),

	// We know that soletta may not be available in the default library search path, so we try to
	// load it manually before loading the bindings, to ensure that loading the bindings does not
	// fail for lack of the soletta libaray.
	solettaHandle = dlfcn.dlopen(
		path.join( __dirname, "build/soletta_sysroot/usr/lib/libsoletta.so" ),
		dlfcn.RTLD_NOW | dlfcn.RTLD_GLOBAL );

process.on( "exit", function() {
	if ( solettaHandle ) {
		dlfcn.dlclose( solettaHandle );
	}
} );

module.exports = require( "bindings" )( "soletta" );
