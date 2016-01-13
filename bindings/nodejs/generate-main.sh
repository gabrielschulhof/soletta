#!/bin/bash

# This file is part of the Soletta Project
#
# Copyright (C) 2015 Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#   * Neither the name of Intel Corporation nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

if test "x${V}x" != "xx"; then
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
