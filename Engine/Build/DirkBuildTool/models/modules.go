package models

// read from .dirkmod files
type ModuleConfig struct {
	Name    string   `json:"name"`
	Std     string   `json:"c_standard"`
	IsLib   bool     `json:"is_lib"`       // lib, exec
	Deps    []string `json:"dependencies"` // project modules
	Ext     []string `json:"external"`     // thirdparty modules
	Defines []string `json:"defines"`
}

// constructed for building
type Module struct {
	Name    string
	CxxStd  string
	IsLib   bool
	Deps    []Dependency
	Defines []string
}

type Thirdparty map[string]*Dependency

type Dependency struct {
	Name         string `json:"name"`
	IsHeaderOnly bool   `json:"header_only"`
	IncludeDir   string `json:"inc_dir"`
	LibDir       string `json:"lib_dir,omitempty"`
}
