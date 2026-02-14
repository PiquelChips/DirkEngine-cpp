package module

import (
	"DirkBuildTool/config"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"slices"
)

type Module interface {
	GetName() string
	GetIncludeDirs() []string
	GetDefines() config.Defines
	GetLibs() []string

	Build(*config.Target, config.Defines) error
	AddDependency(Module)
	GetDependencies() []string
}

func Load(path, name string, buildMode *config.BuildMode) (Module, error) {
	path = fmt.Sprintf("%s/%s", path, name)
	modFile := fmt.Sprintf("%s/%s.dirkmod", path, name)
	data, err := os.ReadFile(modFile)
	if err != nil {
		return nil, nil
	}

	mod := &moduleConfig{}
	err = json.Unmarshal(data, mod)
	if err != nil {
		log.Printf("Error loading module %s: %s\n", name, err.Error())
		return nil, nil
	}

	mod.Path = path

	if len(mod.Platforms) > 0 && !slices.Contains(mod.Platforms, config.Platform.Name) {
		return &NullModule{Name: mod.Name}, nil
	}

	if name != mod.Name {
		return nil, fmt.Errorf("module name does not match folder name. module at %s has name %s but should be %s", path, mod.Name, name)
	}

	return mod.toModule(buildMode), nil
}

type NullModule struct {
	Name string
}

func (m *NullModule) GetName() string                            { return m.Name }
func (m *NullModule) GetIncludeDirs() []string                   { return nil }
func (m *NullModule) GetDefines() config.Defines                 { return nil }
func (m *NullModule) GetLibs() []string                          { return nil }
func (m *NullModule) Build(*config.Target, config.Defines) error { return nil }
func (m *NullModule) GetDependencies() []string                  { return nil }
func (m *NullModule) AddDependency(Module)                       {}

// read from .dirkmod files
type moduleConfig struct {
	Name          string            `json:"name"`
	Type          string            `json:"type"`
	Platforms     []string          `json:"platforms"`
	Path          string            `json:"-"`
	Std           string            `json:"standard"`
	HasEntrypoint bool              `json:"has_entrypoint"`
	Deps          []string          `json:"dependencies"`
	Defines       map[string]string `json:"defines"`
	External      []string          `json:"external"`
	IncludeDirs   []string          `json:"include_dirs"`
}

func (c *moduleConfig) toModule(buildMode *config.BuildMode) Module {
	if c.Type == "" {
		c.Type = "cpp"
	}

	if c.Type != "shaders" {
		if len(c.IncludeDirs) == 0 {
			c.IncludeDirs = []string{"include"}
		}

		for _, dir := range c.IncludeDirs {
			c.IncludeDirs = append(c.IncludeDirs, filepath.Join(c.Path, dir))
		}
	}

	if len(c.External) > 0 {
		config.SetupExternals(c.External...)
	}

	switch c.Type {
	case "shaders":
		return &ShaderModule{
			Name: c.Name,
			Path: c.Path,
		}
	case "cpp":
		return &CppModule{
			Name:         c.Name,
			Path:         c.Path,
			Std:          c.Std,
			Dependencies: nil,
			Config:       c,
			External:     c.External,
			IncludeDirs:  c.IncludeDirs,
			buildMode:    buildMode,
			allDeps:      nil,
		}
	case "header-only":
		return &HeaderModule{
			Name:        c.Name,
			Path:        c.Path,
			External:    c.External,
			IncludeDirs: c.IncludeDirs,
			Defines:     c.Defines,
		}
	default:
		log.Printf("Module type %s used by module %s does not exist.", c.Type, c.Name)
		return nil
	}
}
