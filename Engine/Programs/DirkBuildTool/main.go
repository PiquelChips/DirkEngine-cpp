package main

import (
	"fmt"
	"log"
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

	buildConfig := &setup.BuildConfig{
		Target:    "Editor",
		BuildType: "Development",
		Optimize:  false,
		Shipping:  false,
	}
	if err := setup.Setup(buildConfig); err != nil {
		panic(err)
	}
	log.Printf("Running build tool with config: %s\n", buildConfig.String())
	if err := build.Build(buildConfig); err != nil {
		panic(err)
	}
}
