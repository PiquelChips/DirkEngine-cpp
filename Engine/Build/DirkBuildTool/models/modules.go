package models

// read from .dirkmod files
type ModuleConfig struct {
	Name    string   `json:"name"`
	CxxStd  string   `json:"cxx_std"`
	IsLib   bool     `json:"lib"`
	Deps    []string `json:"dependencies"` // project modules
	Ext     []string `json:"external"`     // thirdparty modules
	Defines []string `json:"defines"`
}

// constructed for building
type Module struct {
	Name    string
	CxxStd  string
	IsLib   bool
	Deps    []Module
	Defines []string
}
