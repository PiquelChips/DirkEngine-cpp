package module

import (
	"DirkBuildTool/config"
	"DirkBuildTool/make"
)

type ShaderModule struct {
	Path, Name string
	isBuilt    bool
}

func (m *ShaderModule) GetName() string            { return m.Name }
func (m *ShaderModule) GetIncludeDirs() []string   { return nil }
func (m *ShaderModule) GetDefines() config.Defines { return nil }
func (m *ShaderModule) GetLibs() []string          { return nil }

func (m *ShaderModule) Build() error {
	err := make.RunMakefile(&make.ShaderMakefile{
		Name: m.Name,
		Path: m.Path,
	})

	m.isBuilt = true
	return err
}
func (m *ShaderModule) IsBuilt() bool { return m.isBuilt }

// shader modules dont have dependencies
func (m *ShaderModule) getDeps() []Module { return nil }
func (m *ShaderModule) getPath() string   { return m.Path }
