package main

import (
	"fmt"
	"os"

	"github.com/PiquelChips/DirkEngine/Build/Source/export"
)

func usage() {
	fmt.Printf("usage ./GenerateProjectFiles.sh <target>\n")
	fmt.Printf("\tSetup project for building. This includes generating\n")
	fmt.Printf("\tfiles for the target build system & creating required\n")
	fmt.Printf("\tdirectories.\n")
}

func main() {
	if len(os.Args) != 2 {
		usage()
		return
	}

	var exporter export.Exporter
	switch os.Args[1] {
	case "Makefile":
		exporter = export.NewMakefileExporter()
	default:
		usage()
		return
	}

	// detect build configurations

	if err := exporter.Export(); err != nil {
		panic(err)
	}
}
