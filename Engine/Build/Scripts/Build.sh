#!/bin/sh

make -C Engine/Build/BuildTool
Engine/Build/Binaries/DirkBuildTool "$@"
