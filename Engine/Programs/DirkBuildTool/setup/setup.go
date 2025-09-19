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
	Target    string `json:"target"`
	BuildType string `json:"build_type"`
	Optimize  bool   `json:"opmtimize"`
	Shipping  bool   `json:"shipping"`
}

func (c *BuildConfig) String() string {
	optStr := "off"
	if c.Optimize {
		optStr = "on"
	}

	shipStr := "off"
	if c.Shipping {
		shipStr = "on"
	}

	return fmt.Sprintf("\n\ttarget: %s\n\tbuild type: %s\n\toptimizations: %s\n\tshipping: %s",
		c.Target, c.BuildType, optStr, shipStr)
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
	if config.LastSetup.Sub(info.ModTime()).Seconds() > 1 {
		return false
	}

	if config.BuildConfig != buildConfig {
		return false
	}

	return true
}

func Setup(buildConfig *BuildConfig) error {
	config = &SetupConfig{}
	if isSetupValid(buildConfig) {
		return nil
	}
	log.Printf("No valid setup file detected. Running setup...\n")

	config.LastSetup = time.Now()
	config.BuildConfig = buildConfig
	// TODO: build glfw

	glfwDir := os.Getenv("GLFW")
	os.Symlink(fmt.Sprintf("%s/lib/libglfw.so", glfwDir), fmt.Sprintf("%s/libglfw.so", output.Dirs.Binaries))
	os.Symlink(fmt.Sprintf("%s/lib/libglfw.so.3", glfwDir), fmt.Sprintf("%s/libglfw.so.3", output.Dirs.Binaries))

	vulkanDir := os.Getenv("VULKAN_SDK")
	os.Symlink(fmt.Sprintf("%s/lib/libvulkan.so", vulkanDir), fmt.Sprintf("%s/libvulkan.so", output.Dirs.Binaries))
	os.Symlink(fmt.Sprintf("%s/lib/libvulkan.so.1", vulkanDir), fmt.Sprintf("%s/libvulkan.so.1", output.Dirs.Binaries))

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
			IncludeDir:   fmt.Sprintf("%s/include", glfwDir),
		},
		"vulkan": {
			Name:         "vulkan",
			IsHeaderOnly: false,
			IncludeDir:   fmt.Sprintf("%s/include", vulkanDir),
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
