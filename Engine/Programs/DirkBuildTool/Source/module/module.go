package module

import (
	"DirkBuildTool/config"
	"DirkBuildTool/models"
	"encoding/json"
	"fmt"
	"log"
	"maps"
	"os"
	"path/filepath"
)

type Module interface {
	GetName() string
	GetIncludeDirs() []string
	GetDefines() models.Defines
	GetLibs() []string

	Build() error

	getDeps() []Module
	getPath() string
}

func LoadModule(path, name string, buildConfig *models.BuildConfig) (Module, error) {
	path = fmt.Sprintf("%s/%s", path, name)
	modFile := fmt.Sprintf("%s/%s.dirkmod", path, name)
	data, err := os.ReadFile(modFile)
	if err != nil {
		return nil, nil
	}

	config := &moduleConfig{}
	err = json.Unmarshal(data, config)
	if err != nil {
		log.Printf("Error loading module %s: %s\n", name, err.Error())
		return nil, nil
	}

	config.Path = path

	return config.toModule(buildConfig), nil
}

// read from .dirkmod files
type moduleConfig struct {
	Name          string            `json:"name"`
	Type          string            `json:"type"`
	Path          string            `json:"-"`
	Std           string            `json:"standard"`
	HasEntrypoint bool              `json:"has_entrypoint"`
	Deps          []string          `json:"dependencies"`
	Defines       map[string]string `json:"defines"`
	External      []string          `json:"external"`
	IncludeDirs   []string          `json:"include_dirs"`
}

func (c *moduleConfig) toModule(buildConfig *models.BuildConfig) Module {
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
