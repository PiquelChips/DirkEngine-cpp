package module

import (
	"DirkBuildTool/make"
	"DirkBuildTool/models"
	"DirkBuildTool/output"
	"fmt"
	"log"
	"os"
	"os/exec"
	"strings"
)

type Module interface {
	ToMakefile() make.Makefile

	getBuildDeps() []Module
	getName() string
	getPath() string

	toDep() *models.Dependency
	getDeps() []*models.Dependency
}

func Build(m Module) error {
	for _, mod := range m.getBuildDeps() {
		if err := Build(mod); err != nil {
			return err
		}
	}

	log.Printf("Building module %s", m.getName())
	makefile, err := m.ToMakefile().ToBytes()
	if err != nil {
		return err
	}

	if err := writeIntFile(m, "Makefile", makefile, true); err != nil {
		return err
	}

	intDir, err := intDir(m)
	if err != nil {
		return err
	}

	makefilePath := fmt.Sprintf("%s/Makefile", intDir)
	cmd := exec.Command("make", "-f", makefilePath, "-j", "8")
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Dir = m.getPath()
	log.Printf("%s\n", makefilePath)

	if err := cmd.Run(); err != nil {
		fmt.Printf("There was an error building %s: %s", m.getName(), err.Error())
		return err
	}

	log.Printf("Successfully built %s", m.getName())
	return nil
}

func writeIntFile(m Module, name string, data []byte, overwrite bool) error {
	intDir, err := intDir(m)
	if err != nil {
		return err
	}

	name = strings.Trim(name, "/")
	name = fmt.Sprintf("%s/%s", intDir, name)

	if overwrite {
		return os.WriteFile(name, data, output.FilePerm)
	}

	f, err := os.OpenFile(name, os.O_APPEND|os.O_CREATE, output.FilePerm)
	if err != nil {
		return err
	}
	defer f.Close()

	f.Write(data)
	return nil
}

func intDir(m Module) (string, error) {
	modDir := fmt.Sprintf("%s/%s", output.Dirs.Intermediate, m.getName())
	return modDir, os.MkdirAll(modDir, output.DirPerm)
}
