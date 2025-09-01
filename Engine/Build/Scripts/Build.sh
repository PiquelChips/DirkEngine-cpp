#!/bin/sh


builddir=Engine/Build
bin=DirkBuildTool

echo "Building the Dirk Build Tool..."
cd $builddir || exit
go build -o Binaries/$bin Source/main.go
cd ../.. || exit
echo "Success!"

$builddir/Binaries/$bin "$@"
