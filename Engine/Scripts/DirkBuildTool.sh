BUILD_TOOL_DIR=Engine/Programs/DirkBuildTool
BIN_DIR=$(pwd)/Engine/Binaries
BUILD_TOOL=$BIN_DIR/DirkBuildTool

echo Building DirkBuildTool tool...
go build -C $BUILD_TOOL_DIR/Source -o "$BUILD_TOOL" main.go
