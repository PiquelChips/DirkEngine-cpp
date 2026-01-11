SCRIPTS_DIR=$(pwd)/Engine/Scripts

if ! command -v go >/dev/null 2>&1; then
  echo "Error: Golang is not installed. Attempting to install it."
fi

echo "Golang is installed. Running the rest of setup."
"$SCRIPTS_DIR"/DirkBuildTool.sh setup
