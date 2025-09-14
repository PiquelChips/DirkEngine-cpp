package models

import (
	"DirkBuildTool/output"
	"fmt"
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

func (m *Module) Build() error {
	_, err := m.ToMakefile().ToBytes()
	if err != nil {
		return err
	}
	return nil
}

type Thirdparty map[string]*Dependency

type Dependency struct {
	Name         string `json:"name"`
	IsHeaderOnly bool   `json:"header_only"`
	IncludeDir   string `json:"inc_dir"`
	LibDir       string `json:"lib_dir,omitempty"`
}
