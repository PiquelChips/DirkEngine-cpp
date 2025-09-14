package models

import "fmt"

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

type Thirdparty map[string]*Dependency

type Dependency struct {
	Name         string `json:"name"`
	IsHeaderOnly bool   `json:"header_only"`
	IncludeDir   string `json:"inc_dir"`
	LibDir       string `json:"lib_dir,omitempty"`
}
