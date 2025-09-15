package setup

import (
	"DirkBuildTool/models"
	"DirkBuildTool/output"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"time"
)

const configFile = "setup.json"

type SetupConfig struct {
	Thirdparty map[string]*models.Dependency `json:"thirdparty"`
	LastSetup  time.Time                     `json:"last_setup"`
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
	if data, err := output.ReadIntFile(configFile); err == nil {
		if err := json.Unmarshal(data, config); err == nil {
			return nil
		}
	}
	fmt.Printf("no valid setup file detected, running setup\n")

	config.LastSetup = time.Now()
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
