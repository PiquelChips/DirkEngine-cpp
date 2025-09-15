package setup

import (
	"DirkBuildTool/models"
	"DirkBuildTool/output"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
)

const configFile = "setup.json"

type SetupConfig struct {
	Thirdparty map[string]*models.Dependency
}

var config *SetupConfig

func Get() *SetupConfig {
	if config == nil {
		if err := Setup(); err != nil {
			panic(err)
		}
	}

	return config
}

func Setup() error {
	config = &SetupConfig{}
	// TODO: build glfw

	glfw := os.Getenv("GLFW")
	vulkan := os.Getenv("VULKAN_SDK")

	// hardcoded deps
	config.Thirdparty = map[string]*models.Dependency{
		"glm": {
			Name:         "glm",
			IsHeaderOnly: true,
			IncludeDir:   ".", // relative to thirdparty dir
		},
		"tinygltf": {
			Name:         "tinygltf",
			IsHeaderOnly: true,
			IncludeDir:   ".", // relative to thirdparty dir
		},
		"glfw": {
			Name:         "glfw",
			IsHeaderOnly: false,
			IncludeDir:   fmt.Sprintf("%s/include", glfw),
			LibDir:       fmt.Sprintf("%s/lib", glfw),
		},
		"vulkan": {
			Name:         "vulkan",
			IsHeaderOnly: false,
			IncludeDir:   fmt.Sprintf("%s/include", vulkan),
			LibDir:       fmt.Sprintf("%s/lib", vulkan),
		},
	}

	// make all paths absolute
	for _, dep := range config.Thirdparty {
		dir, err := getDir(dep.Name)
		if err != nil {
			return nil
		}

		if !filepath.IsAbs(dep.IncludeDir) {
			incDir, err := filepath.Abs(fmt.Sprintf("%s/%s", dir, dep.IncludeDir))
			if err != nil {
				return nil
			}
			dep.IncludeDir = incDir
		}
	}

	// write the file
	data, err := json.Marshal(config)
	if err != nil {
		return nil
	}

	return output.WriteIntFile(configFile, data, true)
}

func getDir(name string) (string, error) {
	return filepath.Abs(fmt.Sprintf("%s/%s", output.Dirs.Thirdparty, name))
}
