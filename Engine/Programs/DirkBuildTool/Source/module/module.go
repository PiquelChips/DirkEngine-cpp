package module

import (
	"DirkBuildTool/make"
	"DirkBuildTool/models"
)

type Module interface {
	ToMakefile() make.Makefile
	toDep() *models.Dependency
	getDeps() []*models.Dependency
	writeIntFile(name string, data []byte, overwrite bool) error
	intDir() (string, error)
	Build() error
	ResolveDependencies(modules map[string]Module, dependants []*models.Dependency)
}
