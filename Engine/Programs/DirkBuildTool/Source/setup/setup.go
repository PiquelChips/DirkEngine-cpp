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

func isSetupValid(buildConfig *models.BuildConfig) bool {
	// attempt to read setup file
	data, err := output.ReadIntFile(setupFile)
	if err != nil {
		return false
	}

	// check json is valid
	if err := json.Unmarshal(data, config.Setup); err != nil {
		return false
	}

	// get file info
	info, err := output.GetIntFileInfo(setupFile)
	if err != nil {
		return false
	}

	// check dif between LastSetup & last file update
	if config.Setup.LastSetup.Sub(info.ModTime()).Seconds() > 1 {
		return false
	}

	if config.Setup.BuildConfig != buildConfig {
		return false
	}

	return true
}

func Setup(buildConfig *models.BuildConfig) error {
	config.Setup = &models.SetupConfig{}
	if isSetupValid(buildConfig) {
		return nil
	}
	log.Printf("No valid setup file detected. Running setup\n")

	config.Setup.LastSetup = time.Now()
	config.Setup.BuildConfig = buildConfig
	config.Setup.Platform = "Linux"

	os.Symlink(fmt.Sprintf("%s/compile_commands.json", config.Dirs.Intermediate), fmt.Sprintf("%s/compile_commands.json", config.Dirs.Root))

	vulkanDir := os.Getenv("VULKAN_SDK")
	os.Symlink(fmt.Sprintf("%s/lib/libvulkan.so", vulkanDir), fmt.Sprintf("%s/libvulkan.so", config.Dirs.Binaries))
	os.Symlink(fmt.Sprintf("%s/lib/libvulkan.so.1", vulkanDir), fmt.Sprintf("%s/libvulkan.so.1", config.Dirs.Binaries))

	// hardcoded deps
	config.Setup.Thirdparty = map[string]*models.Dependency{
		"glm": {
			Name:         "glm",
			IsHeaderOnly: true,
			IncludeDir:   ".",
		},
		"tinygltf": {
			Name:         "tinygltf",
			IsHeaderOnly: true,
			IncludeDir:   ".",
		},
		"vulkan": {
			Name:         "vulkan",
			IsHeaderOnly: false,
			IncludeDir:   fmt.Sprintf("%s/include", vulkanDir),
		},
	}

	// make all paths absolute
	for _, dep := range config.Setup.Thirdparty {
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
	data, err := json.Marshal(config.Setup)
	if err != nil {
		return nil
	}

	log.Printf("Writting setup file\n")
	return output.WriteIntFile(setupFile, data, true)
}

func getDir(name string) (string, error) {
	return filepath.Abs(fmt.Sprintf("%s/%s", config.Dirs.Thirdparty, name))
}
