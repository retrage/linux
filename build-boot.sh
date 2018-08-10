#!/usr/bin/env bash

PWD=`pwd`
LKL="$PWD/tools/lkl"

CC="emcc"
CP="cp"
DIS="llvm-dis"
LINK="llvm-link"
PY="python"

CFLAGS="-m32"
CFLAGS="$CFLAGS -s WASM=0"
CFLAGS="$CFLAGS -s EMULATE_FUNCTION_POINTER_CASTS=1"
CFLAGS="$CFLAGS -fno-short-wchar"
CFLAGS="$CFLAGS -O0"
CFLAGS="$CFLAGS -g4"

echo "LINK boot.bc"
$LINK -o $LKL/tests/boot.bc \
    $LKL/tests/boot-in.o $LKL/lib/liblkl-in.o $LKL/lib/lkl.o
echo "DIS boot.bc"
$DIS -o $LKL/tests/boot.ll $LKL/tests/boot.bc
mkdir -p js
$CP ~/.emscripten_cache/asmjs/dlmalloc.bc js/dlmalloc.bc
$CP ~/.emscripten_cache/asmjs/libc.bc js/libc.bc
$CP ~/.emscripten_cache/asmjs/pthreads.bc js/pthreads.bc
echo "DIS dlmalloc.bc"
$DIS -o js/dlmalloc.ll js/dlmalloc.bc
echo "DIS libc.bc"
$DIS -o js/libc.ll js/libc.bc
echo "DIS pthreads.bc"
$DIS -o js/pthreads.ll js/pthreads.bc
echo "PY rename_symbols.py"
$PY rename_symbols.py $LKL/tests/boot.ll $LKL/tests/boot-mod.ll
echo "EMCC boot.js"
EMCC_DEBUG=1 $CC -o js/boot.html $LKL/tests/boot-mod.ll $CFLAGS -v
echo "PY fix-js.py"
$PY fix-js.py js/boot.js js/boot.js
