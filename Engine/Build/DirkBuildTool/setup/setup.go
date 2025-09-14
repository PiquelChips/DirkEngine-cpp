package setup

import (
	"DirkBuildTool/models"
	"DirkBuildTool/output"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
)

const thirdpartyFile = "thirdparty.json"

func Setup() error {
	// TODO: build glfw

	glfw := os.Getenv("GLFW")
	vulkan := os.Getenv("VULKAN_SDK")

	// hardcoded deps
	thirdparty := models.Thirdparty{
		"glm": &models.ThirdpartyDep{
			Name:         "glm",
			IsHeaderOnly: true,
			IncludeDir:   ".",
		},
		"tinygltf": &models.ThirdpartyDep{
			Name:         "tinygltf",
			IsHeaderOnly: true,
			IncludeDir:   ".",
		},
		"glfw": &models.ThirdpartyDep{
			Name:         "glfw",
			IsHeaderOnly: false,
			IncludeDir:   fmt.Sprintf("%s/include", glfw),
			LibDir:       fmt.Sprintf("%s/lib", glfw),
		},
		"vulkan": &models.ThirdpartyDep{
			Name:         "vulkan",
			IsHeaderOnly: false,
			IncludeDir:   fmt.Sprintf("%s/include", vulkan),
			LibDir:       fmt.Sprintf("%s/lib", vulkan),
		},
	}

	// make all paths absolute
	for _, dep := range thirdparty {
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
	data, err := json.Marshal(thirdparty)
	if err != nil {
		return nil
	}

	return output.WriteIntFile(thirdpartyFile, data)
}

func getDir(name string) (string, error) {
	return filepath.Abs(fmt.Sprintf("%s/%s", output.Dirs.Thirdparty, name))
}

func ReadThirdparty() (*models.Thirdparty, error) {
	data, err := output.ReadIntFile(thirdpartyFile)
	if err != nil {
		return nil, err
	}

	thirdparty := &models.Thirdparty{}
	if err = json.Unmarshal(data, thirdparty); err != nil {
		return nil, err
	}
	return thirdparty, nil
}
