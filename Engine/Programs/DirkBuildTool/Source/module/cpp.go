package module

import (
	"DirkBuildTool/config"
	"DirkBuildTool/make"
	"DirkBuildTool/models"
	"DirkBuildTool/setup"
	"fmt"
	"log"
	"slices"
)

// read from .dirkmod files
type ModuleConfig struct {
	Name    string   `json:"name"`
	Target  string   `json:"target"`
	Type    string   `json:"type"`
	Path    string   `json:"-"`
	Std     string   `json:"c_standard"`
	IsLib   bool     `json:"is_lib"`
	Deps    []string `json:"dependencies"` // project modules
	Ext     []string `json:"external"`     // thirdparty modules
	Defines []string `json:"defines"`
}

func (c *ModuleConfig) ToModule(buildConfig *setup.BuildConfig) Module {
	if c.Type == "" {
		c.Type = "cpp"
	}

	switch c.Type {
	case "shaders":
		return &ShaderModule{
			Name: c.Name,
			Path: c.Path,
		}
	case "cpp":
		return &CppModule{
			Name:   c.Name,
			Target: c.Target,
			Path:   c.Path,
			Std:    c.Std,
			IsLib:  c.IsLib,
			Deps:   nil,
			Ext:    nil,
			Config: c,
			build:  buildConfig,
		}
	default:
		log.Printf("Module type %s used by module %s does not exist. Please use \"shaders\" or \"cpp\"\n", c.Type, c.Name)
		return nil
	}
}

// constructed for building
type CppModule struct {
	Name       string
	Target     string
	Path       string
	Std        string
	IsLib      bool
	Deps       []Module
	Ext        []*models.Dependency
	Dependants []*models.Dependency
	Config     *ModuleConfig
	selfDep    *models.Dependency // itself represented as a dependency
	build      *setup.BuildConfig
}

func (m *CppModule) ToMakefile() make.Makefile {
	log.Printf("Generating Makefile for %s\n", m.Name)

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

		libs = append(libs, dep.Name)

		if defines != nil {
			defines = append(defines, dep.Defines...)
		}
	}

	for _, dep := range m.Dependants {
		if dep.Defines != nil {
			defines = append(defines, dep.Defines...)
		}
	}

	warningFlags := "-Wall -Wextra"

	return &make.CppMakefile{
		Name:      m.Name,
		Target:    m.Target,
		BuildType: m.build.Type.Name,
		RootDir:   config.Dirs.Root,
		IncDirs:   incDirs,
		Libs:      libs,
		Defines:   defines,
		IsLib:     m.IsLib,
		IsStatic:  m.build.Type.Compact,
		Optimize:  m.build.Type.Optimize,
		CFlags:    fmt.Sprintf("-fPIC %s -std=%s", warningFlags, m.Std),
	}
}

func (m *CppModule) getBuildDeps() []Module {
	return m.Deps
}

func (m *CppModule) getName() string {
	return m.Name
}

func (m *CppModule) getPath() string {
	return m.Path
}

func (m *CppModule) toDep() *models.Dependency {
	if m.selfDep != nil {
		return m.selfDep
	}

	m.selfDep = &models.Dependency{
		Name:         m.Name,
		IsHeaderOnly: false,
		IncludeDir:   fmt.Sprintf("%s/include", m.Path),
		Defines:      m.Config.Defines,
	}

	return m.selfDep
}

func (m *CppModule) getDeps() []*models.Dependency {
	deps := m.Ext

	for _, mod := range m.Deps {
		if cppMod, ok := mod.(*CppModule); ok {
			deps = append(deps, cppMod.toDep())
		}

		modDeps := mod.getDeps()
		// cleaning duplicates
		for _, modDep := range modDeps {
			if !slices.Contains(deps, modDep) {
				deps = append(deps, modDep)
			}
		}
	}

	return deps
}

func (m *CppModule) ResolveDependencies(modules map[string]Module, dependants []*models.Dependency) {
	if slices.Contains(dependants, m.toDep()) {
		fmt.Printf("Circular dependency detected. Module %s has already been included\n", m.Name)
		return
	}

	m.Dependants = dependants
	for _, moduleName := range m.Config.Deps {
		mod, ok := modules[moduleName]
		if !ok {
			log.Printf("Module %s required by module %s does not exist\n", moduleName, m.Name)
			continue
		}
		m.Deps = append(m.Deps, mod)
		if engineMod, ok := mod.(*CppModule); ok {
			engineMod.ResolveDependencies(modules, append(dependants, m.toDep()))
		}
	}

	// external dependencies
	for _, depName := range m.Config.Ext {
		dep, ok := setup.Config.Thirdparty[depName]
		if !ok {
			log.Printf("External dependency %s required by module %s does not exist\n", depName, m.Name)
			continue
		}
		m.Ext = append(m.Ext, dep)
	}
}
