#!/bin/sh

cd Engine/Build || exit
go build -o Binaries/DirkBuild Source/main.go
cd ../.. || exit
Engine/Build/Binaries/DirkBuild "$@"
