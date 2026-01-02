package models

type Defines map[string]string

type Dependency interface {
	GetIncludeDir() string
	GetDefines() Defines
	GetLibs() []string
}

type ThirdpartyDependency struct {
	IsHeaderOnly bool     `json:"header_only"`
	External     bool     `json:"external"`
	IncludeDir   string   `json:"include_dir"`    // relative (default is "include")
	Libs         []string `json:"libs,omitempty"` // the names of the libraries needed without (ex: wayland needs wayland-client)
}

func (dep *ThirdpartyDependency) GetIncludeDir() string { return dep.IncludeDir }
func (dep *ThirdpartyDependency) GetDefines() Defines   { return nil }
func (dep *ThirdpartyDependency) GetLibs() []string     { return dep.Libs }

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
	Name     string
	Optimize bool
	Compact  bool // compact the output (essentially statically linking)
	Defines  map[string]string
}
