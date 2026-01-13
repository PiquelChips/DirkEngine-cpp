package main

import (
	"fmt"
	"log"
	"os"

	"DirkBuildTool/build"
	"DirkBuildTool/config"
	"DirkBuildTool/setup"
)

func usage() {
	fmt.Printf("usage: DirkBuildTool [target] [build type]\n")
}

const (
	defaultTarget    = "DirkEditor"
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
		case "setup":
			if err := setup.Setup(); err != nil {
				panic(err)
			}
			return
		}
	}

	targetName := ""
	buildModeName := ""
	switch len(os.Args) {
	case 1:
		targetName = defaultTarget
		buildModeName = defaultBuildType
	case 2:
		targetName = os.Args[1]
		buildModeName = defaultBuildType
	case 3:
		targetName = os.Args[1]
		buildModeName = os.Args[2]
	default:
		fmt.Printf("Invalid number of arguments.\n")
		usage()
		os.Exit(1)
		return
	}

	buildMode, ok := config.BuildModes[buildModeName]
	if !ok {
		fmt.Printf("Build type %s does not exist\n", buildModeName)
		os.Exit(1)
		return
	}

	target, ok := config.Targets[targetName]
	if !ok {
		fmt.Printf("Target %s does not exist\n", targetName)
		os.Exit(1)
		return
	}

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
