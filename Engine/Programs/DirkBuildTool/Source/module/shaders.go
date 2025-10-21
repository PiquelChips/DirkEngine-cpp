package module

import (
	"DirkBuildTool/config"
	"DirkBuildTool/make"
	"DirkBuildTool/models"
)

type ShaderModule struct {
	Path, Name string
}

func (m *ShaderModule) ToMakefile() make.Makefile {
	return &make.ShaderMakefile{
		Name:    m.Name,
		Path:    m.Path,
		RootDir: config.Dirs.Root,
	}
}

// shader modules dont have dependencies
func (m *ShaderModule) getBuildDeps() []Module       { return nil }
func (m *ShaderModule) getPath() string              { return m.Path }
func (m *ShaderModule) getDeps() []models.Dependency { return nil }

func (m *ShaderModule) GetIncludeDir() string      { return "" }
func (m *ShaderModule) GetDefines() models.Defines { return nil }
func (m *ShaderModule) IsLib() bool                { return false }
func (m *ShaderModule) GetName() string            { return m.Name }
