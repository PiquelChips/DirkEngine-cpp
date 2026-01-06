package main

import (
	"fmt"
	"log"
	"os"

	"DirkBuildTool/build"
	"DirkBuildTool/config"
)

func usage() {
	fmt.Printf("usage: DirkBuildTool [target] [build type]\n")
}

const (
	defaultTarget    = "Editor"
	defaultBuildType = "Development"
)

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

	targetName := ""
	buildType := ""
	switch len(os.Args) {
	case 1:
		targetName = defaultTarget
		buildType = defaultBuildType
	case 2:
		targetName = os.Args[1]
		buildType = defaultBuildType
	case 3:
		targetName = os.Args[1]
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

	target, ok := config.Targets[targetName]
	if !ok {
		fmt.Printf("Target %s does not exist\n", targetName)
		os.Exit(1)
		return
	}

	log.Printf("Building targetName %s with %s configuration\n", targetName, buildType)

	buildConfig := &config.BuildConfig{
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
	os.Remove(fmt.Sprintf("%s/compile_commands.json", config.Dirs.Work))
}
