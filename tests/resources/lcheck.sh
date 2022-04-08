#! /bin/bash
#
# Copyright (c) 2022 [Ribose Inc](https://www.ribose.com).
# All rights reserved.
# This file is a part of tebako
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

set -o errexit -o pipefail -o noclobber -o nounset

# Checks referenced shared libraries
# $1 - an array of expected refs to shared libraries
# $2 - an array of actual refs to shared libraries
check_shared_libs() {
    expected_size="${#expected[@]}"
    actual_size="${#actual[@]}"
    assertEquals "The number of references to shared libraries does not meet our expectations" "$expected_size" "$actual_size"

    for exp in "${expected[@]}"; do
        for i in "${!actual[@]}"; do
            if [[ "${actual[i]}" == *"$exp"* ]]; then
                unset 'actual[i]'
            fi
        done
    done

    for unexp in "${actual[@]}"; do
        echo "Unxpected reference to shared library $unexp"
    done

    assertEquals "Unxpected references to shared libraries" 0 "${#actual[@]}"
}

test_linkage() {
    echo "==> References to shared libraries test"
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        expected=("linux-vdso.so" "libpthread.so" "libc.so" "libm.so" "ld-linux-x86-64.so")
        readarray -t actual < <(ldd "$probe.so")
        assertEquals "readarray -t actual < <(ldd $probe.so) failed" 0 "${PIPESTATUS[0]}"
        check_shared_libs
    elif [[ "$OSTYPE" == "linux-musl"* ]]; then
        expected=("libgcc_s.so" "libc.musl-x86_64.so" "ld-musl-x86_64.so")
        readarray -t actual < <(ldd "$probe.so")
        assertEquals "readarray -t actual < <(ldd $probe.so) failed" 0 "${PIPESTATUS[0]}"
        check_shared_libs
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        expected=("libc++.1.dylib" "libSystem.B.dylib" "wr-bin")
        readarray -t actual < <(otool -L "$probe.bundle")
        assertEquals "readarray -t actual < <(otool -L $probe.bundle) failed" 0 "${PIPESTATUS[0]}"
        check_shared_libs "${expected[@]}"
    else
        echo "... unknown - $OSTYPE ... skipping"
    fi
}

# ......................................................................
# main
DIR0="$( cd "$( dirname "$0" )" && pwd )"
DIR1="${DIR_ROOT:="$DIR0/../.."}"
DIR_ROOT="$( cd "$DIR1" && pwd )"

DIR_TESTS="$( cd "$DIR0/.." && pwd)"

echo "Running libemf2svg linkage tests"
probe="$DIR_ROOT/libemf2svg"
# shellcheck source=/dev/null
. "$DIR_TESTS"/shunit2/shunit2
