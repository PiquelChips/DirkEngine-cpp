package models

import (
	"DirkBuildTool/output"
	"fmt"
	"os"
	"os/exec"
)

// read from .dirkmod files
type ModuleConfig struct {
	Name    string   `json:"name"`
	Std     string   `json:"c_standard"`
	IsLib   bool     `json:"is_lib"`
	Deps    []string `json:"dependencies"` // project modules
	Ext     []string `json:"external"`     // thirdparty modules
	Defines []string `json:"defines"`
	Path    string   // shouldn't be in json
}

func (m *ModuleConfig) ToDependency() *Dependency {
	return &Dependency{
		Name:         m.Name,
		IsHeaderOnly: false,
		IncludeDir:   fmt.Sprintf("%s/include", m.Path),
	}
}

// constructed for building
type Module struct {
	Name    string
	Path    string
	Std     string
	IsLib   bool
	Deps    []*Dependency
	Defines []string
}

func (m *Module) ToMakefile() *Makefile {
	var typeStr string
	var ldFlags string
	if m.IsLib {
		typeStr = "shared"
		ldFlags = ldFlags + " -shared"
	} else {
		typeStr = "exec"
	}

	libDirs := []string{}
	incDirs := []string{}
	libs := []string{}
	for _, dep := range m.Deps {
		if dep.IncludeDir != "" {
			incDirs = append(incDirs, dep.IncludeDir)
		}

		if dep.IsHeaderOnly {
			continue
		}

		if dep.LibDir != "" {
			libDirs = append(libDirs, dep.LibDir)
		}
		libs = append(libs, dep.Name)
	}

	return &Makefile{
		Target:  m.Name,
		RootDir: output.Dirs.Root,
		Type:    typeStr,
		LibDirs: libDirs,
		IncDirs: incDirs,
		Libs:    libs,
		Defines: m.Defines,
		LdFlags: ldFlags,
		CFlags:  fmt.Sprintf("-fPIC -Wall -Wextra -std=%s", m.Std),
	}
}

func (m *Module) ModDir() (string, error) {
	modDir := fmt.Sprintf("%s/%s", output.Dirs.Intermediate, m.Name)
	return modDir, os.MkdirAll(modDir, output.DirPerm)
}

func (m *Module) Build() error {
	makefile, err := m.ToMakefile().ToBytes()
	if err != nil {
		return err
	}

	modDir, err := m.ModDir()
	if err != nil {
		return err
	}

	makefilePath := fmt.Sprintf("%s/Makefile", modDir)
	if err := os.WriteFile(makefilePath, makefile, output.FilePerm); err != nil {
		return err
	}

	cmd := exec.Command("make", "-f", makefilePath)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Dir = m.Path

	return cmd.Run()
}

type Thirdparty map[string]*Dependency

type Dependency struct {
	Name         string `json:"name"`
	IsHeaderOnly bool   `json:"header_only"`
	IncludeDir   string `json:"inc_dir"`
	LibDir       string `json:"lib_dir,omitempty"`
}
