var async = require( "async" );
var utils = require( "../assert-to-console" );
var soletta = require( require( "path" )
	.join( require( "../closestSoletta" )( __dirname ), "lowlevel" ) );
var exec = require( "child_process" ).exec;
var initialHostname = require( "os" ).hostname();

console.log( JSON.stringify( { assertionCount: 2 } ) );

delete process.env.LD_PRELOAD;

var hostnameMonitor, timezoneMonitor;

async.series( [
	function setupTimezone( callback ) {
		exec( "sudo timedatectl set-timezone Europe/Mariehamn", callback );
	},

	function runMonitorSequence( callback ) {
		var conditionsMet = 0;
		var maybeContinue = function( error ) {
			if ( error ) {
				callback( error );
			} else {
				conditionsMet++;
				if ( conditionsMet >= 3 ) {
					callback();
				}
			}
		};

		function timezoneChanged( newTimeZone ) {
			utils.assert( "strictEqual", newTimeZone, "Europe/Helsinki",
				"Time zone was set to the expected value" );

			soletta.sol_platform_del_timezone_monitor( timezoneMonitor );
			maybeContinue();
		}

		function hostnameChanged( newName ) {
			utils.assert( "strictEqual", newName, initialHostname,
				"Hostname was set to the expected value" );

			// It is essential to test that we synchronously remove one monitor and attach a second
			soletta.sol_platform_del_hostname_monitor( hostnameMonitor );
			timezoneMonitor = soletta.sol_platform_add_timezone_monitor( timezoneChanged );

			exec( "sudo timedatectl set-timezone Europe/Helsinki", maybeContinue );
		}

		hostnameMonitor = soletta.sol_platform_add_hostname_monitor( hostnameChanged );

		exec( "sudo hostname " + initialHostname, maybeContinue );
	}
], function( error ) {
	console.error( "Done with" + ( error ? "" : "out" ) + " error" );
	if ( error ) {
		utils.die( error.message );
	}
} );

