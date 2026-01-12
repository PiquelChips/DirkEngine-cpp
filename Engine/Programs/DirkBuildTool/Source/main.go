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
		}
	}

	if err := buildTarget(os.Args); err != nil {
		panic(err)
	}
}

func buildTarget(args []string) error {
	targetName := ""
	buildModeName := ""
	switch len(args) {
	case 1:
		targetName = defaultTarget
		buildModeName = defaultBuildType
	case 2:
		targetName = args[1]
		buildModeName = defaultBuildType
	case 3:
		targetName = args[1]
		buildModeName = args[2]
	default:
		usage()
		return fmt.Errorf("Invalid number of arguments.")
	}

	buildMode, ok := config.BuildModes[buildModeName]
	if !ok {
		return fmt.Errorf("Build type %s does not exist\n", buildModeName)
	}

	target, ok := config.Targets[targetName]
	if !ok {
		return fmt.Errorf("Target %s does not exist\n", targetName)
	}

	buildConfig, err := build.SetupBuildConfig(target, buildMode)
	if err != nil {
		return err
	}

	if err := buildConfig.GenerateCompileCommands(); err != nil {
		return err
	}

	return buildConfig.Build()
}

func clean() {
	log.Printf("Cleaning...")
	os.RemoveAll(config.Dirs.Binaries)
	os.RemoveAll(config.Dirs.Intermediate)
	os.RemoveAll(config.Dirs.Saved)
	os.Remove(fmt.Sprintf("%s/compile_commands.json", config.Dirs.Work))
}
