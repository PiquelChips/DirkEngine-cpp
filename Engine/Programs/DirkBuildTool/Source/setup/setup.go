package setup

import (
	"fmt"
	"log"
	"os/exec"
)

type program struct {
	name, command string
}

var programs = []program{
	{name: "GNU Make", command: "make"},
	{name: "GNU C++ Compiler", command: "g++"},
}

func check(prog program) error {
	if _, err := exec.LookPath(prog.command); err != nil {
		return fmt.Errorf("Could not find %s installed on your system. Please install it.", prog.name)
	}

	log.Printf("Found %s.", prog.name)
	return nil
}

func Setup() error {
	log.Printf("Running setup...")

	for _, prog := range programs {
		if err := check(prog); err != nil {
			return err
		}
	}

	return nil
}
