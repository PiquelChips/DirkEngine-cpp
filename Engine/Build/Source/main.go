package main

import (
	"fmt"
	"os"

	"github.com/PiquelChips/DirkEngine/Build/Source/export"
)

func usage() {
	fmt.Printf("usage: DirkBuildTool <target>\n")
	fmt.Printf("\tSetup project for building. This includes generating\n")
	fmt.Printf("\tfiles for the target build system & creating required\n")
	fmt.Printf("\tdirectories.\n")
}

func setup() error {
	fmt.Printf("setup\n")
	exporter := export.NewMakefileExporter()

	// detect build configurations

	return exporter.Export()
}

func generate() error {
	fmt.Printf("generate\n")
	return nil
}

func main() {
	if len(os.Args) != 2 {
		usage()
		return
	}

	var run func() error
	switch os.Args[1] {
	case "setup":
		run = setup
	case "generate":
		run = generate
	default:
		usage()
		return
	}

	if err := run(); err != nil {
		panic(err)
	}
}
