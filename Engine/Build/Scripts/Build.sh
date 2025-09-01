#!/bin/sh


builddir=Engine/Build
bin=DirkBuildTool

if ! [ -f $bin ]; then
    echo "Building the Dirk Build Tool..."
    cd $builddir || exit
    go build -o Binaries/$bin Source/main.go
    cd ../.. || exit
    echo "Success!"
fi

$builddir/Binaries/$bin "$@"
