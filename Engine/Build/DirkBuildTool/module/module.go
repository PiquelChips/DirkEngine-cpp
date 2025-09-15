package module

import (
	"DirkBuildTool/make"
	"DirkBuildTool/models"
	"DirkBuildTool/output"
	"DirkBuildTool/setup"
	"fmt"
	"os"
	"os/exec"
)

// read from .dirkmod files
type ModuleConfig struct {
	Name    string   `json:"name"`
	Std     string   `json:"c_standard"`
	IsLib   bool     `json:"is_lib"`
	Deps    []string `json:"dependencies"` // project modules
	Ext     []string `json:"external"`     // thirdparty modules
	Defines []string `json:"defines"`
}

// constructed for building
type Module struct {
	Name    string
	Path    string
	Std     string
	IsLib   bool
	Deps    []*Module
	Ext     []*models.Dependency
	Defines []string
	Config  *ModuleConfig
}

func (m *Module) ToMakefile() *make.Makefile {
	/*
		var typeStr string
		var ldFlags string
		if m.IsLib {
			typeStr = "shared"
			ldFlags = ldFlags + " -shared"
		} else {
			typeStr = "exec"
		}

		libDirs := []string{}
		incDirs := []string{}
		libs := []string{}
		for _, dep := range m.Deps {
			if dep.IncludeDir != "" {
				incDirs = append(incDirs, dep.IncludeDir)
			}

			if dep.IsHeaderOnly {
				continue
			}

			if dep.LibDir != "" {
				libDirs = append(libDirs, dep.LibDir)
			}
			libs = append(libs, dep.Name)
		}

		return &make.Makefile{
			Target:  m.Name,
			RootDir: output.Dirs.Root,
			Type:    typeStr,
			LibDirs: libDirs,
			IncDirs: incDirs,
			Libs:    libs,
			Defines: m.Defines,
			LdFlags: ldFlags,
			CFlags:  fmt.Sprintf("-fPIC -Wall -Wextra -std=%s", m.Std),
		}
	*/
	return nil
}

func (m *Module) ModDir() (string, error) {
	modDir := fmt.Sprintf("%s/%s", output.Dirs.Intermediate, m.Name)
	return modDir, os.MkdirAll(modDir, output.DirPerm)
}

func (m *Module) Build() error {
	makefile, err := m.ToMakefile().ToBytes()
	if err != nil {
		return err
	}

	modDir, err := m.ModDir()
	if err != nil {
		return err
	}

	makefilePath := fmt.Sprintf("%s/Makefile", modDir)
	if err := os.WriteFile(makefilePath, makefile, output.FilePerm); err != nil {
		return err
	}

	cmd := exec.Command("make", "-f", makefilePath)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Dir = m.Path

	return cmd.Run()
}

func (c *ModuleConfig) ToModule() *Module {
	return &Module{
		Name:    c.Name,
		Path:    fmt.Sprintf("%s/%s", output.Dirs.Source, c.Name),
		Std:     c.Std,
		IsLib:   c.IsLib,
		Deps:    nil,
		Ext:     nil,
		Defines: c.Defines,
		Config:  c,
	}
}

func ResolveDependencies(module *Module, modules map[string]*Module, thirdparty setup.Thirdparty) {
	// TODO: circular dependency detection
	for _, moduleName := range module.Config.Deps {
		mod, ok := modules[moduleName]
		if !ok {
			fmt.Printf("module %s required by module %s does not exist", moduleName, module.Name)
			continue
		}
		module.Deps = append(module.Deps, mod)
		ResolveDependencies(mod, modules, thirdparty)
	}

	// external dependencies
	for _, depName := range module.Config.Ext {
		dep, ok := thirdparty[depName]
		if !ok {
			fmt.Printf("external dependency %s required by module %s does not exist", depName, module.Name)
			continue
		}
		module.Ext = append(module.Ext, dep)
	}

}
