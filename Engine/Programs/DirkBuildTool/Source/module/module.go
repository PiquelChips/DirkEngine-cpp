package module

import (
	"DirkBuildTool/config"
	"encoding/json"
	"fmt"
	"log"
	"maps"
	"os"
	"path/filepath"
	"slices"
)

type Module interface {
	GetName() string
	GetIncludeDirs() []string
	GetDefines() config.Defines
	GetLibs() []string

	Build() error
	IsBuilt() bool

	getDeps() []Module // returns all the dependencies in the dependency tree
	getPath() string
}

func Load(path, name string, buildConfig *config.BuildConfig) (Module, error) {
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

	return mod.toModule(buildConfig), nil
}

func Build(m Module) error {
	if m.IsBuilt() {
		return nil
	}

	if cppMod, ok := m.(*CppModule); ok {
		for _, mod := range cppMod.Dependencies {
			if err := Build(mod); err != nil {
				return err
			}
		}
	}

	return m.Build()
}

type NullModule struct {
	Name string
}

func (m *NullModule) GetName() string            { return m.Name }
func (m *NullModule) GetIncludeDirs() []string   { return nil }
func (m *NullModule) GetDefines() config.Defines { return nil }
func (m *NullModule) GetLibs() []string          { return nil }
func (m *NullModule) Build() error               { return nil }
func (m *NullModule) IsBuilt() bool              { return true }
func (m *NullModule) getDeps() []Module          { return nil }
func (m *NullModule) getPath() string            { return "" }

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

func (c *moduleConfig) toModule(buildConfig *config.BuildConfig) Module {
	if c.Type == "" {
		c.Type = "cpp"
	}

	if c.Type != "shaders" {
		if len(c.IncludeDirs) == 0 {
			c.IncludeDirs = []string{"include"}
		}

		newDirs := []string{}
		for _, dir := range c.IncludeDirs {
			newDirs = append(newDirs, filepath.Join(c.Path, dir))
		}

		c.IncludeDirs = newDirs
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
		if c.Defines == nil {
			c.Defines = map[string]string{}
		}

		if config.Platform.Defines != nil {
			maps.Copy(c.Defines, config.Platform.Defines)
		}

		if buildConfig.Mode.Defines != nil {
			maps.Copy(c.Defines, buildConfig.Mode.Defines)
		}

		c.Defines["SAVED_DIR"] = fmt.Sprintf("%s/%s/%s", config.Dirs.Saved, buildConfig.Target.Name, buildConfig.Mode.Name)
		c.Defines["SHADERS_DIR"] = fmt.Sprintf("%s/Shaders", config.Dirs.Intermediate)
		c.Defines["ASSETS_DIR"] = config.Dirs.Assets

		return &CppModule{
			Name:         c.Name,
			Path:         c.Path,
			Std:          c.Std,
			Dependencies: nil,
			Config:       c,
			External:     c.External,
			IncludeDirs:  c.IncludeDirs,
			build:        buildConfig,
		}
	case "header-only":
		return &HeaderModule{
			Name:        c.Name,
			Path:        c.Path,
			External:    c.External,
			IncludeDirs: c.IncludeDirs,
		}
	default:
		log.Printf("Module type %s used by module %s does not exist. Please use \"shaders\" or \"cpp\"\n", c.Type, c.Name)
		return nil
	}
}
