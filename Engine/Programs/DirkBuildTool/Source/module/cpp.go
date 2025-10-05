package module

import (
	"DirkBuildTool/config"
	"DirkBuildTool/make"
	"DirkBuildTool/models"
	"DirkBuildTool/setup"
	"fmt"
	"io/fs"
	"log"
	"maps"
	"path/filepath"
	"slices"
	"strings"
)

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

func (m *CppModule) GenerateCompileCommands() (models.CompileCommands, error) {
	compileCommands := models.CompileCommands{}

	if err := filepath.WalkDir(fmt.Sprintf("%s/src", m.Path), func(path string, d fs.DirEntry, err error) error {
		if d.IsDir() {
			return nil
		}

		in := strings.Replace(path, m.Path, "", 1)
		in = strings.Trim(in, "/")

		intDir, _ := intDir(m)
		out := fmt.Sprintf("%s/%s/%s", intDir, m.build.Type.Name, in)
		out = strings.Replace(out, "/src/", "/obj/", 1)
		out = strings.Replace(out, ".cpp", ".o", 1)

		incDirs := []string{"include"}
		defines := m.Config.Defines
		for _, dep := range m.getDeps() {
			if dep.IncludeDir != "" {
				incDirs = append(incDirs, dep.IncludeDir)
			}

			maps.Copy(defines, dep.Defines)
		}

		for _, dep := range m.Dependants {
			maps.Copy(defines, dep.Defines)
		}

		command := []string{"g++", "-fPIC", fmt.Sprintf("-std=%s", m.Std)}

		for _, dir := range incDirs {
			command = append(command, fmt.Sprintf("-I%s", dir))
		}

		for key, value := range defines {
			if value == "" {
				command = append(command, fmt.Sprintf("-D%s", key))
			} else {
				command = append(command, fmt.Sprintf("-D%s=\"%s\"", key, value))
			}
		}

		command = append(command, "-c", in, "-o", out)

		compileCommands = append(compileCommands, &models.CompileCommand{
			Directory: m.Path,
			Arguments: command,
			File:      in,
			Output:    out,
		})

		return nil
	}); err != nil {
		return nil, err
	}

	for _, dep := range m.Deps {
		if cppModule, ok := dep.(*CppModule); ok {
			modCommands, err := cppModule.GenerateCompileCommands()
			if err != nil {
				return nil, err
			}
			compileCommands = append(compileCommands, modCommands...)
		}
	}

	return compileCommands, nil
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

		maps.Copy(defines, dep.Defines)

		if dep.IsHeaderOnly {
			continue
		}

		libs = append(libs, dep.Name)
	}

	for _, dep := range m.Dependants {
		maps.Copy(defines, dep.Defines)
	}

	warningFlags := "" // "-Wall -Wextra"

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

func (m *CppModule) ResolveDependencies(modules map[string]Module, dependants []*models.Dependency) error {
	if slices.Contains(dependants, m.toDep()) {
		return fmt.Errorf("Circular dependency detected. Module %s has already been included\n", m.Name)
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

	return nil
}
