var soletta = require( "./index" );

var hostnameChangeCount = 0;

function hostnameChangeHandler( hostname ) {
	console.log( "hostname has changed. The new name is: " + hostname );
	hostnameChangeCount++;
	if ( hostnameChangeCount >= 2 ) {
		setTimeout( function() {
			console.log( "enough hostname changes. Removing monitor" );
			soletta.sol_platform_del_hostname_monitor( hostnameChangeHandler );
			console.log( "enough hostname changes. Monitor removed" );
			hostnameChangeCount = 0;
		}, 1000 );
	}
}

soletta.sol_platform_add_hostname_monitor( hostnameChangeHandler );

var timezoneChangeCount = 0;

function timezoneChangeHandler( timezone ) {
	console.log( "timezone has changed. The new name is: " + timezone );
	timezoneChangeCount++;
	if ( timezoneChangeCount >= 2 ) {
		setTimeout( function() {
			console.log( "enough timezone changes. Removing monitor" );
			soletta.sol_platform_del_timezone_monitor( timezoneChangeHandler );
			console.log( "enough timezone changes. Monitor removed" );
			timezoneChangeCount = 0;
		}, 1000 );
	}
}

soletta.sol_platform_add_timezone_monitor( timezoneChangeHandler );

function localeChangeHandler( category, locale ) {
	var iterator, theCategory;
	for ( iterator in soletta.sol_platform_locale_category ) {
		if ( soletta.sol_platform_locale_category[ iterator ] === category ) {
			theCategory = iterator;
			break;
		}
	}
	console.log( "locale has changed. The new one is: " +
		JSON.stringify( { category: theCategory, locale: locale } ) );
}

soletta.sol_platform_add_locale_monitor( localeChangeHandler );
