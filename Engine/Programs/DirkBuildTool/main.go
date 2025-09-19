package main

import (
	"fmt"
	"log"
	"os"

	"DirkBuildTool/build"
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

	var buildConfig *setup.BuildConfig
	if len(os.Args) == 1 {
		buildConfig = &setup.BuildConfig{
			Target:    "Editor",
			BuildType: "Development",
			Optimize:  false,
			Shipping:  false,
		}
	} else if len(os.Args) == 3 {
		buildConfig = &setup.BuildConfig{
			Target:    os.Args[1],
			BuildType: os.Args[2],
		}

		switch buildConfig.BuildType {
		case "Development":
			buildConfig.Optimize = false
			buildConfig.Shipping = false
		case "Shipping":
			buildConfig.Optimize = true
			buildConfig.Shipping = true
		default:
			log.Printf("Cannot build target %s for %s. Defaulting to Development.\n", buildConfig.Target, buildConfig.BuildType)
			buildConfig.BuildType = "Development"
			buildConfig.Optimize = false
			buildConfig.Shipping = false
		}
	} else {
		fmt.Printf("Invalid number of arguments.\n")
		usage()
		os.Exit(1)
		return
	}

	if err := setup.Setup(buildConfig); err != nil {
		panic(err)
	}
	log.Printf("Running build tool with config: %s\n", buildConfig.String())
	if err := build.Build(buildConfig); err != nil {
		panic(err)
	}
}
