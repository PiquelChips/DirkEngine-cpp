package main

import (
	"fmt"
	"os"
)

func usage() {
	fmt.Printf("usage: DirkBuildTool <command>\n")
	fmt.Printf("\tsetup - setup the dependencies for development\n")
	fmt.Printf("\tprojectfiles - generate makefiles (other solutions later)\n")
	fmt.Printf("\tclangd - generate compile_commands.json\n")
	fmt.Printf("\tbuild - build the editor\n")
}

func setup() error {
	fmt.Printf("setup has not been implemented yet. this is a work in progress\n")
	return nil
}

func generateProjectFiles() error {
	fmt.Printf("project files generation has not been implemented yet. this is a work in progress\n")
	return nil
}

func generateCompileCommands() error {
	fmt.Printf("compile command generation has not been implemented yet. this is a work in progress\n")
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
	case "projectfiles":
		run = generateProjectFiles
	case "clangd":
		run = generateCompileCommands
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
