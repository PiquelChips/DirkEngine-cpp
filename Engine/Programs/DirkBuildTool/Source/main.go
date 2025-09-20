package main

import (
	"fmt"
	"log"
	"os"

	"DirkBuildTool/build"
	"DirkBuildTool/config"
	"DirkBuildTool/output"
	"DirkBuildTool/setup"
)

func usage() {
	fmt.Printf("usage: DirkBuildTool [target] [build type]\n")
}

func main() {
	if err := output.GetOutDirs(); err != nil {
		panic(err)
	}

	config := config.LoadConfig()
	if config == nil {
		os.Exit(1)
		return
	}

	target := ""
	buildType := ""
	switch len(os.Args) {
	case 1:
		target = "Editor"
		buildType = "Development"
	case 2:
		target = os.Args[1]
		buildType = "Development"
	case 3:
		target = os.Args[1]
		buildType = os.Args[2]
	default:
		fmt.Printf("Invalid number of arguments.\n")
		usage()
		os.Exit(1)
		return
	}

	buildConf, ok := config.BuildTypes[buildType]
	if !ok {
		fmt.Printf("Build type %s does not exist. Defaulting to Development\n", buildType)
		os.Exit(1)
		return
	}

	buildConfig := &setup.BuildConfig{
		Target: target,
		Type:   buildConf,
	}

	if err := setup.Setup(buildConfig); err != nil {
		panic(err)
	}
	if err := build.Build(buildConfig); err != nil {
		panic(err)
	}
}
