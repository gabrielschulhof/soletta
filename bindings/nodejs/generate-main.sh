#!/bin/bash

if test "x${V}x" = "x1x"; then
	set -x
fi

cat generated/main.cc.prologue > generated/main.cc || exit 1
cat generated/main.h.prologue > generated/main.h || exit 1

# Add all the bound functions
find src -type f | while read; do
	cat "${REPLY}" | awk '{
		match( $0, /^NAN_METHOD\((bind_([^)]*))\)\s*{$/, nan_method );
		if ( nan_method[1] ) {
			print "NAN_METHOD(" nan_method[1] ");" >> "generated/main.h"
			print "  SET_FUNCTION(exports, " nan_method[2] ");" >> "generated/main.cc"
		}
	}'
done

# Add all the enums
includeDir="$(pwd)/depbuild/soletta/build/soletta_sysroot/usr/include/soletta"
find $includeDir -type f | grep '\.h$' | while read headerFile; do

	includeFile="$( basename "${headerFile}" )"

	# We skip these header files
	if test "x${includeFile}x" = "xsol-glib-integration.hx" -o \
			"x${includeFile}x" = "xsol-flow-static.hx" -o \
			"x${includeFile}x" = "xmagnetometer.hx" -o \
			"x${includeFile}x" = "xsol-iio.hx" -o \
			"x${includeFile}x" = "xgrove.hx" -o \
			"x${includeFile}x" = "xfilter-repeated.hx" -o \
			"x${includeFile}x" = "xtemperature.hx" -o \
			"x${includeFile}x" = "xpiezo-speaker.hx" -o \
			"x${includeFile}x" = "xnetwork.hx" -o \
			"x${includeFile}x" = "xprocess.hx" -o \
			"x${includeFile}x" = "xgyroscope.hx" -o \
			"x${includeFile}x" = "xboolean.hx" -o \
			"x${includeFile}x" = "xform.hx" -o \
			"x${includeFile}x" = "xbyte.hx" -o \
			"x${includeFile}x" = "xevdev.hx" -o \
			"x${includeFile}x" = "xiio.hx" -o \
			"x${includeFile}x" = "xmax31855.hx" -o \
			"x${includeFile}x" = "xunix-socket.hx" -o \
			"x${includeFile}x" = "xled-strip.hx" -o \
			"x${includeFile}x" = "xpwm.hx" -o \
			"x${includeFile}x" = "xpersistence.hx" -o \
			"x${includeFile}x" = "xrandom.hx" -o \
			"x${includeFile}x" = "xcolor.hx" -o \
			"x${includeFile}x" = "xwallclock.hx" -o \
			"x${includeFile}x" = "xsol-i2c.hx"; then
		continue
	fi

	includeFile="$( echo $headerFile | sed "s@^${includeDir}@@" )"
	includeFile="${includeFile##/}"

	echo "#include <${includeFile}>" >> "generated/main.h"
	cat $headerFile | awk -v 'insideEnum=0' -v 'addIncludeFile=0' '{
		if ( insideEnum == 0 ) {
			if ( match( $0, /^#define\s+([A-Za-z0-9_]+)\s+\([^)]*\)$/, constant_declaration ) ) {
				if ( constant_declaration[ 1 ] ) {
					addIncludeFile=1
					print "#ifdef " constant_declaration[ 1 ] >> "generated/main.cc"
					print "  SET_CONSTANT_NUMBER(exports, " constant_declaration[ 1 ] ");" >> "generated/main.cc"
					print "#endif /* " constant_declaration[ 1 ] " */"  >> "generated/main.cc"
				}
			}
			else
			if ( match( $0, /^enum\s*(\S*)\s*{$/, enum_starter ) ) {
				if ( enum_starter[ 1 ] ) {
					insideEnum = 1;
					addIncludeFile=1
					print "\n  SET_ENUM(exports, " enum_starter[ 1 ] ");" >> "generated/main.cc"
				}
			}
		} else {
			if ( $0 ~ /}/ ) {
				insideEnum = 0;
			} else {
				match( $0, /^\s*([A-Za_z0-9_]*)(\s|,|$)/, enum_value );
				if ( enum_value[ 1 ] ) {
					addIncludeFile=1
					print "  SET_CONSTANT_NUMBER(js_" enum_starter[ 1 ] ", " enum_value[ 1 ] ");" >> "generated/main.cc"
				}
			}
		}
	} END {
		if ( addIncludeFile == 1 ) {
			print "#include <'"${includeFile}"'>" >> "generated/main.h"
		} 
	}'
done

cat generated/main.cc.epilogue >> generated/main.cc || exit 1
cat generated/main.h.epilogue >> generated/main.h || exit 1
