package main

import (
	"fmt"
	"log"
	"os"

	"DirkBuildTool/build"
	"DirkBuildTool/config"
	"DirkBuildTool/models"
)

func usage() {
	fmt.Printf("usage: DirkBuildTool [target] [build type]\n")
}

func main() {
	if err := config.LoadConfig(); err != nil {
		panic(err)
	}

	if len(os.Args) == 2 {
		switch os.Args[1] {
		case "clean":
			clean()
			return
		}
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

	buildMode, ok := config.BuildModes[buildType]
	if !ok {
		fmt.Printf("Build type %s does not exist\n", buildType)
		os.Exit(1)
		return
	}

	log.Printf("Building %s for %s\n", target, buildType)

	buildConfig := &models.BuildConfig{
		Target: target,
		Mode:   buildMode,
	}

	if err := build.Build(buildConfig); err != nil {
		panic(err)
	}
}

func clean() {
	log.Printf("Cleaning...")
	os.RemoveAll(config.Dirs.Binaries)
	os.RemoveAll(config.Dirs.Intermediate)
	os.RemoveAll(config.Dirs.Saved)
}
