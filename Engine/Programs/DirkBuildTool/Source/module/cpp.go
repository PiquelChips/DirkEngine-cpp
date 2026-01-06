package module

import (
	"DirkBuildTool/config"
	"DirkBuildTool/make"
	"fmt"
	"io/fs"
	"log"
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
	Config       *moduleConfig
	External     []string
	IncludeDirs  []string
	build        *config.BuildConfig
	allDeps      []Module // all the dependencies, detected recursively (populated by getDeps)
}

func (m *CppModule) GetIncludeDirs() []string   { return m.IncludeDirs }
func (m *CppModule) GetDefines() config.Defines { return m.Config.Defines }
func (m *CppModule) GetName() string            { return m.Name }
func (m *CppModule) GetLibs() []string          { return append(m.External, m.Name) }

func (m *CppModule) GetDependencies() []string   { return m.Config.Deps }
func (m *CppModule) AddDependency(module Module) { m.Dependencies = append(m.Dependencies, module) }

func (m *CppModule) Build(defines config.Defines) error {
	log.Printf("Generating Makefile for %s\n", m.Name)

	name := m.Name
	if m.Config.HasEntrypoint {
		name = m.build.Target.Name
	}

	libs := m.External
	for _, dep := range m.getDeps() {
		libs = append(libs, dep.GetLibs()...)
	}

	err := make.RunMakefile(&make.CppMakefile{
		Name:      name,
		Path:      m.Path,
		BuildMode: m.build.Mode,
		RootDir:   config.Dirs.Work,
		IncDirs:   m.getAllIncludeDirs(),
		Libs:      libs,
		Defines:   defines,
		IsLib:     !m.Config.HasEntrypoint,
		IsStatic:  m.build.Mode.Compact,
		Optimize:  m.build.Mode.Optimize,
		CFlags:    m.getCFlags(),
		LdFlags:   m.build.Mode.LinkerFlags,
	})

	return err
}

func (m *CppModule) GenerateCompileCommands(defines config.Defines) (config.CompileCommands, error) {
	compileCommands := config.CompileCommands{}

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

		command := []string{"g++"}
		command = append(command, m.getCFlags()...)

		for _, dir := range m.getAllIncludeDirs() {
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

		compileCommands = append(compileCommands, &config.CompileCommand{
			Directory: m.Path,
			Arguments: command,
			File:      in,
			Output:    out,
		})

		return nil
	}); err != nil {
		return nil, err
	}

	return compileCommands, nil
}

func (m *CppModule) getCFlags() []string {
	var warningFlags []string
	switch m.build.Mode.WarningLevel {
	case config.WarningLevelNone:
		warningFlags = []string{"-w"}
	case config.WarningLevelLow:
		warningFlags = []string{"-Wall"}
	case config.WarningLevelMedium:
		warningFlags = []string{"-Wall", "-Wextra"}
	case config.WarningLevelMax:
		warningFlags = []string{
			"-Wall",
			"-Wextra",
			"-Wpedantic",
			"-Wshadow",
			"-Wnon-virtual-dtor",
			"-Wold-style-cast",
			"-Woverloaded-virtual",
			"-Wunused-parameter",
			"-Wnull-dereference",
		}
	}

	cFlags := append(m.build.Mode.CompileFlags, warningFlags...)
	cFlags = append(cFlags, "-fPIC", fmt.Sprintf("-std=%s", m.Std))
	return cFlags
}

func (m *CppModule) getAllIncludeDirs() []string {
	incDirs := m.GetIncludeDirs()

	for _, dep := range m.getDeps() {
		incDirs = append(incDirs, dep.GetIncludeDirs()...)
	}

	return incDirs
}

func (m *CppModule) getDeps() []Module {
	if m.allDeps != nil {
		return m.allDeps
	}

	m.allDeps = m.Dependencies
	for _, mod := range m.Dependencies {
		m.allDeps = append(m.allDeps, mod)

		if cppMod, ok := mod.(*CppModule); ok {
			modDeps := cppMod.getDeps()
			// cleaning duplicates
			for _, modDep := range modDeps {
				if !slices.Contains(m.allDeps, modDep) {
					m.allDeps = append(m.allDeps, modDep)
				}
			}
		}
	}

	return m.allDeps
}
