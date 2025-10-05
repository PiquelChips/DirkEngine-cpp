package module

import (
	"DirkBuildTool/config"
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

type Module interface {
	ToMakefile() make.Makefile

	getBuildDeps() []Module
	getName() string
	getPath() string

	getDeps() []*models.Dependency
}

func Build(m Module) error {
	for _, mod := range m.getBuildDeps() {
		if err := Build(mod); err != nil {
			return err
		}
	}

	log.Printf("Building module %s", m.getName())
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
		fmt.Printf("There was an error building %s, see logs for details\n", m.getName())
		return err
	}

	log.Printf("Successfully built %s", m.getName())
	return nil
}

func writeIntFile(m Module, name string, data []byte, overwrite bool) error {
	intDir, err := intDir(m)
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

func intDir(m Module) (string, error) {
	modDir := fmt.Sprintf("%s/%s", config.Dirs.Intermediate, m.getName())
	return modDir, os.MkdirAll(modDir, output.DirPerm)
}

// read from .dirkmod files
type ModuleConfig struct {
	Name    string            `json:"name"`
	Target  string            `json:"target"`
	Type    string            `json:"type"`
	Path    string            `json:"-"`
	Std     string            `json:"c_standard"`
	IsLib   bool              `json:"is_lib"`
	Deps    []string          `json:"dependencies"` // project modules
	Ext     []string          `json:"external"`     // thirdparty modules
	Defines map[string]string `json:"defines"`
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

		if c.Defines == nil {
			c.Defines = map[string]string{}
		}

		switch setup.Config.Platform {
		case "Linux":
			c.Defines["PLATFORM_LINUX"] = ""
		}

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
