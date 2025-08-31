#!/bin/sh

cd Engine/Build || exit
go build -o Binaries/DirkBuild main.go
cd ../.. || exit
Engine/Build/Binaries/DirkBuild "$@"
