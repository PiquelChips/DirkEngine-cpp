package module

import (
	"DirkBuildTool/make"
	"DirkBuildTool/models"
	"DirkBuildTool/output"
	"DirkBuildTool/setup"
	"fmt"
	"log"
	"os"
	"os/exec"
	"strings"
)

// read from .dirkmod files
type ModuleConfig struct {
	Name    string   `json:"name"`
	Target  string   `json:"target"`
	Std     string   `json:"c_standard"`
	IsLib   bool     `json:"is_lib"`
	Deps    []string `json:"dependencies"` // project modules
	Ext     []string `json:"external"`     // thirdparty modules
	Defines []string `json:"defines"`
}

// constructed for building
type Module struct {
	Name    string
	Target  string
	Path    string
	Std     string
	IsLib   bool
	Deps    []*Module
	Ext     []*models.Dependency
	Config  *ModuleConfig
	selfDep *models.Dependency // itself represented as a dependency
}

func (m *Module) ToMakefile() *make.Makefile {
	log.Printf("Generating Makefile for %s...", m.Name)
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
	defines := m.Config.Defines
	for _, dep := range m.getDeps() {
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

		if defines != nil {
			defines = append(defines, dep.Defines...)
		}
	}

	return &make.Makefile{
		Name:    m.Name,
		Target:  m.Target,
		RootDir: output.Dirs.Root,
		Type:    typeStr,
		LibDirs: libDirs,
		IncDirs: incDirs,
		Libs:    libs,
		Defines: defines,
		LdFlags: ldFlags,
		CFlags:  fmt.Sprintf("-fPIC -Wall -Wextra -std=%s", m.Std),
	}
}

func (m *Module) toDep() *models.Dependency {
	if m.selfDep != nil {
		return m.selfDep
	}

	return &models.Dependency{
		Name:         m.Name,
		IsHeaderOnly: false,
		IncludeDir:   fmt.Sprintf("%s/include", m.Path),
		Defines:      m.Config.Defines,
	}
}

func (m *Module) getDeps() []*models.Dependency {
	deps := m.Ext

	for _, mod := range m.Deps {
		deps = append(deps, mod.toDep())
		deps = append(deps, mod.getDeps()...)
	}

	return deps
}

func (m *Module) writeIntFile(name string, data []byte, overwrite bool) error {
	intDir, err := m.intDir()
	if err != nil {
		return err
	}

	name = strings.Trim(name, "/")
	name = fmt.Sprintf("%s/%s", intDir, name)

	if overwrite {
		return os.WriteFile(name, data, output.FilePerm)
	}

	f, err := os.OpenFile(name, os.O_APPEND|os.O_CREATE, output.FilePerm)
	if err != nil {
		return err
	}
	defer f.Close()

	f.Write(data)
	return nil
}

func (m *Module) readIntFile(name string) ([]byte, error) {
	name = strings.Trim(name, "/")
	name = fmt.Sprintf("%s/%s/%s", output.Dirs.Intermediate, m.Name, name)
	return os.ReadFile(name)
}

func (m *Module) intDir() (string, error) {
	modDir := fmt.Sprintf("%s/%s", output.Dirs.Intermediate, m.Name)
	return modDir, os.MkdirAll(modDir, output.DirPerm)
}

func (m *Module) Build() error {
	for _, dep := range m.Deps {
		if err := dep.Build(); err != nil {
			return err
		}
	}
	log.Printf("Building target %s...", m.Name)
	makefile, err := m.ToMakefile().ToBytes()
	if err != nil {
		return err
	}

	if err := m.writeIntFile("Makefile", makefile, true); err != nil {
		return err
	}

	intDir, err := m.intDir()
	if err != nil {
		return err
	}

	makefilePath := fmt.Sprintf("%s/Makefile", intDir)
	cmd := exec.Command("make", "-f", makefilePath, "-j", "8")
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Dir = m.Path

	if err := cmd.Run(); err != nil {
		log.Printf("There was an error building %s", m.Name)
		return err
	}

	log.Printf("Successfully built %s", m.Name)
	return nil
}

func (c *ModuleConfig) ToModule() *Module {
	return &Module{
		Name:   c.Name,
		Target: c.Target,
		Path:   fmt.Sprintf("%s/%s", output.Dirs.Source, c.Name),
		Std:    c.Std,
		IsLib:  c.IsLib,
		Deps:   nil,
		Ext:    nil,
		Config: c,
	}
}

func ResolveDependencies(module *Module, modules map[string]*Module) {
	// TODO: circular dependency detection
	for _, moduleName := range module.Config.Deps {
		mod, ok := modules[moduleName]
		if !ok {
			log.Printf("Module %s required by module %s does not exist\n", moduleName, module.Name)
			continue
		}
		module.Deps = append(module.Deps, mod)
		ResolveDependencies(mod, modules)
	}

	// external dependencies
	for _, depName := range module.Config.Ext {
		dep, ok := setup.Get().Thirdparty[depName]
		if !ok {
			log.Printf("External dependency %s required by module %s does not exist\n", depName, module.Name)
			continue
		}
		module.Ext = append(module.Ext, dep)
	}

}
