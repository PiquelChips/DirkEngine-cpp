package main

import (
	"fmt"
	"os"

	"DirkBuildTool/build"
	"DirkBuildTool/output"
	"DirkBuildTool/setup"
)

func usage() {
	fmt.Printf("usage: DirkBuildTool <command>\n")
	fmt.Printf("\tsetup - setup the dependencies for development\n")
	fmt.Printf("\tbuild - build the editor\n")
}

func main() {
	if len(os.Args) != 2 {
		usage()
		return
	}

	if err := output.GetOutDirs(); err != nil {
		panic(err)
	}

	var run func() error
	switch os.Args[1] {
	case "setup":
		run = setup.Setup
	case "build":
		run = build.Build
	default:
		usage()
		return
	}

	if err := run(); err != nil {
		panic(err)
	}
}
