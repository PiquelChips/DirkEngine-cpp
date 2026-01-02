package module

import (
	"DirkBuildTool/config"
	"DirkBuildTool/make"
	"DirkBuildTool/models"
	"encoding/json"
	"fmt"
	"log"
	"maps"
	"os"
	"os/exec"
	"strings"
)

type Module interface {
	GetName() string
	GetIncludeDir() string
	GetDefines() models.Defines
	GetLibs() []string

	ToMakefile() make.Makefile

	getDeps() []Module
	getPath() string
}

func Build(m Module) error {
	for _, mod := range m.getDeps() {
		if err := Build(mod); err != nil {
			return err
		}
	}

	log.Printf("Building module %s", m.GetName())
	makefile, err := m.ToMakefile().ToBytes()
	if err != nil {
		return err
	}

	if err := writeIntFile(m, "Makefile", makefile, true); err != nil {
		return err
	}

	intDir, _ := intDir(m)

	makefilePath := fmt.Sprintf("%s/Makefile", intDir)
	cmd := exec.Command("make", "-f", makefilePath, "-j", "8")
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Dir = m.getPath()

	if err := cmd.Run(); err != nil {
		fmt.Printf("There was an error building %s, see logs for details\n", m.GetName())
		return err
	}

	log.Printf("Successfully built %s", m.GetName())
	return nil
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

func writeIntFile(m Module, name string, data []byte, overwrite bool) error {
	intDir, err := intDir(m)
	if err != nil {
		return err
	}

	name = strings.Trim(name, "/")
	name = fmt.Sprintf("%s/%s", intDir, name)

	if overwrite {
		return os.WriteFile(name, data, config.FilePerm)
	}

	f, err := os.OpenFile(name, os.O_APPEND|os.O_CREATE, config.FilePerm)
	if err != nil {
		return err
	}
	defer f.Close()

	f.Write(data)
	return nil
}

func intDir(m Module) (string, error) {
	modDir := fmt.Sprintf("%s/%s", config.Dirs.Intermediate, m.GetName())
	return modDir, os.MkdirAll(modDir, config.DirPerm)
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
}

func (c *moduleConfig) toModule(buildConfig *models.BuildConfig) Module {
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
			BuildDeps:    nil,
			Dependencies: nil,
			Config:       c,
			External:     c.External,
			build:        buildConfig,
		}
	case "header-only":
		return &HeaderModule{
			Name:     c.Name,
			Path:     c.Path,
			External: c.External,
		}
	default:
		log.Printf("Module type %s used by module %s does not exist. Please use \"shaders\" or \"cpp\"\n", c.Type, c.Name)
		return nil
	}
}
