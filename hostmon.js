var soletta = require( "./index" );

function hostnameChangeHandler( hostname ) {
	console.log( "hostname has changed. The new name is: " + hostname );

	// This should cause node to quit but currently doesn't - there must be a libuv handle hanging
	// on somewhere
//	soletta.sol_platform_del_hostname_monitor( hostnameChangeHandler );
}

soletta.sol_platform_add_hostname_monitor( hostnameChangeHandler );

function timezoneChangeHandler( timezone ) {
	console.log( "timezone has changed. The new name is: " + timezone );

	// This should cause node to quit but currently doesn't - there must be a libuv handle hanging
	// on somewhere
	soletta.sol_platform_del_timezone_monitor( timezoneChangeHandler );
}

soletta.sol_platform_add_timezone_monitor( timezoneChangeHandler );

function localeChangeHandler( category, locale ) {
	var keyName, iterator;

	for ( iterator in soletta.sol_platform_locale_category ) {
		if ( soletta.sol_platform_locale_category[ iterator ] === category ) {
			keyName = iterator;
			break;
		}
	}

	console.log( "locale has changed. The new locale is: " +
		JSON.stringify( { category: keyName + " (" + category + ")", locale: locale } ) );

	localeChangeHandler.count = ( localeChangeHandler.count ? localeChangeHandler.count : 0 ) + 1;
	if ( localeChangeHandler.count === 17 ) {
		soletta.sol_platform_del_locale_monitor( localeChangeHandler );
	}
}

soletta.sol_platform_add_locale_monitor( localeChangeHandler );
