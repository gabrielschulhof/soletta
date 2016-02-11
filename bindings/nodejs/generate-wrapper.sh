#!/bin/sh

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

if test "$#" -lt 2; then
	echo "Usage: $(basename $0) <libpath> <include_dir_1>[ ... <include_dir_n>]"
	exit 1
fi

if test "x${V}x" != "xx"; then
	set -x
fi

FILES='
sol-platform.h
sol-oic-client.h
sol-oic-server.h
'

# Truncate the generated source file
echo -n '' > bindings/nodejs/generated/libsoletta_wrapper.c

echo 'const char *global_libpath = "'"${1}"'";' >> bindings/nodejs/generated/libsoletta_wrapper.c
shift 1

cat bindings/nodejs/generated/libsoletta_wrapper.c.prologue >> bindings/nodejs/generated/libsoletta_wrapper.c

for file in $FILES; do
	for path in "$@"; do
		CANDIDATE="${path}/${file}"
		if test -f "${CANDIDATE}"; then
			echo "#include <${file}>" >> bindings/nodejs/generated/libsoletta_wrapper.c
			cat "${CANDIDATE}" | node \
				bindings/nodejs/generate-wrapper.js \
				>> bindings/nodejs/generated/libsoletta_wrapper.c
		fi
	done
done
