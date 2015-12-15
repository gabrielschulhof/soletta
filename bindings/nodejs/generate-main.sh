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

cat generated/main.cc.epilogue >> generated/main.cc || exit 1
cat generated/main.h.epilogue >> generated/main.h || exit 1
