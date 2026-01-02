package module

import (
	"DirkBuildTool/config"
	"DirkBuildTool/make"
	"DirkBuildTool/models"
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
	Name         string
	Path         string
	Std          string
	Dependencies []Module
	Dependants   []Module
	Config       *moduleConfig
	External     []string
	IncludeDirs  []string
	build        *models.BuildConfig
}

func (m *CppModule) GetIncludeDirs() []string   { return m.IncludeDirs }
func (m *CppModule) GetDefines() models.Defines { return m.Config.Defines }
func (m *CppModule) GetName() string            { return m.Name }
func (m *CppModule) GetLibs() []string {
	return []string{m.Name}
}

func (m *CppModule) GenerateCompileCommands() (models.CompileCommands, error) {
	compileCommands := models.CompileCommands{}

	if err := filepath.WalkDir(fmt.Sprintf("%s/src", m.Path), func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			return err
		}
		if d.IsDir() {
			return nil
		}

		in := strings.Replace(path, m.Path, "", 1)
		in = strings.Trim(in, "/")

		intDir := fmt.Sprintf("%s/%s", config.Dirs.Intermediate, m.Name)
		out := fmt.Sprintf("%s/%s/%s", intDir, m.build.Mode.Name, in)
		out = strings.Replace(out, "/src/", "/obj/", 1)
		out = strings.Replace(out, ".cpp", ".o", 1)

		incDirs := m.GetIncludeDirs()
		defines := m.Config.Defines

		for _, dep := range m.getDeps() {
			if dep.GetIncludeDirs() != nil {
				incDirs = append(incDirs, dep.GetIncludeDirs()...)
			}

			if dep.GetDefines() != nil {
				maps.Copy(defines, dep.GetDefines())
			}
		}

		for _, dep := range m.Dependants {
			if dep.GetDefines() != nil {
				maps.Copy(defines, dep.GetDefines())
			}
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

	for _, dep := range m.Dependencies {
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

func (m *CppModule) Build() error {
	log.Printf("Generating Makefile for %s\n", m.Name)

	incDirs := m.GetIncludeDirs()
	libs := m.External
	defines := m.Config.Defines

	for _, dep := range m.getDeps() {
		if dep.GetIncludeDirs() != nil {
			incDirs = append(incDirs, dep.GetIncludeDirs()...)
		}

		if dep.GetDefines() != nil {
			maps.Copy(defines, dep.GetDefines())
		}

		libs = append(libs, dep.GetLibs()...)
	}

	for _, dep := range m.Dependants {
		if dep.GetDefines() != nil {
			maps.Copy(defines, dep.GetDefines())
		}
	}

	warningFlags := "" // "-Wall -Wextra"

	return make.RunMakefile(&make.CppMakefile{
		Name:      m.Name,
		Path:      m.Path,
		BuildType: m.build.Mode.Name,
		RootDir:   config.Dirs.Work,
		IncDirs:   incDirs,
		Libs:      libs,
		Defines:   defines,
		IsLib:     !m.Config.HasEntrypoint,
		IsStatic:  m.build.Mode.Compact,
		Optimize:  m.build.Mode.Optimize,
		CFlags:    fmt.Sprintf("-fPIC %s -std=%s", warningFlags, m.Std),
	})
}

func (m *CppModule) getPath() string {
	return m.Path
}

func (m *CppModule) getDeps() []Module {
	deps := m.Dependencies

	for _, mod := range m.Dependencies {
		deps = append(deps, mod)

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

func (m *CppModule) ResolveDependencies(modules map[string]Module, dependants []Module) error {
	for _, dep := range dependants {
		if m == dep {
			return fmt.Errorf("Circular dependency detected. Module %s has already been included in build\n", dep.GetName())
		}
	}

	m.Dependants = dependants
	for _, dep := range m.Config.Deps {
		mod, ok := modules[dep]
		if !ok {
			log.Printf("Module %s required by module %s does not exist\n", dep, m.Name)
		}

		m.Dependencies = append(m.Dependencies, mod)
		if engineMod, ok := mod.(*CppModule); ok {
			if err := engineMod.ResolveDependencies(modules, append(dependants, m)); err != nil {
				return err
			}
		}
	}

	return nil
}
