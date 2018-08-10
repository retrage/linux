#!/usr/bin/env bash

PWD=`pwd`
LKL="$PWD/tools/lkl"

CC="emcc"

CFLAGS="-m32"
CFLAGS="$CFLAGS -s WASM=0"
CFLAGS="$CFLAGS -s EMULATE_FUNCTION_POINTER_CASTS=1"
CFLAGS="$CFLAGS -fno-short-wchar"
CFLAGS="$CFLAGS -O0"
CFLAGS="$CFLAGS -g4"

EMCC_DEBUG=1 $CC -o js/boot.html $LKL/tests/boot-mod.ll $CFLAGS -v
