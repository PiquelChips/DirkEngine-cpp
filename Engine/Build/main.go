package main

import (
	"fmt"
	"os"
)

func run(target string) {
	build(target)
	fmt.Printf("running %s\n", target)
}

func build(target string) {
	fmt.Printf("building %s\n", target)
}

func clean() {
	fmt.Printf("cleaning\n")
}

func usage() {
	fmt.Printf("usage ./Build.sh <command> [OPTIONS]\n")
	fmt.Println()
	fmt.Printf("\trun [target] - builds & runs the target\n")
	fmt.Printf("\tbuild [target] - builds the target\n")
	fmt.Printf("\tclean - cleans all build files\n")
}

func main() {
	pwd := os.Getenv("PWD")
	fmt.Printf("Hello, World!\n")
	fmt.Printf("We are in %s\n", pwd)

	for i, arg := range os.Args {
		fmt.Printf("\t%d - %s\n", i, arg)
	}

	if len(os.Args) < 2 {
		usage()
		return
	}

	switch os.Args[1] {
	case "build":
		if len(os.Args) == 3 {
			build(os.Args[2])
			return
		}
	case "run":
		if len(os.Args) == 3 {
			run(os.Args[2])
			return
		}
	case "clean":
		if len(os.Args) == 2 {
			clean()
			return
		}
	}

	usage()
}
