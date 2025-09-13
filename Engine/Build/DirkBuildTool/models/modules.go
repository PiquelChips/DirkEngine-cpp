package models

// read from .dirkmod files
type ModuleConfig struct {
	Name    string   `json:"name"`
	Std     string   `json:"c_standard"`
	Type    string   `json:"type"`         // lib, exec
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
