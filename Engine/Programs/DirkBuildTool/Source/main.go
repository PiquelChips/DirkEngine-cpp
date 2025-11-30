package main

import (
	"fmt"
	"os"

	"DirkBuildTool/build"
	"DirkBuildTool/config"
	"DirkBuildTool/models"
	"DirkBuildTool/setup"
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
		case "clean-all":
			cleanAll()
			return
		case "clean-setup":
			cleanSetup()
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

	buildConfig := &models.BuildConfig{
		Target:         target,
		Type:           buildConf,
		SearchDirs:     config.Dirs.Modules,
		ErrOnBuildFail: false,
	}

	if err := setup.Setup(buildConfig); err != nil {
		panic(err)
	}
	if err := build.Build(buildConfig); err != nil {
		panic(err)
	}
}

func clean()      {}
func cleanAll()   {}
func cleanSetup() {}
