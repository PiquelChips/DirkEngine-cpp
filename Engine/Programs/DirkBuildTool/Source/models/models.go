package models

import "time"

type Dependency struct {
	Name         string            `json:"name"`
	IsHeaderOnly bool              `json:"header_only"`
	IncludeDir   string            `json:"inc_dir"`
	Defines      map[string]string `json:"defines,omitempty"`
}

type CompileCommands []*CompileCommand

type CompileCommand struct {
	Directory string   `json:"directory"`
	Arguments []string `json:"arguments"`
	File      string   `json:"file"`
	Output    string   `json:"output"`
}

type BuildConfig struct {
	Target string     `json:"target"`
	Type   *BuildType `json:"build_type"`
}

type SetupConfig struct {
	Thirdparty  map[string]*Dependency `json:"thirdparty"`
	LastSetup   time.Time              `json:"last_setup"`
	BuildConfig *BuildConfig           `json:"build_config"`
	Platform    string                 `json:"platform"`
}

type BuildType struct {
	Name     string            `json:"-"`
	Optimize bool              `json:"optimize"`
	Compact  bool              `json:"compact"` // compact the output (essentially statically linking)
	Defines  map[string]string `json:"defines"`
}
