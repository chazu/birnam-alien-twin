#!/bin/sh
set -eu

if [ "$#" -ne 3 ]; then
    echo "usage: build.sh OUTPUT PROJECT_DIR BUILD_DIR" >&2
    exit 2
fi

output=$1
project_dir=$2
build_dir=$3/alien-twin-native
source_dir=$project_dir/vendor/twin

mkdir -p "$build_dir"

if [ ! -f "$build_dir/config.status" ]; then
    cd "$build_dir"
    "$source_dir/configure" \
        --srcdir="$source_dir" \
        --disable-shared \
        --enable-static \
        --with-pic \
        --enable-socket=yes \
        --enable-socket-gz=no \
        --enable-socket-pthreads=no \
        --enable-rcparse=no \
        --enable-hw-tty=no \
        --enable-hw-x11=no \
        --enable-hw-xft=no \
        --enable-hw-twin=no \
        --enable-hw-display=no \
        --enable-server-static=no
fi

make -C "$build_dir/libs/libtw" libtw.la

cc \
    -std=c11 \
    -Wall \
    -Wextra \
    -Werror \
    -fPIC \
    -dynamiclib \
    -Wl,-install_name,@rpath/libtwin.dylib \
    -I"$build_dir/include" \
    -I"$source_dir/include" \
    "$project_dir/native/alien_twin.c" \
    "$build_dir/libs/libtw/.libs/libtw.a" \
    -o "$output"
