package module

import (
	"DirkBuildTool/config"
)

type HeaderModule struct {
	Name, Path  string
	External    []string
	IncludeDirs []string
}

func (m *HeaderModule) GetName() string            { return m.Name }
func (m *HeaderModule) GetIncludeDirs() []string   { return m.IncludeDirs }
func (m *HeaderModule) GetDefines() config.Defines { return nil }
func (m *HeaderModule) GetLibs() []string          { return m.External }

func (m *HeaderModule) Build() error  { return nil }
func (m *HeaderModule) IsBuilt() bool { return true }

func (m *HeaderModule) getDeps() []Module { return nil }
func (m *HeaderModule) getPath() string   { return m.Path }
