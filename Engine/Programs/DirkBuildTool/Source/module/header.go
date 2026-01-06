package module

import (
	"DirkBuildTool/config"
)

type HeaderModule struct {
	Name, Path  string
	External    []string
	IncludeDirs []string
	Defines     config.Defines
}

func (m *HeaderModule) GetName() string            { return m.Name }
func (m *HeaderModule) GetIncludeDirs() []string   { return m.IncludeDirs }
func (m *HeaderModule) GetDefines() config.Defines { return m.Defines }
func (m *HeaderModule) GetLibs() []string          { return m.External }

func (m *HeaderModule) Build(config.Defines) error { return nil }
func (m *HeaderModule) IsBuilt() bool              { return true }
func (m *HeaderModule) GetDependencies() []string  { return nil }
func (m *HeaderModule) AddDependency(Module)       {}
