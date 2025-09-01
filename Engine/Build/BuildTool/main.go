package main

import (
	"fmt"
	"os"
	"path/filepath"

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

	// index all thirdparty libs

	return nil
}

func generate() error {
	fmt.Printf("generate\n")

	exporter := export.NewMakefileExporter()

	sourcePath, err := filepath.Abs(os.Getenv("PWD") + "/Engine")
	if err != nil {
		return err
	}

	fmt.Println(sourcePath)

	// detect build configurations

	return exporter.Export()
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
