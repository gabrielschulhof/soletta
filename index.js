// Export low-level bindings for now
module.exports = function loadSolettaHighLevelAPI( theAPI ) {
	return require( require( "path" )
		.join( __dirname, require( "./bindings-path" ).nodejs, theAPI ) );
};
