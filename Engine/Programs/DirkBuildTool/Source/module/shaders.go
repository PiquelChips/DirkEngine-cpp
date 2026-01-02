package module

import (
	"DirkBuildTool/make"
	"DirkBuildTool/models"
)

type ShaderModule struct {
	Path, Name string
}

func (m *ShaderModule) GetName() string            { return m.Name }
func (m *ShaderModule) GetIncludeDirs() []string   { return nil }
func (m *ShaderModule) GetDefines() models.Defines { return nil }
func (m *ShaderModule) GetLibs() []string          { return nil }

func (m *ShaderModule) Build() error {
	return make.RunMakefile(&make.ShaderMakefile{
		Name: m.Name,
		Path: m.Path,
	})
}

// shader modules dont have dependencies
func (m *ShaderModule) GetDeps() []Module    { return nil }
func (m *ShaderModule) getAllDeps() []Module { return nil }
func (m *ShaderModule) getPath() string      { return m.Path }
