package models

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
