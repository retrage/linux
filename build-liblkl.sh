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

echo "LINK liblkl.bc"
$LINK -o $LKL/lib/liblkl.bc \
    $LKL/lib/liblkl-in.o $LKL/lib/lkl.o
echo "DIS liblkl.bc"
$DIS -o $LKL/lib/liblkl.ll $LKL/lib/liblkl.bc
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
$PY rename_symbols.py $LKL/lib/liblkl.ll $LKL/lib/liblkl-mod.ll

EX_FUNCS="$(grep "@lkl_" $LKL/lib/liblkl-mod.ll \
    | grep "define" | grep -v "internal" \
    | $PY ex_funcs.py)"
CFLAGS="$CFLAGS -s EXPORTED_FUNCTIONS=$EX_FUNCS"

echo "EMCC liblkl.js"
EMCC_DEBUG=1 $CC -o js/liblkl.js $LKL/lib/liblkl-mod.ll $CFLAGS -v
echo "PY fix-js.py"
$PY fix-js.py js/liblkl.js js/liblkl.js
