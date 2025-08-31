#!/bin/sh

if [ "$DIRK_ENGINE_DEV" = "true" ]; then
    echo "Development mode, building the build system..."
    cd Engine/Build || exit
    go build -o Binaries/DirkBuild Source/main.go
    cd ../.. || exit
fi

echo "Generating project files..."
Engine/Build/Binaries/DirkBuild "$@"
