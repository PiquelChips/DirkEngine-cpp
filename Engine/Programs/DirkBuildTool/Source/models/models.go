package models

import "time"

type Defines map[string]string

type Dependency interface {
	GetIncludeDir() string
	GetDefines() Defines
	IsLib() bool
	GetName() string
}

type ThirdpartyDependency struct {
	Name         string  `json:"name"`
	IsHeaderOnly bool    `json:"header_only"`
	IncludeDir   string  `json:"inc_dir"`
	Defines      Defines `json:"defines,omitempty"`
}

func (dep *ThirdpartyDependency) GetIncludeDir() string {
	return dep.IncludeDir
}

func (dep *ThirdpartyDependency) GetDefines() Defines {
	return dep.Defines
}

func (dep *ThirdpartyDependency) IsLib() bool {
	return !dep.IsHeaderOnly
}

func (dep *ThirdpartyDependency) GetName() string {
	return dep.Name
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
	Thirdparty  map[string]*ThirdpartyDependency `json:"thirdparty"`
	LastSetup   time.Time                        `json:"last_setup"`
	BuildConfig *BuildConfig                     `json:"build_config"`
	Platform    string                           `json:"platform"`
}

type BuildType struct {
	Name           string            `json:"-"`
	Optimize       bool              `json:"optimize"`
	Compact        bool              `json:"compact"` // compact the output (essentially statically linking)
	Defines        map[string]string `json:"defines"`
	WarningLevel   int               `json:"warning_level"` // TODO: actually use this
	SearchDirs     []string          `json:"-"`
	ErrOnBuildFail bool              `json:"-"`
}
