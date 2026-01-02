package module

import (
	"DirkBuildTool/make"
	"DirkBuildTool/models"
	"fmt"
)

type HeaderModule struct {
	Name, Path string
	External   []string
}

func (m *HeaderModule) GetName() string            { return m.Name }
func (m *HeaderModule) GetIncludeDir() string      { return fmt.Sprintf("%s/include", m.Path) }
func (m *HeaderModule) GetDefines() models.Defines { return nil }
func (m *HeaderModule) GetLibs() []string          { return m.External }

func (m *HeaderModule) ToMakefile() make.Makefile { return nil } // TODO: replace with build function

func (m *HeaderModule) getDeps() []Module { return nil }
func (m *HeaderModule) getPath() string   { return m.Path }
