package models

type Defines map[string]string

type CompileCommands []*CompileCommand

type CompileCommand struct {
	Directory string   `json:"directory"`
	Arguments []string `json:"arguments"`
	File      string   `json:"file"`
	Output    string   `json:"output"`
}

type BuildConfig struct {
	Target string
	Mode   *BuildMode
}

type BuildMode struct {
	Name         string
	Optimize     bool
	Compact      bool // compact the output (essentially statically linking)
	Defines      map[string]string
	LinkerFlags  []string
	CompileFlags []string
	WarningLevel int
}
