package models

import "DirkBuildTool/config"

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
	Target string            `json:"target"`
	Type   *config.BuildType `json:"build_type"`
}
