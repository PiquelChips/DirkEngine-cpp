package main

import (
	"fmt"
	"os"
)

func usage() {
	fmt.Printf("usage: DirkBuildTool <command>\n")
	fmt.Printf("\tsetup - will setup the dependencies for development\n")
}

func setup() error {
	fmt.Printf("setup has not been implemented yet. this is a work in progress\n")
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
	default:
		usage()
		return
	}

	if err := run(); err != nil {
		panic(err)
	}
}
