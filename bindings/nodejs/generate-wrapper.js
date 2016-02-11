function generateSingleWrapper( theSignature ) {
	var nameAndReturnType = theSignature.split( "(" )[ 0 ].split( " " );
	var returnTypePointer = nameAndReturnType[ nameAndReturnType.length - 1 ]
		.replace( /[^*]/g, "" );
	var theFunctionName = nameAndReturnType[ nameAndReturnType.length - 1 ].replace( "*", "" );
	var theReturnType = nameAndReturnType.slice( 0, -1 ).join( " " ) + " " + returnTypePointer;
	var isVoid = ( nameAndReturnType[ nameAndReturnType.length - 2 ] === "void" );
	var functionCall = "";
	var theWrapper = "";
	var parameters = ( function( parameters ) {

		// Special case: no parameters when the function is declared as
		//     returnType functionName(void);
		if ( parameters.length == 1 && parameters[ 0 ] === "void" ) {
			return [];
		}

		// Establish the real parameters from the naively parsed ones that were passed in, making
		// sure that they have names
		var realParameters = [];
		var parametersLength = parameters.length;
		var theIndex, bracketBalance, bracketBuffer, brackets, theName, theMatch;
		var modifyBracketBalance = function( value ) {
			bracketBalance += ( ( value == "(" ) ? 1 : -1 );
		};
		for ( theIndex = 0, bracketBalance = 0, bracketBuffer = "";
				theIndex < parametersLength;
				theIndex++ ) {
			if ( bracketBalance != 0 ) {
				bracketBuffer += "," + parameters[ theIndex ];
			}

			// If the parameter has brackets, count 'em and buffer until they balance
			if ( parameters[ theIndex ].match( /[()]/ ) ) {
				parameters[ theIndex ]
					.replace( /[^()]/g, "" )
					.split( "" )
					.forEach( modifyBracketBalance );
				if ( bracketBalance == 0 ) {
					parameters[ theIndex ] = bracketBuffer;
				} else {
					bracketBuffer += ( bracketBuffer == "" ? "" : "," ) + parameters[ theIndex ];
				}
			}
			if ( bracketBalance == 0 ) {
				realParameters.push( parameters[ theIndex ].trim() );
				bracketBuffer = "";
			}
		}

		parametersLength = realParameters.length;
		for( theIndex = 0, theName = "param0";
				theIndex < parametersLength ;
				theIndex++, theName = "param" + theIndex ) {

			// Make sure the function pointer parameter has a name
			theMatch = realParameters[ theIndex ].match( /(^[^(]*[(])(\s*[*]\s*)([^)]*)(.*$)/ );
			if ( theMatch && theMatch.length > 4 ) {
				if ( theMatch[ 3 ] ) {
					theName = theMatch[ 3 ];
				} else {
					realParameters[ theIndex ] =
						theMatch[ 1 ] + theMatch[ 2 ] + " " + theName + " " + theMatch[ 4 ];
				}
			} else {

				// Make sure the non-function pointer parameter has a name
				theMatch = realParameters[ theIndex ]
					.match(/((const)?\s*(struct|enum)?\s*[^\s]*\s*[*]?)(.*$)/);
				if ( theMatch && theMatch.length > 1 ) {
					if ( theMatch[ theMatch.length - 1 ] ) {
						theName = theMatch[ theMatch.length - 1 ];
					} else {
						realParameters[ theIndex ] =
							theMatch[ 1 ] + " " + theName;
					}
				}
			}

			realParameters[ theIndex ] = {
				definition: realParameters[ theIndex ],
				theName: theName
			};
		}

		return realParameters;
	} )( theSignature
		.substring(
			theSignature.indexOf( "(" ) + 1,
			theSignature.lastIndexOf( ")" ) )
		.split( "," ) );
	var formalParameters = parameters.length > 0 ?
		parameters.map( function( value ) {
				return value.definition;
			} ).join( ", " ) :
		"void";

	// Having established the various bits of the signature, let's generate the wrapper code
	theWrapper =

		// Declare the wrapper for the original symbol. Its name and signature is identical to the
		// original symbol.
		theReturnType + theFunctionName + "(\n" +
		"        " + formalParameters + ") {\n" +

		// Call the wrapper prologue
		"    wrapper_prologue(\"" + theFunctionName + "\");\n" +

		// Retrieve the original symbol and assign it to a variable, casting it to the symbol's
		// signature to avoid compile-time warnings.
		"    " + theReturnType + "(*orig_" + theFunctionName + ")(" +
			formalParameters + ") = (" + theReturnType.trim() + "(*)(" + formalParameters + "))" +
			"dlsym(get_handle(), \"" + theFunctionName + "\");\n" +

		// If retrieval of the original symbol fails for some reason, abort
		"    if (!orig_" + theFunctionName + ") {\n" +
		"        fprintf(stderr, \"Failed to wrap function \\\"%s\\\": %p\\n\", " +
					"\"" + theFunctionName + "\", orig_" + theFunctionName + ");\n" +
		"        abort();\n" +
		"    }\n";

	functionCall = "orig_" + theFunctionName + "(" +
		parameters
			.map( function( value ) {
				return value.theName;
			} )
			.join( ", " ) + ");";

	theWrapper += ( isVoid ?

		// If the original symbol returns void, we simply call it
		( "    " + functionCall + "\n" ) :

		// If it returns a value, we declare a variable to hold the return value and assign it
		// with the function call.
		// FIXME: Address the case where the return type contains things like extern
		( "    " + theReturnType +
			"returnValue = " + functionCall ) ) + "\n" +

		// Call the wrapper prologue
		"    wrapper_epilogue(\"" + theFunctionName + "\");\n" +

		// If the function returns a value, return the variable we declared earlier wherein we
		// stored the return value of the original symbol.
		( isVoid ? "" : "    return returnValue;" ) + "\n}"

	console.log( theWrapper );
}

var theSignature = "";
var inSignature = false;
function processLine( theLine ) {
	if ( theLine.match( /^\S/ ) &&
			theLine.match( /[(]/ ) &&
			!theLine.match( /^#define/ ) ) {
		inSignature = true;
	}
	if ( inSignature ) {
		if ( theLine.match( /;/ ) ) {
			inSignature = false;
		}
		theLine = theLine.replace( /;/g, "" ).trim();
		theSignature = !theSignature ? theLine :
			theSignature + " " + theLine;
		if ( !inSignature ) {
			generateSingleWrapper( theSignature );

			// Prepare variables for next function
			theSignature = "";
		}
	}
}

var carryOver = "";
process.stdin.on( "data", function( chunk ) {
	var lines = chunk.toString( "utf8" ).split(/[\n\r]/);
	var theirLength = lines.length;
	var lineIndex;

	for ( lineIndex = 0; lineIndex < theirLength ; lineIndex++ ) {
		if ( lineIndex == 0 ) {
			processLine( carryOver + lines[ 0 ] );
			carryOver = "";
		} else if ( lineIndex == theirLength - 1 ) {
			carryOver = lines[ lineIndex ];
		} else {
			processLine( lines[ lineIndex ] );
		}
	}
} );
