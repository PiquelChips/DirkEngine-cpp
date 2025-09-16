package setup

import (
	"DirkBuildTool/models"
	"DirkBuildTool/output"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"time"
)

const configFile = "setup.json"

type BuildConfig struct {
	Target string `json:"target"`
}

func (c *BuildConfig) String() string {
	return c.Target
}

type SetupConfig struct {
	Thirdparty  map[string]*models.Dependency `json:"thirdparty"`
	LastSetup   time.Time                     `json:"last_setup"`
	BuildConfig *BuildConfig                  `json:"build_config"`
}

var config *SetupConfig

func Get() *SetupConfig {
	if config == nil {
		if err := Setup(nil); err != nil {
			panic(err)
		}
	}

	return config
}

func isSetupValid(buildConfig *BuildConfig) bool {
	// attempt to read setup file
	data, err := output.ReadIntFile(configFile)
	if err != nil {
		return false
	}

	// check json is valid
	if err := json.Unmarshal(data, config); err != nil {
		return false
	}

	// get file info
	info, err := output.GetIntFileInfo(configFile)
	if err != nil {
		return false
	}

	// check dif between LastSetup & last file update
	// TODO: doesnt seem to work
	if config.LastSetup.Sub(info.ModTime()).Seconds() > 1 {
		return false
	}

	if config.BuildConfig != buildConfig {
		return false
	}

	return true
}

func Setup(buildConfig *BuildConfig) error {
	config = &SetupConfig{BuildConfig: buildConfig}
	if isSetupValid(buildConfig) {
		return nil
	}
	log.Printf("No valid setup file detected, running setup...\n")

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
		if !filepath.IsAbs(dep.IncludeDir) {
			dir, err := getDir(dep.Name)
			if err != nil {
				return nil
			}

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

	log.Printf("Writting setup file...\n")
	return output.WriteIntFile(configFile, data, true)
}

func getDir(name string) (string, error) {
	return filepath.Abs(fmt.Sprintf("%s/%s", output.Dirs.Thirdparty, name))
}
