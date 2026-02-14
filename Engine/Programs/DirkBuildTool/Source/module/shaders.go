package module

import (
	"DirkBuildTool/config"
	"DirkBuildTool/make"
)

type ShaderModule struct {
	Path, Name string
}

func (m *ShaderModule) GetName() string            { return m.Name }
func (m *ShaderModule) GetIncludeDirs() []string   { return nil }
func (m *ShaderModule) GetDefines() config.Defines { return nil }
func (m *ShaderModule) GetLibs() []string          { return nil }

func (m *ShaderModule) Build(*config.Target, config.Defines) error {
	return make.RunMakefile(&make.ShaderMakefile{
		Name: m.Name,
		Path: m.Path,
	})
}

// shader modules dont have dependencies
func (m *ShaderModule) GetDependencies() []string { return nil }
func (m *ShaderModule) AddDependency(Module)      {}
