package main

import (
	"fmt"
	"os"

	"DirkBuildTool/build"
	"DirkBuildTool/output"
	"DirkBuildTool/setup"
)

func main() {
	if err := output.GetOutDirs(); err != nil {
		panic(err)
	}

	if len(os.Args) != 1 {
		fmt.Printf("cli not implemented yet, please run without params\n")
		return
	}

	buildConfig := &setup.BuildConfig{Target: "Editor"}
	if err := setup.Setup(buildConfig); err != nil {
		panic(err)
	}
	if err := build.Build(buildConfig); err != nil {
		panic(err)
	}
}
