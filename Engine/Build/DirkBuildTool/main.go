package main

import (
	"fmt"
	"os"
)

func usage() {
	fmt.Printf("usage: DirkBuildTool <command>\n")
	fmt.Printf("\tsetup - setup the dependencies for development\n")
	fmt.Printf("\tbuild - build the editor\n")
}

func setup() error {
	fmt.Printf("setup has not been implemented yet. this is a work in progress\n")
	return nil
}

func build() error {
	fmt.Printf("build has not been implemented yet. this is a work in progress\n")
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
	case "build":
		run = build
	default:
		usage()
		return
	}

	if err := run(); err != nil {
		panic(err)
	}
}
