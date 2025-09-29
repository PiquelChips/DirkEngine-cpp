package setup

import (
	"DirkBuildTool/config"
	"DirkBuildTool/models"
	"DirkBuildTool/output"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"time"
)

const setupFile = "setup.json"

type BuildConfig struct {
	Target string            `json:"target"`
	Type   *config.BuildType `json:"build_type"`
}

type SetupConfig struct {
	Thirdparty  map[string]*models.Dependency `json:"thirdparty"`
	LastSetup   time.Time                     `json:"last_setup"`
	BuildConfig *BuildConfig                  `json:"build_config"`
}

var Config *SetupConfig

func isSetupValid(buildConfig *BuildConfig) bool {
	// attempt to read setup file
	data, err := output.ReadIntFile(setupFile)
	if err != nil {
		return false
	}

	// check json is valid
	if err := json.Unmarshal(data, Config); err != nil {
		return false
	}

	// get file info
	info, err := output.GetIntFileInfo(setupFile)
	if err != nil {
		return false
	}

	// check dif between LastSetup & last file update
	if Config.LastSetup.Sub(info.ModTime()).Seconds() > 1 {
		return false
	}

	if Config.BuildConfig != buildConfig {
		return false
	}

	return true
}

func Setup(buildConfig *BuildConfig) error {
	Config = &SetupConfig{}
	if isSetupValid(buildConfig) {
		return nil
	}
	log.Printf("No valid setup file detected. Running setup\n")

	Config.LastSetup = time.Now()
	Config.BuildConfig = buildConfig
	// TODO: build glfw

	glfwDir := os.Getenv("GLFW")
	os.Symlink(fmt.Sprintf("%s/lib/libglfw.so", glfwDir), fmt.Sprintf("%s/libglfw.so", config.Dirs.Binaries))
	os.Symlink(fmt.Sprintf("%s/lib/libglfw.so.3", glfwDir), fmt.Sprintf("%s/libglfw.so.3", config.Dirs.Binaries))

	vulkanDir := os.Getenv("VULKAN_SDK")
	os.Symlink(fmt.Sprintf("%s/lib/libvulkan.so", vulkanDir), fmt.Sprintf("%s/libvulkan.so", config.Dirs.Binaries))
	os.Symlink(fmt.Sprintf("%s/lib/libvulkan.so.1", vulkanDir), fmt.Sprintf("%s/libvulkan.so.1", config.Dirs.Binaries))

	// hardcoded deps
	Config.Thirdparty = map[string]*models.Dependency{
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
			IncludeDir:   fmt.Sprintf("%s/include", glfwDir),
		},
		"vulkan": {
			Name:         "vulkan",
			IsHeaderOnly: false,
			IncludeDir:   fmt.Sprintf("%s/include", vulkanDir),
		},
	}

	// make all paths absolute
	for _, dep := range Config.Thirdparty {
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
	data, err := json.Marshal(Config)
	if err != nil {
		return nil
	}

	log.Printf("Writting setup file\n")
	return output.WriteIntFile(setupFile, data, true)
}

func getDir(name string) (string, error) {
	return filepath.Abs(fmt.Sprintf("%s/%s", config.Dirs.Thirdparty, name))
}
