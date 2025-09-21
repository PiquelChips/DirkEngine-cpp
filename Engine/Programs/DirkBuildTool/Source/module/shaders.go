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
		RootDir: config.Get().Dirs.Root,
	}
}

// shader modules dont have dependencies
func (m *ShaderModule) getBuildDeps() []Module        { return nil }
func (m *ShaderModule) getName() string               { return m.Name }
func (m *ShaderModule) getPath() string               { return m.Path }
func (m *ShaderModule) toDep() *models.Dependency     { return nil }
func (m *ShaderModule) getDeps() []*models.Dependency { return nil }
