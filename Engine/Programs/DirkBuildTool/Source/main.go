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
	config.LoadConfig()

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

	buildConf, ok := config.BuildTypes[buildType]
	if !ok {
		fmt.Printf("Build type %s does not exist\n", buildType)
		os.Exit(1)
		return
	}

	log.Printf("Building %s for %s\n", target, buildType)

	buildConfig := &models.BuildConfig{
		Target: target,
		Type:   buildConf,
	}

	if err := build.Build(buildConfig); err != nil {
		panic(err)
	}

	// TODO: make sure to symlink compile command properly
}

func clean() {
	log.Printf("Cleaning...")
	os.RemoveAll(config.Dirs.Binaries)
	os.RemoveAll(config.Dirs.Intermediate)
	os.RemoveAll(config.Dirs.Saved)
}
