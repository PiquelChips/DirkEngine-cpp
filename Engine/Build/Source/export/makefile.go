package export

import (
	"bufio"
	"fmt"
	"os"
)

type MakefileExporter struct {
	outFile string
}

func NewMakefileExporter() *MakefileExporter {
	return &MakefileExporter{
		outFile: fmt.Sprintf("%s/Makefile", EngineRoot),
	}
}

func (e *MakefileExporter) Export() error {
	file, err := os.Create(e.outFile)
	if err != nil {
		return err
	}
	defer file.Close()

	w := bufio.NewWriter(file)
	_, err = w.WriteString(".PHONY: all\nall:\n\t@echo \"Makefile doesn't do anything yet! yay!\"")
	if err != nil {
		return err
	}
	w.Flush()
	file.Sync()
	return nil
}
