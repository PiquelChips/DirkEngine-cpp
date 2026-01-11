package setup

import (
	"log"
	"os/exec"
)

func check(bin string) error {
	_, err := exec.LookPath(bin)
	return err
}

func Setup() error {
	log.Printf("Running setup...")

	if err := check("g++"); err != nil {
		return err
	}
	log.Printf("Found the C++ compiler.")

	if err := check("make"); err != nil {
		return err
	}
	log.Printf("Found GNU Make.")

	return nil
}
